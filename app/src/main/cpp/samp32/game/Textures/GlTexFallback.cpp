// =============================================================================================
// [Crestwood] Textures compressees pour TOUS les GPU.
//
// Le jeu envoie ses textures au GPU en format compresse (ETC / DXT / PVRTC). Chaque famille de GPU
// n'en lit qu'une partie : PVRTC = PowerVR seulement, DXT (S3TC) = Adreno/Nvidia/..., ETC = presque tous.
// Quand le GPU ne sait pas lire un format, la texture sort blanche, noire ou bruitee (skins blancs,
// voitures granuleuses...). Ici on intercepte glCompressedTexImage2D :
//   - si le GPU supporte le format -> appel d'origine, rien ne change ;
//   - sinon -> decodage logiciel en RGBA 8888 puis glTexImage2D (fonctionne partout).
// ETC1 est envoye en ETC2 quand l'extension ETC1 est absente (ETC2 lit les donnees ETC1).
// Decodeur PVRTC : PowerVR SDK (MIT), voir pvr/LICENSE_PVRTDecompress.md.
// Journal : "[GlTex] ..." dans logcat.txt (client 64 bits) / crestwood_samp_log.txt (32 bits).
// =============================================================================================
#include <GLES3/gl3.h>
#include <GLES2/gl2ext.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <vector>
#include <string>
#include <dlfcn.h>
#include <set>
#include <string.h>
#include <stdio.h>
extern char* g_pszStorage;
#include "../../vendor/armhook/patch.h"
#include "pvr/PVRTDecompress.h"

#if defined(__arm__)
void FLog(const char* fmt, ...);
#define GLTEX_LOG FLog
#else
void Log(const char* fmt, ...);
#define GLTEX_LOG Log
#endif

namespace {
    typedef void (*PFN_glCompressedTexImage2D)(GLenum, GLint, GLenum, GLsizei, GLsizei, GLint, GLsizei, const void*);
    PFN_glCompressedTexImage2D g_origV2 = nullptr, g_origV3 = nullptr;
    typedef void (*PFN_glCompressedTexSubImage2D)(GLenum, GLint, GLint, GLint, GLsizei, GLsizei, GLenum, GLsizei, const void*);
    typedef void (*PFN_glTexStorage2D)(GLenum, GLsizei, GLenum, GLsizei, GLsizei);
    PFN_glCompressedTexSubImage2D g_subV2 = nullptr, g_subV3 = nullptr;
    PFN_glTexStorage2D g_stoV2 = nullptr, g_stoV3 = nullptr;

    // journal : premiere fois que chaque (chemin, format) est vu
    void SeenOnce(const char* path, GLenum fmt, int w, int h)
    {
        static std::set<uint64_t> seen;
        uint64_t key = ((uint64_t)(unsigned char)path[2] << 40) ^ ((uint64_t)(unsigned char)path[8] << 32) ^ (uint64_t)fmt;
        if (seen.insert(key).second && seen.size() < 40)
            GLTEX_LOG("[GlTex] %s : format 0x%04X (%dx%d)", path, (unsigned)fmt, w, h);
    }

    bool g_init = false, g_hasPVR = true, g_hasS3TC = true, g_hasETC1 = true, g_useETC2 = false;
    int  g_swCount = 0;

    void InitCaps()
    {
        g_init = true;
        const char* ext = (const char*)glGetString(GL_EXTENSIONS);
        const char* rnd = (const char*)glGetString(GL_RENDERER);
        std::string e = ext ? ext : "";
        g_hasPVR  = e.find("GL_IMG_texture_compression_pvrtc") != std::string::npos;
        g_hasS3TC = e.find("GL_EXT_texture_compression_s3tc") != std::string::npos
                 || e.find("GL_EXT_texture_compression_dxt1") != std::string::npos
                 || e.find("GL_NV_texture_compression_s3tc") != std::string::npos;
        g_hasETC1 = e.find("GL_OES_compressed_ETC1_RGB8_texture") != std::string::npos;
        // ETC2 (OpenGL ES 3) decode aussi les blocs ETC1 a l'identique, et lit en plus les blocs ETC2 :
        // on envoie donc toute texture "ETC1" en ETC2 (evite le rendu arc-en-ciel si certaines textures sont en fait ETC2).
        const char* ver = (const char*)glGetString(GL_VERSION);
        bool es3 = ver && strstr(ver, "OpenGL ES 3") != nullptr;
        g_useETC2 = es3;
        {
            char pth[512]; snprintf(pth, sizeof(pth), "%sSAMP/etc2.txt", g_pszStorage ? g_pszStorage : "");
            if (FILE* f = fopen(pth, "r")) { char w[8] = {0}; if (fscanf(f, "%7s", w) == 1 && !strncmp(w, "off", 3)) g_useETC2 = false; fclose(f); }
        }
        GLTEX_LOG("[GlTex] GPU : %s | PVRTC=%d S3TC=%d ETC1=%d | ES3=%d | ETC1->ETC2=%d", rnd ? rnd : "?", (int)g_hasPVR, (int)g_hasS3TC, (int)g_hasETC1, (int)es3, (int)g_useETC2);
    }

