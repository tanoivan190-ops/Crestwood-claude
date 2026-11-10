#include <atomic>
//
// Created by x1y2z on 03.02.2023.
//

#include "WidgetGta.h"
#include "main.h"
#include "game/game.h"
#include "net/netgame.h"
#include "vendor/armhook/patch.h"
#include "WidgetRegionLook.h"
#include "TouchInterface.h"
#include "WidgetButton.h"

extern CNetGame *pNetGame;
extern CGame *pGame;

CWidgetGta* m_pWidgets[WidgetIDs::NUM_WIDGETS];

enum eWidgetState {
    STATE_NONE,
    STATE_FIXED
};

WidgetIDs GetWidgetTypeFromWidget(CWidgetGta* pWidget)
{
    if(pWidget)
    {
        if(m_pWidgets[WidgetIDs::WIDGET_ATTACK] && pWidget == m_pWidgets[WidgetIDs::WIDGET_ATTACK]) return WidgetIDs::WIDGET_ATTACK;
        if(m_pWidgets[WidgetIDs::WIDGET_SPRINT] && pWidget == m_pWidgets[WidgetIDs::WIDGET_SPRINT]) return WidgetIDs::WIDGET_SPRINT;
        if(m_pWidgets[WidgetIDs::WIDGET_ACCELERATE] && pWidget == m_pWidgets[WidgetIDs::WIDGET_ACCELERATE]) return WidgetIDs::WIDGET_ACCELERATE;
        if(m_pWidgets[WidgetIDs::WIDGET_ENTER_CAR] && pWidget == m_pWidgets[WidgetIDs::WIDGET_ENTER_CAR]) return WidgetIDs::WIDGET_ENTER_CAR;
        if(m_pWidgets[WidgetIDs::WIDGET_BRAKE] && pWidget == m_pWidgets[WidgetIDs::WIDGET_BRAKE]) return WidgetIDs::WIDGET_BRAKE;
    }

    return static_cast<WidgetIDs>(-1);
}

void SetWidgetFromId(int idWidget, CWidgetGta* pWidget)
{
    m_pWidgets[idWidget] = pWidget;
}

void SetWidgetFromName(const char* name, CWidgetGta* pWidget)
{
    if(!strcmp("accelerate", name)) SetWidgetFromId(WidgetIDs::WIDGET_ACCELERATE, pWidget);
    if(!strcmp("hud_car", name)) SetWidgetFromId(WidgetIDs::WIDGET_ENTER_CAR, pWidget);
    if(!strcmp("brake", name)) SetWidgetFromId(WidgetIDs::WIDGET_BRAKE, pWidget);
}

eWidgetState ProcessFixedWidget(CWidgetGta* pWidget)
{
    WidgetIDs widgetType = GetWidgetTypeFromWidget(pWidget);

    CPlayerPed *pPlayerPed = pGame->FindPlayerPed();
    switch(widgetType)
    {
        case -1:
            return STATE_NONE;
        case WidgetIDs::WIDGET_ATTACK:
        case WidgetIDs::WIDGET_SPRINT:
            if(pPlayerPed->IsInVehicle() ||
               pPlayerPed->IsInJetpackMode())
            {
                return STATE_FIXED;
            }
            break;
        case WidgetIDs::WIDGET_ACCELERATE:
        case WidgetIDs::WIDGET_BRAKE:
            if(!pPlayerPed->IsInVehicle() &&
               !pPlayerPed->IsInJetpackMode())
            {
                return STATE_FIXED;
            }
            break;
        case WidgetIDs::WIDGET_ENTER_CAR:
            if(pPlayerPed->IsInJetpackMode()) return STATE_NONE;

            if(pNetGame)
            {
                CVehiclePool *pVehiclePool = pNetGame->GetVehiclePool();
                if(pVehiclePool)
                {
                    VEHICLEID vehicleId = pVehiclePool->FindNearestToLocalPlayerPed();
                    if(vehicleId == INVALID_VEHICLE_ID) return STATE_FIXED;

                    if(vehicleId != INVALID_VEHICLE_ID)
                    {
                        CVehicle *pVehicle = pVehiclePool->GetAt(vehicleId);
                        if(pVehicle)
                        {
                            if(!pPlayerPed->IsInVehicle() &&
                               pVehicle->m_pVehicle->GetDistanceFromLocalPlayerPed() > 10.0f)
                            {
                                return STATE_FIXED;
                            }
                        }
                    }
                }
            }
            break;
    }

    return STATE_NONE;
}


// ---------------------------------------------------------------------------
// [Crestwood] Controles tactiles v2 : quand le HUD HTML active ses propres controles
// (setVehicleControl id 20 -> g_HudCustomControls), les boutons tactiles natifs de
// San Andreas (joystick, tir, visee, sprint, saut, conduite...) sont masques ET
// desactives : ils ne sont plus dessines et ne reagissent plus au toucher.
// La camera (WIDGET_LOOK / SHOOT_LOOK) et WIDGET_CAM_TOGGLE restent natifs.
// ---------------------------------------------------------------------------
#include <atomic>
#include <dlfcn.h>
extern std::atomic<bool> g_HudCustomControls;

