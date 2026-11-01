// Marqueurs 3D flottants (voir WorldMarkers.h). Dessin ImGui, projection monde -> ecran avec
// CSprite::CalcScreenCoors (meme fonction que PlayerTap). Fichier IDENTIQUE dans samp/ et samp32/.
#include "main.h"
#include "game/game.h"
#include "net/netgame.h"
#include "gui/gui.h"
#include "WorldMarkers.h"
#include "gui/CharacterCreator.h"

#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <mutex>
#include <string>
#include <vector>

extern CGame* pGame;
extern CNetGame* pNetGame;

namespace
{
    struct Marker
    {
        int   type = 0;
        float x = 0.0f, y = 0.0f, z = 0.0f;
        char  label[32] = { 0 };
    };

    constexpr float kMaxDist   = 45.0f;   // au-dela : invisible
    constexpr float kFadeFrom  = 30.0f;   // fondu entre 30 et 45 m
    constexpr size_t kMaxMarkers = 512;

    std::mutex          s_mutex;
    std::vector<Marker> s_markers;

    float NowSec()
    {
        using namespace std::chrono;
        return (float)(duration_cast<milliseconds>(steady_clock::now().time_since_epoch()).count() % 3600000) / 1000.0f;
    }

    bool ToScreen(float x, float y, float z, ImVec2& out, float& depth)
    {
        CVector pos, scr;
        pos.x = x; pos.y = y; pos.z = z;
        ((void (*)(CVector*, CVector*, float*, float*, bool, bool))(g_libGTASA + (VER_x32 ? 0x005C57E8 + 1 : 0x6E9DF8)))(&pos, &scr, 0, 0, 0, 0);
        if (scr.z < 0.5f) return false;   // derriere la camera
        out = ImVec2(scr.x, scr.y);
        depth = scr.z;
        return true;
    }

    ImU32 Col(int r, int g, int b, float a)
    {
        if (a < 0.0f) a = 0.0f;
        if (a > 1.0f) a = 1.0f;
        return IM_COL32(r, g, b, (int)(a * 255.0f));
    }

    // Anneau au sol : vrai cercle 3D projete (donne l'ellipse en perspective)
    void GroundRing(ImDrawList* dl, const Marker& m, float radius, ImU32 col, float thick)
    {
        constexpr int N = 36;
        ImVec2 pts[N];
        int n = 0;
        for (int i = 0; i < N; i++)
        {
            const float a = (float)i / (float)N * 6.2831853f;
            float d;
            if (!ToScreen(m.x + std::cos(a) * radius, m.y + std::sin(a) * radius, m.z + 0.03f, pts[n], d)) return;
            n++;
        }
        dl->AddPolyline(pts, n, col, true, thick);
    }

    // T-shirt : pieces convexes (pas de chevauchement -> transparence uniforme) + contour
    void Shirt(ImDrawList* dl, ImVec2 c, float sx, float sy, float alpha)
    {
        auto P = [&](float x, float y) { return ImVec2(c.x + x * sx, c.y + y * sy); };
        const ImU32 fill = Col(255, 255, 255, 0.34f * alpha);
        const ImU32 line = Col(255, 255, 255, 0.92f * alpha);
        const ImDrawListFlags old = dl->Flags;
        dl->Flags &= ~ImDrawListFlags_AntiAliasedFill;   // pas de liseré entre les pieces
        const ImVec2 torso[4] = { P(-0.55f, -0.6f), P(0.55f, -0.6f), P(0.55f, 0.9f), P(-0.55f, 0.9f) };
        const ImVec2 shL[4]   = { P(-0.55f, -0.75f), P(-0.25f, -0.82f), P(0.0f, -0.6f), P(-0.55f, -0.6f) };
        const ImVec2 shR[4]   = { P(0.0f, -0.6f), P(0.25f, -0.82f), P(0.55f, -0.75f), P(0.55f, -0.6f) };
        const ImVec2 slL[4]   = { P(-0.55f, -0.75f), P(-0.55f, -0.25f), P(-0.78f, -0.05f), P(-1.02f, -0.36f) };
        const ImVec2 slR[4]   = { P(0.55f, -0.75f), P(1.02f, -0.36f), P(0.78f, -0.05f), P(0.55f, -0.25f) };
        dl->AddConvexPolyFilled(torso, 4, fill);
        dl->AddConvexPolyFilled(shL, 4, fill);
        dl->AddConvexPolyFilled(shR, 4, fill);
        dl->AddConvexPolyFilled(slL, 4, fill);
        dl->AddConvexPolyFilled(slR, 4, fill);
        dl->Flags = old;
        const ImVec2 outline[13] = {
            P(-0.25f, -0.82f), P(-0.55f, -0.75f), P(-1.02f, -0.36f), P(-0.78f, -0.05f), P(-0.55f, -0.25f),
            P(-0.55f, 0.9f), P(0.55f, 0.9f), P(0.55f, -0.25f), P(0.78f, -0.05f), P(1.02f, -0.36f),
            P(0.55f, -0.75f), P(0.25f, -0.82f), P(0.0f, -0.6f)
        };
        dl->AddPolyline(outline, 13, line, true, std::fmax(1.5f, sy * 0.07f));
    }

