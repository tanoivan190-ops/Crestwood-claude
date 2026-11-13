// Boutons de combat natifs (voir CombatButtons.h). Fichier IDENTIQUE dans samp/ et samp32/.
#include "main.h"
#include "game/game.h"
#include "net/netgame.h"
#include "gui/gui.h"
#include "CombatButtons.h"
#include "gui/CharacterCreator.h"

#include <atomic>
#include <chrono>
#include <cmath>
#include <mutex>

extern CGame* pGame;
extern CNetGame* pNetGame;
void HudPad_SetControl(int control, bool pressed);   // game/pad.cpp

namespace
{
    // Position des boutons (fraction de l'ecran) : colonne droite, au-dessus des boutons natifs de tir.
    constexpr float kAimX = 0.885f, kAimY = 0.40f;    // VISER (a pied)
    constexpr float kFireX = 0.885f, kFireY = 0.52f;  // TIRER (passager)
    constexpr float kRadiusH = 0.068f;                // rayon = 6,8 % de la hauteur d'ecran

    enum class Mode { None, Aim, PassengerFire };

    struct Btn
    {
        bool  visible = false;
        float x = 0.0f, y = 0.0f, r = 0.0f;
        bool  down = false;
        int   finger = -1;
        float downX = 0.0f, downY = 0.0f;
    };

    std::mutex s_mutex;
    Btn  s_aim, s_fire;
    bool s_aimOn = false;                    // visee permanente active
    std::atomic<bool> s_wantDriveBy{false};  // passager : demande d'entree en drive-by
    std::atomic<bool> s_fireHeld{false};

    float NowSec()
    {
        using namespace std::chrono;
        return (float)(duration_cast<milliseconds>(steady_clock::now().time_since_epoch()).count() % 3600000) / 1000.0f;
    }

    ImU32 Col(int r, int g, int b, float a)
    {
        if (a < 0.0f) a = 0.0f;
        if (a > 1.0f) a = 1.0f;
        return IM_COL32(r, g, b, (int)(a * 255.0f));
    }

    bool IsFirearm(int w) { return w >= 22 && w <= 38; }
    bool IsSmg(int w) { return w == 28 || w == 29 || w == 32; }   // Uzi, MP5, Tec-9 (drive-by SA-MP)

    Mode CurrentMode()
    {
        if (!pGame || !pNetGame) return Mode::None;
        if (CharacterCreator::IsVisible()) return Mode::None;
        CPlayerPed* ped = pGame->FindPlayerPed();
        if (!ped || !ped->m_pPed) return Mode::None;
        const int w = (int)ped->GetCurrentWeapon();
        if (ped->IsInVehicle())
            return (ped->IsAPassenger() && IsSmg(w)) ? Mode::PassengerFire : Mode::None;
        (void) w;
        return Mode::None;   // [v12f] bouton VISER perso retire : on garde le bouton de visee natif de GTA
    }

    void SetAim(bool on)
    {
        if (s_aimOn == on) return;
        s_aimOn = on;
        HudPad_SetControl(12, on);
    }

    void SetFire(bool on)
    {
        if (s_fireHeld.exchange(on) == on) return;
        HudPad_SetControl(11, on);
        if (on) s_wantDriveBy.store(true);
    }

