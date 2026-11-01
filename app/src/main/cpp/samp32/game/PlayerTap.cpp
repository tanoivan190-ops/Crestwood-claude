// Appui long sur un autre joueur -> envoie "/interaction <id> <x> <y>" au serveur, qui ouvre le menu
// d'interaction (assets/ui, style Reduto : boutons empiles a cote du joueur).
//  - le doigt doit rester POSE sur le joueur pendant kHoldMs (5 s) sans glisser ;
//  - un anneau de progression s'affiche sur le joueur des que l'appui dure un peu (pas de flash
//    quand on fait juste tourner la camera) ;
//  - x / y = position du joueur a l'ecran en pour-mille (0-1000) pour placer le menu a cote de lui.
// Projection monde -> ecran : CSprite::CalcScreenCoors (meme repere que les touches et que ImGui).
// Fichier IDENTIQUE dans samp/ (64 bits, moteur 2.11) et samp32/ (32 bits, moteur 2.10).
#include "main.h"
#include "game/game.h"
#include "net/netgame.h"
#include "gui/gui.h"
#include "PlayerTap.h"

#include <chrono>
#include <cmath>
#include <cstdio>

extern CGame* pGame;
extern CNetGame* pNetGame;

namespace
{
    constexpr uint32_t kHoldMs        = 5000;   // duree de l'appui long
    constexpr uint32_t kShowAfterMs   = 350;    // l'anneau n'apparait qu'apres ce delai
    constexpr float    kMaxMove       = 48.0f;  // le doigt peut bouger de ca (px) sans annuler
    constexpr float    kHitRadius     = 120.0f; // distance max (px) entre la touche et le joueur a l'ecran
    constexpr float    kMaxCamDist    = 30.0f;  // joueurs trop loin de la camera ignores
    constexpr uint32_t kCooldownMs    = 1500;

    struct Hold
    {
        bool     active = false;
        int      finger = -1;
        int      target = -1;
        uint32_t startMs = 0;
        float    x = 0.0f, y = 0.0f;
    };

    Hold     s_hold;
    uint32_t s_lastSendMs = 0;

    uint32_t NowMs()
    {
        using namespace std::chrono;
        return (uint32_t)duration_cast<milliseconds>(steady_clock::now().time_since_epoch()).count();
    }

    bool ToScreen(CVector pos, float& sx, float& sy)
    {
        CVector out;
        ((void (*)(CVector*, CVector*, float*, float*, bool, bool))(g_libGTASA + (VER_x32 ? 0x005C57E8 + 1 : 0x6E9DF8)))(&pos, &out, 0, 0, 0, 0);
        if (out.z < 1.0f) return false;   // derriere la camera
        sx = out.x;
        sy = out.y;
        return true;
    }

    CPlayerPed* LocalPed()
    {
        if (!pGame) return nullptr;
        CPlayerPed* local = pGame->FindPlayerPed();
        if (!local || !local->m_pPed || local->IsInVehicle()) return nullptr;
        return local;
    }

    CPlayerPed* RemotePed(int id)
    {
        if (!pNetGame || id < 0 || id >= MAX_PLAYERS) return nullptr;
        CPlayerPool* pool = pNetGame->GetPlayerPool();
        if (!pool || !pool->GetSlotState((PLAYERID)id)) return nullptr;
        CRemotePlayer* rp = pool->GetAt((PLAYERID)id);
        if (!rp || !rp->IsActive() || rp->IsNPC()) return nullptr;
        CPlayerPed* ped = rp->GetPlayerPed();
        if (!ped || !ped->m_pPed || !ped->m_pPed->IsAdded()) return nullptr;
        if (ped->m_pPed->GetDistanceFromCamera() > kMaxCamDist) return nullptr;
        return ped;
    }

    // Position a l'ecran du haut du corps (tete) d'un joueur distant
    bool HeadOnScreen(int id, float& sx, float& sy)
    {
        CPlayerPed* ped = RemotePed(id);
        if (!ped) return false;
        CVector p;
        p.x = 0.0f; p.y = 0.0f; p.z = 0.0f;
        ped->GetBonePosition(8, &p);
        return ToScreen(p, sx, sy);
    }

    int FindPlayerAt(float x, float y)
    {
        if (!pNetGame || !LocalPed()) return -1;
        int best = -1;
        float bestDist = kHitRadius;
        for (int id = 0; id < MAX_PLAYERS; id++)
        {
            CPlayerPed* ped = RemotePed(id);
            if (!ped) continue;
            // tete (8), torse (3), bassin (1) : on garde le point le plus proche de la touche
            static const int bones[] = { 8, 3, 1 };
            for (int b : bones)
            {
                CVector p;
                p.x = 0.0f; p.y = 0.0f; p.z = 0.0f;
                ped->GetBonePosition(b, &p);
                float sx, sy;
                if (!ToScreen(p, sx, sy)) continue;
                float d = std::hypot(sx - x, sy - y);
                if (d < bestDist) { bestDist = d; best = id; }
            }
        }
        return best;
    }