    // ---- main d'interaction 3D (magasins de vetements) ----
    constexpr float    kHandDist2D  = 1.6f;   // distance au point (a plat) pour voir la main (<= portee serveur)
    constexpr float    kHandDistZ   = 2.2f;
    constexpr uint32_t kHandCooldownMs = 900;

    struct HandState
    {
        bool  visible = false;
        float x = 0.0f, y = 0.0f, r = 0.0f;   // position / rayon a l'ecran (image precedente)
        bool  down = false;
        int   finger = -1;
        float downX = 0.0f, downY = 0.0f;
        uint32_t lastSendMs = 0;
        uint32_t pressMs = 0;
    };
    std::mutex s_handMutex;
    HandState  s_hand;

    uint32_t NowMs()
    {
        using namespace std::chrono;
        return (uint32_t)duration_cast<milliseconds>(steady_clock::now().time_since_epoch()).count();
    }

    // Joueur local a pied : position (false sinon)
    bool LocalFootPos(CVector& out)
    {
        if (!pGame) return false;
        CPlayerPed* ped = pGame->FindPlayerPed();
        if (!ped || !ped->m_pPed || ped->IsInVehicle()) return false;
        out = ped->m_pPed->GetPosition();
        return true;
    }

    // Icone "main" : pastille noire, contour bleu Crestwood, main blanche (paume + 4 doigts + pouce)
    void Hand(ImDrawList* dl, ImVec2 c, float r, float alpha, bool pressed)
    {
        const float k = pressed ? 0.9f : 1.0f;
        r *= k;
        dl->AddCircleFilled(ImVec2(c.x, c.y + r * 0.08f), r * 1.08f, Col(0, 0, 0, 0.35f * alpha), 40);
        dl->AddCircleFilled(c, r, Col(0, 0, 0, 0.72f * alpha), 40);
        dl->AddCircle(c, r, Col(255, 255, 255, 0.95f * alpha), 40, std::fmax(2.0f, r * 0.10f));
        const ImU32 w = Col(255, 255, 255, alpha);
        const float u = r / 10.0f;   // unite de dessin
        // paume
        dl->AddRectFilled(ImVec2(c.x - 3.6f * u, c.y - 0.6f * u), ImVec2(c.x + 3.4f * u, c.y + 5.4f * u), w, 2.4f * u);
        // doigts (index -> auriculaire)
        const float fx[4] = { -3.3f, -1.5f, 0.3f, 2.1f };
        const float top[4] = { -6.2f, -7.0f, -6.6f, -5.4f };
        for (int i = 0; i < 4; i++)
            dl->AddRectFilled(ImVec2(c.x + fx[i] * u, c.y + top[i] * u), ImVec2(c.x + (fx[i] + 1.5f) * u, c.y + 1.0f * u), w, 0.75f * u);
        // pouce
        const ImVec2 th[4] = { ImVec2(c.x - 3.6f * u, c.y + 0.4f * u), ImVec2(c.x - 6.2f * u, c.y - 1.6f * u),
                               ImVec2(c.x - 5.0f * u, c.y - 2.9f * u), ImVec2(c.x - 2.6f * u, c.y - 1.2f * u) };
        dl->AddConvexPolyFilled(th, 4, w);
        // onde "appuyer"
        const float pulse = std::fmod(NowSec() * 1.2f, 1.0f);
        dl->AddCircle(c, r * (1.05f + 0.35f * pulse), Col(255, 255, 255, 0.5f * (1.0f - pulse) * alpha), 40, std::fmax(1.5f, r * 0.06f));
    }


