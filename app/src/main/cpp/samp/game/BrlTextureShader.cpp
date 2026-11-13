#include <utility>
#include "BrlTextureShader.h"

#include <algorithm>
#include <mutex>
#include <optional>
#include <sstream>
#include <set>
#include <unordered_map>
#include <vector>

#include "../main.h"
#include "../java/jniutil.h"
#include "RW/RenderWare.h"
#include "game.h"
#include "playerped.h"
#include "vehicle.h"
#include "object.h"
#include "Textures/TextureDatabaseRuntime.h"
#include "RW/rwcore.h"
#include "RW/rpworld.h"
#include "../net/netgame.h"
#include "../net/playerpool.h"
#include "../net/remoteplayer.h"
#include "../net/vehiclepool.h"
#include "../net/objectpool.h"

extern CGame* pGame;
extern CJavaWrapper* pJavaWrapper;
extern CNetGame* pNetGame;

namespace {
	constexpr uint16_t kLocalPlayerShaderId = 0xFFFF;
	constexpr uint16_t kAnyShaderModelId = 0xFFFF;
	constexpr uint32_t kProcessIntervalMs = 750;

	struct ShaderKey {
		BrlShaderTargetType type{};
		uint16_t id{};
		uint8_t subId{};

		bool operator==(const ShaderKey& other) const
		{
			return type == other.type && id == other.id && subId == other.subId;
		}
	};

	struct ShaderKeyHash {
		size_t operator()(const ShaderKey& key) const
		{
			return (static_cast<size_t>(key.type) << 24u) ^ (static_cast<size_t>(key.id) << 8u) ^ key.subId;
		}
	};

	struct ShaderRule {
		std::string sourceTexture;
		std::string replacementTexture;
		uint16_t modelId{kAnyShaderModelId};
	};

	struct ApplyContext {
		uint16_t modelId{};
		ShaderRule* rule{};
	};

	struct RestoreContext {
		uint16_t modelId{};
		std::string sourceTexture;
	};

	std::mutex g_shaderMutex;
	std::unordered_map<ShaderKey, std::vector<ShaderRule>, ShaderKeyHash> g_rules;
	std::unordered_map<std::string, RwTexture*> g_textureCache;
	std::unordered_map<RpMaterial*, RwTexture*> g_originalMaterialTextures;
	std::unordered_map<RpMaterial*, std::string> g_originalMaterialNames;

