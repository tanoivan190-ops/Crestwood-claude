// ============================================================================================
// [Crestwood v47] MODELES 3D DES MAPPINGS MTA, CHARGES DEPUIS L'APK
//   assets/mta/models.txt           id;distance;nom;x,y;x,y...   (tools/mta/mta_vers_apk.py)
//   assets/mta/<id>/model.dff       modele (comme engineReplaceModel de MTA)
//   assets/mta/<id>/model.txd       textures (TXD 32 bits lisible par le jeu mobile)
//   assets/mta/<id>/model.col       collision (COL2/3/4, facultative)
//   assets/mta/<id>/materials.txt   nom de la texture de chaque materiau (filet de securite)
//
// Securite (les essais precedents faisaient planter) :
//   - RIEN au demarrage : un modele n'est remplace que quand le joueur est a moins de ~400 m d'un
//     endroit ou il est place (positions des .map + batiments du jeu trouves dans les pools) ;
//   - un seul modele a la fois, puis 20 s d'observation. Si le jeu plante pendant ces 20 s, le
//     modele est mis sur liste noire au lancement suivant (files/crestwood-mta/off.txt) ;
//     4 plantages = tout est coupe (le jeu reste jouable avec les modeles d'origine) ;
//   - budget memoire (110 Mo en 64 bits, 48 Mo en 32 bits) : au-dela on garde les modeles d'origine.
// Journal : lignes "[mta]" dans crestwood_log.txt.
// ============================================================================================
#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <cmath>
#include <dlfcn.h>
#include <string>
#include <vector>
#include <unordered_map>
#include <sys/stat.h>
#include <unistd.h>

#include "game/Pools.h"
#include "game/Entity/CPedGTA.h"
#include "game/Models/ModelInfo.h"
#include "game/Collision/ColModel.h"
#include "game/RW/RenderWare.h"
#include "game/RW/rpworld.h"
#include "game/RW/rwcore.h"
#include "game/RW/stream.h"
#include "game/Streaming.h"
#include "game/StreamingInfo.h"
#include "game/TxdStore.h"
#include "game/util.h"
#include "main.h"
#include "java/jniutil.h"
#include "vendor/armhook/patch.h"

struct RwTexDictionary;
extern CJavaWrapper* pJavaWrapper;
extern JavaVM* javaVM;
void CwNativeLog(const char* fmt, ...);   // gui/SouthModelLoader.cpp