    // ---- icones 3D des points d'interaction (pastille noire translucide + pictogramme blanc) ----
    void Glyph(ImDrawList* dl, ImGuiRenderer* renderer, int type, ImVec2 c, float r, float alpha)
    {
        const ImU32 w = Col(255, 255, 255, alpha);
        const ImU32 wf = Col(255, 255, 255, 0.92f * alpha);
        const float u = r / 10.0f;
        const float th = std::fmax(1.5f, u * 1.1f);
        auto P = [&](float x, float y) { return ImVec2(c.x + x * u, c.y + y * u); };
        switch (type)
        {
        case 2: // garage : voiture
            dl->AddRectFilled(P(-6.0f, -1.0f), P(6.0f, 3.0f), wf, 1.5f * u);
            { const ImVec2 cab[4] = { P(-3.6f, -1.0f), P(-2.2f, -4.2f), P(2.4f, -4.2f), P(3.8f, -1.0f) }; dl->AddConvexPolyFilled(cab, 4, wf); }
            dl->AddCircleFilled(P(-3.6f, 3.4f), 1.6f * u, Col(0, 0, 0, alpha), 16);
            dl->AddCircleFilled(P(3.6f, 3.4f), 1.6f * u, Col(0, 0, 0, alpha), 16);
            dl->AddCircle(P(-3.6f, 3.4f), 1.6f * u, w, 16, th * 0.8f);
            dl->AddCircle(P(3.6f, 3.4f), 1.6f * u, w, 16, th * 0.8f);
            break;
        case 4: // boutique 24/7 : sac de courses
            dl->AddRectFilled(P(-4.6f, -2.0f), P(4.6f, 5.6f), wf, 1.2f * u);
            dl->PathArcTo(P(0.0f, -2.0f), 2.6f * u, 3.14159f, 6.28318f, 16);
            dl->PathStroke(w, false, th);
            dl->AddCircleFilled(P(-2.2f, 0.4f), 0.7f * u, Col(0, 0, 0, alpha), 10);
            dl->AddCircleFilled(P(2.2f, 0.4f), 0.7f * u, Col(0, 0, 0, alpha), 10);
            break;
        case 5: // vestiaire : casier
            dl->AddRect(P(-4.2f, -5.6f), P(4.2f, 5.6f), w, 1.0f * u, ImDrawCornerFlags_All, th);
            dl->AddLine(P(0.0f, -5.6f), P(0.0f, 5.6f), w, th);
            for (int i = 0; i < 3; i++)
            {
                dl->AddLine(P(-3.0f, -4.0f + i * 1.2f), P(-1.2f, -4.0f + i * 1.2f), w, th * 0.7f);
                dl->AddLine(P(1.2f, -4.0f + i * 1.2f), P(3.0f, -4.0f + i * 1.2f), w, th * 0.7f);
            }
            dl->AddCircleFilled(P(-1.0f, 1.0f), 0.6f * u, w, 8);
            dl->AddCircleFilled(P(1.0f, 1.0f), 0.6f * u, w, 8);
            break;
        case 6: case 12: // banque / braquage : $
            if (renderer)
            {
                const float fs = r * 1.25f;
                ImVec2 ts = renderer->calculateTextSize("$", fs);
                renderer->drawText(ImVec2(c.x - ts.x / 2.0f, c.y - ts.y / 2.0f), ImColor(255, 255, 255, (int)(255 * alpha)), "$", false, fs);
            }
            if (type == 12) dl->AddCircle(c, r * 0.78f, Col(255, 80, 80, 0.9f * alpha), 32, th);
            break;
        case 7: // emploi : mallette
            dl->AddRectFilled(P(-5.4f, -2.2f), P(5.4f, 4.8f), wf, 1.2f * u);
            dl->AddRect(P(-2.0f, -4.6f), P(2.0f, -2.2f), w, 0.6f * u, ImDrawCornerFlags_All, th);
            dl->AddLine(P(-5.4f, 0.8f), P(5.4f, 0.8f), Col(0, 0, 0, alpha), th);
            break;
        case 8: case 9: // entree / sortie : porte + fleche
        {
            dl->AddRect(P(-4.6f, -5.6f), P(0.8f, 5.6f), w, 0.6f * u, ImDrawCornerFlags_All, th);
            dl->AddCircleFilled(P(-0.8f, 0.4f), 0.6f * u, w, 8);
            const float dir = (type == 8) ? -1.0f : 1.0f;      // entree : fleche vers la porte
            const float x0 = (type == 8) ? 6.0f : 2.0f, x1 = (type == 8) ? 2.0f : 6.0f;
            dl->AddLine(P(x0, 0.0f), P(x1, 0.0f), w, th);
            dl->AddTriangleFilled(P(x1 + dir * 1.8f, 0.0f), P(x1, -1.6f), P(x1, 1.6f), w);
            break;
        }
        case 10: // peche : poisson
            {
                ImVec2 el[20];
                for (int k = 0; k < 20; k++) { const float a = (float)k / 20.0f * 6.2831853f; el[k] = P(-0.8f + std::cos(a) * 4.4f, std::sin(a) * 2.6f); }
                dl->AddConvexPolyFilled(el, 20, wf);
            }
            dl->AddTriangleFilled(P(3.0f, 0.0f), P(6.2f, -3.0f), P(6.2f, 3.0f), wf);
            dl->AddCircleFilled(P(-3.2f, -0.6f), 0.6f * u, Col(0, 0, 0, alpha), 8);
            break;
        case 11: // maison
            dl->AddTriangleFilled(P(0.0f, -6.0f), P(-6.0f, -0.6f), P(6.0f, -0.6f), wf);
            dl->AddRectFilled(P(-4.4f, -0.8f), P(4.4f, 5.4f), wf, 0.4f * u);
            dl->AddRectFilled(P(-1.3f, 1.6f), P(1.3f, 5.4f), Col(0, 0, 0, alpha), 0.3f * u);
            break;
        case 13: // accessoires : lunettes
            dl->AddCircle(P(-3.0f, 0.6f), 2.4f * u, w, 20, th);
            dl->AddCircle(P(3.0f, 0.6f), 2.4f * u, w, 20, th);
            dl->AddLine(P(-0.6f, 0.0f), P(0.6f, 0.0f), w, th);
            dl->AddLine(P(-5.4f, 0.0f), P(-6.6f, -1.6f), w, th);
            dl->AddLine(P(5.4f, 0.0f), P(6.6f, -1.6f), w, th);
            break;
        case 14: // barbier : ciseaux
            dl->AddCircle(P(-2.6f, 3.6f), 1.8f * u, w, 16, th);
            dl->AddCircle(P(2.6f, 3.6f), 1.8f * u, w, 16, th);
            dl->AddLine(P(-1.6f, 2.2f), P(3.2f, -5.6f), w, th);
            dl->AddLine(P(1.6f, 2.2f), P(-3.2f, -5.6f), w, th);
            break;
        case 15: // tatouage : aiguille + goutte
            dl->AddLine(P(-4.6f, 4.6f), P(3.0f, -3.0f), w, th * 1.6f);
            dl->AddLine(P(3.0f, -3.0f), P(5.2f, -5.2f), w, th * 0.6f);
            dl->AddCircleFilled(P(-3.0f, -2.6f), 1.4f * u, wf, 12);
            dl->AddTriangleFilled(P(-3.0f, -5.4f), P(-4.3f, -2.9f), P(-1.7f, -2.9f), wf);
            break;
        case 16: // station-service : pompe
            dl->AddRectFilled(P(-5.0f, -5.6f), P(1.6f, 5.6f), wf, 0.8f * u);
            dl->AddRectFilled(P(-3.8f, -4.2f), P(0.4f, -1.0f), Col(0, 0, 0, alpha), 0.4f * u);
            dl->AddLine(P(1.6f, -2.4f), P(4.2f, -2.4f), w, th);
            dl->AddLine(P(4.2f, -2.4f), P(4.2f, 3.6f), w, th);
            dl->AddLine(P(4.2f, 3.6f), P(2.8f, 3.6f), w, th);
            dl->AddLine(P(4.2f, -2.4f), P(5.6f, -4.4f), w, th);
            break;
        case 17: // location : cle
            dl->AddCircle(P(-3.0f, 0.0f), 2.8f * u, w, 20, th * 1.3f);
            dl->AddLine(P(-0.2f, 0.0f), P(6.0f, 0.0f), w, th * 1.3f);
            dl->AddLine(P(4.4f, 0.0f), P(4.4f, 2.4f), w, th * 1.1f);
            dl->AddLine(P(2.4f, 0.0f), P(2.4f, 1.8f), w, th * 1.1f);
            break;
        default: // epingle
            dl->AddCircleFilled(c, 2.6f * u, wf, 16);
            break;
        }
    }