	std::string NormalizeName(std::string value)
	{
		std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c) {
			return static_cast<char>(std::tolower(c));
		});
		return value;
	}

	// Nom de materiau compare sans casse et avec '.' == '_' : les corps du pack AVZ
	// (Femme 2 / Homme 3) nomment leurs parties "body.torso1", le catalogue "body_torso1".
	std::string MaterialKey(std::string value)
	{
		value = NormalizeName(std::move(value));
		std::replace(value.begin(), value.end(), '.', '_');
		return value;
	}

	bool TextureMatches(RwTexture* texture, const std::string& sourceTexture)
	{
		if (sourceTexture.empty() || sourceTexture == "*") return true;
		if (!texture) return false;
		return MaterialKey(texture->name) == MaterialKey(sourceTexture);
	}

	bool MaterialMatches(RpMaterial* material, uint16_t, int, const std::string& sourceTexture)
	{
		if (!material) return false;
		if (sourceTexture.empty() || sourceTexture == "*") return true;

		const std::string normalizedSource = MaterialKey(sourceTexture);

		if (auto it = g_originalMaterialNames.find(material); it != g_originalMaterialNames.end()) {
			if (MaterialKey(it->second) == normalizedSource) return true;
		}

		if (TextureMatches(material->texture, sourceTexture)) return true;

		return false;
	}


    // APK assets, decoded lazily on the game thread. No external cache installation.
    std::unordered_map<std::string, size_t> g_southBytes;
    size_t g_southTotal = 0;
    RwTexture* LoadSouthTexture(const std::string& name) {
        if (!pJavaWrapper || !RwImageCreate || !RwImageAllocatePixels || !RwImageFindRasterFormat ||
            !RwRasterCreate || !RwRasterSetFromImage || !RwTextureCreate) return nullptr;
        JNIEnv* env = nullptr;
        if (javaVM->GetEnv(reinterpret_cast<void**>(&env), JNI_VERSION_1_6) != JNI_OK || !env) return nullptr;
        jclass cls = env->GetObjectClass(pJavaWrapper->activity);
        const auto method = cls ? env->GetMethodID(cls, "loadClothesTexture", "(Ljava/lang/String;)[B") : nullptr;
        if (env->ExceptionCheck()) env->ExceptionClear();
        if (!method) { if (cls) env->DeleteLocalRef(cls); return nullptr; }
        jstring path = env->NewStringUTF(name.c_str());
        auto bytes = static_cast<jbyteArray>(env->CallObjectMethod(pJavaWrapper->activity, method, path));
        env->DeleteLocalRef(path); env->DeleteLocalRef(cls);
        if (env->ExceptionCheck()) { env->ExceptionClear(); if (bytes) env->DeleteLocalRef(bytes); return nullptr; }
        if (!bytes) return nullptr;
        const auto len = env->GetArrayLength(bytes);
        unsigned char header[4]{};
        if (len < 4) { env->DeleteLocalRef(bytes); return nullptr; }
        env->GetByteArrayRegion(bytes, 0, 4, reinterpret_cast<jbyte*>(header));
        const int w = header[0] | (header[1] << 8), h = header[2] | (header[3] << 8);
        if (w <= 0 || h <= 0 || w > 512 || h > 512 || len != 4 + w * h * 4) {
            env->DeleteLocalRef(bytes); return nullptr;
        }
        // Reclaim old previews, retaining every texture referenced by a live player's rules.
        if (g_southTotal + w * h * 4 > 64u * 1024u * 1024u) {
            for (auto it = g_southBytes.begin(); it != g_southBytes.end();) {
                bool used = false;
                for (const auto& [key, rules] : g_rules)
                    for (const auto& rule : rules) if (rule.replacementTexture == it->first) used = true;
                if (used) { ++it; continue; }
                auto cached = g_textureCache.find(it->first);
                if (cached != g_textureCache.end()) {
                    RwTextureDestroy(cached->second);
                    g_textureCache.erase(cached);
                }
                g_southTotal -= it->second;
                it = g_southBytes.erase(it);
            }
        }
        RwImage* image = RwImageCreate(w, h, 32);
        if (!image || !RwImageAllocatePixels(image)) {
            if (image) RwImageDestroy(image);
            env->DeleteLocalRef(bytes); return nullptr;
        }
        for (int y = 0; y < h; ++y)
            env->GetByteArrayRegion(bytes, 4 + y * w * 4, w * 4, reinterpret_cast<jbyte*>(image->cpPixels + y * image->stride));
        env->DeleteLocalRef(bytes);
        int rw = 0, rh = 0, depth = 0, flags = 0;
        RwRaster* raster = nullptr;
        if (RwImageFindRasterFormat(image, rwRASTERTYPETEXTURE, &rw, &rh, &depth, &flags))
            raster = RwRasterCreate(rw, rh, depth, flags);
        if (raster && !RwRasterSetFromImage(raster, image)) { RwRasterDestroy(raster); raster = nullptr; }
        RwImageDestroy(image);
        if (!raster) return nullptr;
        RwTexture* texture = RwTextureCreate(raster);
        if (!texture) { RwRasterDestroy(raster); return nullptr; }
        RwTextureSetFilterMode(texture, rwFILTERLINEAR);
        RwTextureSetAddressing(texture, rwTEXTUREADDRESSWRAP);
        g_southBytes[name] = static_cast<size_t>(w * h * 4);
        g_southTotal += w * h * 4;
        return texture;
    }

	RwTexture* ResolveTexture(const std::string& textureName)
	{
		if (textureName.empty()) return nullptr;

		const std::string normalized = NormalizeName(textureName);
		if (auto it = g_textureCache.find(normalized); it != g_textureCache.end()) {
			return it->second;
		}

		if (textureName.rfind("south/", 0) == 0) {
            RwTexture* loaded = LoadSouthTexture(textureName);
            if (loaded) g_textureCache[normalized] = loaded;
            return loaded;
        }

		RwTexture* texture = TextureDatabaseRuntime::GetTexture(textureName.c_str());
		if (!texture && normalized != textureName) {
			texture = TextureDatabaseRuntime::GetTexture(normalized.c_str());
		}

		if (texture) {
			RwTextureSetFilterMode(texture, rwFILTERLINEARMIPLINEAR);
			RwTextureSetAddressing(texture, rwTEXTUREADDRESSWRAP);
		}

		if (texture) {
			g_textureCache[normalized] = texture;
		}
		return texture;
	}

	RpAtomic* ApplyAtomicCallback(RpAtomic* atomic, void* data)
	{
		auto* context = static_cast<ApplyContext*>(data);
		if (!atomic || !context || !context->rule || *(uint8_t*) atomic != 1) return atomic;
		ShaderRule* rule = context->rule;
		if (rule->modelId != kAnyShaderModelId && rule->modelId != context->modelId) return atomic;

		RpGeometry* geometry = atomic->geometry;
		if (!geometry || geometry->matList.numMaterials <= 0 || !geometry->matList.materials) return atomic;

		RwTexture* replacement = ResolveTexture(rule->replacementTexture);
		if (!replacement) return atomic;

		for (int i = 0; i < geometry->matList.numMaterials; ++i) {
			RpMaterial* material = geometry->matList.materials[i];
			if (!material || !MaterialMatches(material, context->modelId, i, rule->sourceTexture)) continue;

			if (!g_originalMaterialTextures.contains(material)) {
				g_originalMaterialTextures[material] = material->texture;
				g_originalMaterialNames[material] = material->texture ? material->texture->name : rule->sourceTexture;
			}
			material->texture = replacement;
		}

		return atomic;
	}

	RpAtomic* RestoreAtomicCallback(RpAtomic* atomic, void*)
	{
		if (!atomic || *(uint8_t*) atomic != 1) return atomic;

		RpGeometry* geometry = atomic->geometry;
		if (!geometry || geometry->matList.numMaterials <= 0 || !geometry->matList.materials) return atomic;

		for (int i = 0; i < geometry->matList.numMaterials; ++i) {
			RpMaterial* material = geometry->matList.materials[i];
			if (!material) continue;

			auto it = g_originalMaterialTextures.find(material);
			if (it != g_originalMaterialTextures.end()) {
				material->texture = it->second;
				g_originalMaterialTextures.erase(it);
			}
			g_originalMaterialNames.erase(material);
		}

		return atomic;
	}

	RpAtomic* RestoreMatchingAtomicCallback(RpAtomic* atomic, void* data)
	{
		auto* context = static_cast<RestoreContext*>(data);
		if (!atomic || !context || *(uint8_t*) atomic != 1) return atomic;

		RpGeometry* geometry = atomic->geometry;
		if (!geometry || geometry->matList.numMaterials <= 0 || !geometry->matList.materials) return atomic;

		for (int i = 0; i < geometry->matList.numMaterials; ++i) {
			RpMaterial* material = geometry->matList.materials[i];
			if (!material || !MaterialMatches(material, context->modelId, i, context->sourceTexture)) continue;

			auto it = g_originalMaterialTextures.find(material);
			if (it != g_originalMaterialTextures.end()) {
				material->texture = it->second;
				g_originalMaterialTextures.erase(it);
			}
			g_originalMaterialNames.erase(material);
		}

		return atomic;
	}

	void ForEachAtomic(RwObject* rwObject, RpAtomicCallBack callback, void* data)
	{
		if (!rwObject || !callback) return;

		const uint8_t rwType = *(uint8_t*) rwObject;
		if (rwType == rpATOMIC) {
			callback(reinterpret_cast<RpAtomic*>(rwObject), data);
			return;
		}

		if (rwType == rpCLUMP) {
			RpClumpForAllAtomics(reinterpret_cast<RpClump*>(rwObject), callback, data);
			return;
		}

		auto* parent = reinterpret_cast<RwFrame*>(rwObject->parent);
		if (parent) {
			RwFrameForAllObjects(parent, reinterpret_cast<RwObjectCallBack>(callback), data);
		}
	}

