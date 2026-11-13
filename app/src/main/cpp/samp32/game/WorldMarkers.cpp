// Marqueurs 3D flottants (voir WorldMarkers.h). Dessin ImGui, projection monde -> ecran avec
// CSprite::CalcScreenCoors (meme fonction que PlayerTap). Fichier IDENTIQUE dans samp/ et samp32/.
#include "main.h"
#include "game/game.h"
#include "net/netgame.h"
#include "gui/gui.h"
#include "WorldMarkers.h"
#include "gui/CharacterCreator.h"
#include "World.h"

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

    constexpr float kMaxDist   = 90.0f;   // [v22] au-dela : invisible (nom + distance visibles de plus loin)
    constexpr float kFadeFrom  = 65.0f;   // fondu entre 65 et 90 m
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

    // [v22] Icone "main" : le marqueur bleu (carre arrondi) avec une main blanche, onde bleue = "touche ici"
    void Hand(ImDrawList* dl, ImVec2 c, float r, float alpha, bool pressed)
    {
        const float k = pressed ? 0.9f : 1.0f;
        r *= k;
        const float t = NowSec(), rr = r * 0.42f;
        const float pulse = std::fmod(t * 1.1f, 1.0f);
        const float pr = r * (1.08f + 0.38f * pulse);
        dl->AddRect(ImVec2(c.x - pr, c.y - pr), ImVec2(c.x + pr, c.y + pr), Col(59, 130, 255, 0.55f * (1.0f - pulse) * alpha), rr * (pr / r), ImDrawCornerFlags_All, std::fmax(1.5f, r * 0.07f));
        dl->AddRectFilled(ImVec2(c.x - r, c.y - r + r * 0.12f), ImVec2(c.x + r, c.y + r + r * 0.12f), Col(0, 0, 0, 0.32f * alpha), rr);
        dl->AddRectFilled(ImVec2(c.x - r, c.y - r), ImVec2(c.x + r, c.y + r), pressed ? Col(37, 99, 220, alpha) : Col(59, 130, 255, 0.97f * alpha), rr);
        dl->AddRectFilled(ImVec2(c.x - r, c.y - r), ImVec2(c.x + r, c.y - r * 0.15f), Col(255, 255, 255, 0.12f * alpha), rr, ImDrawCornerFlags_Top);
        dl->AddRect(ImVec2(c.x - r, c.y - r), ImVec2(c.x + r, c.y + r), Col(255, 255, 255, 0.55f * alpha), rr, ImDrawCornerFlags_All, std::fmax(1.2f, r * 0.06f));
        const ImVec2 tri[3] = { ImVec2(c.x - r * 0.24f, c.y + r - 1.0f), ImVec2(c.x + r * 0.24f, c.y + r - 1.0f), ImVec2(c.x, c.y + r * 1.3f) };
        dl->AddConvexPolyFilled(tri, 3, pressed ? Col(37, 99, 220, alpha) : Col(59, 130, 255, 0.97f * alpha));
        // main blanche : 4 doigts en gelule, paume arrondie, pouce
        const ImU32 w = Col(255, 255, 255, alpha);
        const float u = r / 10.0f, fw = 1.55f * u;
        const float fx[4] = { -3.25f, -1.45f, 0.35f, 2.15f };
        const float top[4] = { -5.6f, -6.5f, -6.1f, -4.9f };
        for (int i = 0; i < 4; i++)
            dl->AddRectFilled(ImVec2(c.x + fx[i] * u, c.y + top[i] * u), ImVec2(c.x + fx[i] * u + fw, c.y + 1.2f * u), w, fw * 0.5f);
        dl->AddRectFilled(ImVec2(c.x - 3.3f * u, c.y - 0.4f * u), ImVec2(c.x + 3.75f * u, c.y + 4.6f * u), w, 2.6f * u);
        const ImVec2 th[4] = { ImVec2(c.x - 3.2f * u, c.y + 1.6f * u), ImVec2(c.x - 5.7f * u, c.y - 0.9f * u),
                               ImVec2(c.x - 4.6f * u, c.y - 2.1f * u), ImVec2(c.x - 2.4f * u, c.y - 0.2f * u) };
        dl->AddConvexPolyFilled(th, 4, w);
        dl->AddCircleFilled(ImVec2(c.x - 5.15f * u, c.y - 1.5f * u), 0.85f * u, w, 12);
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

    const char* TypeName(int type)
    {
        switch (type)
        {
            case 1: return "Vêtements";      case 2: return "Garage";        case 4: return "Boutique 24/7";
            case 5: return "Vestiaire";      case 6: return "Banque";        case 7: return "Emploi";
            case 8: return "Entrée";         case 9: return "Sortie";        case 10: return "Pêche";
            case 11: return "Maison";        case 12: return "Braquage";     case 13: return "Accessoires";
            case 14: return "Barbier";       case 15: return "Tatouage";     case 16: return "Station-service";
            case 17: return "Location";      case 18: return "PNJ";
        }
        return "Point d'intérêt";
    }

    // [v22] marqueur bleu facon "point de collecte" : carre bleu arrondi + pictogramme blanc + pointe,
    //       nom du lieu et distance a droite
    void Badge(ImDrawList* dl, ImGuiRenderer* renderer, int type, ImVec2 c, float r, float alpha, const char* name, float dist)
    {
        const float rr = r * 0.42f;
        dl->AddRectFilled(ImVec2(c.x - r, c.y - r + r * 0.14f), ImVec2(c.x + r, c.y + r + r * 0.14f), Col(0, 0, 0, 0.30f * alpha), rr);
        dl->AddRectFilled(ImVec2(c.x - r, c.y - r), ImVec2(c.x + r, c.y + r), Col(59, 130, 255, 0.96f * alpha), rr);
        dl->AddRectFilled(ImVec2(c.x - r, c.y - r), ImVec2(c.x + r, c.y - r * 0.15f), Col(255, 255, 255, 0.10f * alpha), rr, ImDrawCornerFlags_Top);
        dl->AddRect(ImVec2(c.x - r, c.y - r), ImVec2(c.x + r, c.y + r), Col(255, 255, 255, 0.45f * alpha), rr, ImDrawCornerFlags_All, std::fmax(1.0f, r * 0.07f));
        const ImVec2 tri[3] = { ImVec2(c.x - r * 0.26f, c.y + r - 1.0f), ImVec2(c.x + r * 0.26f, c.y + r - 1.0f), ImVec2(c.x, c.y + r * 1.34f) };
        dl->AddConvexPolyFilled(tri, 3, Col(59, 130, 255, 0.96f * alpha));
        Glyph(dl, renderer, type, c, r * 0.80f, alpha);
        if (!renderer || !name || !name[0]) return;
        const float fs = UISettings::fontSize();
        const float lf = std::fmax(fs * 0.62f, std::fmin(r * 0.82f, fs * 0.9f)), df = lf * 0.78f;
        const std::string nm(name);
        char d[16];
        if (dist >= 1000.0f) snprintf(d, sizeof(d), "%.1f km", dist / 1000.0f); else snprintf(d, sizeof(d), "%d m", (int)(dist + 0.5f));
        const std::string ds(d);
        const ImVec2 ns = renderer->calculateTextSize(nm, lf);
        const float tx = c.x + r * 1.32f, ty = c.y - ns.y * 0.92f;
        const int a = (int)(255.0f * alpha);
        renderer->drawText(ImVec2(tx + 1.0f, ty + 1.0f), ImColor(0, 0, 0, (int)(a * 0.7f)), nm, false, lf);
        renderer->drawText(ImVec2(tx, ty), ImColor(255, 255, 255, a), nm, false, lf);
        renderer->drawText(ImVec2(tx + 1.0f, ty + ns.y * 1.04f + 1.0f), ImColor(0, 0, 0, (int)(a * 0.7f)), ds, false, df);
        renderer->drawText(ImVec2(tx, ty + ns.y * 1.04f), ImColor(160, 192, 255, a), ds, false, df);
    }

}


