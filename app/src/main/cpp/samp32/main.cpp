#include <jni.h>
#include <pthread.h>
#include <syscall.h>

#include "main.h"
#include "game/Camera.h"
#include "gui/SouthModelLoader.h"
#include <unistd.h>
#include "game/game.h"
#include "net/netgame.h"
#include "gui/gui.h"
#include "playertags.h"
#include "audiostream.h"
#include "java/jniutil.h"
#include <dlfcn.h>
#include "StackTrace.h"

// voice
#include "voice_new/Plugin.h"

#include "vendor/armhook/patch.h"
#include "vendor/str_obfuscator/str_obfuscator.hpp"

#include "settings.h"
#include "gui/CharacterCreator.h"

#include "crashlytics.h"

#include <mutex>
#include <string>
#include <vector>
#include <chrono>
#include <cmath>


/*
Peerapol Unarak
*/

JavaVM* javaVM;

char* g_pszStorage = nullptr;

UI* pUI = nullptr;
CGame *pGame = nullptr;

CNetGame *pNetGame = nullptr;
CPlayerTags* pPlayerTags = nullptr;
CSnapShotHelper* pSnapShotHelper = nullptr;
CAudioStream* pAudioStream = nullptr;
CJavaWrapper* pJavaWrapper = nullptr;
CSettings* pSettings = nullptr;
//CVoice* pVoice = nullptr;

MaterialTextGenerator* pMaterialTextGenerator = nullptr;

bool bDebug = false;
bool bGameInited = false;
bool bNetworkInited = false;

uintptr_t g_libGTASA = 0x00;
uintptr_t g_libSAMP = 0x00;

void ApplyGlobalPatches();
void ApplyPatches_level0();
void ApplyMultiTouchPatches();
void InstallGlobalHooks();
void InstallSpecialHooks();
void InitRenderWareFunctions();
void FLog(const char* fmt, ...);
//void MyLog(const char* fmt, ...);

int work = 0;

void ReadSettingFile()
{
	/*char path[255] = { 0 };
	//sprintf(path, "%ssamp.set", g_pszStorage);
	sprintf(path, "%sNickName.ini", g_pszStorage);

	FILE* fp = fopen(path, "r");
	if (fp == NULL) return;

	char buf[1024];

	// nickname
	if (fgets(buf, 1024, fp) != NULL) {
		buf[strcspn(buf, "\n\r")] = 0;
		strcpy(g_nick, buf);
	}

	fclose(fp);*/

	pSettings = new CSettings();

	firebase::crashlytics::SetUserId(pSettings->Get().szNickName);
}

int hashing(const char* str) {
	int hashing = 5381;
	int c;
	while (c = *str++) {
		hashing = ((hashing << 5) + hashing) + c; /* hash * 33 + c */
		if (hashing < 0) hashing = 100;
	}
	if (hashing < 0) hashing = 100;
	return hashing;
}

void DoDebugLoop()
{
	// ...
}

void DoDebugStuff()
{
	// ...

	RwMatrix mat = pGame->FindPlayerPed()->m_pPed->GetMatrix().ToRwMatrix();
	
	for (int i = 0; i < 100; i++)
	{
		CPlayerPed* ped = pGame->NewPlayer(i, mat.pos.x + i, mat.pos.y, mat.pos.z, 0.0f, false, false);
		//ped->SetCollisionChecking(false);
		//ped->SetGravityProcessing(false);
	}
}
struct sigaction act_old;
struct sigaction act1_old;
struct sigaction act2_old;
struct sigaction act3_old;

extern int g_iLastProcessedSkinCollision, g_iLastProcessedEntityCollision, g_iLastRenderedObject;
extern uintptr_t g_dwLastRetAddrCrash;
void handler(int signum, siginfo_t *info, void* contextPtr)
{
	ucontext* context = (ucontext_t*)contextPtr;

	// on journalise AVANT de chainer : le handler systeme termine le processus
	if(info->si_signo == SIGSEGV)
	{
		FLog("SIGSEGV | Fault address: 0x%x", info->si_addr);

		PRINT_CRASH_STATES(context);

		CStackTrace::printBacktrace();
	}

	if (act_old.sa_sigaction)
	{
		act_old.sa_sigaction(signum, info, contextPtr);
	}

	return;
}

void handler1(int signum, siginfo_t *info, void* contextPtr)
{
	ucontext* context = (ucontext_t*)contextPtr;

	// on journalise AVANT de chainer : le handler systeme termine le processus
	if(info->si_signo == SIGABRT)
	{
		FLog("SIGABRT | Fault address: 0x%x", info->si_addr);

		PRINT_CRASH_STATES(context);

		CStackTrace::printBacktrace();
	}

	if (act1_old.sa_sigaction)
	{
		act1_old.sa_sigaction(signum, info, contextPtr);
	}

	return;
}