bool SetRule(const ShaderKey& key, const std::string& sourceTexture, const std::string& replacementTexture, uint16_t modelId)
{
	if (sourceTexture.empty() || replacementTexture.empty()) return false;
	if (!ResolveTexture(replacementTexture)) {
		Log("[BrlShader] replacement texture '%s' not found; skipping rule.", replacementTexture.c_str());
		return false;
	}

	std::lock_guard lock(g_shaderMutex);
	auto& rules = g_rules[key];
	auto it = std::find_if(rules.begin(), rules.end(), [&](const ShaderRule& rule) {
		return NormalizeName(rule.sourceTexture) == NormalizeName(sourceTexture) && rule.modelId == modelId;
	});

	if (it != rules.end()) {
		it->replacementTexture = replacementTexture;
	}
	else {
		rules.push_back({sourceTexture, replacementTexture, modelId});
	}

	return true;
}

	bool ClearRuleGroup(const ShaderKey& key)
	{
		std::lock_guard lock(g_shaderMutex);
		return g_rules.erase(key) > 0;
	}

	bool ClearRuleSource(const ShaderKey& key, const std::string& sourceTexture)
	{
		if (sourceTexture.empty()) return false;

		std::lock_guard lock(g_shaderMutex);
		auto it = g_rules.find(key);
		if (it == g_rules.end()) return false;

		const std::string normalizedSource = NormalizeName(sourceTexture);
		auto& rules = it->second;
		const auto oldSize = rules.size();
		rules.erase(std::remove_if(rules.begin(), rules.end(), [&](const ShaderRule& rule) {
			return NormalizeName(rule.sourceTexture) == normalizedSource;
		}), rules.end());

		const bool removed = rules.size() != oldSize;
		if (rules.empty()) {
			g_rules.erase(it);
		}

		return removed;
	}

	void ApplyRulesToObject(const ShaderKey& key, RwObject* rwObject, uint16_t modelId)
	{
		if (!rwObject) return;

		std::vector<ShaderRule> rules;
		{
			std::lock_guard lock(g_shaderMutex);
			auto it = g_rules.find(key);
			if (it == g_rules.end()) return;
			rules = it->second;
		}

		for (auto& rule : rules) {
			ApplyContext context{modelId, &rule};
			ForEachAtomic(rwObject, ApplyAtomicCallback, &context);
		}
	}

	void RestoreObject(RwObject* rwObject)
	{
		ForEachAtomic(rwObject, RestoreAtomicCallback, nullptr);
	}

	void RestoreSourceOnObject(RwObject* rwObject, uint16_t modelId, const std::string& sourceTexture)
	{
		RestoreContext context{modelId, sourceTexture};
		ForEachAtomic(rwObject, RestoreMatchingAtomicCallback, &context);
	}

	CPlayerPed* FindLocalPed()
	{
		if (pNetGame && pNetGame->GetPlayerPool() && pNetGame->GetPlayerPool()->GetLocalPlayer()) {
			return pNetGame->GetPlayerPool()->GetLocalPlayer()->GetPlayerPed();
		}
		if (pGame) {
			return pGame->FindPlayerPed();
		}
		return nullptr;
	}

	CPlayerPed* FindPlayerPed(uint16_t playerId)
	{
		if (playerId == kLocalPlayerShaderId) return FindLocalPed();
		if (pNetGame && pNetGame->GetPlayerPool()) {
			if (CRemotePlayer* remote = pNetGame->GetPlayerPool()->GetAt(static_cast<PLAYERID>(playerId))) {
				return remote->GetPlayerPed();
			}
		}
		return nullptr;
	}

	uint16_t CurrentPlayerModelId(uint16_t playerId)
	{
		if (CPlayerPed* ped = FindPlayerPed(playerId)) {
			if (ped->m_pPed) {
				return static_cast<uint16_t>(ped->m_pPed->m_nModelIndex);
			}
		}
		return kAnyShaderModelId;
	}

	uint16_t CurrentVehicleModelId(uint16_t vehicleId)
	{
		if (pNetGame && pNetGame->GetVehiclePool()) {
			if (CVehicle* vehicle = pNetGame->GetVehiclePool()->GetAt(vehicleId)) {
				if (vehicle->m_pVehicle) {
					return static_cast<uint16_t>(vehicle->m_pVehicle->m_nModelIndex);
				}
			}
		}
		return kAnyShaderModelId;
	}

	uint16_t CurrentObjectModelId(uint16_t objectId)
	{
		if (pNetGame && pNetGame->GetObjectPool()) {
			if (CObject* object = pNetGame->GetObjectPool()->GetAt(objectId)) {
				if (object->m_pEntity) {
					return static_cast<uint16_t>(object->m_pEntity->m_nModelIndex);
				}
			}
		}
		return kAnyShaderModelId;
	}
}

