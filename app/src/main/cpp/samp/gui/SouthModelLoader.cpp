//
// Shared character model loading — one mega-DFF + TXD set per gender.
//
// The RenderWare path (memory stream -> TexDictionary merge -> clump ->
// CPedModelInfo::SetClump) follows MTA's CRenderWareSA::ReplacePedModel and is
// the sequence that was already proven to work on this client.
//

#include "SouthModelLoader.h"

#include <cstdio>
#include <cstring>
#include <dirent.h>
#include <strings.h>
#include <unordered_map>
#include <vector>


#include "game/Entity/CPedGTA.h"
#include "game/Models/ClumpModelInfo.h"
#include "game/Pools.h"
#include "game/Models/ModelInfo.h"
#include "game/Models/PedModelInfo.h"
#include "game/RW/RenderWare.h"
#include "game/RW/rpworld.h"
#include "game/RW/rwcore.h"
#include "game/RW/stream.h"
#include "game/RW/RwHelper.h"
#include "game/Streaming.h"
#include "game/StreamingInfo.h"
#include "game/TxdDef.h"
#include "game/TxdStore.h"
#include "main.h"
#include "java/jniutil.h"
extern CJavaWrapper* pJavaWrapper;
#include "vendor/armhook/patch.h"

namespace {
struct RwTexDictionary;

// Resolve RenderWare exports through the same hook API used by this engine.
static RwTexture* SouthFindTexture(RwTexDictionary* dict, const char* name) {
    return CHook::CallFunction<RwTexture*>("_Z31RwTexDictionaryFindNamedTextureP15RwTexDictionaryPKc", dict, name);
}
static RwTexture* SouthRemoveTexture(RwTexture* texture) {
    return CHook::CallFunction<RwTexture*>("_Z28RwTexDictionaryRemoveTextureP9RwTexture", texture);
}
static RwTexture* SouthAddTexture(RwTexDictionary* dict, RwTexture* texture) {
    return CHook::CallFunction<RwTexture*>("_Z25RwTexDictionaryAddTextureP15RwTexDictionaryP9RwTexture", dict, texture);
}
static RwTexDictionary* SouthReadTxd(RwStream* stream) {
    return CHook::CallFunction<RwTexDictionary*>("_Z25RwTexDictionaryStreamReadP8RwStream", stream);
}
static void SouthEachTexture(RwTexDictionary* dict, RwTexture* (*callback)(RwTexture*,void*), void* data) {
    CHook::CallFunction<RwTexDictionary*>("_Z29RwTexDictionaryForAllTexturesPK15RwTexDictionaryPFP9RwTextureS3_PvES4_", dict, callback, data);
}
static RpClump* SouthReadClump(RwStream* stream) {
    return CHook::CallFunction<RpClump*>("_Z17RpClumpStreamReadP8RwStream", stream);
}
static RpClump* SouthCloneClump(RpClump* clump) {
    return CHook::CallFunction<RpClump*>("_Z12RpClumpCloneP7RpClump", clump);
}
static RwBool SouthFindChunk(RwStream* stream, RwUInt32 type, RwUInt32* length, RwUInt32* version) {
    return CHook::CallFunction<RwBool>("_Z17RwStreamFindChunkP8RwStreamjPjS1_", stream,type,length,version);
}


constexpr uint32 kRwIdTexDictionary = 0x16; // rwTEXDICTIONARY
constexpr uint32 kRwIdClump         = 0x10; // rwCLUMP

struct SLoadedModel {
    RpClump* sourceClump{nullptr};
    bool     loaded{false};
};

std::unordered_map<int, SLoadedModel> s_models;

// libc read — RwStreamOpen(FILENAME) fails outside the GTASA OS_File root.
bool ReadEntireFile(const char* path, std::vector<uint8_t>& out, std::string& err) {
    out.clear();
    FILE* f = fopen(path, "rb");
    if (!f) {
        err = std::string("fopen failed: ") + path;
        return false;
    }
    fseek(f, 0, SEEK_END);
    const long size = ftell(f);
    fseek(f, 0, SEEK_SET);
    if (size <= 0) {
        fclose(f);
        err = "empty/invalid size";
        return false;
    }
    out.resize((size_t)size);
    const size_t got = fread(out.data(), 1, out.size(), f);
    fclose(f);
    if (got != out.size()) {
        out.clear();
        err = "fread short";
        return false;
    }
    return true;
}

RwStream* OpenMemoryStream(std::vector<uint8_t>& data) {
    if (data.empty()) {
        return nullptr;
    }
    RwMemory mem{};
    mem.start = data.data();
    mem.length = (RwUInt32)data.size();
    return RwStreamOpen(rwSTREAMMEMORY, rwSTREAMREAD, &mem);
}

RwTexture* MoveTextureToDictCB(RwTexture* texture, void* data) {
    auto* dest = static_cast<RwTexDictionary*>(data);
    if (!texture || !dest) {
        return texture;
    }
    if (RwTexture* existing = SouthFindTexture(dest, texture->name)) {
        SouthRemoveTexture(existing);
    }
    SouthRemoveTexture(texture);
    SouthAddTexture(dest, texture);
    return texture;
}

bool MergeTxdFileIntoDict(const char* path, RwTexDictionary* dest, std::string& err) {
    if (!path || !dest) {
        err = "null args";
        return false;
    }
    std::vector<uint8_t> blob;
    if (!ReadEntireFile(path, blob, err)) {
        return false;
    }
    RwStream* stream = OpenMemoryStream(blob);
    if (!stream) {
        err = "RwStreamOpen MEMORY failed (txd)";
        return false;
    }
    if (!SouthFindChunk(stream, kRwIdTexDictionary, nullptr, nullptr)) {
        RwStreamClose(stream, nullptr);
        err = "FindChunk TEXDICTIONARY failed";
        return false;
    }
    RwTexDictionary* src = SouthReadTxd(stream);
    RwStreamClose(stream, nullptr);
    if (!src) {
        err = "RwTexDictionaryStreamRead null";
        return false;
    }
    SouthEachTexture(src, MoveTextureToDictCB, dest);
    CHook::CallFunction<void>("_Z22RwTexDictionaryDestroyP15RwTexDictionary", src);
    return true;
}

RpClump* LoadDffFile(const char* path, std::string& err) {
    std::vector<uint8_t> blob;
    if (!ReadEntireFile(path, blob, err)) {
        return nullptr;
    }
    if (blob.size() < 12) {
        err = "dff too small";
        return nullptr;
    }
    const uint32 rootType = (uint32)blob[0] | ((uint32)blob[1] << 8) | ((uint32)blob[2] << 16) |
                            ((uint32)blob[3] << 24);
    if (rootType != kRwIdClump) {
        char buf[64];
        snprintf(buf, sizeof(buf), "bad root chunk 0x%X (want 0x10)", rootType);
        err = buf;
        return nullptr;
    }

    RwStream* stream = OpenMemoryStream(blob);
    if (!stream) {
        err = "RwStreamOpen MEMORY failed (dff)";
        return nullptr;
    }
    if (!SouthFindChunk(stream, kRwIdClump, nullptr, nullptr)) {
        RwStreamClose(stream, nullptr);
        stream = OpenMemoryStream(blob);
        if (!stream) {
            err = "FindChunk CLUMP failed + reopen failed";
            return nullptr;
        }
        RpClump* direct = SouthReadClump(stream);
        RwStreamClose(stream, nullptr);
        if (!direct) {
            err = "FindChunk failed and RpClumpStreamRead null";
            return nullptr;
        }
        err = "ok (direct StreamRead)";
        return direct;
    }
    RpClump* clump = SouthReadClump(stream);
    RwStreamClose(stream, nullptr);
    if (!clump) {
        err = "RpClumpStreamRead returned null (after FindChunk)";
        return nullptr;
    }
    err = "ok";
    return clump;
}

RwTexDictionary* EnsureModelDictionary(CBaseModelInfo* modelInfo) {
    if (!modelInfo) {
        return nullptr;
    }
    const int16 txdIndex = modelInfo->m_nTxdIndex;
    if (txdIndex < 0 || !CTxdStore::ms_pTxdPool) {
        return nullptr;
    }
    TxdDef* def = CTxdStore::ms_pTxdPool->GetAt(txdIndex);
    if (!def) {
        return nullptr;
    }
    if (!def->m_pRwDictionary) {
        def->m_pRwDictionary =
            reinterpret_cast<uintptr*>(CHook::CallFunction<RwTexDictionary*>("_Z21RwTexDictionaryCreatev"));
    }
    return reinterpret_cast<RwTexDictionary*>(def->m_pRwDictionary);
}

// MTA CRenderWareSA::ReplacePedModel: hierarchy check, double clone (render
// order flips per clone), SetClump, destroy the previous model-info clump.
bool ReplacePedModel(CBaseModelInfo* modelInfo, RpClump* source, std::string& err) {
    if (!modelInfo || !source) {
        err = "null modelInfo/clump";
        return false;
    }
    RpHAnimHierarchy* hier = GetAnimHierarchyFromSkinClump(source);
    if (!hier) {
        hier = GetAnimHierarchyFromSkinClump(source);
    }
    if (!hier) {
        err = "no anim hierarchy on DFF (MTA refuses ReplacePedModel)";
        return false;
    }

    auto* oldClump = reinterpret_cast<RpClump*>(modelInfo->m_pRwObject);
    if (oldClump == source) {
        err = "source is already model-info clump";
        return true;
    }

    RpClump* temp = SouthCloneClump(source);
    if (!temp) {
        err = "RpClumpClone (1st) failed";
        return false;
    }
    RpClump* clone = SouthCloneClump(temp);
    RpClumpDestroy(temp);
    if (!clone) {
        err = "RpClumpClone (2nd) failed";
        return false;
    }

    CHook::CallFunction<void>("_ZN13CPedModelInfo8SetClumpEP7RpClump",
                              reinterpret_cast<CPedModelInfo*>(modelInfo), clone);
    if (!modelInfo->m_pRwObject) {
        RpClumpDestroy(clone);
        err = "SetClump left model info without RW object";
        return false;
    }
    if (oldClump && oldClump != clone && oldClump != source) {
        RpClumpDestroy(oldClump);
    }
    err = "ok (mta ReplacePedModel)";
    return true;
}

// Drop the RW instance of every ped currently using this model.
//
// ReplacePedModel destroys the model-info clump, and any ped already instanced
// from it keeps pointing at the freed geometry — the next render then walks
// dead memory. CustomDrop used models 14/10, which almost nothing spawns as,
// so a local-ped-only guard was enough there. The DFS set uses 7 and 9, which
// ARE common pedestrian models, so every instance has to be released.
//
// MTA does the same thing under the name RestreamPeds.
std::vector<CPedGTA*> ReleasePedsUsingModel(int modelId) {
    std::vector<CPedGTA*> released;
    auto* pool = CPools::ms_pPedPool;
    if (!pool) {
        return released;
    }
    const int32 size = pool->GetSize();
    for (int32 i = 0; i < size; ++i) {
        CPedGTA* ped = pool->GetAt(i);
        if (!ped || (int)ped->m_nModelIndex != modelId) {
            continue;
        }
        if (ped->m_pRwObject) {
            ped->DeleteRwObject();
            released.push_back(ped);
        }
        ped->m_apBones.fill(nullptr);
    }
    if (!released.empty()) {
        Log("CharacterModelLoader: released %zu ped instance(s) for model %d replacement",
            released.size(), modelId);
    }
    return released;
}

void RecreateReleasedPeds(const std::vector<CPedGTA*>& released, int modelId) {
    size_t recreated = 0;
    for (CPedGTA* ped : released) {
        // The ped pool owns these objects; replacing model-info does not remove
        // the CPedGTA itself.  Recreate the RW clump immediately so no render or
        // ProcessControl pass can dereference the clump destroyed above.
        if (!ped || (int)ped->m_nModelIndex != modelId) {
            continue;
        }
        // CEntityGTA::DeleteRwObject uses slot 9; CreateRwObject is slot 8.
        const auto table = *reinterpret_cast<uintptr_t**>(ped);
        reinterpret_cast<void (*)(CPedGTA*)>(table[8])(ped);
        if (ped->m_pRwObject) {
            ++recreated;
        } else {
            Log("CharacterModelLoader: failed to recreate ped %p for model %d", ped, modelId);
        }
    }
    if (!released.empty()) {
        Log("CharacterModelLoader: recreated %zu/%zu ped instance(s) for model %d",
            recreated, released.size(), modelId);
    }
}

bool EndsWithIgnoreCase(const char* name, const char* ext) {
    if (!name || !ext) {
        return false;
    }
    const size_t n = strlen(name), e = strlen(ext);
    return n >= e && strcasecmp(name + n - e, ext) == 0;
}

} // namespace

