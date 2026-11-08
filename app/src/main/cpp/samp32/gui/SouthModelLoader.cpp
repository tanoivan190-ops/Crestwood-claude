//
// Shared character model loading — one mega-DFF + TXD set per gender.
//
// The RenderWare path (memory stream -> TexDictionary merge -> clump ->
// CPedModelInfo::SetClump) follows MTA's CRenderWareSA::ReplacePedModel and is
// the sequence that was already proven to work on this client.
//

#include <dlfcn.h>
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
    bool     failed{false};      // [fix 08/11] un echec n'est PAS retente (relire 5 Mo a chaque image figeait le jeu)
    std::string failReason;
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

// [fix 08/11] Le mega-DFF contient de nombreuses atomics (corps + vetements) : la premiere n'est pas forcement
// la peau animee. GetAnimHierarchyFromSkinClump() s'arrete a la premiere atomic et renvoyait nullptr
// ("no anim hierarchy on DFF"). On parcourt toutes les atomics jusqu'a trouver une hierarchie.
RpAtomic* FindAnyHierarchyCB(RpAtomic* atomic, void* data) {
    RpHAnimHierarchy* hier = RpSkinAtomicGetHAnimHierarchy(atomic);
    if (hier) {
        *reinterpret_cast<RpHAnimHierarchy**>(data) = hier;
        return nullptr;   // trouve : on arrete le parcours
    }
    return atomic;        // sinon on continue
}

RpHAnimHierarchy* FindAnimHierarchyInClump(RpClump* clump) {
    RpHAnimHierarchy* hier = nullptr;
    if (clump) {
        RpClumpForAllAtomics(clump, FindAnyHierarchyCB, &hier);
        if (!hier) {
            // Un DFF fraichement lu n'a pas encore de hierarchie sur ses atomics : le jeu la pose
            // dans CPedModelInfo::SetClump a partir de la hierarchie portee par les FRAMES.
            hier = CHook::CallFunction<RpHAnimHierarchy*>("_Z25GetAnimHierarchyFromClumpP7RpClump", clump);
        }
    }
    return hier;
}