    void Badge(ImDrawList* dl, ImGuiRenderer* renderer, int type, ImVec2 c, float r, float alpha)
    {
        dl->AddCircleFilled(ImVec2(c.x, c.y + r * 0.06f), r * 1.06f, Col(0, 0, 0, 0.25f * alpha), 40);
        dl->AddCircleFilled(c, r, Col(0, 0, 0, 0.62f * alpha), 40);
        dl->AddCircle(c, r, Col(255, 255, 255, 0.85f * alpha), 40, std::fmax(1.5f, r * 0.08f));
        Glyph(dl, renderer, type, c, r * 0.78f, alpha);
    }

}

void WorldMarkers::Set(const char* data)
{
    std::vector<Marker> next;
    if (data)
    {
        std::string all(data);
        size_t pos = 0;
        while (pos < all.size() && next.size() < kMaxMarkers)
        {
            size_t end = all.find(';', pos);
            if (end == std::string::npos) end = all.size();
            std::string item = all.substr(pos, end - pos);
            pos = end + 1;
            Marker m;
            char label[32] = { 0 };
            int n = sscanf(item.c_str(), "%d,%f,%f,%f,%31[^;]", &m.type, &m.x, &m.y, &m.z, label);
            if (n < 4 || m.type <= 0) continue;
            strncpy(m.label, label, sizeof(m.label) - 1);
            next.push_back(m);
        }
    }
    std::lock_guard<std::mutex> lock(s_mutex);
    s_markers.swap(next);
}