bool BrlTextureShader::HasTexture(const std::string& textureName)
{
	return ResolveTexture(textureName) != nullptr;
}

bool BrlTextureShader::SetPlayerTexture(uint16_t playerId, const std::string& sourceTexture, const std::string& replacementTexture)
{
	return SetRule({BrlShaderTargetType::Player, playerId, 0}, sourceTexture, replacementTexture, CurrentPlayerModelId(playerId));
}

bool BrlTextureShader::SetLocalPlayerTexture(const std::string& sourceTexture, const std::string& replacementTexture)
{
	// Pas d'application directe sur les materiaux (partages entre tous les peds du meme
	// modele) : la regle est posee au rendu de NOTRE ped seulement (BeginPedRender).
	return SetPlayerTexture(kLocalPlayerShaderId, sourceTexture, replacementTexture);
}

void BrlTextureShader::ResetLocalPlayer()
{
	if (CPlayerPed* ped = FindLocalPed(); ped && ped->m_pPed && ped->m_pPed->m_pRwObject) {
		RestoreObject(ped->m_pPed->m_pRwObject);
	}

	const ShaderKey key{BrlShaderTargetType::Player, kLocalPlayerShaderId, 0};
	std::lock_guard lock(g_shaderMutex);
	g_rules.erase(key);
}