static CWidgetGta** Crestwood_WidgetArray()
{
    static CWidgetGta** s_arr = nullptr;
    static bool s_tried = false;
    if (!s_tried) {
        s_tried = true;
        const char* libs[] = { "libGTASA.so" };
        for (const char* name : libs) {
            void* h = dlopen(name, RTLD_NOLOAD | RTLD_LAZY);
            if (!h) continue;
            s_arr = (CWidgetGta**)dlsym(h, "_ZN15CTouchInterface10m_pWidgetsE");
            if (s_arr) break;
        }
    }
    return s_arr;
}

static int Crestwood_WidgetId(const void* pWidget)
{
    CWidgetGta** arr = Crestwood_WidgetArray();
    if (!arr || !pWidget) return -1;
    for (int i = 0; i < NUM_WIDGETS; i++) {
        if (arr[i] == pWidget) return i;
    }
    return -1;
}

bool Crestwood_IsReplacedWidget(const void* pWidget)
{
    if (pWidget && pNetGame && !g_HudCustomControls.load()) {
        // [10/11] seuls Courir et Frapper sont remplaces par les boutons HTML (le stick natif reste)
        switch (Crestwood_WidgetId(pWidget)) {
            case WIDGET_SPRINT:
            case WIDGET_BUTTON_SPRINT:
            case WIDGET_ATTACK: return true;
            default: break;
        }
    }
    if (!pWidget || !g_HudCustomControls.load() || !pNetGame) return false;
    switch (Crestwood_WidgetId(pWidget)) {
        // a pied
        case WIDGET_PED_MOVE:
        case WIDGET_THUMB_CIRCLE:
        case WIDGET_SPRINT:
        case WIDGET_BUTTON_SPRINT:
        case WIDGET_BUTTON_CROUCH:
        case WIDGET_BUTTON_DIVE:
        case WIDGET_BUTTON_SWIM:
        case WIDGET_ENTER_CAR:
        // armes
        case WIDGET_ATTACK:
        case WIDGET_ENTER_TARGETING:
        case WIDGET_EXIT_TARGETING:
        case WIDGET_SWAP_WEAPONS:
        case WIDGET_VC_SHOOT:
        case WIDGET_VC_SHOOT_ALT:
        // vehicule
        case WIDGET_ACCELERATE:
        case WIDGET_BRAKE:
        case WIDGET_HANDBRAKE:
        case WIDGET_VEHICLE_STEER_LEFT:
        case WIDGET_VEHICLE_STEER_RIGHT:
        case WIDGET_VEHICLE_STEER_ANALOG:
        case WIDGET_HORN:
        case WIDGET_NITRO:
        case WIDGET_CAR_SHOOT:
        case WIDGET_VEHICLE_SHOOT_LEFT:
        case WIDGET_VEHICLE_SHOOT_RIGHT:
        case WIDGET_HYDRAULICS:
        case WIDGET_AUTO_HYDRAULICS:
            return true;
        default:
            return false;
    }
}

// ---------------------------------------------------------------------------
// [v15] Icones Crestwood des boutons natifs (assets/models/buttons/*.png, styles fournis par Anzo) :
// voiture (entrer), poing / balle (attaque selon l'arme), basket (sprint), saut, accroupi.
// Les textures d'origine sont gardees et remises si l'option "Boutons Crestwood" est coupee.
// ---------------------------------------------------------------------------
RwTexture* Crestwood_LoadApkTexture(const char* path);
std::atomic<bool> g_cwCustomButtons{true};
static RwTexture* s_cwOrigTex[NUM_WIDGETS];
static bool s_cwOrigSaved[NUM_WIDGETS];

static const char* Crestwood_ButtonIcon(int id)
{
    switch (id) {
        case WIDGET_ENTER_CAR: return "models/buttons/car.png";
        case WIDGET_ATTACK: {
            CPlayerPed* ped = pGame ? pGame->FindPlayerPed() : nullptr;
            const int w = ped ? (int)ped->GetCurrentWeapon() : 0;
            return (w >= 22 && w <= 38) ? "models/buttons/bullet.png" : "models/buttons/fist.png";
        }
        case WIDGET_BUTTON_SPRINT:
        case WIDGET_BUTTON_SWIM: return "models/buttons/shoe.png";
        case WIDGET_BUTTON_CROUCH: return "models/buttons/crouch.png";
        case WIDGET_BUTTON_DIVE: return "models/buttons/jump.png";
        default: return nullptr;
    }
}