void WorldMarkers::Render(ImGuiRenderer* renderer)
{
    std::vector<Marker> list;
    {
        std::lock_guard<std::mutex> lock(s_mutex);
        if (s_markers.empty()) return;
        list = s_markers;
    }
    if (!pGame) return;
    if (CharacterCreator::IsVisible()) return;   // [v12h] pas d'icone 3D (t-shirt...) devant le perso pendant l'essayage
    ImDrawList* dl = ImGui::GetBackgroundDrawList();
    if (!dl) return;

    const float t = NowSec();
    CVector me;
    const bool onFoot = LocalFootPos(me) && !CharacterCreator::IsVisible();   // pas de main pendant l'essayage
    bool  handShown = false;
    float handX = 0.0f, handY = 0.0f, handR = 0.0f, bestD = 1e9f;
    for (const Marker& m : list)
    {
        // main d'interaction : TOUS les points (vetements, boutique, vestiaire, banque, garage...) tout pres du joueur
        if (onFoot)
        {
            const float dx = me.x - m.x, dy = me.y - m.y;
            const float d2 = std::sqrt(dx * dx + dy * dy);
            if (d2 < kHandDist2D && std::fabs(me.z - m.z) < kHandDistZ && d2 < bestD)
            {
                ImVec2 hc, hb, hu;
                float dh, db, du;
                const float bob = std::sin(t * 2.4f) * 0.05f;
                if (ToScreen(m.x, m.y, m.z + 1.85f + bob, hc, dh) && ToScreen(m.x, m.y, m.z, hb, db) && ToScreen(m.x, m.y, m.z + 1.0f, hu, du))
                {
                    const float ppmH = std::fabs(hb.y - hu.y);
                    bestD = d2;
                    handShown = true;
                    handX = hc.x; handY = hc.y;
                    handR = std::fmax(UISettings::fontSize() * 1.25f, std::fmin(ppmH * 0.26f, UISettings::fontSize() * 2.6f));
                }
            }
        }
    }
    for (const Marker& m : list)
    {
        ImVec2 base;
        float depth;
        if (!ToScreen(m.x, m.y, m.z, base, depth)) continue;
        if (depth > kMaxDist) continue;
        float alpha = depth <= kFadeFrom ? 1.0f : 1.0f - (depth - kFadeFrom) / (kMaxDist - kFadeFrom);

        // pixels par metre a cette distance (projection d'un point 1 m plus haut)
        ImVec2 up;
        float d2;
        if (!ToScreen(m.x, m.y, m.z + 1.0f, up, d2)) continue;
        const float ppm = std::fabs(base.y - up.y);
        if (ppm < 2.0f) continue;

        if (m.type == 1)
        {
            // t-shirt : flotte a ~1 m du sol, monte/descend et tourne sur lui-meme
            const float bob = std::sin(t * 2.0f) * 0.08f;
            ImVec2 c;
            float dc;
            if (!ToScreen(m.x, m.y, m.z + 0.95f + bob, c, dc)) continue;
            float spin = std::cos(t * 1.6f);
            float sx = ppm * 0.42f * (0.25f + 0.75f * std::fabs(spin));
            float sy = ppm * 0.42f;
            // ombre douce au sol
            dl->AddCircleFilled(base, ppm * 0.3f, Col(0, 0, 0, 0.18f * alpha), 24);
            GroundRing(dl, m, 0.45f, Col(255, 255, 255, 0.35f * alpha), std::fmax(1.0f, ppm * 0.025f));
            Shirt(dl, c, sx, sy, alpha);
        }
        else
        {
            // icone 3D : pastille qui flotte a ~1 m du sol + anneau au sol
            const float pulse = 0.5f + 0.5f * std::sin(t * 3.0f);
            GroundRing(dl, m, 0.55f, Col(255, 255, 255, 0.75f * alpha), std::fmax(1.5f, ppm * 0.04f));
            GroundRing(dl, m, 0.55f + 0.25f * pulse, Col(255, 255, 255, 0.3f * (1.0f - pulse) * alpha), std::fmax(1.0f, ppm * 0.03f));
            ImVec2 c;
            float dc;
            const float bob = std::sin(t * 2.0f) * 0.06f;
            if (!ToScreen(m.x, m.y, m.z + 1.0f + bob, c, dc)) continue;
            dl->AddLine(base, ImVec2(c.x, c.y + ppm * 0.3f), Col(255, 255, 255, 0.35f * alpha), std::fmax(1.0f, ppm * 0.02f));
            Badge(dl, renderer, m.type, c, ppm * 0.3f, alpha);
        }
    }

    bool pressed = false;
    {
        std::lock_guard<std::mutex> lock(s_handMutex);
        s_hand.visible = handShown;
        s_hand.x = handX; s_hand.y = handY; s_hand.r = handR;
        if (!handShown) s_hand.down = false;
        pressed = s_hand.down;
    }
    if (handShown)
    {
        ImDrawList* fg = ImGui::GetForegroundDrawList();
        if (fg)
        {
            Hand(fg, ImVec2(handX, handY), handR, 1.0f, pressed);
            // [Crestwood] pas de texte : la main suffit

        }
    }
}

