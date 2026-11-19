// [Crestwood] Dossier SAMP/modloader/ : les fichiers .dff .col .ipl .ifp qui s'y trouvent remplacent ceux des
// archives .img (gta3.img, gta_int.img...), sans toucher a ces archives.
//
// Principe : au premier RequestModel (les repertoires des .img sont alors charges), on
//  1. liste le dossier (sous-dossiers compris) ;
//  2. ecrit une archive "texdb/cwmods.img" qui contient ces fichiers (secteurs de 2048 octets) ;
//  3. l'ajoute a la liste des archives du moteur ;
//  4. relit le repertoire de chaque .img : pour chaque entree du meme nom, le modele du moteur est redirige
//     vers cwmods.img (CStreamingInfo : numero d'archive, position, taille).
// Les .txd ne sont pas pris en charge ici (les textures sont dans texdb, format different).
#include "Streaming.h"
#include "Models/ModelInfo.h"
#include "StreamingInfo.h"
#include "../vendor/armhook/patch.h"
#include <dirent.h>
#include <sys/stat.h>
#include <cstdio>
#include <cstring>
#include <cctype>
#include <string>
#include <vector>
#include <unordered_map>
#include <atomic>
#include <cmath>
#include "ModLoaderTemplate.h"
#include <cmath>

extern char* g_pszStorage;
void Log(const char* fmt, ...);

namespace {
    struct ModFile { std::string name; std::string path; uint32_t size; };

    std::string Lower(std::string s) { for (auto& c : s) c = (char)tolower((unsigned char)c); return s; }

    void Scan(const std::string& dir, std::vector<ModFile>& out, int& skippedTxd, int depth = 0) {
        if (depth > 6) return;
        DIR* d = opendir(dir.c_str());
        if (!d) return;
        while (dirent* e = readdir(d)) {
            std::string n = e->d_name;
            if (n == "." || n == "..") continue;
            std::string full = dir + "/" + n;
            struct stat st{};
            if (stat(full.c_str(), &st) != 0) continue;
            if (S_ISDIR(st.st_mode)) { Scan(full, out, skippedTxd, depth + 1); continue; }
            std::string l = Lower(n);
            auto dot = l.rfind('.');
            if (dot == std::string::npos) continue;
            std::string ext = l.substr(dot);
            if (ext == ".txd") { skippedTxd++; continue; }
            if (ext != ".dff" && ext != ".col" && ext != ".ipl" && ext != ".ifp") continue;
            if (st.st_size <= 0 || st.st_size > 64 * 1024 * 1024) continue;
            out.push_back({l, full, (uint32_t)st.st_size});
        }
        closedir(d);
    }


    // ---- Modeles integres : blocs 'Concrete..' d'objets SA-MP absents de ton gta3.img (ids 18762...) ----
    struct Chunk { static void Put(std::vector<uint8_t>& o, uint32_t t, const std::vector<uint8_t>& d) {
        uint32_t v[3] = {t, (uint32_t)d.size(), 0x1803FFFF};
        const uint8_t* p = (const uint8_t*)v; o.insert(o.end(), p, p + 12); o.insert(o.end(), d.begin(), d.end()); } };
    template<class T> void Add(std::vector<uint8_t>& o, T v) { const uint8_t* p = (const uint8_t*)&v; o.insert(o.end(), p, p + sizeof(T)); }