void handler2(int signum, siginfo_t *info, void* contextPtr)
{
	ucontext* context = (ucontext_t*)contextPtr;

	// on journalise AVANT de chainer : le handler systeme termine le processus
	if(info->si_signo == SIGFPE)
	{
		FLog("SIGFPE | Fault address: 0x%x", info->si_addr);

		PRINT_CRASH_STATES(context);

		CStackTrace::printBacktrace();
	}

	if (act2_old.sa_sigaction)
	{
		act2_old.sa_sigaction(signum, info, contextPtr);
	}

	return;
}

void handler3(int signum, siginfo_t *info, void* contextPtr)
{
	ucontext* context = (ucontext_t*)contextPtr;

	// on journalise AVANT de chainer : le handler systeme termine le processus
	if(info->si_signo == SIGBUS)
	{
		FLog("SIGBUS | Fault address: 0x%x", info->si_addr);

		PRINT_CRASH_STATES(context);

		CStackTrace::printBacktrace();
	}

	if (act3_old.sa_sigaction)
	{
		act3_old.sa_sigaction(signum, info, contextPtr);
	}

	return;
}

// ---------------------------------------------------------------------------------------------
// [Crestwood] Pont avec le HUD HTML du client 32 bits (com.gta.game32.SAMP) :
//  - commandes / messages envoyes par les pages HTML (file d'attente videe dans MainLoop, thread du jeu)
//  - ouverture du chat SA-MP natif depuis le bouton "chat" du HUD
//  - g_bHtmlHud : le HUD HTML remplace celui de GTA (radar, vie, argent) -> CHud::Draw n'est plus appele
// ---------------------------------------------------------------------------------------------
bool g_bHtmlHud = false;
static std::mutex g_hudCmdMutex;
static std::vector<std::string> g_hudCmdQueue;
static bool g_hudOpenChat = false;

static void FlushHudCommands()
{
	std::vector<std::string> cmds;
	bool openChat = false;
	{
		std::lock_guard<std::mutex> lock(g_hudCmdMutex);
		cmds.swap(g_hudCmdQueue);
		openChat = g_hudOpenChat;
		g_hudOpenChat = false;
	}
	if (openChat && pUI && pUI->keyboard() && pUI->chat()) pUI->keyboard()->show(pUI->chat());
	if (!pNetGame) return;
	for (const std::string& c : cmds) {
		if (c.empty()) continue;
		if (c[0] == '/') pNetGame->SendChatCommand(c.c_str());
		else {
			pNetGame->SendChatMessage(c.c_str());
			// bulle au-dessus de sa propre tete (texte en Windows-1252)
			CPlayerPool* pPlayerPool = pNetGame->GetPlayerPool();
			if (pPlayerPool && pPlayerPool->GetLocalPlayer() && pNetGame->GetPlayerBubblePool())
				pNetGame->GetPlayerBubblePool()->NewChat(pPlayerPool->GetLocalPlayerID(), c.c_str(),
					pPlayerPool->GetLocalPlayer()->GetPlayerColorAsRGBA(), false);
		}
	}
}

