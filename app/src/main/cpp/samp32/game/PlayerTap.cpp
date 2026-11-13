// [Crestwood] Interaction avec les joueurs ET les vehicules DANS LE MONDE 3D (natif, plus de page HTML)
//  - un simple appui (court) sur un autre joueur -> "/interaction3d <id>" au serveur ;
//    sur un vehicule -> "/interaction3dv <id>" ;
//  - le serveur valide (distance, monde) et renvoie le dialogue 9636 "@inter3d" :
//        joueur   : "id|Nom RP|police 0/1|gouv 0/1|action:Libelle;action:Libelle..."
//        vehicule : "V|id|Modele|action:Libelle;..."
//    (le serveur choisit les options ; sans 5e champ = liste par defaut)
//    (SAMP.java -> JNI onInteract3D -> PlayerTap::OnServerMenu) ;
//  - le menu est ANCRE AU JOUEUR dans le monde : carte du nom au-dessus de sa tete, boutons empiles
//    a cote de son torse, projetes chaque image (ils suivent le joueur et la camera, taille selon la
//    distance) ; un bouton -> "/uiact <action>" (memes actions que l'ancien menu HTML).
//  - se ferme : joueur trop loin / plus visible, vehicule, appui hors des boutons, 25 s sans rien.
// Projection monde -> ecran : CSprite::CalcScreenCoors (meme repere que les touches et que ImGui).
// Fichier identique dans samp/ et samp32/ a l'adresse de CalcScreenCoors pres.
#include "main.h"
#include "game/game.h"
#include "net/netgame.h"
#include "gui/gui.h"
#include "PlayerTap.h"