    // Boite aux dimensions voulues : on part d'un vrai cube du jeu (format Android, geometrie 'native') et
    // on ne remplace que les positions des 24 sommets (+ la sphere englobante).
    std::vector<uint8_t> BuildBoxDff(const char* /*name*/, float sx, float sy, float sz) {
        std::vector<uint8_t> d(kBoxTemplate, kBoxTemplate + sizeof(kBoxTemplate));
        auto findHdr = [&](uint32_t type, uint32_t size) -> long {
            for (size_t i = 0; i + 12 <= d.size(); i += 1) {
                uint32_t w[3]; memcpy(w, &d[i], 12);
                if (w[0] == type && w[1] == size && w[2] == 0x1803FFFF) return (long)i;
            }
            return -1;
        };
        long nat = findHdr(0x510, 556);
        long gs = findHdr(0x01, 40);
        if (nat < 0 || gs < 0) return d;
        uint8_t* v = &d[nat + 12 + 76];            // 24 sommets de 20 octets (position = 3 float)
        float mn[3] = {1e30f, 1e30f, 1e30f}, mx[3] = {-1e30f, -1e30f, -1e30f};
        for (int i = 0; i < 24; i++) for (int a = 0; a < 3; a++) {
            float f; memcpy(&f, v + i * 20 + a * 4, 4);
            if (f < mn[a]) mn[a] = f; if (f > mx[a]) mx[a] = f;
        }
        const float h[3] = {sx / 2, sy / 2, sz / 2};
        for (int i = 0; i < 24; i++) for (int a = 0; a < 3; a++) {
            float f; memcpy(&f, v + i * 20 + a * 4, 4);
            float o = (f > (mn[a] + mx[a]) / 2) ? h[a] : -h[a];
            memcpy(v + i * 20 + a * 4, &o, 4);
        }
        float sph[4] = {0, 0, 0, sqrtf(h[0] * h[0] + h[1] * h[1] + h[2] * h[2])};
        memcpy(&d[gs + 12 + 16], sph, 16);          // sphere englobante apres les 4 mots d'en-tete
        return d;
    }
    struct BuiltinModel { int id; const char* name; float x, y, z; };
    const BuiltinModel kBuiltin[] = {
        {18762, "Concrete1mx1mx5m", 1, 1, 5}, {18766, "Concrete10mx1mx5m", 10, 1, 5}, {18980, "Concrete1mx1mx25m", 1, 1, 25},
        {18763, "Concrete3mx3mx5m", 3, 3, 5}, {18764, "Concrete5mx5mx5m", 5, 5, 5}, {18765, "Concrete10mx10mx5m", 10, 10, 5}};

    // Les modeles ajoutes n'ont pas de fichier de textures (leur .txd n'existe dans aucune archive) : le moteur
    // plantait en lisant la texture du materiau. On leur donne le dictionnaire de textures d'un vrai petit objet
    // du jeu (celui dont on a copie la forme, 'temp_cardbox') pour que tout soit charge normalement.
    int TemplateTxdIndex() {
        static int cached = -2;
        if (cached != -2) return cached;
        cached = -1;
        for (int i = 0; i < (int)CModelInfo::NUM_MODEL_INFOS; i++) {
            CBaseModelInfo* mi = CModelInfo::GetModelInfo(i);
            if (mi && strncmp(mi->m_modelName, "temp_cardbox", 12) == 0 && mi->m_modelName[12] == 0) { cached = mi->m_nTxdIndex; break; }
        }
        return cached;
    }
    void GiveTemplateTxd(int modelId, bool force) {
        CBaseModelInfo* mi = CModelInfo::GetModelInfo(modelId);
        int tx = TemplateTxdIndex();
        if (!mi || tx < 0) { Log("[modloader] modele %d : info modele ou dictionnaire de reference introuvable", modelId); return; }
        CStreamingInfo& ti = CStreaming::ms_aInfoForModel[(int)RESOURCE_ID_TXD + (mi->m_nTxdIndex < 0 ? 0 : mi->m_nTxdIndex)];
        if (!force && mi->m_nTxdIndex >= 0 && ti.HasCdPosnAndSize()) return;
        Log("[modloader] modele %d : dictionnaire de textures %d -> %d", modelId, (int)mi->m_nTxdIndex, tx);
        mi->m_nTxdIndex = (int16_t)tx;
    }

    FILE* OpenImg(const char* imgName) {
        std::string rel = imgName;
        for (auto& c : rel) if (c == '\\') c = '/';
        std::string root = g_pszStorage ? g_pszStorage : "";
        FILE* f = fopen((root + rel).c_str(), "rb");
        if (!f) f = fopen((root + Lower(rel)).c_str(), "rb");
        return f;
    }

