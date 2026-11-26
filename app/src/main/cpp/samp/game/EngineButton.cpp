// Bouton moteur natif (voir EngineButton.h). Fichier IDENTIQUE dans samp/ et samp32/.
#include "main.h"
#include "game/game.h"
#include "net/netgame.h"
#include "gui/gui.h"
#include "EngineButton.h"
#include "gui/CharacterCreator.h"
#include "game/Widgets/TouchInterface.h"

#include <atomic>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <mutex>

extern CGame* pGame;
extern CNetGame* pNetGame;
RwTexture* Crestwood_LoadApkTexture(const char* path);   // game/BrlTextureShader.cpp

namespace
{
    constexpr float kRadiusH = 0.074f;      // rayon = 7,4 % de la hauteur d'ecran (taille des pedales natives)
    constexpr float kHoldMove = 0.6f;       // appui long -> deplacer

    std::mutex s_mutex;
    bool  s_visible = false;
    float s_x = 0.0f, s_y = 0.0f, s_r = 0.0f;
    bool  s_down = false, s_moving = false, s_movedFar = false;
    int   s_finger = -1;
    float s_downX = 0.0f, s_downY = 0.0f, s_downT = 0.0f;
    float s_userX = -1.0f, s_userY = -1.0f;  // position choisie (fraction de l'ecran), -1 = automatique
    bool  s_loaded = false;
    std::atomic<bool> s_sendToggle{false};
    float s_pressAnim = 0.0f;

    float NowSec()
    {
        using namespace std::chrono;
        return (float)(duration_cast<milliseconds>(steady_clock::now().time_since_epoch()).count() % 3600000) / 1000.0f;
    }

    void PosFile(char* out, size_t n) { snprintf(out, n, "%screstwood_engine_btn.txt", g_pszStorage ? g_pszStorage : ""); }

    void LoadPos()
    {
        if (s_loaded) return;
        s_loaded = true;
        char path[512]; PosFile(path, sizeof path);
        FILE* f = fopen(path, "r");
        if (!f) return;
        float x = -1.0f, y = -1.0f;
        if (fscanf(f, "%f %f", &x, &y) == 2 && x > 0.0f && x < 1.0f && y > 0.0f && y < 1.0f) { s_userX = x; s_userY = y; }
        fclose(f);
    }

    void SavePos()
    {
        char path[512]; PosFile(path, sizeof path);
        FILE* f = fopen(path, "w");
        if (!f) return;
        fprintf(f, "%.4f %.4f\n", s_userX, s_userY);
        fclose(f);
    }

    bool DriverInfo(bool& engineOn)
    {
        engineOn = false;
        if (!pGame || !pNetGame) return false;
        if (CharacterCreator::IsVisible()) return false;
        if (pGame->IsGamePaused()) return false;
        CPlayerPed* ped = pGame->FindPlayerPed();
        if (!ped || !ped->m_pPed || !ped->IsInVehicle() || ped->IsAPassenger()) return false;
        CVehicleGTA* v = ped->GetGtaVehicle();
        if (!v) return false;
        const int model = v->m_nModelIndex;
        if (model == 481 || model == 509 || model == 510) return false;   // velos : pas de moteur
        engineOn = v->m_nVehicleFlags.bEngineOn;
        return true;
    }

    // position automatique : au-dessus du bouton de frein natif (sinon colonne droite)
    void AutoPos(const ImVec2& ds, float r, float& x, float& y)
    {
        x = ds.x * 0.80f; y = ds.y * 0.56f;
        if (!CTouchInterface::m_pWidgets) return;
        CWidgetGta* b = CTouchInterface::m_pWidgets[WIDGET_BRAKE];
        if (!b) return;
        const float l = std::fmin(b->m_RectScreen.left, b->m_RectScreen.right);
        const float rr = std::fmax(b->m_RectScreen.left, b->m_RectScreen.right);
        const float t = std::fmin(b->m_RectScreen.top, b->m_RectScreen.bottom);
        const float bt = std::fmax(b->m_RectScreen.top, b->m_RectScreen.bottom);
        if (rr - l < 8.0f || bt - t < 8.0f || rr > ds.x * 1.2f || bt > ds.y * 1.2f || l < -ds.x * 0.2f) return;
        x = (l + rr) * 0.5f;
        y = t - r * 1.45f;
        if (y < r * 1.2f) y = r * 1.2f;
    }
}