// Etat du joueur pour le HUD HTML (vie, gilet, vehicule, position + camera pour la minimap/GPS), 10 fois par seconde.
static void PushHudState()
{
	if (!pJavaWrapper || !pGame) return;
	static auto lastHudUpdate = std::chrono::steady_clock::now();
	const auto nowHud = std::chrono::steady_clock::now();
	const float hudDelta = std::chrono::duration<float>(nowHud - lastHudUpdate).count();
	if (hudDelta < 0.10f) return;
	lastHudUpdate = nowHud;

	CPlayerPed* localPed = pGame->FindPlayerPed();
	if (!localPed || !localPed->m_pPed) return;
	const bool dead = localPed->IsDead();
	const bool running = !dead && (localPed->m_pPed->m_nMoveState == PEDMOVE_RUN || localPed->m_pPed->m_nMoveState == PEDMOVE_SPRINT);
	CVehicleGTA* vehicle = localPed->IsInVehicle() ? localPed->GetGtaVehicle() : nullptr;
	const bool inVehicle = vehicle != nullptr;
	float vehicleSpeed = 0.0f;
	float vehicleHealth = 100.0f;
	bool handbrake = false;
	if (vehicle) {
		const CVector velocity = vehicle->m_vecMoveSpeed;
		vehicleSpeed = std::sqrt(velocity.x * velocity.x + velocity.y * velocity.y + velocity.z * velocity.z) * 180.0f;
		if (vehicleSpeed < 0.0f) vehicleSpeed = 0.0f;
		if (vehicleSpeed > 240.0f) vehicleSpeed = 240.0f;
		// [v15] vraie sante du vehicule pour la barre rouge du compteur : 1000 = neuf, 250 = en feu (0 %)
					vehicleHealth = vehicle->physicalFlags.bDestroyed ? 0.0f : (vehicle->fHealth - 250.0f) / 7.5f;
					if (vehicleHealth < 0.0f) vehicleHealth = 0.0f;
					if (vehicleHealth > 100.0f) vehicleHealth = 100.0f;
		handbrake = (vehicle->m_nVehicleUpperFlags & (1u << 5)) != 0;
	}
	static float energy = 100.0f;
	static bool wasDead = false;
	if (dead) energy = 0.0f;
	else if (wasDead) energy = 100.0f;
	else if (running) energy -= 24.0f * hudDelta;
	else energy += 18.0f * hudDelta;
	if (energy < 0.0f) energy = 0.0f;
	if (energy > 100.0f) energy = 100.0f;
	const bool exhausted = energy <= 0.1f;
	wasDead = dead;

	CCamera& hudCamera = *reinterpret_cast<CCamera*>(g_libGTASA + (VER_x32 ? 0x00951FA8 : 0xBBA8D0));
	const CVector hudPos = vehicle ? vehicle->GetPosition() : localPed->m_pPed->GetPosition();
	const CVector hudCamFwd = hudCamera.GetForwardVector();
	const float hudHeading = vehicle ? vehicle->GetHeading() : localPed->m_pPed->GetHeading();
	{
				// Argent : envoye au HUD a chaque changement (et toutes les 5 s, au cas ou le HUD a ete recharge)
				static int lastHudMoney = -2147483647;
				static uint32_t lastHudMoneyMs = 0;
				const int money = pGame->GetLocalMoney();
				const uint32_t nowMs = GetTickCount();
				if (money != lastHudMoney || nowMs - lastHudMoneyMs > 5000) {
					lastHudMoney = money;
					lastHudMoneyMs = nowMs;
					pJavaWrapper->UpdateHudMoney(money);
				}
			}
			pJavaWrapper->UpdateHudState(localPed->GetHealth(), localPed->GetArmour(), 100.0f, 100.0f, energy, dead, running, exhausted,
		inVehicle, vehicleSpeed, vehicleHealth, handbrake, hudPos.x, hudPos.y, hudCamFwd.x, hudCamFwd.y, hudHeading);
}

void DoInitStuff()
{
	if (bGameInited == false)
	{
		pPlayerTags = new CPlayerTags();
		pSnapShotHelper = new CSnapShotHelper();
		pMaterialTextGenerator = new MaterialTextGenerator();
		pAudioStream = new CAudioStream();
		pAudioStream->Initialize();

		pUI->splashscreen()->setVisible(false);
		// [Crestwood] meme comportement que le client 64 bits : la barre de touches SA-MP (TAB, ALT, H, Y, N, F, G...)
		// n'est plus affichee, le HUD HTML a ses propres boutons. Le chat natif est masque des que le HUD HTML
		// est actif (voir MainLoop), sinon il reste visible pour ne pas perdre le chat si le HUD ne charge pas.
		pUI->chat()->setVisible(!g_bHtmlHud);
		pUI->buttonpanel()->setVisible(false);  // [Crestwood] barre de touches SA-MP (TAB, Y, N, H, ALT...) supprimee

		pGame->Initialize();
		pGame->SetMaxStats();
		pGame->ToggleThePassingOfTime(false);

		// voice
		/*LogVoice("[dbg:samp:load] : module loading...");

		for (const auto& loadCallback : Samp::loadCallbacks) {
			if (loadCallback != nullptr) {
				loadCallback();
			}
		}

		Samp::loadStatus = true;*/

		LogVoice("[dbg:samp:load] : module loaded");

		if (bDebug)
		{
            CCamera& TheCamera = *reinterpret_cast<CCamera*>(g_libGTASA + (VER_x32 ? 0x00951FA8 : 0xBBA8D0));
            //TheCamera.Restore();
            CCamera::SetBehindPlayer();
			pGame->DisplayHUD(true);
			pGame->EnableClock(false);

			DoDebugStuff();
		}

		bGameInited = true;
	}

	if (!bNetworkInited && !bDebug)
	{
		//ReadSettingFile();

		// [Crestwood] serveur lu dans SAMP/settings.ini (ecrit par le launcher) au lieu d'une IP en dur
		FLog("Pre-CNetGame: host='%s' port=%d nick='%s'", pSettings->Get().szHost, pSettings->Get().iPort, pSettings->Get().szNickName);
		pNetGame = new CNetGame(pSettings->Get().szHost, pSettings->Get().iPort, pSettings->Get().szNickName, pSettings->Get().szPassword);
		bNetworkInited = true;

        FLog("DoInitStuff end");
	}
}