    inline bool IsPow2(int v) { return v > 0 && (v & (v - 1)) == 0; }

    // --- DXT (S3TC) logiciel -------------------------------------------------------------------
    static void Rgb565(uint16_t c, uint8_t* o)
    {
        o[0] = (uint8_t)(((c >> 11) & 31) * 255 / 31);
        o[1] = (uint8_t)(((c >> 5) & 63) * 255 / 63);
        o[2] = (uint8_t)((c & 31) * 255 / 31);
    }

    static void DecodeDXT(const uint8_t* src, int w, int h, int kind /*1,3,5*/, uint8_t* dst)
    {
        const int bw = (w + 3) / 4, bh = (h + 3) / 4;
        const int bsz = (kind == 1) ? 8 : 16;
        for (int by = 0; by < bh; by++) for (int bx = 0; bx < bw; bx++) {
            const uint8_t* b = src + (size_t)(by * bw + bx) * bsz;
            uint8_t alpha[16]; for (int i = 0; i < 16; i++) alpha[i] = 255;
            const uint8_t* cb = b;
            if (kind == 3) {
                for (int i = 0; i < 8; i++) { alpha[i * 2] = (b[i] & 15) * 17; alpha[i * 2 + 1] = (b[i] >> 4) * 17; }
                cb = b + 8;
            } else if (kind == 5) {
                uint8_t a0 = b[0], a1 = b[1], tab[8]; tab[0] = a0; tab[1] = a1;
                if (a0 > a1) for (int i = 1; i < 7; i++) tab[i + 1] = (uint8_t)(((7 - i) * a0 + i * a1) / 7);
                else { for (int i = 1; i < 5; i++) tab[i + 1] = (uint8_t)(((5 - i) * a0 + i * a1) / 5); tab[6] = 0; tab[7] = 255; }
                uint64_t bits = 0; for (int i = 0; i < 6; i++) bits |= (uint64_t)b[2 + i] << (8 * i);
                for (int i = 0; i < 16; i++) alpha[i] = tab[(bits >> (3 * i)) & 7];
                cb = b + 8;
            }
            uint16_t c0 = cb[0] | (cb[1] << 8), c1 = cb[2] | (cb[3] << 8);
            uint8_t col[4][4]; Rgb565(c0, col[0]); Rgb565(c1, col[1]); col[0][3] = col[1][3] = 255;
            if (c0 > c1 || kind != 1) {
                for (int k = 0; k < 3; k++) { col[2][k] = (uint8_t)((2 * col[0][k] + col[1][k]) / 3); col[3][k] = (uint8_t)((col[0][k] + 2 * col[1][k]) / 3); }
                col[2][3] = col[3][3] = 255;
            } else {
                for (int k = 0; k < 3; k++) { col[2][k] = (uint8_t)((col[0][k] + col[1][k]) / 2); col[3][k] = 0; }
                col[2][3] = 255; col[3][3] = 0;
            }
            uint32_t idx = cb[4] | (cb[5] << 8) | (cb[6] << 16) | ((uint32_t)cb[7] << 24);
            for (int py = 0; py < 4; py++) for (int px = 0; px < 4; px++) {
                int x = bx * 4 + px, y = by * 4 + py; if (x >= w || y >= h) continue;
                int i = py * 4 + px; const uint8_t* c = col[(idx >> (2 * i)) & 3];
                uint8_t* o = dst + ((size_t)y * w + x) * 4;
                o[0] = c[0]; o[1] = c[1]; o[2] = c[2];
                o[3] = (kind == 1) ? c[3] : alpha[i];
            }
        }
    }

    static void UploadRGBA(GLenum target, GLint level, int w, int h, const uint8_t* rgba)
    {
        GLint prev = 4; glGetIntegerv(GL_UNPACK_ALIGNMENT, &prev);
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        glTexImage2D(target, level, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, rgba);
        glPixelStorei(GL_UNPACK_ALIGNMENT, prev);
    }