void EngineButton::Render(ImGuiRenderer* renderer)
{
    (void)renderer;
    // l'appui a ete vu par OnTouch (fil tactile) : la commande part sur le fil du jeu
    if (s_sendToggle.exchange(false) && pNetGame) pNetGame->SendChatCommand("/moteur");

    bool engineOn = false;
    const bool driver = DriverInfo(engineOn);
    const ImVec2 ds = ImGui::GetIO().DisplaySize;
    std::lock_guard<std::mutex> lock(s_mutex);
    LoadPos();
    // [v38] le bouton n'apparait que moteur COUPE : un appui le demarre et le bouton disparait.
    //       Pour couper le moteur : appui long (5 s) sur le vehicule -> roue (PlayerTap).
    s_visible = driver && !engineOn && ds.x > 1.0f && ds.y > 1.0f;
    if (!s_visible) { s_down = false; s_moving = false; return; }

    s_r = ds.y * kRadiusH;
    if (!s_moving)
    {
        if (s_userX > 0.0f) { s_x = ds.x * s_userX; s_y = ds.y * s_userY; }
        else AutoPos(ds, s_r, s_x, s_y);
    }

    RwTexture* tex = Crestwood_LoadApkTexture(engineOn ? "models/buttons/engine_on.png" : "models/buttons/engine_off.png");
    ImDrawList* dl = ImGui::GetForegroundDrawList();
    if (!dl) return;
    const float target = s_down ? 1.0f : 0.0f;
    s_pressAnim += (target - s_pressAnim) * 0.35f;
    const float r = s_r * (1.0f - 0.07f * s_pressAnim);
    const ImVec2 a(s_x - r, s_y - r), b(s_x + r, s_y + r);
    if (tex && tex->raster)
    {
        dl->AddImage((ImTextureID)tex->raster, a, b, ImVec2(0, 0), ImVec2(1, 1), IM_COL32(255, 255, 255, 235));
    }
    else
    {
        // texture pas encore chargee : cercle de verre dessine
        dl->AddCircleFilled(ImVec2(s_x, s_y), r, IM_COL32(220, 225, 235, 70), 48);
        dl->AddCircle(ImVec2(s_x, s_y), r, IM_COL32(255, 255, 255, 150), 48, std::fmax(2.0f, r * 0.05f));
        const ImU32 w = engineOn ? IM_COL32(120, 255, 160, 255) : IM_COL32(255, 255, 255, 240);
        dl->PathArcTo(ImVec2(s_x, s_y), r * 0.42f, -1.047f, 4.189f, 32);
        dl->PathStroke(w, 0, std::fmax(2.0f, r * 0.11f));
        dl->AddLine(ImVec2(s_x, s_y - r * 0.56f), ImVec2(s_x, s_y - r * 0.05f), w, std::fmax(2.0f, r * 0.11f));
    }
    if (s_moving)
    {
        const float pulse = std::fmod(NowSec() * 1.6f, 1.0f);
        dl->AddCircle(ImVec2(s_x, s_y), r * (1.05f + 0.15f * pulse), IM_COL32(255, 210, 63, (int)(200 * (1.0f - pulse))), 48, std::fmax(2.0f, r * 0.06f));
    }
}

bool EngineButton::OnTouch(int type, int finger, int x, int y)
{
    std::lock_guard<std::mutex> lock(s_mutex);
    const float fx = (float)x, fy = (float)y;
    // type : 2 = appui, 3 = deplacement, 1 = relachement
    if (type == 2)
    {
        if (!s_visible || s_down) return false;
        if (std::hypot(fx - s_x, fy - s_y) > s_r * 1.1f) return false;
        s_down = true; s_moving = false; s_movedFar = false;
        s_finger = finger; s_downX = fx; s_downY = fy; s_downT = NowSec();
        return true;
    }
    if (!s_down) return false;
    const bool mine = (finger == s_finger) || std::hypot(fx - s_downX, fy - s_downY) < s_r * 3.0f || s_moving;
    if (!mine) return false;
    float held = NowSec() - s_downT;
    if (held < 0.0f) held += 3600.0f;
    if (type == 3)
    {
        if (std::hypot(fx - s_downX, fy - s_downY) > s_r * 0.35f) s_movedFar = true;
        if (!s_moving && !s_movedFar && held >= kHoldMove) s_moving = true;
        if (s_moving) { s_x = fx; s_y = fy; }
        return true;
    }
    if (type == 1)
    {
        if (s_moving)
        {
            const ImVec2 ds = ImGui::GetIO().DisplaySize;
            if (ds.x > 1.0f && ds.y > 1.0f)
            {
                s_userX = std::fmin(0.97f, std::fmax(0.03f, fx / ds.x));
                s_userY = std::fmin(0.95f, std::fmax(0.05f, fy / ds.y));
                SavePos();
            }
        }
        else if (!s_movedFar)
        {
            s_sendToggle.store(true);
        }
        s_down = false; s_moving = false; s_finger = -1;
        return true;
    }
    return true;
}