#include <atomic>
std::atomic<bool> g_bHudVoiceTalk{false};
void HudPad_SetControl(int control, bool pressed);   // game/pad.cpp
void HudPad_SetStick(int x, int y, bool active);    // game/pad.cpp
#include "game/WorldMarkers.h"

// [v15] game/CwSettings.cpp
void Crestwood_ApplyGameSetting(int key, int value);
void Crestwood_SetHiddenPlayers(const char* csv);
void Crestwood_ProcessHiddenPlayers();

extern "C" {
	JNIEXPORT void JNICALL Java_com_gta_game32_SAMP_initializeSAMP(JNIEnv *pEnv, jobject thiz)
	{
		pJavaWrapper = new CJavaWrapper(pEnv, thiz);

	}
	JNIEXPORT void JNICALL Java_com_gta_game32_SAMP_onInputEnd(JNIEnv *pEnv, jobject thiz, jbyteArray str)
	{
		if(pUI)
		{
			pUI->keyboard()->sendForGB(pEnv, thiz, str);
		}
	}
	// [Crestwood] HUD HTML -> serveur : commande ou message (octets Windows-1252)
	JNIEXPORT void JNICALL Java_com_gta_game32_SAMP_sendChatBytes(JNIEnv *pEnv, jobject thiz, jbyteArray str)
	{
		if (!pEnv || !str) return;
		jsize length = pEnv->GetArrayLength(str);
		if (length <= 0 || length > 255) return;
		jbyte* data = pEnv->GetByteArrayElements(str, nullptr);
		if (!data) return;
		std::string text((const char*)data, (size_t)length);
		pEnv->ReleaseByteArrayElements(str, data, JNI_ABORT);
		std::lock_guard<std::mutex> lock(g_hudCmdMutex);
		if (g_hudCmdQueue.size() < 64) g_hudCmdQueue.push_back(text);
	}
	// [Crestwood] bouton micro du HUD HTML : true tant que le joueur maintient le bouton (vocal SampVoice)
	// [v15] parametres en jeu (page assets/config) -> moteur (FPS, distance, effets, boutons)
	JNIEXPORT void JNICALL Java_com_gta_game32_SAMP_applyGameSetting(JNIEnv *pEnv, jobject thiz, jint key, jint value)
	{
		Crestwood_ApplyGameSetting((int)key, (int)value);
	}
	// [v15] joueurs invisibles (admins en mode invisible) : "3,7" (dialogue serveur "@hide" 9517)
	JNIEXPORT void JNICALL Java_com_gta_game32_SAMP_setHiddenPlayers(JNIEnv *pEnv, jobject thiz, jstring data)
	{
		if (!data) { Crestwood_SetHiddenPlayers(nullptr); return; }
		const char* text = pEnv->GetStringUTFChars(data, nullptr);
		Crestwood_SetHiddenPlayers(text);
		if (text) pEnv->ReleaseStringUTFChars(data, text);
	}
	JNIEXPORT void JNICALL Java_com_gta_game32_SAMP_setVoiceTalk(JNIEnv *pEnv, jobject thiz, jboolean talk)
	{
		g_bHudVoiceTalk.store(talk == JNI_TRUE);
	}
	// [Crestwood] touches de conduite / a pied du HUD HTML -> touches SA-MP (voir game/pad.cpp)
	JNIEXPORT void JNICALL Java_com_gta_game32_SAMP_setVehicleControl(JNIEnv *pEnv, jobject thiz, jint control, jboolean pressed)
	{
		HudPad_SetControl((int)control, pressed == JNI_TRUE);
	}
	// [Crestwood] joystick analogique du HUD HTML (-128..128) -> marche / direction (voir game/pad.cpp)
	JNIEXPORT void JNICALL Java_com_gta_game32_SAMP_setHudStick(JNIEnv *pEnv, jobject thiz, jint x, jint y, jboolean active)
	{
		HudPad_SetStick((int)x, (int)y, active == JNI_TRUE);
	}
	// [Crestwood] marqueurs 3D flottants : "type,x,y,z,libelle;..." (voir game/WorldMarkers.h)
	JNIEXPORT void JNICALL Java_com_gta_game32_SAMP_setWorldMarkers(JNIEnv *pEnv, jobject thiz, jstring data)
	{
		if (!data) { WorldMarkers::Set(nullptr); return; }
		const char* text = pEnv->GetStringUTFChars(data, nullptr);
		WorldMarkers::Set(text);
		if (text) pEnv->ReleaseStringUTFChars(data, text);
	}
	JNIEXPORT void JNICALL Java_com_gta_game32_SAMP_openChatNative(JNIEnv *pEnv, jobject thiz)
	{
		std::lock_guard<std::mutex> lock(g_hudCmdMutex);
		g_hudOpenChat = true;
	}
	JNIEXPORT void JNICALL Java_com_gta_game32_SAMP_setHtmlHud(JNIEnv *pEnv, jobject thiz, jboolean enabled)
	{
		g_bHtmlHud = enabled;
	}
	// [Crestwood] vetements / creation de personnage (memes appels que le client 64 bits)
	JNIEXPORT void JNICALL Java_com_gta_game32_SAMP_openClothesTest(JNIEnv *pEnv, jobject thiz)
	{
		CharacterCreator::RequestOpenTest();
	}
	JNIEXPORT void JNICALL Java_com_gta_game32_SAMP_onClothesDialog(JNIEnv *pEnv, jobject thiz, jint dialogId, jstring json)
	{
		std::string payload;
		if (json) {
			const char* text = pEnv->GetStringUTFChars(json, nullptr);
			if (text) { payload = text; pEnv->ReleaseStringUTFChars(json, text); }
		}
		CharacterCreator::OnServerDialog(static_cast<int>(dialogId), payload);
	}
	JNIEXPORT void JNICALL Java_com_gta_game32_SAMP_clothesCommand(JNIEnv *pEnv, jobject thiz, jint type, jstring key, jint value)
	{
		std::string k;
		if (key) {
			const char* text = pEnv->GetStringUTFChars(key, nullptr);
			if (text) { k = text; pEnv->ReleaseStringUTFChars(key, text); }
		}
		CharacterCreator::OnHtmlCommand(static_cast<int>(type), k, static_cast<int>(value));
	}
	// [Crestwood] dialogues HTML (com.gta.game.ui.dialog.DialogManager, partage avec le client 64 bits)
	JNIEXPORT void JNICALL Java_com_gta_game_ui_dialog_DialogManager_sendDialogResponse(JNIEnv* pEnv, jobject thiz, jint i3, jint i, jint i2, jbyteArray str)
	{
		if (!pEnv || !str) return;
		jbyte* pMsg = pEnv->GetByteArrayElements(str, nullptr);
		jsize length = pEnv->GetArrayLength(str);
		if (!pMsg || length < 0) return;
		std::string szStr((char*)pMsg, length);
		if (pNetGame) pNetGame->SendDialogResponse(i, i3, i2, (char*)szStr.c_str());
		pEnv->ReleaseByteArrayElements(str, pMsg, JNI_ABORT);
	}
	JNIEXPORT void JNICALL Java_com_gta_game32_SAMP_onEventBackPressed(JNIEnv *pEnv, jobject thiz)
	{
		if(pSettings)
		{
			if(pSettings->Get().iAndroidKeyboard)
				pJavaWrapper->HideKeyboard();
		}
	}
	JNIEXPORT void JNICALL Java_com_gta_game32_ui_dialog_DialogManager_sendDialogResponse(JNIEnv* pEnv, jobject thiz, jint i3, jint i, jint i2, jbyteArray str)
	{
		jboolean isCopy = true;

		jbyte* pMsg = pEnv->GetByteArrayElements(str, &isCopy);
		jsize length = pEnv->GetArrayLength(str);

		std::string szStr((char*)pMsg, length);
		
		if(pNetGame) {
			pNetGame->SendDialogResponse(i, i3, i2, (char*)szStr.c_str());
			//pGame->FindPlayerPed()->TogglePlayerControllableWithoutLock(true);
		}

		pEnv->ReleaseByteArrayElements(str, pMsg, JNI_ABORT);
	}
}