namespace {

struct MtaModel {
    int id = 0;
    float dist = 0;
    std::string name;
    std::vector<std::pair<float, float>> pos;
    int state = 0;            // 0 en attente, 1 modele d'origine demande, 2 remplace, -1 abandonne
    uint32_t reqAt = 0;
};

std::vector<MtaModel> s_list;
bool s_init = false, s_off = false, s_scanned = false;
std::string s_stateDir;
long s_used = 0, s_budget = 0;
int s_guardId = 0;
uint32_t s_guardUntil = 0, s_next = 0, s_spawnAt = 0, s_objScanAt = 0;
std::unordered_map<int, RwTexDictionary*> s_dicts;

// ------------------------------------------------------------------ Java
std::string JavaStr(const char* method, const char* arg) {
    std::string out;
    if (!pJavaWrapper || !javaVM) return out;
    JNIEnv* env = nullptr;
    if (javaVM->GetEnv(reinterpret_cast<void**>(&env), JNI_VERSION_1_6) != JNI_OK || !env) return out;
    jclass cls = env->GetObjectClass(pJavaWrapper->activity);
    jmethodID m = cls ? env->GetMethodID(cls, method, "(Ljava/lang/String;)Ljava/lang/String;") : nullptr;
    if (env->ExceptionCheck()) env->ExceptionClear();
    if (m) {
        jstring a = env->NewStringUTF(arg ? arg : "");
        auto r = static_cast<jstring>(env->CallObjectMethod(pJavaWrapper->activity, m, a));
        if (a) env->DeleteLocalRef(a);
        if (env->ExceptionCheck()) { env->ExceptionClear(); r = nullptr; }
        if (r) {
            const char* c = env->GetStringUTFChars(r, nullptr);
            if (c) { out = c; env->ReleaseStringUTFChars(r, c); }
            env->DeleteLocalRef(r);
        }
    }
    if (cls) env->DeleteLocalRef(cls);
    return out;
}

// ------------------------------------------------------------------ fichiers
bool ReadFile(const std::string& path, std::vector<uint8_t>& out) {
    out.clear();
    FILE* f = fopen(path.c_str(), "rb");
    if (!f) return false;
    fseek(f, 0, SEEK_END);
    long n = ftell(f);
    fseek(f, 0, SEEK_SET);
    if (n <= 0 || n > 96L * 1024 * 1024) { fclose(f); return false; }
    out.resize((size_t)n);
    bool ok = fread(out.data(), 1, out.size(), f) == out.size();
    fclose(f);
    return ok;
}
long FileSize(const std::string& p) { struct stat st{}; return stat(p.c_str(), &st) == 0 ? (long)st.st_size : 0; }
void WriteText(const std::string& p, const std::string& t, bool append) {
    FILE* f = fopen(p.c_str(), append ? "a" : "w");
    if (!f) return;
    fwrite(t.data(), 1, t.size(), f);
    fflush(f);
    fsync(fileno(f));
    fclose(f);
}
std::string ReadText(const std::string& p) {
    std::vector<uint8_t> b;
    if (!ReadFile(p, b)) return "";
    return std::string(b.begin(), b.end());
}

void* Sym(const char* s) { return CHook::lib ? dlsym(CHook::lib, s) : nullptr; }

RwStream* MemStream(std::vector<uint8_t>& d) {
    RwMemory mem{};
    mem.start = d.data();
    mem.length = (RwUInt32)d.size();
    return RwStreamOpen(rwSTREAMMEMORY, rwSTREAMREAD, &mem);
}

RwTexDictionary* LoadTxd(const std::string& path, std::string& err) {
    std::vector<uint8_t> blob;
    if (!ReadFile(path, blob) || blob.size() < 12) { err = "txd illisible"; return nullptr; }
    RwStream* s = MemStream(blob);
    if (!s) { err = "txd : flux"; return nullptr; }
    RwTexDictionary* d = nullptr;
    if (CHook::CallFunction<RwBool>("_Z17RwStreamFindChunkP8RwStreamjPjS1_", s, (RwUInt32)0x16, (RwUInt32*)nullptr, (RwUInt32*)nullptr))
        d = CHook::CallFunction<RwTexDictionary*>("_Z25RwTexDictionaryStreamReadP8RwStream", s);
    RwStreamClose(s, nullptr);
    if (!d) err = "txd : lecture impossible";
    return d;
}

RpClump* LoadDff(const std::string& path, std::string& err) {
    std::vector<uint8_t> blob;
    if (!ReadFile(path, blob) || blob.size() < 12) { err = "dff illisible"; return nullptr; }
    if (blob[0] != 0x10 || blob[1] || blob[2] || blob[3]) { err = "dff : pas un clump"; return nullptr; }
    RwStream* s = MemStream(blob);
    if (!s) { err = "dff : flux"; return nullptr; }
    RpClump* c = nullptr;
    if (CHook::CallFunction<RwBool>("_Z17RwStreamFindChunkP8RwStreamjPjS1_", s, (RwUInt32)0x10, (RwUInt32*)nullptr, (RwUInt32*)nullptr))
        c = CHook::CallFunction<RpClump*>("_Z17RpClumpStreamReadP8RwStream", s);
    RwStreamClose(s, nullptr);
    if (!c) err = "dff : lecture impossible";
    return c;
}

bool LoadCol(const std::string& path, CBaseModelInfo* mi, std::string& err) {
    std::vector<uint8_t> blob;
    if (!ReadFile(path, blob) || blob.size() < 40) { err = "col illisible"; return false; }
    const uint32 size = (uint32)blob[4] | ((uint32)blob[5] << 8) | ((uint32)blob[6] << 16) | ((uint32)blob[7] << 24);
    if (size < 24 || size + 8 > blob.size()) { err = "col : taille"; return false; }
    const char* sym = nullptr;
    if (!memcmp(blob.data(), "COL3", 4)) sym = "_ZN11CFileLoader22LoadCollisionModelVer3EPhjR9CColModelPKc";
    else if (!memcmp(blob.data(), "COL2", 4)) sym = "_ZN11CFileLoader22LoadCollisionModelVer2EPhjR9CColModelPKc";
    else if (!memcmp(blob.data(), "COL4", 4)) sym = "_ZN11CFileLoader22LoadCollisionModelVer4EPhjR9CColModelPKc";
    if (!sym || !Sym(sym)) { err = "col : format non gere"; return false; }
#if VER_x32
    const char* newSym = "_ZN9CColModelnwEj";
#else
    const char* newSym = "_ZN9CColModelnwEm";
#endif
    if (!Sym(newSym) || !Sym("_ZN9CColModelC1Ev") || !Sym("_ZN14CBaseModelInfo11SetColModelEP9CColModelb")) { err = "col : fonctions absentes"; return false; }
    void* mem = CHook::CallFunction<void*>(newSym, (size_t)sizeof(CColModel));
    if (!mem) { err = "col : memoire"; return false; }
    CHook::CallFunction<void>("_ZN9CColModelC1Ev", mem);
    auto* col = static_cast<CColModel*>(mem);
    auto* data = new uint8_t[size - 24];            // garde vivant : le chargeur peut y faire reference
    memcpy(data, blob.data() + 32, size - 24);
    CHook::CallFunction<void>(sym, data, (uint32)(size - 24), col, mi->m_modelName);
    col->m_nColSlot = 0;
    CHook::CallFunction<void>("_ZN14CBaseModelInfo11SetColModelEP9CColModelb", mi, col, true);
    return true;
}

RpAtomic* FirstAtomicCB(RpAtomic* a, void* data) {
    auto** out = static_cast<RpAtomic**>(data);
    if (!*out) *out = a;
    return a;
}

// ------------------------------------------------------------------ etat (garde anti-plantage)
void LoadState() {
    s_stateDir = JavaStr("mtaStateDir", "");
    if (s_stateDir.empty()) { s_off = true; CwNativeLog("[mta] dossier d'etat introuvable : modeles MTA coupes"); return; }
    std::string guard = s_stateDir + "/guard.txt", offf = s_stateDir + "/off.txt";
    std::string g = ReadText(guard);
    if (!g.empty()) {
        int id = atoi(g.c_str());
        if (id > 0) {
            WriteText(offf, std::to_string(id) + "\n", true);
            CwNativeLog("[mta] le modele %d a fait planter le jeu la derniere fois : desactive", id);
        }
        unlink(guard.c_str());
    }
    std::string off = ReadText(offf);
    int n = 0;
    for (size_t p = 0; p < off.size();) {
        size_t e = off.find('\n', p); if (e == std::string::npos) e = off.size();
        int id = atoi(off.substr(p, e - p).c_str());
        if (id > 0) {
            n++;
            for (auto& m : s_list) if (m.id == id) m.state = -1;
        }
        p = e + 1;
    }
    if (n >= 4) { s_off = true; CwNativeLog("[mta] %d modeles ont fait planter le jeu : tous les modeles MTA sont coupes (vider le cache de l'appli pour reessayer)", n); }
    if (access((s_stateDir + "/disable.txt").c_str(), F_OK) == 0) { s_off = true; CwNativeLog("[mta] coupe par disable.txt"); }
}

void Init() {
    s_init = true;
#if VER_x32
    s_budget = 48L * 1024 * 1024;
#else
    s_budget = 110L * 1024 * 1024;
#endif
    std::string txt = JavaStr("mtaModelsList", "");
    if (txt.empty()) { s_off = true; return; }
    for (size_t p = 0; p < txt.size();) {
        size_t e = txt.find('\n', p); if (e == std::string::npos) e = txt.size();
        std::string line = txt.substr(p, e - p);
        p = e + 1;
        while (!line.empty() && (line.back() == '\r' || line.back() == ' ')) line.pop_back();
        if (line.empty() || line[0] == '#') continue;
        MtaModel m;
        std::vector<std::string> parts;
        for (size_t a = 0; a <= line.size();) {
            size_t b = line.find(';', a); if (b == std::string::npos) b = line.size();
            parts.push_back(line.substr(a, b - a));
            a = b + 1;
        }
        if (parts.size() < 3) continue;
        m.id = atoi(parts[0].c_str());
        m.dist = (float)atof(parts[1].c_str());
        m.name = parts[2];
        for (size_t k = 3; k < parts.size(); k++) {
            float x, y;
            if (sscanf(parts[k].c_str(), "%f,%f", &x, &y) == 2) m.pos.push_back({x, y});
        }
        if (m.id <= 0 || m.id >= 20000) continue;
        bool dup = false;
        for (auto& o : s_list) if (o.id == m.id) dup = true;
        if (!dup) s_list.push_back(m);
    }
    LoadState();
    CwNativeLog("[mta] %d modele(s) MTA dans l'APK%s", (int)s_list.size(), s_off ? " (COUPES)" : "");
}

// batiments du jeu (et faux-objets) qui utilisent ces modeles : leurs positions = ou charger
template <typename PoolT>
void ScanPool(PoolT* pool, std::unordered_map<int, MtaModel*>& want) {
    if (!pool) return;
    for (int i = 0; i < pool->GetSize(); i++) {
        auto* e = pool->GetAt(i);
        if (!e) continue;
        auto it = want.find((int)e->m_nModelIndex);
        if (it == want.end()) continue;
        const CVector& v = e->GetPosition();
        auto& pos = it->second->pos;
        bool near = false;
        for (auto& p : pos) if ((p.first - v.x) * (p.first - v.x) + (p.second - v.y) * (p.second - v.y) < 150.f * 150.f) { near = true; break; }
        if (!near && pos.size() < 24) pos.push_back({v.x, v.y});
    }
}

template <typename PoolT>
void RefreshPool(PoolT* pool, int id, int& n) {
    if (!pool) return;
    for (int i = 0; i < pool->GetSize(); i++) {
        auto* e = pool->GetAt(i);
        if (!e || (int)e->m_nModelIndex != id || !e->m_pRwObject) continue;
        e->DeleteRwObject();   // le moteur le recree avec le nouveau modele (comme apres DeleteRwObjectsBehindCamera)
        n++;
    }
}

bool Replace(MtaModel& m, std::string& err) {
    CBaseModelInfo* mi = CModelInfo::GetModelInfo(m.id);
    if (!mi) { err = "pas de model info"; return false; }
    if (!mi->m_pRwObject || *reinterpret_cast<uint8_t*>(mi->m_pRwObject) != 1) { err = "pas un objet simple (atomic)"; return false; }
    std::string dir = JavaStr("prepareMtaModel", std::to_string(m.id).c_str());
    if (dir.empty()) { err = "fichiers de l'APK introuvables"; return false; }
    dir += "/";

    RwTexDictionary* dict = nullptr;
    if (FileSize(dir + "model.txd") > 0) {
        std::string terr;
        dict = LoadTxd(dir + "model.txd", terr);
        if (!dict) CwNativeLog("[mta] %d : %s (textures d'origine)", m.id, terr.c_str());
    }
    CTxdStore::PushCurrentTxd();
    CTxdStore::SetCurrentTxd(mi->m_nTxdIndex);
    if (dict) CHook::CallFunction<RwTexDictionary*>("_Z25RwTexDictionarySetCurrentP15RwTexDictionary", dict);
    std::string derr;
    RpClump* clump = LoadDff(dir + "model.dff", derr);
    CTxdStore::PopCurrentTxd();
    if (!clump) { err = derr; return false; }

    RpAtomic* atomic = nullptr;
    RpClumpForAllAtomics(clump, FirstAtomicCB, &atomic);
    if (!atomic || !atomic->geometry) { RpClumpDestroy(clump); err = "dff sans atomic"; return false; }
    CHook::CallFunction<RpClump*>("_Z19RpClumpRemoveAtomicP7RpClumpP8RpAtomic", clump, atomic);
    CHook::CallFunction<RpAtomic*>("_Z16RpAtomicSetFrameP8RpAtomicP7RwFrame", atomic, RwFrameCreate());
    CHook::CallFunction<void>("_ZN18CVisibilityPlugins23SetAtomicRenderCallbackEP8RpAtomicPFS1_S1_E", atomic, nullptr);

    // filet de securite : texture de chaque materiau par son nom (materials.txt)
    int fixed = 0, mats = atomic->geometry->matList.numMaterials;
    void* setTex = Sym("_Z20RpMaterialSetTextureP10RpMaterialP9RwTexture");
    if (dict && setTex) {
        std::string list = ReadText(dir + "materials.txt");
        size_t p = 0;
        for (int i = 0; i < mats && p < list.size(); i++) {
            size_t e = list.find('\n', p); if (e == std::string::npos) e = list.size();
            std::string nm = list.substr(p, e - p);
            p = e + 1;
            while (!nm.empty() && (nm.back() == '\r' || nm.back() == ' ')) nm.pop_back();
            RpMaterial* mat = atomic->geometry->matList.materials[i];
            if (nm.empty() || !mat) continue;
            RwTexture* t = CHook::CallFunction<RwTexture*>("_Z31RwTexDictionaryFindNamedTextureP15RwTexDictionaryPKc", dict, nm.c_str());
            if (t && mat->texture != t) { ((RpMaterial* (*)(RpMaterial*, RwTexture*))setTex)(mat, t); fixed++; }
        }
    }
    CHook::CallFunction<void>("_ZN16CAtomicModelInfo9SetAtomicEP8RpAtomic", mi, atomic);
    RpClumpDestroy(clump);
    if (m.dist > 0) mi->m_fDrawDistance = m.dist > 1500.f ? 1500.f : m.dist;

    std::string cerr = "absente";
    bool colOk = false;
    if (FileSize(dir + "model.col") > 0) colOk = LoadCol(dir + "model.col", mi, cerr);
    if (dict) s_dicts[m.id] = dict;   // garde en vie (les materiaux pointent dessus)

    int refreshed = 0;
    RefreshPool(GetBuildingPool(), m.id, refreshed);
    RefreshPool(GetDummyPool(), m.id, refreshed);
    char buf[260];
    snprintf(buf, sizeof buf, "modele %d (%s) remplace : %d materiaux, %d corriges, collision %s, %d batiment(s) rafraichi(s)",
             m.id, m.name.c_str(), mats, fixed, colOk ? "ok" : cerr.c_str(), refreshed);
    err = buf;
    return true;
}

} // namespace