bool BrlTextureShader::ClearLocalPlayerTexture(const std::string& sourceTexture)
{
	const ShaderKey key{BrlShaderTargetType::Player, kLocalPlayerShaderId, 0};
	if (!ClearRuleSource(key, sourceTexture)) return false;

	if (CPlayerPed* ped = FindLocalPed(); ped && ped->m_pPed && ped->m_pPed->m_pRwObject) {
		RestoreSourceOnObject(ped->m_pPed->m_pRwObject, ped->m_pPed->m_nModelIndex, sourceTexture);
	}

	return true;
}

bool BrlTextureShader::ClearPlayerTexture(uint16_t playerId, const std::string& sourceTexture)
{
	if (playerId == kLocalPlayerShaderId) return ClearLocalPlayerTexture(sourceTexture);

	const ShaderKey key{BrlShaderTargetType::Player, playerId, 0};
	if (!ClearRuleSource(key, sourceTexture)) return false;

	if (pNetGame && pNetGame->GetPlayerPool()) {
		if (CRemotePlayer* remote = pNetGame->GetPlayerPool()->GetAt(static_cast<PLAYERID>(playerId))) {
			CPlayerPed* ped = remote->GetPlayerPed();
			if (ped && ped->m_pPed && ped->m_pPed->m_pRwObject) {
				RestoreSourceOnObject(ped->m_pPed->m_pRwObject, ped->m_pPed->m_nModelIndex, sourceTexture);
			}
		}
	}
	return true;
}

bool BrlTextureShader::SetVehicleTexture(uint16_t vehicleId, const std::string& sourceTexture, const std::string& replacementTexture)
{
	return SetRule({BrlShaderTargetType::Vehicle, vehicleId, 0}, sourceTexture, replacementTexture, CurrentVehicleModelId(vehicleId));
}

bool BrlTextureShader::SetObjectTexture(uint16_t objectId, const std::string& sourceTexture, const std::string& replacementTexture)
{
	return SetRule({BrlShaderTargetType::Object, objectId, 0}, sourceTexture, replacementTexture, CurrentObjectModelId(objectId));
}

bool BrlTextureShader::SetWeaponTexture(uint16_t playerId, uint8_t weaponId, const std::string& sourceTexture, const std::string& replacementTexture)
{
	return SetRule({BrlShaderTargetType::Weapon, playerId, weaponId}, sourceTexture, replacementTexture, CurrentPlayerModelId(playerId));
}

bool BrlTextureShader::ClearPlayer(uint16_t playerId)
{
	return ClearRuleGroup({BrlShaderTargetType::Player, playerId, 0});
}

bool BrlTextureShader::ClearVehicle(uint16_t vehicleId)
{
	return ClearRuleGroup({BrlShaderTargetType::Vehicle, vehicleId, 0});
}

bool BrlTextureShader::ClearObject(uint16_t objectId)
{
	return ClearRuleGroup({BrlShaderTargetType::Object, objectId, 0});
}

bool BrlTextureShader::ClearWeapon(uint16_t playerId, uint8_t weaponId)
{
	return ClearRuleGroup({BrlShaderTargetType::Weapon, playerId, weaponId});
}