    void Impl(PFN_glCompressedTexImage2D orig, GLenum target, GLint level, GLenum fmt, GLsizei w, GLsizei h, GLint border, GLsizei size, const void* data)
    {
        if (!g_init) InitCaps();
        SeenOnce("glCompressedTexImage2D", fmt, w, h);
        if (data && w > 0 && h > 0) {
            // PVRTC sur un GPU sans PVRTC
            if (fmt >= 0x8C00 && fmt <= 0x8C03 && !g_hasPVR && IsPow2(w) && IsPow2(h)) {
                std::vector<uint8_t> out((size_t)w * h * 4);
                pvr::PVRTDecompressPVRTC(data, (fmt == 0x8C01 || fmt == 0x8C03) ? 1 : 0, (uint32_t)w, (uint32_t)h, out.data());
                if (g_swCount++ < 3) GLTEX_LOG("[GlTex] PVRTC non supporte par ce GPU : decodage logiciel (%dx%d niveau %d)", w, h, level);
                UploadRGBA(target, level, w, h, out.data());
                return;
            }
            // DXT / S3TC sur un GPU sans S3TC
            if (fmt >= 0x83F0 && fmt <= 0x83F3 && !g_hasS3TC) {
                int kind = (fmt == 0x83F2) ? 3 : (fmt == 0x83F3 ? 5 : 1);
                std::vector<uint8_t> out((size_t)w * h * 4);
                DecodeDXT((const uint8_t*)data, w, h, kind, out.data());
                if (g_swCount++ < 3) GLTEX_LOG("[GlTex] S3TC/DXT non supporte par ce GPU : decodage logiciel (%dx%d niveau %d)", w, h, level);
                UploadRGBA(target, level, w, h, out.data());
                return;
            }
        }
        if (fmt == 0x8D64 && (!g_hasETC1 || g_useETC2)) fmt = 0x9274; // GL_COMPRESSED_RGB8_ETC2
        orig(target, level, fmt, w, h, border, size, data);
    }

    inline bool Unsupported(GLenum fmt)
    {
        if (fmt >= 0x8C00 && fmt <= 0x8C03) return !g_hasPVR;
        if (fmt >= 0x83F0 && fmt <= 0x83F3) return !g_hasS3TC;
        return false;
    }

    void SubImpl(PFN_glCompressedTexSubImage2D orig, GLenum target, GLint level, GLint xo, GLint yo, GLsizei w, GLsizei h, GLenum fmt, GLsizei size, const void* data)
    {
        if (!g_init) InitCaps();
        SeenOnce("glCompressedTexSubImage2D", fmt, w, h);
        if (data && w > 0 && h > 0 && Unsupported(fmt)) {
            std::vector<uint8_t> out((size_t)w * h * 4);
            bool ok = true;
            if (fmt <= 0x8C03) { if (IsPow2(w) && IsPow2(h)) pvr::PVRTDecompressPVRTC(data, (fmt == 0x8C01 || fmt == 0x8C03) ? 1 : 0, (uint32_t)w, (uint32_t)h, out.data()); else ok = false; }
            else DecodeDXT((const uint8_t*)data, w, h, (fmt == 0x83F2) ? 3 : (fmt == 0x83F3 ? 5 : 1), out.data());
            if (ok) {
                if (g_swCount++ < 6) GLTEX_LOG("[GlTex] SubImage : decodage logiciel du format 0x%04X (%dx%d)", (unsigned)fmt, w, h);
                GLint prev = 4; glGetIntegerv(GL_UNPACK_ALIGNMENT, &prev); glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
                glTexSubImage2D(target, level, xo, yo, w, h, GL_RGBA, GL_UNSIGNED_BYTE, out.data());
                glPixelStorei(GL_UNPACK_ALIGNMENT, prev);
                return;
            }
        }
        if (fmt == 0x8D64 && (!g_hasETC1 || g_useETC2)) fmt = 0x9274;
        orig(target, level, xo, yo, w, h, fmt, size, data);
    }

    void StoImpl(PFN_glTexStorage2D orig, GLenum target, GLsizei levels, GLenum fmt, GLsizei w, GLsizei h)
    {
        if (!g_init) InitCaps();
        SeenOnce("glTexStorage2D", fmt, w, h);
        if (Unsupported(fmt)) {
            if (g_swCount++ < 6) GLTEX_LOG("[GlTex] TexStorage2D : format 0x%04X non supporte -> RGBA8 (%dx%d)", (unsigned)fmt, w, h);
            fmt = GL_RGBA8;
        }
        if (fmt == 0x8D64 && (!g_hasETC1 || g_useETC2)) fmt = 0x9274;
        orig(target, levels, fmt, w, h);
    }

    void Sub_V2(GLenum t, GLint l, GLint x, GLint y, GLsizei w, GLsizei h, GLenum f, GLsizei s, const void* d) { SubImpl(g_subV2, t, l, x, y, w, h, f, s, d); }
    void Sub_V3(GLenum t, GLint l, GLint x, GLint y, GLsizei w, GLsizei h, GLenum f, GLsizei s, const void* d) { SubImpl(g_subV3, t, l, x, y, w, h, f, s, d); }
    void Sto_V2(GLenum t, GLsizei n, GLenum f, GLsizei w, GLsizei h) { StoImpl(g_stoV2, t, n, f, w, h); }
    void Sto_V3(GLenum t, GLsizei n, GLenum f, GLsizei w, GLsizei h) { StoImpl(g_stoV3, t, n, f, w, h); }