RwTexDictionary* EnsureModelDictionary(CBaseModelInfo* modelInfo) {
    if (!modelInfo) {
        return nullptr;
    }
    const int16 txdIndex = modelInfo->m_nTxdIndex;
    // [fix 08/11] CTxdStore::ms_pTxdPool de NOTRE classe n'est jamais renseigne (il vaut toujours nullptr) :
    // le chargement du mega-skin echouait avec "no texture dictionary for model 7" et le ped restait blanc.
    // On lit donc le vrai pool de textures du jeu (symbole exporte par libGTASA.so).
    static CTxdPool* s_gamePool = nullptr;
    CTxdPool* pool = CTxdStore::ms_pTxdPool;
    if (!pool) {
        if (!s_gamePool) {
            if (void* sym = dlsym(CHook::lib, "_ZN9CTxdStore11ms_pTxdPoolE")) {
                s_gamePool = *reinterpret_cast<CTxdPool**>(sym);
            }
            FLog("[models] pool de textures du jeu : %p", (void*)s_gamePool);
        }
        pool = s_gamePool;
    }
    if (txdIndex < 0 || !pool) {
        return nullptr;
    }
    TxdDef* def = pool->GetAt(txdIndex);
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
    RpHAnimHierarchy* hier = FindAnimHierarchyInClump(source);
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
        FLog("CharacterModelLoader: released %zu ped instance(s) for model %d replacement",
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
        // [fix 08/11] CreateRwObject (slot 8) recree seulement le clump : il manque l'initialisation des
        // animations (RpAnimBlendClumpInit + FillFrameArray) que fait CPed::SetModelIndex. Sans elle, le ped
        // n'a aucune donnee d'animation -> SIGSEGV dans CPed::PlayFootSteps / RpAnimBlendClumpGetFirstAssociation
        // (crestwood_samp_log : fault 0x2c, r0 = 0). On rappelle donc la vraie CPed::SetModelIndex du jeu
        // (comme MTA RestreamPeds), qui cree le clump ET initialise les animations.
        CHook::CallFunction<void>("_ZN4CPed13SetModelIndexEj", ped, (uint32_t)modelId);
        if (ped->m_pRwObject) {
            // Remet la pose de base (sinon le ped reste fige / en T tant qu'aucune animation n'est lancee).
            // ANIM_ID_IDLE = 3 : on appelle la fonction DU JEU (la reimplementation locale de CAnimManager n'est pas fiable ici).
            CHook::CallFunction<void*>("_ZN12CAnimManager14BlendAnimationEP7RpClump12AssocGroupId11AnimationIdf",
                                       reinterpret_cast<RpClump*>(ped->m_pRwObject), (int)ped->m_nAnimGroup, 3, 4.0f);
        }
        if (ped->m_pRwObject) {
            ++recreated;
        } else {
            FLog("CharacterModelLoader: failed to recreate ped %p for model %d", ped, modelId);
        }
    }
    if (!released.empty()) {
        FLog("CharacterModelLoader: recreated %zu/%zu ped instance(s) for model %d",
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
    const bool ok = LoadForModelImpl(modelId, err);
    if (!ok && (modelId == 7 || modelId == 9 || modelId == 12 || modelId == 163)) {
        SLoadedModel& e = s_models[modelId];
        if (!e.loaded && !e.failed) {
            e.failed = true;
            e.failReason = err;
            FLog("[models] mega-skin %d : echec memorise, plus de nouvel essai (%s)", modelId, err.c_str());
        }
    }
    return ok;
}

bool SouthModelLoader::LoadForModelImpl(int modelId, std::string& err) {
    if (!(modelId == 7 || modelId == 9 || modelId == 12 || modelId == 163)) {
        err = "unsupported model " + std::to_string(modelId);
        return false;
    }
    if (IsLoaded(modelId)) {
        err = "already loaded";
        return true;
    }
    {
        auto it = s_models.find(modelId);
        if (it != s_models.end() && it->second.failed) {
            err = it->second.failReason;   // deja tente : on renvoie le meme echec sans rien refaire
            return false;
        }
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
            FLog("CharacterModelLoader: txd '%s' skipped: %s", file.c_str(), txdErr.c_str());
        }
    }

    char dffPath[900];
    snprintf(dffPath, sizeof(dffPath), "%s%s", dir, dffFile.c_str());
    std::string dffErr;
    RpClump* source = LoadDffFile(dffPath, dffErr);
    if (!source) {
        // Aucun ped n'a encore ete libere : rien a restaurer.
        CTxdStore::PopCurrentTxd();
        err = std::string("dff load failed: ") + dffErr;
        return false;
    }

    // [fix 08/11] On valide le DFF AVANT de liberer les peds : liberer puis recreer un ped sans remplacer son
    // modele laissait un ped sans animations -> SIGSEGV dans CPed::PlayFootSteps.
    if (!FindAnimHierarchyInClump(source)) {
        CTxdStore::PopCurrentTxd();
        RpClumpDestroy(source);
        err = "replace failed: no anim hierarchy on DFF (aucun ped touche)";
        return false;
    }

    // Release every existing instance BEFORE the clump under them is replaced.
    std::vector<CPedGTA*> releasedPeds = ReleasePedsUsingModel(modelId);

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
    FLog("CharacterModelLoader: %s", msg);
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

// [v14] objets de l'APK (LSPD, JBL) : includes supplementaires
#include "game/Collision/ColModel.h"
#include "game/BrlTextureShader.h"
#include "game/game.h"

bool SouthModelLoader::PreloadBodies() {
    // [fix] AVANT : nouvel essai toutes les 1,5 s SANS FIN, et chaque essai bloquait le thread de rendu
    // 2 x 3 s (TryLoadModel) -> jeu fige puis plantage du pilote graphique. MAINTENANT : non bloquant,
    // 20 essais maximum (~30 s), puis abandon propre (les objets d'origine restent).
    // [31/10] LSPD (1862) et JBL (2226) de l'APK RETIRES a la demande d'Anzo (plantages au chargement).
    // Plus aucun modele d'objet n'est remplace : on ne touche plus au streaming au demarrage.
    return true;
    static bool s_done = false;
    static uint32_t s_next = 0;
    static int s_tries = 0;
    if (s_done) return true;
    const uint32_t now = GetTickCount();
    if (now < s_next) return false;
    s_next = now + 1500;
    if (PreloadObjects()) { s_done = true; return true; }
    if (++s_tries >= 20) {
        s_done = true;
        FLog("[models] abandon du remplacement des objets APK apres %d essais (modeles d'origine gardes)", s_tries);
    }
    return s_done;
}

// ============================================================
// [v14] Modeles d'OBJETS remplaces depuis l'APK (assets/models/<nom>/model.dff|txd|col),
// une fois au demarrage, avant que le serveur ne cree le moindre objet (comme engineReplaceModel
// + engineReplaceCOL de MTA). Utilise pour le batiment du nouveau LSPD (modele 1862).
// ============================================================
namespace {
struct CwObjModel { int id; const char* asset; float drawDist; };
const CwObjModel kCwObjModels[] = {
    { 1862, "lspd", 1000.0f },   // LSPD Unity Station (Atlas Studio)
    { 2226, "jbl", 150.0f },     // enceinte JBL (objet de l'inventaire)
};
std::unordered_map<int, bool> s_objDone;

bool PrepareAssetDir(const char* asset, char* out, size_t outSize) {
    if (!pJavaWrapper || !javaVM || !asset || !out) return false;
    JNIEnv* env = nullptr;
    if (javaVM->GetEnv(reinterpret_cast<void**>(&env), JNI_VERSION_1_6) != JNI_OK || !env) return false;
    jclass cls = env->GetObjectClass(pJavaWrapper->activity);
    const auto method = cls ? env->GetMethodID(cls, "prepareCustomModel", "(Ljava/lang/String;)Ljava/lang/String;") : nullptr;
    if (env->ExceptionCheck()) env->ExceptionClear();
    if (!method) { if (cls) env->DeleteLocalRef(cls); return false; }
    jstring a = env->NewStringUTF(asset);
    auto path = static_cast<jstring>(env->CallObjectMethod(pJavaWrapper->activity, method, a));
    env->DeleteLocalRef(a); env->DeleteLocalRef(cls);
    if (env->ExceptionCheck()) { env->ExceptionClear(); if (path) env->DeleteLocalRef(path); return false; }
    if (!path) return false;
    const char* chars = env->GetStringUTFChars(path, nullptr);
    if (!chars) { env->DeleteLocalRef(path); return false; }
    const int length = snprintf(out, outSize, "%s/", chars);
    env->ReleaseStringUTFChars(path, chars); env->DeleteLocalRef(path);
    return length > 0 && static_cast<size_t>(length) < outSize;
}

RpAtomic* FirstAtomicCB(RpAtomic* atomic, void* data) {
    auto** out = static_cast<RpAtomic**>(data);
    if (!*out) *out = atomic;
    return atomic;
}

bool LoadColInto(const char* path, CBaseModelInfo* mi, std::string& err) {
    std::vector<uint8_t> blob;
    if (!ReadEntireFile(path, blob, err)) return false;
    if (blob.size() < 40) { err = "col trop petit"; return false; }
    const uint32 size = (uint32)blob[4] | ((uint32)blob[5] << 8) | ((uint32)blob[6] << 16) | ((uint32)blob[7] << 24);
    if (size < 24 || size + 8 > blob.size()) { err = "col : taille invalide"; return false; }
    const char* fourcc = reinterpret_cast<const char*>(blob.data());
    const char* sym = nullptr;
    if (!strncmp(fourcc, "COL3", 4)) sym = "_ZN11CFileLoader22LoadCollisionModelVer3EPhjR9CColModelPKc";
    else if (!strncmp(fourcc, "COL2", 4)) sym = "_ZN11CFileLoader22LoadCollisionModelVer2EPhjR9CColModelPKc";
    else if (!strncmp(fourcc, "COL4", 4)) sym = "_ZN11CFileLoader22LoadCollisionModelVer4EPhjR9CColModelPKc";
    if (!sym) { err = "col : format non gere (COLL)"; return false; }
#if VER_x32
    void* mem = CHook::CallFunction<void*>("_ZN9CColModelnwEj", (uint32)sizeof(CColModel));
#else
    void* mem = CHook::CallFunction<void*>("_ZN9CColModelnwEm", (size_t)sizeof(CColModel));
#endif
    if (!mem) { err = "col : allocation impossible"; return false; }
    CHook::CallFunction<void>("_ZN9CColModelC1Ev", mem);
    auto* col = static_cast<CColModel*>(mem);
    // le tampon doit vivre aussi longtemps que le jeu (le chargeur peut y faire reference)
    auto* data = new uint8_t[size - 24];
    memcpy(data, blob.data() + 32, size - 24);
    CHook::CallFunction<void>(sym, data, (uint32)(size - 24), col, mi->m_modelName);
    col->m_nColSlot = 0;   // slot "generique" : jamais decharge par le streaming des collisions
    CHook::CallFunction<void>("_ZN14CBaseModelInfo11SetColModelEP9CColModelb", mi, col, true);
    return true;
}
} // namespace

bool SouthModelLoader::ReplaceObjectModel(int modelId, const char* asset, float drawDist, std::string& err) {
    char dir[640];
    if (!PrepareAssetDir(asset, dir, sizeof(dir))) { err = std::string("assets models/") + asset + " introuvables"; return false; }
    // non bloquant : on demande le modele au streaming et on revient au prochain essai s'il n'est pas pret
    if (!CStreaming::GetInfo(modelId).IsLoaded()) {
        CStreaming::RequestModel(modelId, STREAMING_GAME_REQUIRED | STREAMING_KEEP_IN_MEMORY);
        err = "modele d'origine pas encore charge (attente du streaming)";
        return false;
    }
    CStreaming::GetInfo(modelId).m_nFlags = (uint8)(CStreaming::GetInfo(modelId).m_nFlags | STREAMING_KEEP_IN_MEMORY | STREAMING_GAME_REQUIRED);
    CBaseModelInfo* mi = CModelInfo::GetModelInfo(modelId);
    if (!mi) { err = "pas de model info"; return false; }
    if (mi->m_pRwObject && *reinterpret_cast<uint8_t*>(mi->m_pRwObject) != 1) { err = "ce modele n'est pas un objet simple (atomic)"; return false; }
    // Textures : le TXD PC (D3D9) n'est pas lisible par le moteur mobile -> textures converties en
    // PNG/JPG (assets/models/<nom>/tex) et posees materiau par materiau (materials.txt = 1 ligne par materiau).
    char path[900];
    CTxdStore::PushCurrentTxd();
    CTxdStore::SetCurrentTxd(mi->m_nTxdIndex);
    snprintf(path, sizeof(path), "%smodel.dff", dir);
    std::string dffErr;
    RpClump* clump = LoadDffFile(path, dffErr);
    CTxdStore::PopCurrentTxd();
    if (!clump) { err = "dff : " + dffErr; return false; }

    RpAtomic* atomic = nullptr;
    RpClumpForAllAtomics(clump, FirstAtomicCB, &atomic);
    if (!atomic) { RpClumpDestroy(clump); err = "dff sans atomic"; return false; }
    CHook::CallFunction<RpClump*>("_Z19RpClumpRemoveAtomicP7RpClumpP8RpAtomic", clump, atomic);
    CHook::CallFunction<RpAtomic*>("_Z16RpAtomicSetFrameP8RpAtomicP7RwFrame", atomic, RwFrameCreate());
    CHook::CallFunction<void>("_ZN18CVisibilityPlugins23SetAtomicRenderCallbackEP8RpAtomicPFS1_S1_E", atomic, nullptr);
    // textures des materiaux (dans l'ordre du DFF)
    int texOk = 0, texWanted = 0;
    {
        std::vector<uint8_t> list;
        std::string lerr;
        snprintf(path, sizeof(path), "%smaterials.txt", dir);
        if (ReadEntireFile(path, list, lerr) && atomic->geometry && atomic->geometry->matList.materials) {
            std::string all(list.begin(), list.end());
            size_t pos = 0;
            for (int i = 0; i < atomic->geometry->matList.numMaterials && pos <= all.size(); ++i) {
                size_t end = all.find('\n', pos);
                if (end == std::string::npos) end = all.size();
                std::string file = all.substr(pos, end - pos);
                pos = end + 1;
                while (!file.empty() && (file.back() == '\r' || file.back() == ' ')) file.pop_back();
                if (file.empty()) continue;
                ++texWanted;
                RwTexture* t = BrlTextureShader::GetApkTexture(std::string("models/") + asset + "/tex/" + file);
                RpMaterial* mat = atomic->geometry->matList.materials[i];
                if (t && mat) { mat->texture = t; ++texOk; }
            }
        }
    }
    CHook::CallFunction<void>("_ZN16CAtomicModelInfo9SetAtomicEP8RpAtomic", mi, atomic);
    RpClumpDestroy(clump);
    if (drawDist > 0.0f) mi->m_fDrawDistance = drawDist;

    snprintf(path, sizeof(path), "%smodel.col", dir);
    std::string colErr = "ok";
    bool colOk = false;
    if (FILE* probe = fopen(path, "rb")) { fclose(probe); colOk = LoadColInto(path, mi, colErr); }
    else colErr = "absente (collision d'origine gardee)";
    char msg[300];
    snprintf(msg, sizeof(msg), "objet %d <- APK '%s' : textures %d/%d, dff ok, col %s", modelId, asset,
             texOk, texWanted, colOk ? "ok" : colErr.c_str());
    err = msg;
    return true;
}

bool SouthModelLoader::PreloadObjects() {
    bool all = true;
    for (const auto& m : kCwObjModels) {
        if (s_objDone[m.id]) continue;
        std::string err;
        if (ReplaceObjectModel(m.id, m.asset, m.drawDist, err)) { s_objDone[m.id] = true; FLog("[models] %s", err.c_str()); }
        else { all = false; FLog("[models] objet %d (%s) : %s", m.id, m.asset, err.c_str()); }
    }
    return all;
}