    void DrawButton(ImGuiRenderer* renderer, ImDrawList* dl, const Btn& b, const char* label, bool active, bool danger)
    {
        const float r = b.down ? b.r * 0.93f : b.r;
        const ImVec2 c(b.x, b.y);
        dl->AddCircleFilled(c, r, active ? Col(59, 130, 255, 0.55f) : Col(0, 0, 0, 0.45f), 48);
        dl->AddCircle(c, r, danger ? Col(255, 80, 80, 0.95f) : Col(255, 255, 255, 0.85f), 48, std::fmax(2.0f, r * 0.07f));
        // reticule
        const ImU32 w = Col(255, 255, 255, 0.95f);
        const float t = std::fmax(1.5f, r * 0.06f);
        dl->AddCircle(c, r * 0.36f, w, 32, t);
        dl->AddLine(ImVec2(c.x, c.y - r * 0.62f), ImVec2(c.x, c.y - r * 0.18f), w, t);
        dl->AddLine(ImVec2(c.x, c.y + r * 0.18f), ImVec2(c.x, c.y + r * 0.62f), w, t);
        dl->AddLine(ImVec2(c.x - r * 0.62f, c.y), ImVec2(c.x - r * 0.18f, c.y), w, t);
        dl->AddLine(ImVec2(c.x + r * 0.18f, c.y), ImVec2(c.x + r * 0.62f, c.y), w, t);
        dl->AddCircleFilled(c, std::fmax(1.5f, r * 0.06f), w, 12);
        if (active)
        {
            const float pulse = std::fmod(NowSec() * 1.4f, 1.0f);
            dl->AddCircle(c, r * (1.04f + 0.18f * pulse), Col(59, 130, 255, 0.6f * (1.0f - pulse)), 48, t);
        }
        if (renderer && label)
        {
            const float fs = UISettings::fontSize() * 0.5f;
            ImVec2 ts = renderer->calculateTextSize(label, fs);
            renderer->drawText(ImVec2(c.x - ts.x / 2.0f, c.y + r + 3.0f), ImColor(255, 255, 255, 235), label, true, fs);
        }
    }

    bool Hit(const Btn& b, float x, float y, float k) { return b.visible && std::hypot(x - b.x, y - b.y) <= b.r * k; }
}

bool CombatButtons::WantsPassengerDriveBy()
{
    return s_wantDriveBy.exchange(false);
}

void CombatButtons::Render(ImGuiRenderer* renderer)
{
    const Mode mode = CurrentMode();
    const ImVec2 ds = ImGui::GetIO().DisplaySize;
    std::lock_guard<std::mutex> lock(s_mutex);

    // plus d'arme a feu / monte en vehicule : la visee permanente s'arrete
    if (mode != Mode::Aim) { SetAim(false); s_aim.down = false; }
    if (mode != Mode::PassengerFire) { SetFire(false); s_fire.down = false; }

    const float r = ds.y * kRadiusH;
    s_aim.visible = (mode == Mode::Aim) && ds.x > 1.0f;
    s_aim.x = ds.x * kAimX; s_aim.y = ds.y * kAimY; s_aim.r = r;
    s_fire.visible = (mode == Mode::PassengerFire) && ds.x > 1.0f;
    s_fire.x = ds.x * kFireX; s_fire.y = ds.y * kFireY; s_fire.r = r;

    if (!s_aim.visible && !s_fire.visible) return;
    ImDrawList* dl = ImGui::GetForegroundDrawList();
    if (!dl) return;
    if (s_aim.visible) DrawButton(renderer, dl, s_aim, s_aimOn ? "VISEE ON" : "VISER", s_aimOn, false);
    if (s_fire.visible) DrawButton(renderer, dl, s_fire, "TIRER", s_fireHeld.load(), true);
}

bool CombatButtons::OnTouch(int type, int finger, int x, int y)
{
    std::lock_guard<std::mutex> lock(s_mutex);
    const float fx = (float)x, fy = (float)y;
    // type : 2 = appui, 3 = deplacement, 1 = relachement
    if (type == 2)
    {
        if (!s_aim.down && Hit(s_aim, fx, fy, 1.15f))
        {
            s_aim.down = true; s_aim.finger = finger; s_aim.downX = fx; s_aim.downY = fy;
            SetAim(!s_aimOn);          // bascule : visee permanente
            return true;
        }
        if (!s_fire.down && Hit(s_fire, fx, fy, 1.15f))
        {
            s_fire.down = true; s_fire.finger = finger; s_fire.downX = fx; s_fire.downY = fy;
            SetFire(true);             // maintenu : drive-by
            return true;
        }
        return false;
    }
    auto mine = [&](const Btn& b) { return b.down && (finger == b.finger || std::hypot(fx - b.downX, fy - b.downY) < b.r * 2.5f); };
    if (mine(s_aim))
    {
        if (type == 1) s_aim.down = false;
        return true;
    }
    if (mine(s_fire))
    {
        if (type == 1) { s_fire.down = false; SetFire(false); }
        return true;
    }
    return false;
}