#include <algorithm>
#include <atomic>
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
    constexpr uint32_t kTapMaxMs      = 380;    // appui court
    constexpr float    kTapMaxMove    = 26.0f;  // le doigt ne doit presque pas bouger
    constexpr float    kHitRadius     = 110.0f; // distance max (px) entre la touche et le joueur a l'ecran
    constexpr float    kMaxCamDist    = 30.0f;  // joueurs trop loin de la camera ignores
    constexpr float    kMenuRange     = 10.0f;  // au-dela : le menu se ferme
    constexpr uint32_t kCooldownMs    = 900;
    constexpr uint32_t kMenuTimeoutMs = 25000;
    constexpr uint32_t kRequestWaitMs = 2500;   // reponse serveur attendue

    struct Tap { bool active = false; int finger = -1; int target = -1; uint32_t startMs = 0; float x = 0, y = 0; };

    struct Option { std::string action; std::string label; };

    struct Rect { float x0, y0, x1, y1; int index; };

    Tap      s_tap;
    bool     s_tapVehicle = false;     // l'appui en cours vise un vehicule
    bool     s_vehicle = false;        // menu ouvert sur un vehicule
    uint32_t s_lastSendMs = 0;
    int      s_pendingTarget = -1;     // demande envoyee, en attente du serveur
    uint32_t s_pendingMs = 0;

    // menu ouvert (thread du jeu) ; la reponse serveur arrive par Java (autre thread) -> s_inbox
    std::mutex        s_inboxMutex;
    std::string       s_inbox;
    std::atomic<bool> s_hasInbox{false};

    bool                s_open = false;
    int                 s_target = -1;
    std::string         s_name;
    std::vector<Option> s_options;
    uint32_t            s_openMs = 0;
    float               s_anim = 0.0f;    // 0..1 apparition
    int                 s_pressed = -1;   // bouton sous le doigt

    std::mutex          s_rectMutex;
    std::vector<Rect>   s_rects;          // zones des boutons (image precedente)
    // [v21] roue du vehicule centree a l'ecran : zones en coordonnees polaires (anneau + moyeu)
    struct WheelHit { bool on = false; float cx = 0, cy = 0, rin = 0, rout = 0, hub = 0, a0 = 0; int n = 0; int hubIndex = -1; int idx[16] = {0}; };
    WheelHit            s_wheel;
    Rect                s_closeRect{-1, -1, -2, -2, -2};   // pas de bouton X : zone vide

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

    CVehicleGTA* VehicleEnt(int id)
    {
        if (!pNetGame || id <= 0 || id >= MAX_VEHICLES) return nullptr;
        CVehiclePool* pool = pNetGame->GetVehiclePool();
        if (!pool) return nullptr;
        CVehicle* v = pool->GetAt((VEHICLEID)id);
        if (!v || !v->m_pVehicle || !v->m_pVehicle->IsAdded()) return nullptr;
        if (v->m_pVehicle->GetDistanceFromCamera() > kMaxCamDist) return nullptr;
        return v->m_pVehicle;
    }

    CVector VehPos(CVehicleGTA* e, float dz)
    {
        CVector p = e->GetPosition();
        CVector o;
        o.x = p.x; o.y = p.y; o.z = p.z + dz;
        return o;
    }

    int FindVehicleAt(float x, float y)
    {
        if (!pNetGame || !LocalPed()) return -1;
        int best = -1;
        float bestDist = kHitRadius * 1.35f;
        for (int id = 1; id < MAX_VEHICLES; id++)
        {
            CVehicleGTA* e = VehicleEnt(id);
            if (!e) continue;
            float sx, sy;
            if (!ToScreen(VehPos(e, 0.3f), sx, sy)) continue;
            float d = std::hypot(sx - x, sy - y);
            if (d < bestDist) { bestDist = d; best = id; }
        }
        return best;
    }

    CVector Bone(CPlayerPed* ped, int bone)
    {
        CVector p;
        p.x = 0.0f; p.y = 0.0f; p.z = 0.0f;
        ped->GetBonePosition(bone, &p);
        return p;
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
            static const int bones[] = { 8, 3, 1 };   // tete, torse, bassin
            for (int b : bones)
            {
                float sx, sy;
                if (!ToScreen(Bone(ped, b), sx, sy)) continue;
                float d = std::hypot(sx - x, sy - y);
                if (d < bestDist) { bestDist = d; best = id; }
            }
        }
        return best;
    }

    void CloseMenu()
    {
        s_open = false;
        s_vehicle = false;
        s_target = -1;
        s_options.clear();
        s_pressed = -1;
        std::lock_guard<std::mutex> lock(s_rectMutex);
        s_rects.clear();
        s_wheel.on = false;
        s_closeRect = Rect{-1, -1, -2, -2, -2};
    }

    void SendCmd(const char* cmd)
    {
        if (pNetGame) pNetGame->SendChatCommand(cmd);
    }

    void Choose(int index)
    {
        if (index < 0 || index >= (int)s_options.size()) return;
        const std::string action = s_options[(size_t)index].action;
        CloseMenu();
        if (action == "close") { SendCmd("/uiact close"); return; }
        char cmd[48];
        snprintf(cmd, sizeof(cmd), "/uiact %s", action.c_str());
        SendCmd(cmd);
    }

    std::vector<std::string> Split(const std::string& raw, char sep)
    {
        std::vector<std::string> parts;
        size_t start = 0;
        for (size_t i = 0; i <= raw.size(); i++)
        {
            if (i == raw.size() || raw[i] == sep) { parts.push_back(raw.substr(start, i - start)); start = i + 1; }
        }
        return parts;
    }

    bool ValidAction(const std::string& a)
    {
        if (a.empty() || a.size() > 16) return false;
        for (char c : a) if (!((c >= 'a' && c <= 'z') || c == '_')) return false;
        return true;
    }

    // "action:Libelle;action:Libelle" -> options (actions verifiees : a-z et _ uniquement)
    void ParseOptions(const std::string& field)
    {
        s_options.clear();
        for (const std::string& item : Split(field, ';'))
        {
            size_t c = item.find(':');
            if (c == std::string::npos) continue;
            Option o;
            o.action = item.substr(0, c);
            o.label = item.substr(c + 1, 40);
            if (ValidAction(o.action) && !o.label.empty()) s_options.push_back(o);
            if (s_options.size() >= 10) break;
        }
        bool hasClose = false;
        for (const Option& o : s_options) if (o.action == "close") hasClose = true;
        if (!hasClose) s_options.push_back({"close", "Fermer"});
    }

    // joueur : "id|nom|police|gouv[|options]"   vehicule : "V|id|titre|options"
    void OpenFromServer(const std::string& raw)
    {
        std::vector<std::string> parts = Split(raw, '|');
        if (parts.size() < 2) return;
        if (parts[0] == "V")
        {
            if (parts.size() < 4) return;
            const int vid = std::atoi(parts[1].c_str());
            if (!VehicleEnt(vid)) return;
            ParseOptions(parts[3]);
            s_vehicle = true;
            s_target = vid;
            s_name = parts[2].substr(0, 40);
            s_open = true;
            s_openMs = NowMs();
            s_anim = 0.0f;
            s_pressed = -1;
            s_pendingTarget = -1;
            return;
        }
        const int id = std::atoi(parts[0].c_str());
        if (!RemotePed(id)) return;
        const bool police = parts.size() > 2 && parts[2] == "1";
        const bool gov    = parts.size() > 3 && parts[3] == "1";
        s_vehicle = false;
        if (parts.size() > 4 && !parts[4].empty())
        {
            ParseOptions(parts[4]);
            s_target = id;
            s_name = parts[1].substr(0, 40);
            for (char& c : s_name) if (c == '_') c = ' ';
            s_open = true;
            s_openMs = NowMs();
            s_anim = 0.0f;
            s_pressed = -1;
            s_pendingTarget = -1;
            return;
        }
        s_options.clear();
        s_options.push_back({"hello",  "Saluer"});
        s_options.push_back({"money",  "Donner de l'argent"});
        s_options.push_back({"papers", "Montrer mes papiers"});
        if (police) s_options.push_back({"police", "Contrôle de police"});
        if (gov)
        {
            s_options.push_back({"gov_check", "Vérifier ses licences"});
            s_options.push_back({"gov_give",  "Délivrer une licence"});
            s_options.push_back({"gov_take",  "Retirer une licence"});
        }
        s_options.push_back({"close", "Fermer"});
        s_target = id;
        s_name = parts[1].substr(0, 40);
        for (char& c : s_name) if (c == '_') c = ' ';
        s_open = true;
        s_openMs = NowMs();
        s_anim = 0.0f;
        s_pressed = -1;
        s_pendingTarget = -1;
    }


    // [v20] pictogrammes de la roue d'interaction vehicule (blancs, traits simples)
    void ActionGlyph(ImDrawList* dl, const std::string& a, ImVec2 c, float r, int alpha)
    {
        const ImU32 w = IM_COL32(255, 255, 255, alpha);
        const float u = r / 10.0f;
        const float th = std::max(1.6f, u * 1.3f);
        auto P = [&](float x, float y) { return ImVec2(c.x + x * u, c.y + y * u); };
        if (a == "engine")            // bouton marche / arret
        {
            dl->PathClear();
            dl->PathArcTo(c, 7.0f * u, -1.05f, 4.19f, 28);
            dl->PathStroke(w, false, th);
            dl->AddLine(P(0.0f, -9.0f), P(0.0f, -1.5f), w, th);
        }
        else if (a == "lock")         // cadenas
        {
            dl->AddRectFilled(P(-6.0f, -1.0f), P(6.0f, 8.0f), w, 1.2f * u);
            dl->PathClear();
            dl->PathArcTo(P(0.0f, -1.0f), 4.2f * u, 3.14159f, 6.28318f, 16);
            dl->PathStroke(w, false, th);
            dl->AddCircleFilled(P(0.0f, 3.2f), 1.3f * u, IM_COL32(17, 20, 25, alpha), 10);
        }
        else if (a == "lights")       // phare + faisceau
        {
            dl->PathClear();
            dl->PathArcTo(P(-2.0f, 0.0f), 5.5f * u, 1.5708f, 4.7124f, 16);
            dl->PathStroke(w, true, th);
            for (int i = -1; i <= 1; i++) dl->AddLine(P(2.0f, i * 3.6f), P(8.5f, i * 4.4f), w, th);
        }
        else if (a == "boot" || a == "bonnet")   // voiture de profil, coffre ou capot souleve
        {
            const float k = (a == "boot") ? 1.0f : -1.0f;
            dl->AddRectFilled(P(-8.0f, 0.0f), P(8.0f, 4.0f), w, 1.0f * u);
            const ImVec2 cab[4] = { P(-4.5f, 0.0f), P(-3.0f, -3.6f), P(3.0f, -3.6f), P(4.5f, 0.0f) };
            dl->AddConvexPolyFilled(cab, 4, w);
            dl->AddLine(P(5.5f * k, 0.0f), P(9.0f * k, -4.5f), w, th * 1.2f);
            dl->AddCircleFilled(P(-5.0f, 4.6f), 1.8f * u, IM_COL32(17, 20, 25, alpha), 12);
            dl->AddCircleFilled(P(5.0f, 4.6f), 1.8f * u, IM_COL32(17, 20, 25, alpha), 12);
        }
        else if (a == "doors")        // portiere
        {
            const ImVec2 d[5] = { P(-6.0f, -7.0f), P(4.0f, -7.0f), P(6.0f, -2.0f), P(6.0f, 7.0f), P(-6.0f, 7.0f) };
            dl->AddPolyline(d, 5, w, true, th);
            dl->AddLine(P(-6.0f, -1.0f), P(6.0f, -1.0f), w, th * 0.8f);
            dl->AddLine(P(1.0f, 2.0f), P(4.0f, 2.0f), w, th);
        }
        else if (a == "enter")        // siege conducteur
        {
            dl->AddLine(P(-3.0f, -8.0f), P(-4.5f, 5.0f), w, th * 1.6f);
            dl->AddLine(P(-4.5f, 5.0f), P(5.5f, 5.0f), w, th * 1.6f);
            dl->AddLine(P(5.5f, 5.0f), P(5.5f, 8.0f), w, th);
        }
        else if (a == "vinfo")        // plaque
        {
            dl->AddRect(P(-8.0f, -4.5f), P(8.0f, 4.5f), w, 1.0f * u, ImDrawCornerFlags_All, th);
            dl->AddLine(P(-5.0f, 0.0f), P(5.0f, 0.0f), w, th * 1.3f);
        }
        else if (a == "vcheck")       // bouclier
        {
            const ImVec2 sh6[6] = { P(0.0f, -8.0f), P(7.0f, -5.0f), P(6.0f, 3.0f), P(0.0f, 8.0f), P(-6.0f, 3.0f), P(-7.0f, -5.0f) };
            dl->AddPolyline(sh6, 6, w, true, th);
        }
        else dl->AddCircleFilled(c, 2.4f * u, w, 12);
    }

    int HitButton(float x, float y)
    {
        std::lock_guard<std::mutex> lock(s_rectMutex);
        if (s_wheel.on)
        {
            const float dx = x - s_wheel.cx, dy = y - s_wheel.cy, d = std::sqrt(dx * dx + dy * dy);
            if (d <= s_wheel.hub) return s_wheel.hubIndex;
            if (d >= s_wheel.rin * 0.92f && d <= s_wheel.rout * 1.08f && s_wheel.n > 0)
            {
                float a = std::atan2(dy, dx) - s_wheel.a0;
                while (a < 0.0f) a += 6.2831853f;
                while (a >= 6.2831853f) a -= 6.2831853f;
                int k = (int)(a / (6.2831853f / (float)s_wheel.n));
                if (k >= 0 && k < s_wheel.n) return s_wheel.idx[k];
            }
            return -1;
        }
        for (const Rect& r : s_rects)
            if (x >= r.x0 && x <= r.x1 && y >= r.y0 && y <= r.y1) return r.index;
        if (x >= s_closeRect.x0 && x <= s_closeRect.x1 && y >= s_closeRect.y0 && y <= s_closeRect.y1) return -2;
        return -1;
    }
}