void MainLoop()
{
	if (pGame->bIsGameExiting) return;

	DoInitStuff();
	Crestwood_ProcessHiddenPlayers();   // [v15] admins invisibles
	{   // [fix 09/11] HUD HTML actif : le HUD natif (argent vert, heure, radar) ne doit plus etre dessine,
		// y compris par l'appel interne du jeu -> CHud::Draw est neutralise une fois pour toutes.
		static bool s_hudKilled = false;
		{   // [09/11c] le HUD natif restait affiche malgre le RET : on coupe aussi son drapeau a chaque image
		static uint8_t* s_wants = nullptr;
		if (!s_wants && CHook::lib) s_wants = (uint8_t*)dlsym(CHook::lib, "_ZN4CHud19m_Wants_To_Draw_HudE");
		if (s_wants) *s_wants = 0;
	}
	/* [09/11b] sans condition : g_bHtmlHud restait faux sur cet appareil */ if (bGameInited && !s_hudKilled) { s_hudKilled = true; CHook::RET("_ZN4CHud4DrawEv"); CHook::RET("_ZN17CWidgetPlayerInfo4DrawEv");   /* [09/11d] widget argent / heure / arme / sante */ FLog("[hud] HUD natif desactive (CHud::Draw), g_bHtmlHud=%d", (int)g_bHtmlHud); }
	}
	{   // [diag 08/11] etat du joueur local toutes les 2 s (deplacement / boutons)
		static uint32_t s_diagNext = 0; static int s_diagCount = 0;
		void HudPad_GetStick(int*, int*, int*); int sx = 0, sy = 0, son = 0; HudPad_GetStick(&sx, &sy, &son);
		if (pGame && pNetGame && s_diagCount < 40 && GetTickCount() >= s_diagNext) {
			s_diagNext = GetTickCount() + 2000;
			CPlayerPed* lp = pGame->FindPlayerPed();
			if (lp && lp->m_pPed) {
				++s_diagCount;
				CPedGTA* pd = lp->m_pPed;
				{   // camera / orientation : le stick est relatif a la camera
					void HudPad_GetPad(int*, int*); int plr = 0, pud = 0; HudPad_GetPad(&plr, &pud);
					CCamera* cam = reinterpret_cast<CCamera*>(g_libGTASA + (VER_x32 ? 0x00951FA8 : 0xBBA8D0));
					{ const CCam& ac = cam->m_aCams[cam->m_nActiveCam % 3];
					  FLog("[diag-cam2] orientation=%.2f horizAngle=%.2f alpha=%.2f mode=%d activeCam=%d", cam->m_fOrientation, ac.m_fHorizontalAngle, ac.Alpha, (int)ac.m_nMode, (int)cam->m_nActiveCam); }
					FLog("[diag-cam] camHeading=%.2f pedHeading=%.2f padLR=%d padUD=%d camPos=%.1f,%.1f,%.1f",
						cam->GetHeading(), pd->GetHeading(), plr, pud,
						cam->GetPosition().x, cam->GetPosition().y, cam->GetPosition().z);
				}
				{   // animations et taches du joueur local
					void* clump = pd->m_pRwObject;
					int init = -1, nassoc = -1, firstAnim = -1;
					if (clump) {
						init = (int)CHook::CallFunction<unsigned int>("_Z29RpAnimBlendClumpIsInitializedP7RpClump", clump);
						nassoc = (int)CHook::CallFunction<int>("_Z34RpAnimBlendClumpGetNumAssociationsP7RpClump", clump);
						void* a = CHook::CallFunction<void*>("_Z35RpAnimBlendClumpGetFirstAssociationP7RpClump", clump);
						if (a) firstAnim = (int)*reinterpret_cast<uint16_t*>(reinterpret_cast<uintptr_t>(a) + 0x2c);
					}
					unsigned prim = 0;
					if (pd->m_pIntelligence) for (int i = 0; i < 5; ++i) if (pd->m_pIntelligence->m_TaskMgr.m_aPrimaryTasks[i]) prim |= (1u << i);
					int tDef = -1, tSub = -1;
					if (pd->m_pIntelligence) {
						CTask* t = pd->m_pIntelligence->m_TaskMgr.m_aPrimaryTasks[4];
						if (t) { tDef = (int)t->GetTaskType(); CTask* st = t->GetSubTask(); if (st) tSub = (int)st->GetTaskType(); }
					}
					FLog("[diag-anim] clump=%p init=%d assoc=%d premiereAnim=%d tachesPrimaires=0x%x tacheDefaut=%d sousTache=%d", clump, init, nassoc, firstAnim, prim, tDef, tSub);
				}
				FLog("[diag] modele=%d etat=%d veh=%d jet=%d stick=%d/%d on=%d moveBlend=%.2f pos=%.1f,%.1f,%.1f",
					(int)pd->m_nModelIndex, (int)pd->m_nPedState, (int)lp->IsInVehicle(), (int)lp->IsInJetpackMode(),
					sx, sy, son,
					pd->m_pPlayerData ? pd->m_pPlayerData->m_fMoveBlendRatio : -1.0f,
					pd->GetPosition().x, pd->GetPosition().y, pd->GetPosition().z);
			}
		}
	}
	if (bGameInited) SouthModelLoader::PreloadBodies();   // [v13] corps Homme/Femme de l'APK sur leurs emplacements dedies

	// [Crestwood] HUD HTML actif : chat SA-MP natif et barre de touches natives toujours masques
	// (certaines actions du client les reaffichaient).
	if (g_bHtmlHud && pUI) {
		if (pUI->chat() && pUI->chat()->visible()) pUI->chat()->setVisible(false);
		// [Crestwood] la barre de touches SA-MP native reste visible (boutons natifs reactives)
		if (pUI->buttonpanel() && pUI->buttonpanel()->visible()) pUI->buttonpanel()->setVisible(false);   // [Crestwood] barre SA-MP supprimee
	}

	FlushHudCommands();
	PushHudState();

	if (bDebug) {
		DoDebugLoop();
	}

	if (pNetGame) {
		pNetGame->Process();
	}

	if (pAudioStream) {
		pAudioStream->Process();
	}

}

