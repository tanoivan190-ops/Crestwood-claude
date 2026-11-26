// ============================================================================
//  [v15] Crestwood : parametres en jeu (page assets/config) + joueurs caches (admin invisible)
//  Fichier IDENTIQUE dans samp/ (64 bits) et samp32/ (32 bits).
//    Crestwood_ApplyGameSetting(cle, valeur)  <- JNI applyGameSetting (SAMP.java)
//      1 compteur FPS (0/1)          2 limite FPS (30..120)
//      3 distance d'affichage (%)    4 effets visuels / post-traitement (0/1)
//      5 bouton micro (0/1)          6 icones Crestwood des boutons (0/1)
//    Crestwood_SetHiddenPlayers("3,7")    <- JNI setHiddenPlayers (dialogue serveur "@hide" 9517)
//    Crestwood_ProcessHiddenPlayers()     <- MainLoop, chaque image
// ============================================================================
#include "main.h"
#include "game/game.h"
#include "net/netgame.h"
#include "settings.h"
#include <atomic>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <dlfcn.h>
#include <mutex>
#include <set>

extern CNetGame* pNetGame;
extern CSettings* pSettings;
extern std::atomic<bool> g_cwCustomButtons;   // game/Widgets/WidgetGta.cpp
void ApplyFPSPatch(uint8_t fps);              // game/patches.cpp

std::atomic<bool> g_cwVoiceButton{true};

static void* Cw_GameSym(const char* sym)
{
	static void* s_lib = nullptr;
	if (!s_lib) {
		const char* libs[] = { "libGame.so", "libGTASA.so" };
		for (const char* name : libs) {
			s_lib = dlopen(name, RTLD_NOLOAD | RTLD_LAZY);
			if (s_lib) break;
		}
	}
	return s_lib ? dlsym(s_lib, sym) : nullptr;
}

void Crestwood_ApplyGameSetting(int key, int value)
{
	switch (key) {
		case 1:
			if (pSettings) pSettings->GetWrite().iFPSCounter = value != 0;
			break;
		case 2: {
			int fps = value;
			if (fps < 20) fps = 20;
			if (fps > 120) fps = 120;
			ApplyFPSPatch((uint8_t)fps);
			if (pSettings) pSettings->GetWrite().iFPSCount = fps;
			break;
		}
		case 3: {
			// distance des niveaux de detail : 100 % = jeu d'origine
			float* lod = (float*)Cw_GameSym("_ZN9CRenderer15ms_lodDistScaleE");
			int pct = value < 30 ? 30 : (value > 200 ? 200 : value);
			if (lod) *lod = (float)pct / 100.0f;
			break;
		}
		case 4: {
			const char* syms[] = { "_ZN12CPostEffects23m_bDisableAllPostEffectE", "_ZN12CPostEffects24m_bDisableAllPostEffectE" };
			for (const char* s : syms) {
				bool* off = (bool*)Cw_GameSym(s);
				if (off) { *off = (value == 0); break; }
			}
			break;
		}
		case 5:
			g_cwVoiceButton.store(value != 0);
			break;
		case 6:
			g_cwCustomButtons.store(value != 0);
			break;
		default:
			break;
	}
}

// ---------------------------------------------------------------- joueurs caches
static std::mutex s_hideMutex;
static std::set<int> s_hidden;     // demandes par le serveur
static std::set<int> s_applied;    // actuellement caches sur ce client

void Crestwood_SetHiddenPlayers(const char* csv)
{
	std::set<int> next;
	if (csv) {
		const char* p = csv;
		while (*p) {
			while (*p == ',' || *p == ' ' || *p == ';') p++;
			if (!*p) break;
			char* end = nullptr;
			long id = strtol(p, &end, 10);
			if (end == p) { p++; continue; }
			if (id >= 0 && id < 1004) next.insert((int)id);
			p = end;
		}
	}
	std::lock_guard<std::mutex> lock(s_hideMutex);
	s_hidden.swap(next);
}

static void Cw_SetPedVisible(CPlayerPool* pool, int id, bool visible)
{
	if (!pool || !pool->GetSlotState((PLAYERID)id)) return;
	CRemotePlayer* rp = pool->GetAt((PLAYERID)id);
	if (!rp) return;
	CPlayerPed* ped = rp->GetPlayerPed();
	if (!ped || !ped->m_pPed) return;
	ped->m_pPed->m_bIsVisible = visible;
}

void Crestwood_ProcessHiddenPlayers()
{
	if (!pNetGame) return;
	CPlayerPool* pool = pNetGame->GetPlayerPool();
	if (!pool) return;
	std::set<int> cur;
	{
		std::lock_guard<std::mutex> lock(s_hideMutex);
		if (s_hidden.empty() && s_applied.empty()) return;
		cur = s_hidden;
	}
	for (int id : s_applied)
		if (!cur.count(id)) Cw_SetPedVisible(pool, id, true);
	for (int id : cur)
		Cw_SetPedVisible(pool, id, false);
	s_applied.swap(cur);
}


// ============================================================================
//  [v46] Identifiant du jour au-dessus des tetes (au lieu du pseudo)
//    Crestwood_SetPlayerTags("1|3:4821,7:1290")  <- JNI setPlayerTags (dialogue serveur "@tags" 9518)
//      1 = afficher "(4821)" ; 0 = pseudos normaux (staff avec /nomsstaff)
//    Crestwood_FormatPlayerTag(id, buf, taille)  <- playertags.cpp (texte a dessiner, false = pseudo)
// ============================================================================
static std::mutex s_cwTagMx;
static int s_cwTagMode = 0;
static int s_cwTags[1024];

void Crestwood_SetPlayerTags(const char* data)
{
	std::lock_guard<std::mutex> lock(s_cwTagMx);
	for (int i = 0; i < 1024; i++) s_cwTags[i] = 0;
	s_cwTagMode = 0;
	if (!data || !*data) return;
	const char* p = data;
	s_cwTagMode = (*p == '1') ? 1 : 0;
	while (*p && *p != '|') p++;
	if (*p == '|') p++;
	while (*p) {
		char* end = nullptr;
		long id = strtol(p, &end, 10);
		if (!end || end == p) break;
		p = end;
		if (*p != ':') { while (*p && *p != ',') p++; if (*p == ',') p++; continue; }
		p++;
		long tag = strtol(p, &end, 10);
		if (end && end != p) p = end;
		if (id >= 0 && id < 1024 && tag > 0 && tag < 100000) s_cwTags[id] = (int)tag;
		while (*p && *p != ',') p++;
		if (*p == ',') p++;
	}
}

bool Crestwood_FormatPlayerTag(int id, char* out, size_t size)
{
	std::lock_guard<std::mutex> lock(s_cwTagMx);
	if (!s_cwTagMode || !out || size < 8) return false;
	int t = (id >= 0 && id < 1024) ? s_cwTags[id] : 0;
	if (t > 0) snprintf(out, size, "(%04d)", t);
	else snprintf(out, size, "(....)");
	return true;
}