    void Cancel() { s_hold = Hold(); }

    void Fire(float sx, float sy)
    {
        const int id = s_hold.target;
        Cancel();
        s_lastSendMs = NowMs();
        if (!pNetGame) return;
        const ImVec2 ds = ImGui::GetIO().DisplaySize;
        int px = 500, py = 500;
        if (ds.x > 1.0f && ds.y > 1.0f)
        {
            px = (int)(sx / ds.x * 1000.0f);
            py = (int)(sy / ds.y * 1000.0f);
            if (px < 0) px = 0; if (px > 1000) px = 1000;
            if (py < 0) py = 0; if (py > 1000) py = 1000;
        }
        char cmd[48];
        snprintf(cmd, sizeof(cmd), "/interaction %d %d %d", id, px, py);
        pNetGame->SendChatCommand(cmd);
    }
}

void PlayerTap::OnTouch(int type, int finger, int x, int y)
{
    // type : 2 = appui, 3 = deplacement, 1 = relachement
    if (type == 2)
    {
        if (s_hold.active) return;                       // un appui long est deja en cours (autre doigt)
        if (NowMs() - s_lastSendMs < kCooldownMs) return;
        const int id = FindPlayerAt((float)x, (float)y);
        if (id < 0) return;
        s_hold.active = true;
        s_hold.finger = finger;
        s_hold.target = id;
        s_hold.startMs = NowMs();
        s_hold.x = (float)x;
        s_hold.y = (float)y;
        return;
    }
    if (!s_hold.active) return;
    const bool nearHold = std::fabs((float)x - s_hold.x) <= kMaxMove * 2.0f && std::fabs((float)y - s_hold.y) <= kMaxMove * 2.0f;
    if (type == 1)
    {
        // doigt leve avant la fin : rien. Le numero de doigt n'est pas toujours fiable selon le moteur :
        // on annule aussi si le relachement a lieu la ou l'appui a commence.
        if (finger == s_hold.finger || nearHold) Cancel();
        return;
    }
    if (type == 3 && finger == s_hold.finger)
    {
        if (std::fabs((float)x - s_hold.x) > kMaxMove || std::fabs((float)y - s_hold.y) > kMaxMove) Cancel();
    }
}

void PlayerTap::Render(ImGuiRenderer* renderer)
{
    if (!s_hold.active) return;
    if (!LocalPed()) { Cancel(); return; }

    float sx, sy;
    if (!HeadOnScreen(s_hold.target, sx, sy)) { Cancel(); return; }

    const uint32_t elapsed = NowMs() - s_hold.startMs;
    if (elapsed >= kHoldMs) { Fire(sx, sy); return; }
    if (elapsed < kShowAfterMs) return;

    ImDrawList* dl = ImGui::GetBackgroundDrawList();
    if (!dl) return;

    const float fs = UISettings::fontSize();
    const float r = fs * 1.15f;
    const ImVec2 c(sx, sy - r * 1.6f);
    const float k = (float)(elapsed - kShowAfterMs) / (float)(kHoldMs - kShowAfterMs);
    const float a0 = -1.5707963f;                           // depart en haut
    const float a1 = a0 + 6.2831853f * (k > 1.0f ? 1.0f : k);

    // pastille sombre + anneau gris + arc de progression bleu Crestwood
    dl->AddCircleFilled(c, r * 1.18f, IM_COL32(8, 12, 22, 200), 40);
    dl->AddCircle(c, r, IM_COL32(255, 255, 255, 60), 40, r * 0.18f);
    dl->PathClear();
    dl->PathArcTo(c, r, a0, a1, 40);
    dl->PathStroke(IM_COL32(59, 130, 255, 255), false, r * 0.22f);

    // secondes restantes au centre
    char sec[4];
    const int left = (int)((kHoldMs - elapsed + 999) / 1000);
    snprintf(sec, sizeof(sec), "%d", left);
    if (renderer)
    {
        const float tf = fs * 0.95f;
        ImVec2 ts = renderer->calculateTextSize(sec, tf);
        renderer->drawText(ImVec2(c.x - ts.x / 2.0f, c.y - ts.y / 2.0f), ImColor(255, 255, 255, 255), sec, false, tf);

        const char* hint = "Maintiens pour interagir";
        const float hf = fs * 0.62f;
        ImVec2 hs = renderer->calculateTextSize(hint, hf);
        ImVec2 ha(c.x - hs.x / 2.0f - 8.0f, c.y + r * 1.35f);
        dl->AddRectFilled(ha, ImVec2(ha.x + hs.x + 16.0f, ha.y + hs.y + 8.0f), IM_COL32(8, 12, 22, 190), 6.0f);
        renderer->drawText(ImVec2(ha.x + 8.0f, ha.y + 4.0f), ImColor(232, 238, 252, 255), hint, false, hf);
    }
}