void InitGui()
{
	// new voice
	Plugin::OnPluginLoad();
	Plugin::OnSampLoad();

	std::string font_path = string_format("%sSAMP/fonts/%s", g_pszStorage, FONT_NAME);
	// [Crestwood] meme data que le 64 bits : pas de SAMP/fonts/, la police est dans <data>/fonts/
	// (sans fichier, ImGui::AddFontFromFileTTF declenche un assert -> SIGABRT)
	if (access(font_path.c_str(), R_OK) != 0)
	{
		std::string alt = string_format("%sfonts/%s", g_pszStorage, FONT_NAME);
		if (access(alt.c_str(), R_OK) == 0)
		{
			FLog("Police %s introuvable, utilisation de %s", font_path.c_str(), alt.c_str());
			font_path = alt;
		}
		else FLog("[FATAL]: police introuvable (%s et %s)", font_path.c_str(), alt.c_str());
	}
	pUI = new UI(ImVec2(RsGlobal->maximumWidth, RsGlobal->maximumHeight), font_path.c_str());
	pUI->initialize();
	pUI->performLayout();
}

#include "game/multitouch.h"
#include "armhook/patch.h"
#include "util/CUtil.h"
void SetUpGLHooks();
jint JNI_OnLoad(JavaVM* vm, void* reserved)
{
	javaVM = vm;
	LOGI("SA-MP library loaded! Build time: " __DATE__ " " __TIME__);

	g_libGTASA = CUtil::FindLib("libGTASA.so");
	if (g_libGTASA == 0x00) {
		LOGE("libGTASA.so address was not found! ");
		return JNI_VERSION_1_6;
	}

	g_libSAMP = CUtil::FindLib("libsamp.so");
	if (g_libSAMP == 0x00) {
		LOGE("libsamp.so address was not found! ");
		return JNI_VERSION_1_6;
	}

	firebase::crashlytics::Initialize();

	uintptr_t libgtasa = CUtil::FindLib("libGTASA.so");
	uintptr_t libsamp = CUtil::FindLib("libsamp.so");
	uintptr_t libc = CUtil::FindLib("libc.so");

	FLog("libGTASA.so: 0x%x", libgtasa);
	FLog("libsamp.so: 0x%llx", (unsigned long long)libsamp);
	FLog("libc.so: 0x%llx", (unsigned long long)libc);

	char str[100];

	sprintf(str, "0x%x", libgtasa);
	firebase::crashlytics::SetCustomKey("libGTASA.so", str);
	
	sprintf(str, "0x%x", libsamp);
	firebase::crashlytics::SetCustomKey("libsamp.so", str);

	sprintf(str, "0x%x", libc);
	firebase::crashlytics::SetCustomKey("libc.so", str);

	CHook::InitHookStuff();
	InstallSpecialHooks();
	ApplyPatches_level0();
    //SetUpGLHooks();
    InitRenderWareFunctions();
    MultiTouch::initialize();

	pGame = new CGame();

	//pVoice = new CVoice();
	//pVoice->Initialize(VOICE_FREQUENCY, CODEC_FREQUENCY, VOICE_SENDRRATE);

	//pthread_t thread;
	//pthread_create(&thread, 0, Init, 0);

	struct sigaction act;
	act.sa_sigaction = handler;
	sigemptyset(&act.sa_mask);
	act.sa_flags = SA_SIGINFO;
	sigaction(SIGSEGV, &act, &act_old);

	struct sigaction act1;
	act1.sa_sigaction = handler1;
	sigemptyset(&act1.sa_mask);
	act1.sa_flags = SA_SIGINFO;
	sigaction(SIGABRT, &act1, &act1_old);

	struct sigaction act2;
	act2.sa_sigaction = handler2;
	sigemptyset(&act2.sa_mask);
	act2.sa_flags = SA_SIGINFO;
	sigaction(SIGFPE, &act2, &act2_old);

	struct sigaction act3;
	act3.sa_sigaction = handler3;
	sigemptyset(&act3.sa_mask);
	act3.sa_flags = SA_SIGINFO;
	sigaction(SIGBUS, &act3, &act3_old);
		
	return JNI_VERSION_1_6;
}