// ======================================================================
//  [v20] GPS 3D : chevrons au sol + balise d'arrivee
// ======================================================================
namespace
{
    std::mutex         s_gpsMutex;
    bool               s_gpsOn = false;
    float              s_gpsTx = 0.0f, s_gpsTy = 0.0f, s_gpsTz = -10000.0f;
    std::vector<float> s_gpsPts;        // x0,y0,x1,y1,...
    uint32_t           s_gpsVersion = 0;

    // copie du thread du jeu (pas de verrou pendant le dessin)
    uint32_t           g_ver = 0xFFFFFFFFu;
    std::vector<float> g_pts;
    std::vector<float> g_z;             // hauteur du sol par point (NAN = pas encore calcule)
    float              g_tx = 0, g_ty = 0, g_tz = -10000.0f, g_tzFound = -10000.0f;
    bool               g_on = false;
    int                g_idx = -1;

    bool PlayerPos(CVector& out)
    {
        if (!pGame) return false;
        CPlayerPed* ped = pGame->FindPlayerPed();
        if (!ped || !ped->m_pPed) return false;
        out = ped->m_pPed->GetPosition();
        return true;
    }

    float GroundAt(float x, float y, float fallback)
    {
        const float z = CWorld::FindGroundZForCoord(x, y);
        if (!(z > -100.0f && z < 1500.0f) || std::fabs(z) < 0.001f) return fallback;   // zone pas encore chargee
        return z;
    }