bool WorldMarkers::OnTouch(int type, int finger, int x, int y)
{
    // type : 2 = appui, 3 = deplacement, 1 = relachement
    bool fire = false;
    {
        std::lock_guard<std::mutex> lock(s_handMutex);
        HandState& h = s_hand;
        const float fx = (float)x, fy = (float)y;
        if (type == 2)
        {
            if (!h.visible || h.down) return false;
            if (std::hypot(fx - h.x, fy - h.y) > h.r * 1.35f) return false;
            h.down = true;
            h.finger = finger;
            h.downX = fx; h.downY = fy;
            h.pressMs = NowMs();
            return true;
        }
        if (!h.down) return false;
        const bool mine = (finger == h.finger) || std::hypot(fx - h.downX, fy - h.downY) < h.r * 2.0f;
        if (!mine) return false;
        if (type == 3) return true;
        if (type == 1)
        {
            h.down = false;
            const bool inside = std::hypot(fx - h.x, fy - h.y) <= h.r * 2.0f || (fx == 0.0f && fy == 0.0f);
            if (inside && NowMs() - h.lastSendMs > kHandCooldownMs)
            {
                h.lastSendMs = NowMs();
                fire = true;
            }
        }
    }
    if (fire && pNetGame) pNetGame->SendChatCommand("/interagir");
    return true;
}