uint32_t GetTickCount()
{
    return CTimer::m_snTimeInMillisecondsNonClipped;
}	

void FLog(const char* fmt, ...)
{
	char buffer[0xFF];
	static FILE* flLog = nullptr;
	const char* pszStorage = g_pszStorage;


	// Journal natif du client 32 bits : TOUJOURS dans le dossier propre a l'appli (ecriture sans permission,
	// et pas melange avec les samp_log.txt d'autres clients SA-MP installes sur le telephone).
	//   /storage/emulated/0/Android/data/com.crestwood.rp/files/crestwood_samp_log.txt
	// (a cote de crestwood_32_log.txt, le journal Java du chargement). Dernier recours : dossier des donnees.
	(void)pszStorage;
	static int tries = 0;
	if (flLog == nullptr && tries < 40)
	{
		tries++;
		static char path[256];
		static const char* kPaths[] = {
			"/storage/emulated/0/data/crestwood_samp_log.txt", // dossier des donnees : visible dans le gestionnaire de fichiers
			"/storage/emulated/0/Android/data/com.crestwood.rp/files/crestwood_samp_log.txt",
			"/sdcard/Android/data/com.crestwood.rp/files/crestwood_samp_log.txt",
			"/data/data/com.crestwood.rp/files/crestwood_samp_log.txt",
		};
		for (const char* f : kPaths)
		{
			flLog = fopen(f, "a");
			if (flLog) { snprintf(path, sizeof(path), "%s", f); break; }
		}
		if (flLog == nullptr && g_pszStorage != nullptr)
		{
			snprintf(path, sizeof(path), "%s/crestwood_samp_log.txt", g_pszStorage);
			flLog = fopen(path, "a");
		}
		if (flLog)
		{
			// pas plus de 2 Mo : on repart d'un fichier vide
			fseek(flLog, 0, SEEK_END);
			if (ftell(flLog) > 2 * 1024 * 1024) { fclose(flLog); flLog = fopen(path, "w"); }
			if (flLog) fprintf(flLog, "\n===== nouvelle session (natif 32 bits) =====\n");
		}
	}

	memset(buffer, 0, sizeof(buffer));

	va_list arg;
	va_start(arg, fmt);
	vsnprintf(buffer, sizeof(buffer), fmt, arg);
	va_end(arg);

	LOGI("%s", buffer);
	firebase::crashlytics::Log(buffer);

	if (flLog == nullptr) return;
	fprintf(flLog, "%s\n", buffer);
	fflush(flLog);

	return;
}