void PlayerTap::OnServerMenu(const char* data)
{
    if (!data) return;
    std::lock_guard<std::mutex> lock(s_inboxMutex);
    s_inbox = data;
    s_hasInbox = true;
}

bool PlayerTap::OnTouch(int type, int finger, int x, int y)
{
    // type : 2 = appui, 3 = deplacement, 1 = relachement
    const float fx = (float)x, fy = (float)y;

    // ---- menu 3D ouvert : les boutons ont la priorite ----
    if (s_open)
    {
        const int hit = HitButton(fx, fy);
        if (type == 2)
        {
            if (hit != -1) { s_pressed = hit; return true; }   // touche reservee au menu
            CloseMenu();                                        // appui ailleurs = fermer (la touche continue vers le jeu)
            SendCmd("/uiact close");
            return false;
        }
        if (type == 3) { if (s_pressed != -1 && hit != s_pressed) s_pressed = -1; return s_pressed != -1; }
        if (type == 1)
        {
            const int pressed = s_pressed;
            s_pressed = -1;
            if (pressed == -1) return false;
            if (hit == pressed)
            {
                if (pressed == -2) { CloseMenu(); SendCmd("/uiact close"); }
                else Choose(pressed);
            }
            return true;
        }
        return false;
    }

    // ---- appui court sur un joueur ----
    if (type == 2)
    {
        if (s_tap.active) return false;
        if (NowMs() - s_lastSendMs < kCooldownMs) return false;
        int id = FindPlayerAt(fx, fy);
        s_tapVehicle = false;
        if (id < 0) { id = FindVehicleAt(fx, fy); s_tapVehicle = id >= 0; }
        if (id < 0) return false;
        s_tap.active = true;
        s_tap.finger = finger;
        s_tap.target = id;
        s_tap.startMs = NowMs();
        s_tap.x = fx;
        s_tap.y = fy;
        return false;   // la camera peut encore tourner : on ne decide qu'au relachement
    }
    if (!s_tap.active) return false;
    if (type == 3)
    {
        if (finger == s_tap.finger && (std::fabs(fx - s_tap.x) > kTapMaxMove || std::fabs(fy - s_tap.y) > kTapMaxMove)) s_tap = Tap();
        return false;
    }
    if (type == 1)
    {
        const bool near = std::fabs(fx - s_tap.x) <= kTapMaxMove && std::fabs(fy - s_tap.y) <= kTapMaxMove;
        const bool quick = NowMs() - s_tap.startMs <= kTapMaxMs;
        const int id = s_tap.target;
        s_tap = Tap();
        const bool okTarget = s_tapVehicle ? (VehicleEnt(id) != nullptr) : (RemotePed(id) != nullptr);
        if (near && quick && LocalPed() && okTarget)
        {
            char cmd[40];
            snprintf(cmd, sizeof(cmd), s_tapVehicle ? "/interaction3dv %d" : "/interaction3d %d", id);
            SendCmd(cmd);
            s_lastSendMs = NowMs();
            s_pendingTarget = id;
            s_pendingMs = s_lastSendMs;
        }
    }
    return false;
}