void Crestwood_SkinWidget(void* pWidget)
{
    if (!pWidget) return;
    const int id = Crestwood_WidgetId(pWidget);
    if (id < 0 || id >= NUM_WIDGETS) return;
    const char* icon = Crestwood_ButtonIcon(id);
    if (!icon) return;
    CWidgetGta* w = (CWidgetGta*)pWidget;
    if (!g_cwCustomButtons.load()) {
        if (s_cwOrigSaved[id]) { w->m_Sprite.m_pTexture = s_cwOrigTex[id]; s_cwOrigSaved[id] = false; }
        return;
    }
    RwTexture* tex = Crestwood_LoadApkTexture(icon);
    if (!tex) return;
    if (w->m_Sprite.m_pTexture == tex) return;
    if (!s_cwOrigSaved[id]) { s_cwOrigTex[id] = w->m_Sprite.m_pTexture; s_cwOrigSaved[id] = true; }
    w->m_Sprite.m_pTexture = tex;
}

void (*CWidgetRegion__Draw)(void* thiz);
void CWidgetRegion__Draw_hook(void* thiz)
{
    if (Crestwood_IsReplacedWidget(thiz)) return;
    CWidgetRegion__Draw(thiz);
}

void (*CWidgetButton__Draw32)(void* thiz);
void CWidgetButton__Draw32_hook(void* thiz)
{
    if (Crestwood_IsReplacedWidget(thiz)) return;
    Crestwood_SkinWidget(thiz);   // [v15] icones Crestwood
    CWidgetButton__Draw32(thiz);
}

void CWidgetGta::SetEnabled(bool bEnabled) {
    m_bEnabled = bEnabled;
}

bool (*CWidget__IsTouched)(uintptr_t *thiz, CVector2D *pVecOut);
bool CWidget__IsTouched_hook(uintptr_t *thiz, CVector2D *pVecOut) {
//    if(*thiz == CWidgetGta::pWidgets[WIDGET_POSITION_HORN]) {
//        return true;
//    }
//    if(!CHUD::bIsShow)
//        return false;


    return CWidget__IsTouched(thiz, pVecOut);
}

uintptr_t (*CWidget)(CWidgetButton* thiz, const char* name, uintptr_t* a3, int a4, uintptr_t* a5);
uintptr_t CWidget_hook(CWidgetButton* thiz, const char* name, uintptr_t*  a3, int a4, uintptr_t* a5)
{
    FLog("New Widget: \"%s\" 0x%X", name, thiz-g_libGTASA);

    SetWidgetFromName(name, thiz);
    return CWidget(thiz, name, a3, a4, a5);
}

void (*CWidget__SetEnabled)(CWidgetGta* pWidget, bool bEnabled);
void CWidget__SetEnabled_hook(CWidgetGta* pWidget, bool bEnabled)
{
    if (Crestwood_IsReplacedWidget(pWidget)) {
        CWidget__SetEnabled(pWidget, false);
        pWidget->m_bEnabled = false;
        return;
    }
    if(pNetGame)
    {
        switch(ProcessFixedWidget(pWidget))
        {
            case STATE_NONE: break;
            case STATE_FIXED:
                bEnabled = false;
                break;
        }
    }

    CWidget__SetEnabled(pWidget, bEnabled);
}

void (*CWidgetButton__Update)(CWidgetButton* thiz);
void CWidgetButton__Update_hook(CWidgetButton* thiz) {
    if (Crestwood_IsReplacedWidget(thiz)) {
        ((CWidgetGta*)thiz)->m_bEnabled = false;
        return;
    }
    if(pNetGame)
    {
        switch(ProcessFixedWidget(thiz))
        {
            case STATE_NONE: break;
            case STATE_FIXED: return;
        }
    }

    CWidgetButton__Update(thiz);
    Crestwood_SkinWidget(thiz);   // [v15] icones Crestwood
}

void CWidgetGta::InjectHooks() {
    CHook::InlineHook("_ZN13CWidgetRegion4DrawEv", &CWidgetRegion__Draw_hook, &CWidgetRegion__Draw);
    CHook::InlineHook("_ZN13CWidgetButton4DrawEv", &CWidgetButton__Draw32_hook, &CWidgetButton__Draw32);
    CHook::InstallPLT(g_libGTASA + (VER_x32 ? 0x00671424 : 0x842408), &CWidgetButton__Update_hook, &CWidgetButton__Update);
    CHook::InlineHook("_ZN13CWidgetButtonC2EPKcRK14WidgetPositionjj10HIDMapping", &CWidget_hook, &CWidget);
    CHook::InlineHook("_ZN7CWidget10SetEnabledEb", &CWidget__SetEnabled_hook, &CWidget__SetEnabled);
}

void CWidgetGta::SetTexture(const char *name) {
    m_Sprite.m_pTexture = CUtil::LoadTextureFromDB("mobile", name);
}

bool CWidgetGta::IsReleased(CVector2D *pVecOut) {
    return CHook::CallFunction<bool>(g_libGTASA + (VER_x32 ? 0x002B3484 + 1 : 0x372794), this, pVecOut);
}

bool CWidgetGta::IsTouched(CVector2D *pVecOut) {
    return CHook::CallFunction<bool>(g_libGTASA + (VER_x32 ? 0x002B3324 + 1 : 0x3725D0), this, pVecOut);
}