bool SouthModelLoader::GetSkinDir(int modelId, char* out, size_t outSize) {
    const char* gender = (modelId == 7 || modelId == 163) ? "male" : (modelId == 9 || modelId == 12) ? "female" : nullptr;
    if (!gender || !out) {
        return false;
    }
    if (!pJavaWrapper || !javaVM) return false;
    JNIEnv* env = nullptr;
    if (javaVM->GetEnv(reinterpret_cast<void**>(&env), JNI_VERSION_1_6) != JNI_OK || !env) return false;
    jclass cls = env->GetObjectClass(pJavaWrapper->activity);
    const auto method = cls ? env->GetMethodID(cls, "prepareClothesModel", "(Ljava/lang/String;)Ljava/lang/String;") : nullptr;
    if (env->ExceptionCheck()) env->ExceptionClear();
    if (!method) { if (cls) env->DeleteLocalRef(cls); return false; }
    jstring g = env->NewStringUTF(gender);
    auto path = static_cast<jstring>(env->CallObjectMethod(pJavaWrapper->activity, method, g));
    env->DeleteLocalRef(g); env->DeleteLocalRef(cls);
    if (env->ExceptionCheck()) { env->ExceptionClear(); if (path) env->DeleteLocalRef(path); return false; }
    if (!path) return false;
    const char* chars = env->GetStringUTFChars(path, nullptr);
    if (!chars) { env->DeleteLocalRef(path); return false; }
    const int length = snprintf(out, outSize, "%s/", chars);
    env->ReleaseStringUTFChars(path, chars); env->DeleteLocalRef(path);
    if (length < 0 || static_cast<size_t>(length) >= outSize) return false;
    return true;
}