void ChatLog(const char* fmt, ...)
{
	char buffer[0xFF];
	static FILE* flLog = nullptr;
	const char* pszStorage = g_pszStorage;


	if (flLog == nullptr && pszStorage != nullptr)
	{
		sprintf(buffer, "%s/chat_log.txt", pszStorage);
		flLog = fopen(buffer, "a");
	}

	memset(buffer, 0, sizeof(buffer));

	va_list arg;
	va_start(arg, fmt);
	vsnprintf(buffer, sizeof(buffer), fmt, arg);
	va_end(arg);

	if (flLog == nullptr) return;
	fprintf(flLog, "%s\n", buffer);
	fflush(flLog);

	return;
}

void MyLog(const char* fmt, ...)
{
	char buffer[0xFF];
	static FILE* flLog = nullptr;
	const char* pszStorage = g_pszStorage;


	if (flLog == nullptr && pszStorage != nullptr)
	{
		sprintf(buffer, "%s/samp_log.txt", pszStorage);
		//LOGI("buffer: %s", buffer);
		flLog = fopen(buffer, "a");
	}

	memset(buffer, 0, sizeof(buffer));

	va_list arg;
	va_start(arg, fmt);
	vsnprintf(buffer, sizeof(buffer), fmt, arg);
	va_end(arg);

	if (flLog == nullptr) return;
	fprintf(flLog, "%s\n", buffer);
	fflush(flLog);

	return;
}

void MyLog2(const char* fmt, ...)
{
	char buffer[0xFF];
	static FILE* flLog = nullptr;
	const char* pszStorage = g_pszStorage;


	if (flLog == nullptr && pszStorage != nullptr)
	{
		sprintf(buffer, "%s/samp_log.txt", pszStorage);
		//LOGI("buffer: %s", buffer);
		flLog = fopen(buffer, "a");
	}

	memset(buffer, 0, sizeof(buffer));

	va_list arg;
	va_start(arg, fmt);
	vsnprintf(buffer, sizeof(buffer), fmt, arg);
	va_end(arg);

	if (pUI) pUI->chat()->addDebugMessage(buffer);

	if (flLog == nullptr) return;
	fprintf(flLog, "%s\n", buffer);
	fflush(flLog);
	return;
}

void LogVoice(const char* fmt, ...)
{
	char buffer[0xFF];
	static FILE* flLog = nullptr;
	const char* pszStorage = g_pszStorage;

	if (flLog == nullptr && pszStorage != nullptr)
	{
		sprintf(buffer, "%sSAMP/%s", pszStorage, SV::kLogFileName);
		flLog = fopen(buffer, "w");
	}

	memset(buffer, 0, sizeof(buffer));

	va_list arg;
	va_start(arg, fmt);
	vsnprintf(buffer, sizeof(buffer), fmt, arg);
	va_end(arg);

	__android_log_write(ANDROID_LOG_INFO, "AXL", buffer);

	if (flLog == nullptr) return;
	fprintf(flLog, "%s\n", buffer);
	fflush(flLog);

	return;
}