    void FormatDist(float m, char* out, size_t n)
    {
        if (m < 1000.0f) snprintf(out, n, "%d m", (int)(m / 5.0f + 0.5f) * 5);
        else snprintf(out, n, "%.1f km", m / 1000.0f);
    }
}

void WorldMarkers::SetGps(const char* data)
{
    std::lock_guard<std::mutex> lock(s_gpsMutex);
    s_gpsVersion++;
    s_gpsPts.clear();
    s_gpsOn = false;
    if (!data || !data[0]) return;
    std::string all(data);
    size_t pos = 0;
    bool first = true;
    while (pos < all.size() && s_gpsPts.size() < 4000)
    {
        size_t end = all.find(';', pos);
        if (end == std::string::npos) end = all.size();
        const std::string item = all.substr(pos, end - pos);
        pos = end + 1;
        float a = 0, b = 0, z = -10000.0f;
        const int n = sscanf(item.c_str(), "%f,%f,%f", &a, &b, &z);
        if (n < 2) continue;
        if (first) { s_gpsTx = a; s_gpsTy = b; s_gpsTz = (n >= 3) ? z : -10000.0f; first = false; s_gpsOn = true; continue; }
        s_gpsPts.push_back(a);
        s_gpsPts.push_back(b);
    }
}

static void RenderGps(ImGuiRenderer* renderer)
{
    {
        std::lock_guard<std::mutex> lock(s_gpsMutex);
        if (s_gpsVersion != g_ver)
        {
            g_ver = s_gpsVersion;
            g_on = s_gpsOn;
            g_pts = s_gpsPts;
            g_z.assign(g_pts.size() / 2, NAN);
            g_tx = s_gpsTx; g_ty = s_gpsTy; g_tz = s_gpsTz; g_tzFound = -10000.0f;
            g_idx = -1;
        }
    }
    if (!g_on || !pGame) return;
    ImDrawList* dl = ImGui::GetBackgroundDrawList();
    if (!dl) return;
    CVector me;
    if (!PlayerPos(me)) return;
    const float t = NowSec();
    const float fs = UISettings::fontSize();

    // ---- chevrons au sol sur l'itineraire, devant le joueur ----
    const int n = (int)(g_pts.size() / 2);
    if (n >= 2)
    {
        // point du trace le plus proche du joueur (recherche locale apres la premiere fois)
        int from = 0, to = n - 1;
        if (g_idx >= 0) { from = std::max(0, g_idx - 6); to = std::min(n - 1, g_idx + 60); }
        float best = 1e12f;
        int bi = g_idx < 0 ? 0 : g_idx;
        for (int i = from; i <= to; i++)
        {
            const float dx = g_pts[i * 2] - me.x, dy = g_pts[i * 2 + 1] - me.y;
            const float d = dx * dx + dy * dy;
            if (d < best) { best = d; bi = i; }
        }
        g_idx = bi;
        int newZ = 0;
        auto zAt = [&](int i) -> float {
            float& z = g_z[(size_t)i];
            if (std::isnan(z))
            {
                const float dx = g_pts[i * 2] - me.x, dy = g_pts[i * 2 + 1] - me.y;
                if (newZ < 6 && dx * dx + dy * dy < 140.0f * 140.0f) { z = GroundAt(g_pts[i * 2], g_pts[i * 2 + 1], NAN); newZ++; }
                if (std::isnan(z)) return me.z - 0.9f;
            }
            return z;
        };
        constexpr float kStep = 7.5f, kAhead = 85.0f, kSkip = 5.0f;
        float walked = 0.0f, nextAt = kSkip;
        float cx = me.x, cy = me.y;
        for (int i = bi + 1; i < n && walked < kAhead; i++)
        {
            const float ax = (i == bi + 1) ? cx : g_pts[(i - 1) * 2], ay = (i == bi + 1) ? cy : g_pts[(i - 1) * 2 + 1];
            const float bx = g_pts[i * 2], by = g_pts[i * 2 + 1];
            const float sx = bx - ax, sy = by - ay;
            const float len = std::sqrt(sx * sx + sy * sy);
            if (len < 0.01f) continue;
            const float ux = sx / len, uy = sy / len, nx = -uy, ny = ux;
            const float za = (i == bi + 1) ? zAt(bi) : zAt(i - 1), zb = zAt(i);
            while (nextAt <= walked + len && walked < kAhead)
            {
                const float k = (nextAt - walked) / len;
                const float px = ax + sx * k, py = ay + sy * k, pz = za + (zb - za) * k + 0.12f;
                const float fade = 1.0f - std::min(1.0f, nextAt / kAhead) * 0.65f;
                const float a = fade * (nextAt < kSkip + 4.0f ? (nextAt - kSkip) / 4.0f : 1.0f);
                // chevron "V" pointe vers l'avant : deux bras (quads convexes), contour sombre puis bleu
                const float L = 0.75f, W = 1.05f, T = 0.32f;
                CVector q[6];
                auto P = [&](float f, float s) { CVector v; v.x = px + ux * f + nx * s; v.y = py + uy * f + ny * s; v.z = pz; return v; };
                q[0] = P(-L, -W); q[1] = P(L, 0.0f); q[2] = P(-L, W);
                q[3] = P(-L + T * 1.4f, -W); q[4] = P(L - T * 1.6f, 0.0f); q[5] = P(-L + T * 1.4f, W);
                ImVec2 s2[6];
                bool ok = true;
                float dd;
                for (int j = 0; j < 6 && ok; j++) ok = ToScreen(q[j].x, q[j].y, q[j].z, s2[j], dd);
                if (ok)
                {
                    const ImU32 blue = Col(59, 130, 255, 0.92f * a), edge = Col(10, 20, 45, 0.55f * a);
                    dl->AddQuadFilled(s2[0], s2[1], s2[4], s2[3], blue);
                    dl->AddQuadFilled(s2[4], s2[1], s2[2], s2[5], blue);
                    const ImVec2 outline[6] = { s2[0], s2[1], s2[2], s2[5], s2[4], s2[3] };
                    dl->AddPolyline(outline, 6, edge, true, std::max(1.0f, fs * 0.06f));
                }
                nextAt += kStep;
            }
            walked += len;
        }
    }

    // ---- balise d'arrivee : colonne de lumiere + epingle + distance ----
    const float dx = g_tx - me.x, dy = g_ty - me.y;
    const float dist = std::sqrt(dx * dx + dy * dy);
    float gz = g_tz;
    if (gz < -9000.0f)
    {
        if (g_tzFound > -9000.0f) gz = g_tzFound;
        else if (dist < 220.0f) { const float z = GroundAt(g_tx, g_ty, NAN); if (!std::isnan(z)) { g_tzFound = z; gz = z; } }
        if (gz < -9000.0f) gz = me.z - 1.0f;
    }
    ImVec2 base, top, pin, pinUp;
    float d0, d1, d2, d3;
    const bool baseOk = ToScreen(g_tx, g_ty, gz, base, d0);
    const bool topOk = ToScreen(g_tx, g_ty, gz + 28.0f, top, d1);
    if (baseOk && topOk)
    {
        // colonne : segments de plus en plus transparents vers le haut
        constexpr int K = 10;
        const float wcol = std::max(2.0f, std::min(fs * 0.5f, 900.0f / std::max(8.0f, d0)));
        for (int k = 0; k < K; k++)
        {
            const float f0 = (float)k / K, f1 = (float)(k + 1) / K;
            const ImVec2 a(base.x + (top.x - base.x) * f0, base.y + (top.y - base.y) * f0);
            const ImVec2 b(base.x + (top.x - base.x) * f1, base.y + (top.y - base.y) * f1);
            dl->AddLine(a, b, Col(59, 130, 255, 0.55f * (1.0f - f0)), wcol);
        }
    }
    const float bob = std::sin(t * 2.2f) * 0.18f;
    if (ToScreen(g_tx, g_ty, gz + 3.2f + bob, pin, d2) && ToScreen(g_tx, g_ty, gz + 4.2f + bob, pinUp, d3))
    {
        const float ppm = std::fabs(pin.y - pinUp.y);
        const float r = std::max(fs * 0.85f, std::min(fs * 1.9f, ppm * 0.55f));
        // epingle : pastille bleue (filet blanc interieur facon plaque) + pointe vers le sol
        dl->AddTriangleFilled(ImVec2(pin.x - r * 0.55f, pin.y + r * 0.55f), ImVec2(pin.x + r * 0.55f, pin.y + r * 0.55f), ImVec2(pin.x, pin.y + r * 1.55f), Col(59, 130, 255, 0.98f));
        dl->AddCircleFilled(ImVec2(pin.x, pin.y + r * 0.08f), r * 1.08f, Col(0, 0, 0, 0.30f), 32);
        dl->AddCircleFilled(pin, r, Col(59, 130, 255, 0.98f), 32);
        dl->AddCircle(pin, r * 0.80f, Col(255, 255, 255, 0.85f), 32, std::max(1.2f, r * 0.08f));
        // drapeau d'arrivee
        const float u = r / 10.0f;
        dl->AddLine(ImVec2(pin.x - 2.6f * u, pin.y - 4.8f * u), ImVec2(pin.x - 2.6f * u, pin.y + 4.8f * u), Col(255, 255, 255, 1.0f), std::max(1.5f, u * 1.2f));
        const ImVec2 flag[4] = { ImVec2(pin.x - 2.4f * u, pin.y - 4.8f * u), ImVec2(pin.x + 4.2f * u, pin.y - 3.2f * u), ImVec2(pin.x + 4.2f * u, pin.y - 3.2f * u), ImVec2(pin.x - 2.4f * u, pin.y - 0.8f * u) };
        dl->AddTriangleFilled(flag[0], flag[1], flag[3], Col(255, 255, 255, 1.0f));
        if (renderer)
        {
            char lab[24];
            FormatDist(dist, lab, sizeof(lab));
            const float tfs = std::max(fs * 0.85f, r * 0.62f);
            const ImVec2 ts = renderer->calculateTextSize(std::string(lab), tfs);
            const ImVec2 p0(pin.x - ts.x * 0.5f - 6.0f, pin.y - r - ts.y - 10.0f), p1(pin.x + ts.x * 0.5f + 6.0f, pin.y - r - 4.0f);
            dl->AddRectFilled(p0, p1, Col(17, 20, 25, 0.86f), 2.0f);
            renderer->drawText(ImVec2(p0.x + 6.0f, p0.y + 3.0f), ImColor(238, 241, 245, 255), std::string(lab), false, tfs);
        }
    }
    // anneaux au sol quand on approche
    if (dist < 70.0f && baseOk)
    {
        const float pulse = std::fmod(t * 0.8f, 1.0f);
        Marker m; m.x = g_tx; m.y = g_ty; m.z = gz;
        GroundRing(dl, m, 2.2f, Col(59, 130, 255, 0.85f), std::max(2.0f, fs * 0.12f));
        GroundRing(dl, m, 2.2f + 3.0f * pulse, Col(59, 130, 255, 0.6f * (1.0f - pulse)), std::max(1.5f, fs * 0.08f));
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
    if (pGame && !CharacterCreator::IsVisible()) RenderGps(renderer);   // [v20] GPS 3D
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
    const Marker* handM = nullptr;
    for (const Marker& m : list)
    {
        // main d'interaction : TOUS les points (vetements, boutique, vestiaire, banque, garage...) tout pres du joueur
        if (onFoot)
        {
            const float dx = me.x - m.x, dy = me.y - m.y;
            const float d2 = std::sqrt(dx * dx + dy * dy);
            const float lim = (m.type == 18) ? 2.4f : kHandDist2D;   // PNJ derriere un comptoir
            if (d2 < lim && std::fabs(me.z - m.z) < kHandDistZ && d2 < bestD)
            {
                ImVec2 hc, hb, hu;
                float dh, db, du;
                const float bob = std::sin(t * 2.4f) * 0.05f;
                const float hz = (m.type == 18) ? 2.25f : 1.85f;   // PNJ : au-dessus de la tete
                if (ToScreen(m.x, m.y, m.z + hz + bob, hc, dh) && ToScreen(m.x, m.y, m.z, hb, db) && ToScreen(m.x, m.y, m.z + 1.0f, hu, du))
                {
                    const float ppmH = std::fabs(hb.y - hu.y);
                    bestD = d2;
                    handM = &m;
                    handShown = true;
                    handX = hc.x; handY = hc.y;
                    handR = std::fmax(UISettings::fontSize() * 1.25f, std::fmin(ppmH * 0.26f, UISettings::fontSize() * 2.6f));
                }
            }
        }
    }
    for (const Marker& m : list)
    {
        if (handShown && &m == handM) continue;   // [v22] tout pres : la main bleue remplace le marqueur
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

        if (m.type == 18) continue;   // [v18] PNJ : pas de pastille (la main apparait de pres)
        if (m.type == 1 && false)   // [v22] le t-shirt devient un marqueur bleu comme les autres
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
            GroundRing(dl, m, 0.55f + 0.25f * pulse, Col(59, 130, 255, 0.4f * (1.0f - pulse) * alpha), std::fmax(1.0f, ppm * 0.03f));
            ImVec2 c;
            float dc;
            const float bob = std::sin(t * 2.0f) * 0.06f;
            if (!ToScreen(m.x, m.y, m.z + 1.0f + bob, c, dc)) continue;
            dl->AddLine(base, ImVec2(c.x, c.y + ppm * 0.3f), Col(255, 255, 255, 0.35f * alpha), std::fmax(1.0f, ppm * 0.02f));
            float dist = depth;
            if (onFoot) { const float ex = me.x - m.x, ey = me.y - m.y, ez = me.z - m.z; dist = std::sqrt(ex * ex + ey * ey + ez * ez); }
            const float fsz = UISettings::fontSize();
            const float br = std::fmax(fsz * 0.85f, std::fmin(ppm * 0.26f, fsz * 1.5f));
            Badge(dl, renderer, m.type, c, br, alpha, m.label[0] ? m.label : TypeName(m.type), dist);
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
            // [v22] nom du lieu + "Toucher pour interagir" a droite de la main
            if (renderer && handM)
            {
                const std::string nm(handM->label[0] ? handM->label : TypeName(handM->type));
                const std::string hint("Toucher pour interagir");
                const float fs = UISettings::fontSize();
                const float lf = std::fmax(fs * 0.7f, std::fmin(handR * 0.62f, fs * 1.0f)), hf = lf * 0.74f;
                const ImVec2 ns = renderer->calculateTextSize(nm, lf);
                const float tx = handX + handR * 1.35f, ty = handY - ns.y * 0.95f;
                renderer->drawText(ImVec2(tx + 1.0f, ty + 1.0f), ImColor(0, 0, 0, 180), nm, false, lf);
                renderer->drawText(ImVec2(tx, ty), ImColor(255, 255, 255, 255), nm, false, lf);
                renderer->drawText(ImVec2(tx + 1.0f, ty + ns.y * 1.05f + 1.0f), ImColor(0, 0, 0, 170), hint, false, hf);
                renderer->drawText(ImVec2(tx, ty + ns.y * 1.05f), ImColor(160, 192, 255, 255), hint, false, hf);
            }

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