bool SouthModelLoader::IsLoaded(int modelId) {
    auto it = s_models.find(modelId);
    return it != s_models.end() && it->second.loaded;
}

bool SouthModelLoader::LoadForModel(int modelId, std::string& err) {
    if (!(modelId == 7 || modelId == 9 || modelId == 12 || modelId == 163)) {
        err = "unsupported model " + std::to_string(modelId);
        return false;
    }
    if (IsLoaded(modelId)) {
        err = "already loaded";
        return true;
    }

    char dir[640];
    if (!GetSkinDir(modelId, dir, sizeof(dir))) {
        err = "cannot resolve skin dir";
        return false;
    }

    DIR* handle = opendir(dir);
    if (!handle) {
        err = std::string("cannot open '") + dir + "'";
        return false;
    }
    std::vector<std::string> txdFiles;
    std::string dffFile;
    while (dirent* entry = readdir(handle)) {
        const char* name = entry->d_name;
        if (name[0] == '.') {
            continue;
        }
        if (EndsWithIgnoreCase(name, ".txd")) {
            txdFiles.emplace_back(name);
        } else if (EndsWithIgnoreCase(name, ".dff") && dffFile.empty()) {
            dffFile = name;
        }
    }
    closedir(handle);

    if (dffFile.empty()) {
        err = std::string("no .dff in '") + dir + "'";
        return false;
    }

    // The model must be streamed in before its model info / TXD slot is usable.
    if (!CStreaming::TryLoadModel(modelId)) {
        err = "CStreaming::TryLoadModel failed for " + std::to_string(modelId);
        return false;
    }
    CStreaming::GetInfo(modelId).m_nFlags = (uint8)(CStreaming::GetInfo(modelId).m_nFlags |
                                                   STREAMING_KEEP_IN_MEMORY | STREAMING_GAME_REQUIRED);

    CBaseModelInfo* modelInfo = CModelInfo::GetModelInfo(modelId);
    if (!modelInfo) {
        err = "no model info for " + std::to_string(modelId);
        return false;
    }
    RwTexDictionary* dict = EnsureModelDictionary(modelInfo);
    if (!dict) {
        err = "no texture dictionary for model " + std::to_string(modelId);
        return false;
    }

    // Release every existing instance BEFORE the clump under them is replaced.
    std::vector<CPedGTA*> releasedPeds = ReleasePedsUsingModel(modelId);

    // RpClumpStreamRead resolves each material's texture BY NAME against the
    // *current* texture dictionary (same as MTA's ReadDFF). Without this the
    // materials come back with null textures and the ped renders solid white.
    CTxdStore::PushCurrentTxd();
    CTxdStore::SetCurrentTxd(modelInfo->m_nTxdIndex);
    CHook::CallFunction<RwTexDictionary*>("_Z25RwTexDictionarySetCurrentP15RwTexDictionary", dict);

    size_t mergedTxd = 0;
    for (const std::string& file : txdFiles) {
        char path[900];
        snprintf(path, sizeof(path), "%s%s", dir, file.c_str());
        std::string txdErr;
        if (MergeTxdFileIntoDict(path, dict, txdErr)) {
            ++mergedTxd;
        } else {
            Log("CharacterModelLoader: txd '%s' skipped: %s", file.c_str(), txdErr.c_str());
        }
    }

    char dffPath[900];
    snprintf(dffPath, sizeof(dffPath), "%s%s", dir, dffFile.c_str());
    std::string dffErr;
    RpClump* source = LoadDffFile(dffPath, dffErr);
    if (!source) {
        // ReleasePedsUsingModel() already removed the live instances.  Restore
        // them even if the replacement DFF is corrupt/missing, otherwise the
        // next render or ProcessControl pass dereferences a missing clump.
        RecreateReleasedPeds(releasedPeds, modelId);
        CTxdStore::PopCurrentTxd();
        err = std::string("dff load failed: ") + dffErr;
        return false;
    }

    std::string replaceErr;
    if (!ReplacePedModel(modelInfo, source, replaceErr)) {
        // Keep existing players valid even when replacement fails.  They can
        // safely use the model-info object that remained active.
        RecreateReleasedPeds(releasedPeds, modelId);
        CTxdStore::PopCurrentTxd();
        RpClumpDestroy(source);
        err = std::string("replace failed: ") + replaceErr;
        return false;
    }
    RecreateReleasedPeds(releasedPeds, modelId);
    CTxdStore::PopCurrentTxd();

    // Keep the source clump alive: cloned geometry references it, and a later
    // re-replace (model reload) reuses it instead of re-reading from disk.
    SLoadedModel& entry = s_models[modelId];
    if (entry.sourceClump && entry.sourceClump != source) {
        RpClumpDestroy(entry.sourceClump);
    }
    entry.sourceClump = source;
    entry.loaded = true;

    char msg[256];
    snprintf(msg, sizeof(msg), "model %d ready: %zu/%zu txd merged, dff '%s'", modelId, mergedTxd,
             txdFiles.size(), dffFile.c_str());
    err = msg;
    Log("CharacterModelLoader: %s", msg);
    return true;
}

void SouthModelLoader::ReleaseSource(int modelId) {
    auto it = s_models.find(modelId);
    if (it == s_models.end()) {
        return;
    }
    if (it->second.sourceClump) {
        RpClumpDestroy(it->second.sourceClump);
        it->second.sourceClump = nullptr;
    }
}

void SouthModelLoader::Shutdown() {
    for (auto& [modelId, entry] : s_models) {
        (void)modelId;
        if (entry.sourceClump) {
            RpClumpDestroy(entry.sourceClump);
        }
    }
    s_models.clear();
}

// [31/10] LSPD (1862) et JBL (2226) MTA retires : plus aucun modele d'objet n'est remplace au demarrage.
bool SouthModelLoader::PreloadBodies() { return true; }