void BrlTextureShader::ClearAll()
{
	if (pNetGame) {
		if (CPlayerPool* playerPool = pNetGame->GetPlayerPool()) {
			if (CLocalPlayer* localPlayer = playerPool->GetLocalPlayer()) {
				if (CPlayerPed* ped = localPlayer->GetPlayerPed(); ped && ped->m_pPed) {
					RestoreObject(ped->m_pPed->m_pRwObject);
				}
			}

			for (uint16_t playerId = 0; playerId < MAX_PLAYERS; ++playerId) {
				CRemotePlayer* remotePlayer = playerPool->GetAt(playerId);
				if (!remotePlayer) continue;
				if (CPlayerPed* ped = remotePlayer->GetPlayerPed(); ped && ped->m_pPed) {
					RestoreObject(ped->m_pPed->m_pRwObject);
				}
			}
		}

		if (CVehiclePool* vehiclePool = pNetGame->GetVehiclePool()) {
			for (uint16_t vehicleId = 0; vehicleId < MAX_VEHICLES; ++vehicleId) {
				CVehicle* vehicle = vehiclePool->GetAt(vehicleId);
				if (vehicle && vehicle->m_pVehicle) RestoreObject(vehicle->m_pVehicle->m_pRwObject);
			}
		}

		if (CObjectPool* objectPool = pNetGame->GetObjectPool()) {
			for (uint16_t objectId = 0; objectId < MAX_OBJECTS; ++objectId) {
				CObject* object = objectPool->GetAt(objectId);
				if (object && object->m_pEntity) RestoreObject(object->m_pEntity->m_pRwObject);
			}
		}
	}
	else if (pGame && pGame->FindPlayerPed() && pGame->FindPlayerPed()->m_pPed) {
		RestoreObject(pGame->FindPlayerPed()->m_pPed->m_pRwObject);
	}

	std::lock_guard lock(g_shaderMutex);
	g_rules.clear();
    for (const auto& [name, bytes] : g_southBytes) {
        const auto it = g_textureCache.find(name);
        if (it != g_textureCache.end()) RwTextureDestroy(it->second);
    }
    g_southBytes.clear(); g_southTotal = 0;
	g_textureCache.clear();
	g_originalMaterialTextures.clear();
	g_originalMaterialNames.clear();
}

void BrlTextureShader::ApplyToPed(uint16_t playerId, CPlayerPed* ped)
{
	if (!ped || !ped->m_pPed || !ped->m_pPed->m_pRwObject) return;

	const uint16_t rulePlayerId = playerId == kLocalPlayerShaderId ? kLocalPlayerShaderId : playerId;
	ApplyRulesToObject({BrlShaderTargetType::Player, rulePlayerId, 0}, ped->m_pPed->m_pRwObject, ped->m_pPed->m_nModelIndex);

	const uint8_t currentWeapon = ped->GetCurrentWeapon();
	ApplyRulesToObject({BrlShaderTargetType::Weapon, rulePlayerId, currentWeapon}, ped->m_pPed->m_pRwObject, ped->m_pPed->m_nModelIndex);
}

void BrlTextureShader::ApplyToVehicle(uint16_t vehicleId, CVehicle* vehicle)
{
	if (!vehicle || !vehicle->m_pVehicle || !vehicle->m_pVehicle->m_pRwObject) return;
	ApplyRulesToObject({BrlShaderTargetType::Vehicle, vehicleId, 0}, vehicle->m_pVehicle->m_pRwObject, vehicle->m_pVehicle->m_nModelIndex);
}

void BrlTextureShader::ApplyToObject(uint16_t objectId, CObject* object)
{
	if (!object || !object->m_pEntity || !object->m_pEntity->m_pRwObject) return;
	ApplyRulesToObject({BrlShaderTargetType::Object, objectId, 0}, object->m_pEntity->m_pRwObject, object->m_pEntity->m_nModelIndex);
}

bool BrlTextureShader::ApplyToRwObject(RwObject* rwObject, const std::string& sourceTexture, const std::string& replacementTexture)
{
	if (!rwObject || sourceTexture.empty() || replacementTexture.empty()) return false;
	ShaderRule rule{sourceTexture, replacementTexture};
	ApplyContext context{0, &rule};
	ForEachAtomic(rwObject, ApplyAtomicCallback, &context);
	return true;
}