    // --- Option de test : sans mipmaps (SAMP/nomips.txt = "on") ---------------------------------
    // Une texture dont la chaine de mipmaps est incomplete est lue en NOIR par le GPU. En forcant un filtre sans mipmaps,
    // seul le niveau 0 est lu : les textures noires deviennent visibles (au prix d'un peu de scintillement a distance).
    typedef void (*PFN_glTexParameteri)(GLenum, GLenum, GLint);
    PFN_glTexParameteri g_tpV2 = nullptr, g_tpV3 = nullptr;
    bool NoMips()
    {
        static int v = -1;
        if (v < 0) {
            v = 0;
            char pth[512]; snprintf(pth, sizeof(pth), "%sSAMP/nomips.txt", g_pszStorage ? g_pszStorage : "");
            if (FILE* f = fopen(pth, "r")) { char w[8] = {0}; if (fscanf(f, "%7s", w) == 1 && !strncmp(w, "on", 2)) v = 1; fclose(f); }
            GLTEX_LOG("[GlTex] sans mipmaps : %s", v ? "ACTIVE (nomips.txt)" : "non");
        }
        return v == 1;
    }
    void TpImpl(PFN_glTexParameteri orig, GLenum target, GLenum pname, GLint param)
    {
        if (pname == 0x2801 /*GL_TEXTURE_MIN_FILTER*/ && param >= 0x2700 && param <= 0x2703 && NoMips())
            param = (param == 0x2700 || param == 0x2702) ? 0x2600 /*NEAREST*/ : 0x2601 /*LINEAR*/;
        orig(target, pname, param);
    }
    void Tp_V2(GLenum t, GLenum n, GLint p) { TpImpl(g_tpV2, t, n, p); }
    void Tp_V3(GLenum t, GLenum n, GLint p) { TpImpl(g_tpV3, t, n, p); }

    void Hook_V2(GLenum t, GLint l, GLenum f, GLsizei w, GLsizei h, GLint b, GLsizei s, const void* d) { Impl(g_origV2, t, l, f, w, h, b, s, d); }
    void Hook_V3(GLenum t, GLint l, GLenum f, GLsizei w, GLsizei h, GLint b, GLsizei s, const void* d) { Impl(g_origV3, t, l, f, w, h, b, s, d); }
}

void GlTexFallback_Install()
{
    static bool done = false;
    if (done) return;
    done = true;
    // libGLESv2.so et libGLESv3.so sont souvent la meme bibliotheque : on ne pose qu'un hook dans ce cas.
    void* h2 = dlopen("libGLESv2.so", RTLD_NOW | RTLD_NOLOAD);
    void* h3 = dlopen("libGLESv3.so", RTLD_NOW | RTLD_NOLOAD);
    void* a2 = h2 ? dlsym(h2, "glCompressedTexImage2D") : nullptr;
    void* a3 = h3 ? dlsym(h3, "glCompressedTexImage2D") : nullptr;
    bool ok = false;
    if (a2) {
        ok = shadowhook_hook_sym_name("libGLESv2.so", "glCompressedTexImage2D", (void*)Hook_V2, (void**)&g_origV2) != nullptr;
        GLTEX_LOG("[GlTex] hook libGLESv2.so : %s", ok ? "ok" : "echec");
        shadowhook_hook_sym_name("libGLESv2.so", "glCompressedTexSubImage2D", (void*)Sub_V2, (void**)&g_subV2);
        shadowhook_hook_sym_name("libGLESv2.so", "glTexStorage2D", (void*)Sto_V2, (void**)&g_stoV2);
        shadowhook_hook_sym_name("libGLESv2.so", "glTexParameteri", (void*)Tp_V2, (void**)&g_tpV2);
    }
    if (a3 && a3 != a2) {
        bool ok3 = shadowhook_hook_sym_name("libGLESv3.so", "glCompressedTexImage2D", (void*)Hook_V3, (void**)&g_origV3) != nullptr;
        GLTEX_LOG("[GlTex] hook libGLESv3.so : %s", ok3 ? "ok" : "echec");
        ok = ok || ok3;
        shadowhook_hook_sym_name("libGLESv3.so", "glCompressedTexSubImage2D", (void*)Sub_V3, (void**)&g_subV3);
        shadowhook_hook_sym_name("libGLESv3.so", "glTexStorage2D", (void*)Sto_V3, (void**)&g_stoV3);
        shadowhook_hook_sym_name("libGLESv3.so", "glTexParameteri", (void*)Tp_V3, (void**)&g_tpV3);
    }
    if (!ok) GLTEX_LOG("[GlTex] aucun hook installe (a2=%p a3=%p) : decodage logiciel indisponible", a2, a3);
}