    void Apply() {
        std::string root = g_pszStorage ? g_pszStorage : "";
        std::string modDir = root + "SAMP/modloader";
        std::vector<ModFile> mods;
        int skippedTxd = 0;
        Scan(modDir, mods, skippedTxd);
        if (skippedTxd) Log("[modloader] %d fichier(s) .txd ignore(s) (pas pris en charge pour l'instant)", skippedTxd);
        if (mods.empty()) Log("[modloader] dossier SAMP/modloader vide ou absent (seuls les modeles integres sont ajoutes)");

        // 1) archive cwmods.img
        std::string cache = root + "texdb/cwmods.img";
        FILE* out = fopen(cache.c_str(), "wb");
        if (!out) { Log("[modloader] impossible d'ecrire %s", cache.c_str()); return; }
        std::unordered_map<std::string, std::pair<uint32_t, uint32_t>> placed; // nom -> (secteur, nb secteurs)
        uint32_t sector = 0;
        std::vector<char> buf;
        static const char zeros[2048] = {0};
        for (auto& m : mods) {
            if (placed.count(m.name)) continue; // 1er trouve gagne
            FILE* in = fopen(m.path.c_str(), "rb");
            if (!in) continue;
            buf.resize(m.size);
            size_t got = fread(buf.data(), 1, m.size, in);
            fclose(in);
            if (got != m.size) { Log("[modloader] lecture incomplete : %s", m.path.c_str()); continue; }
            fwrite(buf.data(), 1, m.size, out);
            uint32_t sectors = (m.size + 2047) / 2048;
            uint32_t pad = sectors * 2048 - m.size;
            if (pad) fwrite(zeros, 1, pad, out);
            placed[m.name] = {sector, sectors};
            sector += sectors;
        }
        // modeles integres (si l'utilisateur n'a pas fourni son propre fichier du meme nom)
        std::vector<std::pair<int, std::string>> builtinIds;
        for (auto& b : kBuiltin) {
            std::string n = Lower(std::string(b.name) + ".dff");
            if (placed.count(n)) continue;
            std::vector<uint8_t> d = BuildBoxDff(b.name, b.x, b.y, b.z);
            fwrite(d.data(), 1, d.size(), out);
            uint32_t sectors = ((uint32_t)d.size() + 2047) / 2048;
            uint32_t pad = sectors * 2048 - (uint32_t)d.size();
            if (pad) fwrite(zeros, 1, pad, out);
            placed[n] = {sector, sectors};
            sector += sectors;
            builtinIds.push_back({b.id, n});
        }
        fclose(out);
        if (placed.empty()) return;

        // 2) archive ajoutee au moteur (slot libre)
        int newSlot = CStreaming::AddImageToList("TEXDB\\CWMODS.IMG", true);
        if (newSlot < 0 || newSlot >= (int)TOTAL_IMG_ARCHIVES) { Log("[modloader] plus de place pour une archive"); return; }

        // 3) (archive, position) -> id du modele
        std::unordered_map<uint64_t, int> byPos;
        for (int id = 0; id < (int)RESOURCE_ID_TOTAL; id++) {
            CStreamingInfo& si = CStreaming::ms_aInfoForModel[id];
            if (!si.HasCdPosnAndSize()) continue;
            byPos[((uint64_t)si.m_nImgId << 32) | si.m_nCdPosn] = id;
        }

        // 4) repertoires des .img
        int replaced = 0;
        std::unordered_map<std::string, bool> used;
        for (int slot = 0; slot < (int)TOTAL_IMG_ARCHIVES; slot++) {
            if (slot == newSlot || !CStreaming::ms_files[slot].m_szName[0]) continue;
            FILE* f = OpenImg(CStreaming::ms_files[slot].m_szName);
            if (!f) continue;
            char hd[8];
            if (fread(hd, 1, 8, f) != 8 || memcmp(hd, "VER2", 4) != 0) { fclose(f); continue; }
            uint32_t n; memcpy(&n, hd + 4, 4);
            if (n > 200000) { fclose(f); continue; }
            std::vector<uint8_t> dir(32u * n);
            size_t rd = fread(dir.data(), 1, dir.size(), f);
            fclose(f);
            n = (uint32_t)(rd / 32);
            for (uint32_t i = 0; i < n; i++) {
                const uint8_t* e = &dir[i * 32];
                uint32_t off; memcpy(&off, e, 4);
                char nm[25]; memcpy(nm, e + 8, 24); nm[24] = 0;
                auto it = placed.find(Lower(nm));
                if (it == placed.end()) continue;
                auto pos = byPos.find(((uint64_t)slot << 32) | off);
                if (pos == byPos.end()) continue;
                int id = pos->second;
                CStreamingInfo& si = CStreaming::ms_aInfoForModel[id];
                if (si.m_nLoadState != eStreamingLoadState::LOADSTATE_NOT_LOADED) {
                    Log("[modloader] %s : deja charge, ignore", nm);
                    continue;
                }
                // plus de lecture "a la suite" de ce modele dans l'ancienne archive
                for (int j = 0; j < (int)RESOURCE_ID_TOTAL; j++) {
                    CStreamingInfo& o = CStreaming::ms_aInfoForModel[j];
                    if (o.m_nNextIndexOnCd == id) o.m_nNextIndexOnCd = -1;
                }
                si.m_nNextIndexOnCd = -1;
                si.m_nImgId = (uint8)newSlot;
                si.SetCdPosnAndSize(it->second.first, it->second.second);
                replaced++;
                used[it->first] = true;
                Log("[modloader] %s -> modele %d (remplace l'entree de %s)", nm, id, CStreaming::ms_files[slot].m_szName);
            }
        }
        for (auto& bi : builtinIds) {
            CStreamingInfo& si = CStreaming::ms_aInfoForModel[bi.first];
            if (si.HasCdPosnAndSize() || si.m_nLoadState != eStreamingLoadState::LOADSTATE_NOT_LOADED) continue; // deja fourni par une archive
            auto pl = placed[bi.second];
            si.m_nNextIndexOnCd = -1;
            si.m_nImgId = (uint8)newSlot;
            si.SetCdPosnAndSize(pl.first, pl.second);
            used[bi.second] = true;
            GiveTemplateTxd(bi.first, true);
            Log("[modloader] modele integre %d (%s) ajoute", bi.first, bi.second.c_str());
        }
        // 5) modeles qui n'existent dans AUCUNE archive (ex. objets SA-MP 18762...) : on les trouve par leur nom
        //    dans les fichiers .ide, et on les ajoute (a condition que le moteur n'ait pas deja leurs donnees).
        std::unordered_map<std::string, int> ideByName;
        const char* ideDirs[] = {"texdb", "SAMP", "data", "DATA"};
        for (const char* dn : ideDirs) {
            std::string d = root + dn;
            DIR* dd = opendir(d.c_str());
            if (!dd) continue;
            while (dirent* e = readdir(dd)) {
                std::string fn = Lower(e->d_name);
                if (fn.size() < 5 || fn.compare(fn.size() - 4, 4, ".ide") != 0) continue;
                FILE* f = fopen((d + "/" + e->d_name).c_str(), "r");
                if (!f) continue;
                char line[512];
                while (fgets(line, sizeof line, f)) {
                    int id = -1; char dff[128] = {0};
                    if (sscanf(line, " %d , %127[^, ]", &id, dff) == 2 && id >= 612 && id < (int)RESOURCE_ID_TOTAL)
                        ideByName.emplace(Lower(std::string(dff) + ".dff"), id);
                }
                fclose(f);
            }
            closedir(dd);
        }
        int added = 0;
        for (auto& kv : placed) {
            if (used.count(kv.first)) continue;
            auto ide = ideByName.find(kv.first);
            if (ide == ideByName.end()) continue;
            CStreamingInfo& si = CStreaming::ms_aInfoForModel[ide->second];
            if (si.HasCdPosnAndSize()) continue;
            if (si.m_nLoadState != eStreamingLoadState::LOADSTATE_NOT_LOADED) continue;
            si.m_nNextIndexOnCd = -1;
            si.m_nImgId = (uint8)newSlot;
            si.SetCdPosnAndSize(kv.second.first, kv.second.second);
            GiveTemplateTxd(ide->second, false);
            added++;
            Log("[modloader] %s -> modele %d AJOUTE (absent des archives)", kv.first.c_str(), ide->second);
        }
        Log("[modloader] %d noms lus dans les .ide, %d modele(s) ajoute(s)", (int)ideByName.size(), added);
        Log("[modloader] %d fichier(s) dans le dossier, %d modele(s) remplace(s)", (int)placed.size(), replaced);
    }

    void (*RequestModel_orig)(int, int) = nullptr;
    std::atomic<bool> s_done{false};
    void RequestModel_hook(int modelId, int flags) {
        if (!s_done.exchange(true)) {
            Apply();
        }
        RequestModel_orig(modelId, flags);
    }
}

void ModLoader_Install() {
    CHook::InlineHook("_ZN10CStreaming12RequestModelEii", &RequestModel_hook, &RequestModel_orig);
}