namespace {
	struct DebugTextureList {
		std::set<std::string> names;
		int maxTextures{};
	};

	RpAtomic* CollectTexturesAtomicCallback(RpAtomic* atomic, void* data)
	{
		auto* list = static_cast<DebugTextureList*>(data);
		if (!atomic || !list || *(uint8_t*) atomic != rpATOMIC) return atomic;

		RpGeometry* geometry = atomic->geometry;
		if (!geometry || geometry->matList.numMaterials <= 0 || !geometry->matList.materials) return atomic;

		for (int i = 0; i < geometry->matList.numMaterials; ++i) {
			RpMaterial* material = geometry->matList.materials[i];
			if (!material || !material->texture) continue;
			list->names.insert(material->texture->name);
			if (static_cast<int>(list->names.size()) >= list->maxTextures) break;
		}

		return atomic;
	}
}

std::string BrlTextureShader::DebugRwObjectTextures(RwObject* rwObject, int maxTextures)
{
	if (!rwObject) return "sem RwObject";
	DebugTextureList list;
	list.maxTextures = std::max(1, maxTextures);
	ForEachAtomic(rwObject, CollectTexturesAtomicCallback, &list);

	std::ostringstream out;
	out << "tipo=" << static_cast<int>(*(uint8_t*) rwObject) << " texturas=" << list.names.size() << ": ";
	bool first = true;
	for (const auto& name : list.names) {
		if (!first) out << ", ";
		out << name;
		first = false;
	}
	return out.str();
}

void BrlTextureShader::Process()
{
	static uint32_t lastProcess = 0;
	uint32_t now = GetTickCount();
	if (now - lastProcess < kProcessIntervalMs) return;
	lastProcess = now;

	if (!pNetGame) return;

	CPlayerPool* playerPool = pNetGame->GetPlayerPool();
	if (playerPool) {
		// Joueurs : appliques au rendu (BeginPedRender), pas ici (materiaux partages).
		(void) playerPool;
	}

	if (CVehiclePool* vehiclePool = pNetGame->GetVehiclePool()) {
		for (uint16_t vehicleId = 0; vehicleId < MAX_VEHICLES; ++vehicleId) {
			CVehicle* vehicle = vehiclePool->GetAt(vehicleId);
			if (vehicle) ApplyToVehicle(vehicleId, vehicle);
		}
	}

	if (CObjectPool* objectPool = pNetGame->GetObjectPool()) {
		for (uint16_t objectId = 0; objectId < MAX_OBJECTS; ++objectId) {
			CObject* object = objectPool->GetAt(objectId);
			if (object) ApplyToObject(objectId, object);
		}
	}
}


// ============================================================
// Application au rendu, ped par ped
// ============================================================
namespace {
	struct PedSwap {
		RpMaterial* material;
		RwTexture* original;
	};
	std::vector<PedSwap> g_pedSwaps;
	std::vector<size_t> g_pedSwapMarks;

	struct PedRenderContext {
		const std::vector<ShaderRule>* lists[2]{};
		uint16_t modelId{};
	};

	RpAtomic* PedRenderAtomicCallback(RpAtomic* atomic, void* data)
	{
		auto* ctx = static_cast<PedRenderContext*>(data);
		if (!atomic || !ctx || *(uint8_t*) atomic != rpATOMIC) return atomic;

		RpGeometry* geometry = atomic->geometry;
		if (!geometry || geometry->matList.numMaterials <= 0 || !geometry->matList.materials) return atomic;

		for (int i = 0; i < geometry->matList.numMaterials; ++i) {
			RpMaterial* material = geometry->matList.materials[i];
			if (!material) continue;
			bool done = false;
			for (const auto* list : ctx->lists) {
				if (!list || done) continue;
				for (const ShaderRule& rule : *list) {
					if (rule.modelId != kAnyShaderModelId && rule.modelId != ctx->modelId) continue;
					if (!MaterialMatches(material, ctx->modelId, i, rule.sourceTexture)) continue;
					RwTexture* replacement = ResolveTexture(rule.replacementTexture);
					if (replacement && replacement != material->texture) {
						g_pedSwaps.push_back({material, material->texture});
						material->texture = replacement;
					}
					done = true;
					break;
				}
			}
		}
		return atomic;
	}