// appele a chaque image depuis SouthModelLoader::PreloadBodies (jeu lance)
void MtaModels_Tick() {
    const uint32_t now = GetTickCount();
    if (now < s_next) return;
    s_next = now + 250;
    if (!s_init) Init();
    if (s_off || s_list.empty()) return;

    // fin de la periode d'observation : le modele n'a pas fait planter le jeu
    if (s_guardId && now >= s_guardUntil) {
        unlink((s_stateDir + "/guard.txt").c_str());
        s_guardId = 0;
    }
    if (s_guardId) return;   // un seul modele a la fois

    CPedGTA* ped = GamePool_FindPlayerPed();
    if (!ped) return;
    const CVector& me = ped->GetPosition();
    if (fabsf(me.x) < 1.f && fabsf(me.y) < 1.f) return;
    if (!s_spawnAt) { s_spawnAt = now; return; }
    if (now - s_spawnAt < 6000) return;          // laisse le monde se charger apres l'apparition

    if (!s_scanned) {
        s_scanned = true;
        std::unordered_map<int, MtaModel*> want;
        for (auto& m : s_list) if (m.state == 0) want[m.id] = &m;
        ScanPool(GetBuildingPool(), want);
        ScanPool(GetDummyPool(), want);
        int none = 0;
        for (auto& m : s_list) if (m.state == 0 && m.pos.empty()) none++;
        CwNativeLog("[mta] positions trouvees ; %d modele(s) sans position (charges quand un objet du serveur les utilise)", none);
    }
    // objets du serveur (mappings) : un objet deja la avec ce modele = position
    if (now >= s_objScanAt) {
        s_objScanAt = now + 2000;
        std::unordered_map<int, MtaModel*> want;
        for (auto& m : s_list) if (m.state == 0) want[m.id] = &m;
        if (!want.empty()) ScanPool(GetObjectPoolGta(), want);
    }

    // modele en attente de chargement (modele d'origine demande au streaming)
    for (auto& m : s_list) {
        if (m.state != 1) continue;
        if (!CStreaming::GetInfo(m.id).IsLoaded()) {
            if (now - m.reqAt > 15000) { m.state = -1; CwNativeLog("[mta] %d : le modele d'origine ne se charge pas, abandon", m.id); }
            return;
        }
        CStreaming::GetInfo(m.id).m_nFlags = (uint8)(CStreaming::GetInfo(m.id).m_nFlags | STREAMING_KEEP_IN_MEMORY | STREAMING_GAME_REQUIRED);
        // garde ecrite AVANT de toucher au modele
        WriteText(s_stateDir + "/guard.txt", std::to_string(m.id) + "\n", false);
        s_guardId = m.id;
        s_guardUntil = now + 20000;
        std::string msg;
        if (Replace(m, msg)) { m.state = 2; CwNativeLog("[mta] %s", msg.c_str()); }
        else {
            m.state = -1;
            unlink((s_stateDir + "/guard.txt").c_str());
            s_guardId = 0;
            CwNativeLog("[mta] modele %d (%s) garde d'origine : %s", m.id, m.name.c_str(), msg.c_str());
        }
        return;
    }

    // prochain modele : le plus proche du joueur, dans son rayon
    MtaModel* best = nullptr;
    float bestD = 1e12f;
    for (auto& m : s_list) {
        if (m.state != 0 || m.pos.empty()) continue;
        float r = m.dist > 250.f ? m.dist + 150.f : 400.f;
        if (r > 1600.f) r = 1600.f;
        for (auto& p : m.pos) {
            float d = (p.first - me.x) * (p.first - me.x) + (p.second - me.y) * (p.second - me.y);
            if (d < r * r && d < bestD) { bestD = d; best = &m; }
        }
    }
    if (!best) return;
    std::string dir = JavaStr("prepareMtaModel", std::to_string(best->id).c_str());
    long cost = dir.empty() ? 0 : FileSize(dir + "/model.txd") + FileSize(dir + "/model.dff");
    if (dir.empty()) { best->state = -1; CwNativeLog("[mta] %d : fichiers absents de l'APK", best->id); return; }
    if (s_used + cost > s_budget) {
        best->state = -1;
        CwNativeLog("[mta] %d (%s) : budget memoire atteint (%ld Mo), modele d'origine garde", best->id, best->name.c_str(), s_used / (1024 * 1024));
        return;
    }
    s_used += cost;
    best->state = 1;
    best->reqAt = now;
    CStreaming::RequestModel(best->id, STREAMING_GAME_REQUIRED | STREAMING_KEEP_IN_MEMORY);
}