void PlayerTap::Render(ImGuiRenderer* renderer)
{
    // reponse du serveur (thread Java) -> ouverture sur le thread du jeu
    if (s_hasInbox.exchange(false))
    {
        std::string raw;
        { std::lock_guard<std::mutex> lock(s_inboxMutex); raw = s_inbox; }
        OpenFromServer(raw);
    }

    ImDrawList* dl = ImGui::GetBackgroundDrawList();
    if (!dl || !renderer) return;
    const float fs = UISettings::fontSize();
    const uint32_t now = NowMs();

    // petit halo d'attente sur le joueur touche (le temps que le serveur reponde)
    if (!s_open && s_pendingTarget >= 0)
    {
        float sx, sy;
        bool okPos = false;
        if (s_tapVehicle) { CVehicleGTA* e = VehicleEnt(s_pendingTarget); okPos = e && ToScreen(VehPos(e, 1.4f), sx, sy); }
        else { CPlayerPed* ped = RemotePed(s_pendingTarget); okPos = ped && ToScreen(Bone(ped, 8), sx, sy); }
        if (!okPos || now - s_pendingMs > kRequestWaitMs) { s_pendingTarget = -1; return; }
        const float r = fs * 0.75f;
        const float a0 = (float)(now % 1000) / 1000.0f * 6.2831853f;
        dl->PathClear();
        dl->PathArcTo(ImVec2(sx, sy - r * 2.2f), r, a0, a0 + 4.2f, 32);
        dl->PathStroke(IM_COL32(59, 130, 255, 230), false, r * 0.2f);
        return;
    }
    if (!s_open) return;

    CPlayerPed* local = LocalPed();
    CPlayerPed* ped = s_vehicle ? nullptr : RemotePed(s_target);
    CVehicleGTA* veh = s_vehicle ? VehicleEnt(s_target) : nullptr;
    if (!local || (!ped && !veh) || now - s_openMs > kMenuTimeoutMs) { CloseMenu(); return; }

    const CVector me = Bone(local, 1);
    const CVector head = veh ? VehPos(veh, 1.3f) : Bone(ped, 8);
    const CVector chest = veh ? VehPos(veh, 0.5f) : Bone(ped, 3);
    const float dx = chest.x - me.x, dy = chest.y - me.y, dz = chest.z - me.z;
    if (std::sqrt(dx * dx + dy * dy + dz * dz) > kMenuRange) { CloseMenu(); SendCmd("/uiact close"); return; }

    // repere du menu dans le monde : a droite du joueur vu depuis nous (horizontal)
    float hl = std::sqrt(dx * dx + dy * dy);
    float rx = 1.0f, ry = 0.0f;
    if (hl > 0.01f) { rx = dy / hl; ry = -dx / hl; }

    s_anim = std::min(1.0f, s_anim + 0.12f);
    const float ease = 1.0f - (1.0f - s_anim) * (1.0f - s_anim);

    // echelle : hauteur a l'ecran de 0.20 m a la position du menu (perspective)
    CVector anchor;
    const float side = s_vehicle ? 1.6f : 0.55f;   // un vehicule est plus large
    anchor.x = chest.x + rx * side; anchor.y = chest.y + ry * side; anchor.z = head.z + 0.05f;
    CVector anchorLow = anchor;
    anchorLow.z -= 0.20f;
    float ax, ay, bx, by;
    if (!ToScreen(anchor, ax, ay) || !ToScreen(anchorLow, bx, by)) return;   // derriere la camera : on attend
    float unit = std::fabs(by - ay);
    const float minH = fs * 1.55f, maxH = fs * 2.6f;
    float btnH = std::max(minH, std::min(maxH, unit));
    const float gap = btnH * 0.16f;
    const float txt = btnH * 0.42f;
    const ImVec2 ds = ImGui::GetIO().DisplaySize;

    // largeur selon le libelle le plus long
    float btnW = 0.0f;
    for (const Option& o : s_options) btnW = std::max(btnW, renderer->calculateTextSize(o.label, txt).x);
    btnW += btnH * 1.35f;
    btnW = std::max(btnW, btnH * 5.2f);

    // pile verticale centree sur le point d'ancrage (decalee pendant l'apparition)
    const float total = s_options.size() * btnH + (s_options.size() - 1) * gap;
    float x0 = ax + (1.0f - ease) * btnH * 0.8f;
    float y0 = ay - btnH * 0.4f;
    if (x0 + btnW > ds.x - 8.0f) x0 = ds.x - 8.0f - btnW;
    if (y0 + total > ds.y - 8.0f) y0 = ds.y - 8.0f - total;
    if (y0 < 8.0f) y0 = 8.0f;
    const int alpha = (int)(255.0f * ease);

    // ---- [v21] VEHICULE : roue d'interaction CENTREE a l'ecran, anneau segmente (style Crestwood) ----
    if (s_vehicle && veh)
    {
        int n = 0, closeIndex = -1;
        for (size_t i = 0; i < s_options.size(); i++) { if (s_options[i].action == "close") closeIndex = (int)i; else n++; }
        if (n > 16) n = 16;
        const float shortSide = std::min(ds.x, ds.y);
        const float rout = shortSide * 0.40f * (0.86f + 0.14f * ease);
        const float rin = rout * 0.50f;
        const float hubR = rin * 0.80f;
        const float cx = ds.x * 0.5f, cy = ds.y * 0.5f;
        const ImVec2 cc(cx, cy);
        const float twoPi = 6.2831853f;
        const float seg = twoPi / (float)std::max(1, n);
        const float a0 = -1.5707963f - seg * 0.5f;     // 1re action en haut
        const float gapA = std::min(0.05f, seg * 0.08f);
        // voile leger + halo
        dl->AddRectFilled(ImVec2(0, 0), ds, IM_COL32(4, 6, 10, (int)(84 * ease)));
        dl->AddCircleFilled(cc, rout + shortSide * 0.035f, IM_COL32(8, 10, 14, (int)(120 * ease)), 96);
        dl->AddCircle(cc, rout + shortSide * 0.035f, IM_COL32(59, 130, 255, (int)(70 * ease)), 96, 1.5f);
        WheelHit hit;
        hit.on = true; hit.cx = cx; hit.cy = cy; hit.rin = rin; hit.rout = rout; hit.hub = hubR; hit.a0 = a0; hit.n = n; hit.hubIndex = closeIndex;
        int k = 0;
        for (size_t i = 0; i < s_options.size() && k < n; i++)
        {
            if ((int)i == closeIndex) continue;
            hit.idx[k] = (int)i;
            const float sa = a0 + seg * k + gapA, ea = a0 + seg * (k + 1) - gapA;
            k++;
            const bool pressed = s_pressed == (int)i;
            const ImU32 fill = pressed ? IM_COL32(59, 130, 255, alpha) : IM_COL32(22, 26, 33, (int)(232 * ease));
            // secteur d'anneau : petits quadrilateres convexes
            const int steps = 14;
            for (int q = 0; q < steps; q++)
            {
                const float t0 = sa + (ea - sa) * q / steps, t1 = sa + (ea - sa) * (q + 1) / steps;
                const ImVec2 quad[4] = {
                    ImVec2(cx + std::cos(t0) * rin, cy + std::sin(t0) * rin), ImVec2(cx + std::cos(t0) * rout, cy + std::sin(t0) * rout),
                    ImVec2(cx + std::cos(t1) * rout, cy + std::sin(t1) * rout), ImVec2(cx + std::cos(t1) * rin, cy + std::sin(t1) * rin) };
                dl->AddConvexPolyFilled(quad, 4, fill);
            }
            // liseres : bord exterieur clair, bord interieur bleu
            dl->PathArcTo(cc, rout - 1.0f, sa, ea, 24);
            dl->PathStroke(IM_COL32(255, 255, 255, pressed ? (int)(200 * ease) : (int)(46 * ease)), false, 1.5f);
            dl->PathArcTo(cc, rin + 1.5f, sa, ea, 18);
            dl->PathStroke(IM_COL32(59, 130, 255, pressed ? alpha : (int)(150 * ease)), false, 2.5f);
            // icone + libelle au milieu du secteur
            const float mid = (sa + ea) * 0.5f, mr = (rin + rout) * 0.5f;
            const ImVec2 c(cx + std::cos(mid) * mr, cy + std::sin(mid) * mr);
            const float band = rout - rin;
            ActionGlyph(dl, s_options[i].action, ImVec2(c.x, c.y - band * 0.12f), band * 0.17f, alpha);
            float lf = std::max(fs * 0.62f, band * 0.135f);
            ImVec2 ls = renderer->calculateTextSize(s_options[i].label, lf);
            const float maxW = std::min(band * 1.05f, 2.0f * mr * std::sin((ea - sa) * 0.5f) * 0.92f);
            if (ls.x > maxW) { lf *= maxW / ls.x; ls = renderer->calculateTextSize(s_options[i].label, lf); }
            renderer->drawText(ImVec2(c.x - ls.x * 0.5f, c.y + band * 0.16f), pressed ? ImColor(255, 255, 255, alpha) : ImColor(226, 232, 240, alpha), s_options[i].label, false, lf);
        }
        // moyeu : nom du vehicule, toucher = fermer
        const bool hubPressed = closeIndex >= 0 && s_pressed == closeIndex;
        dl->AddCircleFilled(cc, hubR, hubPressed ? IM_COL32(226, 72, 76, alpha) : IM_COL32(12, 14, 19, (int)(240 * ease)), 64);
        dl->AddCircle(cc, hubR, IM_COL32(59, 130, 255, alpha), 64, 2.5f);
        dl->AddCircle(cc, hubR - 5.0f, IM_COL32(255, 255, 255, (int)(26 * ease)), 64, 1.0f);
        float nf = hubR * 0.26f;
        ImVec2 ns = renderer->calculateTextSize(s_name, nf);
        if (ns.x > hubR * 1.6f) { nf *= hubR * 1.6f / ns.x; ns = renderer->calculateTextSize(s_name, nf); }
        renderer->drawText(ImVec2(cx - ns.x * 0.5f, cy - ns.y * 0.95f), ImColor(255, 255, 255, alpha), s_name, false, nf);
        const std::string closeLbl = closeIndex >= 0 ? s_options[(size_t)closeIndex].label : std::string("Fermer");
        const float cf = hubR * 0.17f;
        ImVec2 cs = renderer->calculateTextSize(closeLbl, cf);
        renderer->drawText(ImVec2(cx - cs.x * 0.5f, cy + ns.y * 0.25f), ImColor(150, 186, 255, alpha), closeLbl, false, cf);
        std::lock_guard<std::mutex> lock(s_rectMutex);
        s_rects.clear();
        s_wheel = hit;
        return;
    }

    // ---- carte du nom au-dessus de la tete (ancree a la tete) ----
    float hx, hy;
    CVector top = head;
    top.z += 0.42f;
    if (ToScreen(top, hx, hy))
    {
        const float nf = btnH * 0.46f, sf = btnH * 0.30f;
        char idLine[32];
        snprintf(idLine, sizeof(idLine), s_vehicle ? "VÉHICULE  #%d" : "JOUEUR  #%d", s_target);
        ImVec2 ns = renderer->calculateTextSize(s_name, nf);
        ImVec2 is = renderer->calculateTextSize(std::string(idLine), sf);
        const float cw = std::max(ns.x, is.x) + btnH * 0.9f;
        const float ch = ns.y + is.y + btnH * 0.45f;
        ImVec2 p0(hx - cw * 0.5f, hy - ch);
        ImVec2 p1(hx + cw * 0.5f, hy);
        dl->AddRectFilled(p0, p1, IM_COL32(10, 11, 14, (int)(214 * ease)), 3.0f);
        dl->AddRect(p0, p1, IM_COL32(255, 255, 255, (int)(40 * ease)), 3.0f);
        dl->AddRectFilled(ImVec2(p0.x, p1.y - 2.0f), ImVec2(p1.x, p1.y), IM_COL32(59, 130, 255, alpha));
        renderer->drawText(ImVec2(hx - is.x * 0.5f, p0.y + btnH * 0.18f), ImColor(120, 166, 255, alpha), std::string(idLine), false, sf);
        renderer->drawText(ImVec2(hx - ns.x * 0.5f, p0.y + btnH * 0.22f + is.y), ImColor(245, 246, 248, alpha), s_name, false, nf);
        // petit trait vers la tete
        float tx, ty;
        if (ToScreen(head, tx, ty)) dl->AddLine(ImVec2(hx, hy), ImVec2(tx, ty - btnH * 0.2f), IM_COL32(59, 130, 255, (int)(120 * ease)), 1.5f);
    }

    // ---- boutons ----
    std::vector<Rect> rects;
    for (size_t i = 0; i < s_options.size(); i++)
    {
        const float y = y0 + i * (btnH + gap);
        ImVec2 p0(x0, y), p1(x0 + btnW, y + btnH);
        const bool pressed = s_pressed == (int)i;
        const bool isClose = s_options[i].action == "close";
        dl->AddRectFilled(p0, p1, pressed ? IM_COL32(59, 130, 255, alpha) : IM_COL32(10, 11, 14, (int)(206 * ease)), 3.0f);
        dl->AddRect(p0, p1, IM_COL32(255, 255, 255, (int)(34 * ease)), 3.0f);
        if (!pressed && !isClose) dl->AddRectFilled(p0, ImVec2(p0.x + 3.0f, p1.y), IM_COL32(59, 130, 255, alpha));
        ImVec2 ts = renderer->calculateTextSize(s_options[i].label, txt);
        const ImColor tc = pressed ? ImColor(11, 11, 13, alpha) : (isClose ? ImColor(160, 166, 174, alpha) : ImColor(243, 244, 246, alpha));
        renderer->drawText(ImVec2(p0.x + btnH * 0.5f, y + (btnH - ts.y) * 0.5f), tc, s_options[i].label, false, txt);
        // chevron
        if (!isClose)
        {
            const float cx = p1.x - btnH * 0.42f, cy = y + btnH * 0.5f, k = btnH * 0.12f;
            const ImU32 cc = pressed ? IM_COL32(11, 11, 13, alpha) : IM_COL32(59, 130, 255, alpha);
            dl->AddLine(ImVec2(cx - k * 0.6f, cy - k), ImVec2(cx + k * 0.4f, cy), cc, 2.0f);
            dl->AddLine(ImVec2(cx + k * 0.4f, cy), ImVec2(cx - k * 0.6f, cy + k), cc, 2.0f);
        }
        rects.push_back(Rect{p0.x, p0.y, p1.x, p1.y, (int)i});
    }
    // fil reliant la pile au joueur
    float cx, cy;
    if (ToScreen(chest, cx, cy)) dl->AddLine(ImVec2(cx, cy), ImVec2(x0, y0 + btnH * 0.5f), IM_COL32(59, 130, 255, (int)(90 * ease)), 1.5f);

    std::lock_guard<std::mutex> lock(s_rectMutex);
    s_wheel.on = false;
    s_rects.swap(rects);
}