	// Id SA-MP du ped dessine : 0xFFFF = joueur local, -1 = PNJ / inconnu.
	int PlayerIdForPed(void* gtaPed)
	{
		if (!gtaPed) return -1;
		if (pNetGame && pNetGame->GetPlayerPool()) {
			CPlayerPool* pool = pNetGame->GetPlayerPool();
			if (CLocalPlayer* local = pool->GetLocalPlayer()) {
				CPlayerPed* ped = local->GetPlayerPed();
				if (ped && (void*) ped->m_pPed == gtaPed) return kLocalPlayerShaderId;
			}
			for (uint16_t id = 0; id < MAX_PLAYERS; ++id) {
				CRemotePlayer* remote = pool->GetAt(static_cast<PLAYERID>(id));
				if (!remote) continue;
				CPlayerPed* ped = remote->GetPlayerPed();
				if (ped && (void*) ped->m_pPed == gtaPed) return id;
			}
			return -1;
		}
		if (pGame) {
			CPlayerPed* ped = pGame->FindPlayerPed();
			if (ped && (void*) ped->m_pPed == gtaPed) return kLocalPlayerShaderId;
		}
		return -1;
	}
}

void BrlTextureShader::BeginPedRender(void* gtaPed)
{
	g_pedSwapMarks.push_back(g_pedSwaps.size());

	const int playerId = PlayerIdForPed(gtaPed);
	if (playerId < 0) return;

	auto* ped = static_cast<CPedGTA*>(gtaPed);
	if (!ped->m_pRwObject) return;

	CPlayerPed* playerPed = FindPlayerPed(static_cast<uint16_t>(playerId));
	const uint8_t weapon = playerPed ? playerPed->GetCurrentWeapon() : 0;

	std::lock_guard lock(g_shaderMutex);
	PedRenderContext ctx;
	ctx.modelId = static_cast<uint16_t>(ped->m_nModelIndex);
	auto it = g_rules.find({BrlShaderTargetType::Player, static_cast<uint16_t>(playerId), 0});
	if (it != g_rules.end()) ctx.lists[0] = &it->second;
	auto wit = g_rules.find({BrlShaderTargetType::Weapon, static_cast<uint16_t>(playerId), weapon});
	if (wit != g_rules.end()) ctx.lists[1] = &wit->second;
	if (!ctx.lists[0] && !ctx.lists[1]) return;

	ForEachAtomic(ped->m_pRwObject, PedRenderAtomicCallback, &ctx);
}

void BrlTextureShader::EndPedRender()
{
	if (g_pedSwapMarks.empty()) return;
	const size_t mark = g_pedSwapMarks.back();
	g_pedSwapMarks.pop_back();
	while (g_pedSwaps.size() > mark) {
		const PedSwap& swap = g_pedSwaps.back();
		swap.material->texture = swap.original;
		g_pedSwaps.pop_back();
	}
}


// [v15] textures embarquees dans l'APK hors vetements (icones des boutons)
RwTexture* Crestwood_LoadApkTexture(const char* path)
{
	if (!path || !*path) return nullptr;
	struct Retry { int tries = 0; int wait = 0; };
	static std::unordered_map<std::string, RwTexture*> s_apkTextures;
	static std::unordered_map<std::string, Retry> s_apkFails;
	const std::string key(path);
	auto it = s_apkTextures.find(key);
	if (it != s_apkTextures.end()) return it->second;
	if (!pJavaWrapper) return nullptr;
	Retry& r = s_apkFails[key];
	// Ne pas verrouiller definitivement une icone apres quelques echecs transitoires
	// (JNI pas encore pret, activite en cours de demarrage, etc.).
	// On retente au maximum une fois toutes les 120 frames jusqu'au chargement.
	if (r.wait > 0) { r.wait--; return nullptr; }
	RwTexture* tex = LoadSouthTexture(key);
	if (!tex) { if (r.tries < 1000000) r.tries++; r.wait = 120; return nullptr; }
	auto bytes = g_southBytes.find(key);
	if (bytes != g_southBytes.end()) { g_southTotal -= bytes->second; g_southBytes.erase(bytes); }
	tex->refCount += 1000;
	s_apkTextures[key] = tex;
	return tex;
}

RwTexture* BrlTextureShader::GetApkTexture(const std::string& path)
{
	return Crestwood_LoadApkTexture(path.c_str());
}
