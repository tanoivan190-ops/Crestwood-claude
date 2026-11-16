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
        if (mods.empty()) { Log("[modloader] dossier SAMP/modloader vide ou absent"); return; }

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
                Log("[modloader] %s -> modele %d (remplace l'entree de %s)", nm, id, CStreaming::ms_files[slot].m_szName);
            }
        }
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
