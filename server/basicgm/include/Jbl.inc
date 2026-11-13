// ============================================================
//  ENCEINTE JBL + BARRE D'OBJETS RAPIDES (v14, 31/10/2026)
// ------------------------------------------------------------
//  JBL (inspire du script MTA "Crs JBL") : l'objet "enceinte_jbl" s'achete dans les
//  superettes 24/7. Utiliser l'objet (inventaire, barre rapide ou /jbl) :
//    - la JBL va dans la main (modele 2226, remplace par l'APK) et le panneau s'ouvre ;
//    - "Jouer" un lien (radio / mp3 / flux) : les joueurs autour l'entendent ;
//    - "Poser au sol" / "Reprendre" / "Ranger".
//  Page : assets/jbl/index.html  ("@jbl" 9850 ouvrir / 9851 maj / 9852 fermer), /jblact
//  Commandes : /jbl  /musique <lien>  /posejbl  /prendrejbl  /rangerjbl
//              (aliases du script d'origine : /musica /soltarjbl /pegarjbl /guardarjbl)
//
//  BARRE RAPIDE : des qu'un objet est mis dans un emplacement rapide de l'inventaire,
//  une barre apparait sur le HUD (dialogue "@hotbar" 9840). Toucher un emplacement =
//  utiliser l'objet (/hotbar <1-5>), sans ouvrir l'inventaire.
// ============================================================

#define JBL_DIALOG_OPEN       9850
#define JBL_DIALOG_UPDATE     9851
#define JBL_DIALOG_CLOSE      9852
#define HOT_DIALOG            9840
#define JBL_MODEL             2226
#define JBL_ITEM              "enceinte_jbl"
#define JBL_ATTACH_SLOT       7
#define JBL_RANGE             30.0
#define JBL_PICK_RANGE        2.5
#define JBL_COL               0x33CCFFFF
#define JBL_COL_ERR           0xFF6347FF

new g_JblMode[MAX_PLAYERS];          // 0 rangee, 1 en main, 2 posee au sol
new g_JblObj[MAX_PLAYERS];           // objet au sol
new Float:g_JblPos[MAX_PLAYERS][3];
new g_JblWorld[MAX_PLAYERS];
new g_JblUrl[MAX_PLAYERS][120];
new g_JblHear[MAX_PLAYERS] = {-1, ...};   // proprietaire de la JBL entendue (-1 = aucune)
new g_JblPage[MAX_PLAYERS];
new g_HotPending[MAX_PLAYERS];
new g_JblTimerStarted;
forward Jbl_Tick();

// ------------------------------------------------------------
//  Utilitaires
// ------------------------------------------------------------
stock Jbl_HasItem(playerid) { return Inv_GetQty(playerid, JBL_ITEM) > 0; }

stock Jbl_Source(owner, &Float:x, &Float:y, &Float:z)
{
	if (g_JblMode[owner] == 2) { x = g_JblPos[owner][0]; y = g_JblPos[owner][1]; z = g_JblPos[owner][2]; return 1; }
	if (g_JblMode[owner] == 1) { GetPlayerPos(owner, x, y, z); return 1; }
	return 0;
}

stock Jbl_StopListener(i)
{
	if (g_JblHear[i] != -1) { StopAudioStreamForPlayer(i); g_JblHear[i] = -1; }
	return 1;
}

stock Jbl_StopAllFor(owner)
{
	for (new i = 0, m = GetMaxPlayers(); i < m; i++) if (IsPlayerConnected(i) && g_JblHear[i] == owner) Jbl_StopListener(i);
	return 1;
}

stock Jbl_Json(const in[], out[], outsize)
{
	new j = 0;
	for (new i = 0; in[i] != 0 && j < outsize - 2; i++)
	{
		if (in[i] == '"' || in[i] == '\\') out[j++] = '\\';
		out[j++] = in[i];
	}
	out[j] = 0;
	return 1;
}

stock Jbl_Send(playerid, dialogid, ok = 1, const msg[] = "")
{
	new json[420], url[160], m[100];
	Jbl_Json(g_JblUrl[playerid], url, sizeof url);
	Jbl_Json(msg, m, sizeof m);
	new nearGround = (g_JblMode[playerid] == 2 && IsPlayerInRangeOfPoint(playerid, JBL_PICK_RANGE, g_JblPos[playerid][0], g_JblPos[playerid][1], g_JblPos[playerid][2])) ? 1 : 0;
	format(json, sizeof json, "{\"mode\":%d,\"url\":\"%s\",\"near\":%d,\"has\":%d,\"ok\":%d,\"msg\":\"%s\"}",
		g_JblMode[playerid], url, nearGround, Jbl_HasItem(playerid), ok, m);
	return ShowPlayerDialog(playerid, dialogid, DIALOG_STYLE_MSGBOX, "@jbl", json, "OK", "");
}

stock Jbl_Reply(playerid, ok, const msg[])
{
	if (g_JblPage[playerid]) return Jbl_Send(playerid, JBL_DIALOG_UPDATE, ok, msg);
	return SendClientMessage(playerid, ok ? JBL_COL : JBL_COL_ERR, msg);
}

stock Jbl_OpenPage(playerid)
{
	g_JblPage[playerid] = 1;
	return Jbl_Send(playerid, JBL_DIALOG_OPEN);
}

stock Jbl_StartTimer()
{
	if (g_JblTimerStarted) return 0;
	g_JblTimerStarted = 1;
	SetTimer("Jbl_Tick", 2000, true);
	return 1;
}

// ------------------------------------------------------------
//  Actions
// ------------------------------------------------------------
stock Jbl_TakeInHand(playerid)
{
	if (!Jbl_HasItem(playerid)) return Jbl_Reply(playerid, 0, "Tu n'as pas d'enceinte JBL (en vente dans les superettes 24/7).");
	if (g_JblMode[playerid] == 2)
	{
		if (!IsPlayerInRangeOfPoint(playerid, JBL_PICK_RANGE, g_JblPos[playerid][0], g_JblPos[playerid][1], g_JblPos[playerid][2]))
			return Jbl_Reply(playerid, 0, "Ta JBL est posee ailleurs : va la reprendre.");
		DestroyObject(g_JblObj[playerid]);
		g_JblObj[playerid] = 0;
	}
	SetPlayerAttachedObject(playerid, JBL_ATTACH_SLOT, JBL_MODEL, 6, 0.09, 0.02, 0.0, 0.0, 270.0, 0.0, 1.0, 1.0, 1.0);
	g_JblMode[playerid] = 1;
	Jbl_StartTimer();
	return Jbl_Reply(playerid, 1, "JBL en main.");
}

// Utiliser l'objet (inventaire, barre rapide, /jbl)
stock Jbl_Use(playerid)
{
	// [31/10] JBL retiree (modele APK 2226 : plantages). L'objet n'est plus vendu.
	SendClientMessage(playerid, JBL_COL_ERR, "L'enceinte JBL n'est plus disponible.");
	return 0;
	if (!Jbl_HasItem(playerid)) { SendClientMessage(playerid, JBL_COL_ERR, "Tu n'as pas d'enceinte JBL (en vente dans les superettes 24/7)."); return 0; }
	if (g_JblMode[playerid] == 0) Jbl_TakeInHand(playerid);
	Jbl_OpenPage(playerid);
	return 1;
}

stock Jbl_Play(playerid, const url[])
{
	if (g_JblMode[playerid] == 0) return Jbl_Reply(playerid, 0, "Prends d'abord ta JBL (/jbl).");
	new len = strlen(url);
	if (len < 8 || len > sizeof(g_JblUrl[]) - 1 || (strfind(url, "http://", true) != 0 && strfind(url, "https://", true) != 0))
		return Jbl_Reply(playerid, 0, "Lien invalide : il doit commencer par http:// ou https:// (radio, mp3...).");
	for (new i = 0; i < len; i++) if (url[i] == '"' || url[i] == '\\' || url[i] == ' ') return Jbl_Reply(playerid, 0, "Lien invalide.");
	format(g_JblUrl[playerid], sizeof(g_JblUrl[]), "%s", url);
	Jbl_StopAllFor(playerid);   // tout le monde reprend le nouveau morceau au prochain passage
	Jbl_Tick();
	return Jbl_Reply(playerid, 1, "Musique lancee : les joueurs autour l'entendent.");
}

stock Jbl_Stop(playerid)
{
	g_JblUrl[playerid][0] = 0;
	Jbl_StopAllFor(playerid);
	return Jbl_Reply(playerid, 1, "Musique arretee.");
}

stock Jbl_Drop(playerid)
{
	if (g_JblMode[playerid] != 1) return Jbl_Reply(playerid, 0, "Tu n'as pas la JBL en main.");
	if (IsPlayerInAnyVehicle(playerid)) return Jbl_Reply(playerid, 0, "Descends du vehicule.");
	new Float:x, Float:y, Float:z, Float:a;
	GetPlayerPos(playerid, x, y, z);
	GetPlayerFacingAngle(playerid, a);
	x += 0.8 * floatsin(-a, degrees);
	y += 0.8 * floatcos(-a, degrees);
	z -= 0.95;
	RemovePlayerAttachedObject(playerid, JBL_ATTACH_SLOT);
	g_JblObj[playerid] = CreateObject(JBL_MODEL, x, y, z, 0.0, 0.0, a, 150.0);
	g_JblPos[playerid][0] = x; g_JblPos[playerid][1] = y; g_JblPos[playerid][2] = z;
	g_JblWorld[playerid] = GetPlayerVirtualWorld(playerid);
	g_JblMode[playerid] = 2;
	Jbl_StopAllFor(playerid);   // le son repart depuis la position au sol
	ApplyAnimation(playerid, "BOMBER", "BOM_Plant", 4.1, 0, 0, 0, 0, 0, 1);
	return Jbl_Reply(playerid, 1, "JBL posee au sol.");
}

stock Jbl_Store(playerid, quiet = 0)
{
	if (g_JblMode[playerid] == 2)
	{
		if (!quiet && !IsPlayerInRangeOfPoint(playerid, JBL_PICK_RANGE, g_JblPos[playerid][0], g_JblPos[playerid][1], g_JblPos[playerid][2]))
			return Jbl_Reply(playerid, 0, "Va a cote de ta JBL pour la ranger.");
		if (g_JblObj[playerid]) DestroyObject(g_JblObj[playerid]);
		g_JblObj[playerid] = 0;
	}
	if (g_JblMode[playerid] == 1) RemovePlayerAttachedObject(playerid, JBL_ATTACH_SLOT);
	g_JblMode[playerid] = 0;
	g_JblUrl[playerid][0] = 0;
	Jbl_StopAllFor(playerid);
	if (quiet) return 1;
	return Jbl_Reply(playerid, 1, "JBL rangee dans le sac.");
}

// Toutes les 2 s : qui entend quelle JBL (la plus proche a portee)
public Jbl_Tick()
{
	new m = GetMaxPlayers();
	for (new i = 0; i < m; i++)
	{
		if (!IsPlayerConnected(i)) continue;
		new best = -1, Float:bd = JBL_RANGE + 1.0, Float:px, Float:py, Float:pz;
		GetPlayerPos(i, px, py, pz);
		for (new o = 0; o < m; o++)
		{
			if (!IsPlayerConnected(o) || g_JblMode[o] == 0 || !g_JblUrl[o][0]) continue;
			new Float:x, Float:y, Float:z;
			if (!Jbl_Source(o, x, y, z)) continue;
			new vw = (g_JblMode[o] == 2) ? g_JblWorld[o] : GetPlayerVirtualWorld(o);
			if (vw != GetPlayerVirtualWorld(i)) continue;
			new Float:d = floatsqroot((px - x) * (px - x) + (py - y) * (py - y) + (pz - z) * (pz - z));
			if (d < bd) { bd = d; best = o; }
		}
		if (best == g_JblHear[i]) continue;
		Jbl_StopListener(i);
		if (best == -1) continue;
		new Float:x, Float:y, Float:z;
		Jbl_Source(best, x, y, z);
		// au sol : son 3D a sa position ; en main : son normal (la position bouge avec le joueur)
		if (g_JblMode[best] == 2) PlayAudioStreamForPlayer(i, g_JblUrl[best], x, y, z, JBL_RANGE, 1);
		else PlayAudioStreamForPlayer(i, g_JblUrl[best]);
		g_JblHear[i] = best;
	}
	return 1;
}

stock Jbl_Action(playerid, const params[])
{
	new idx = 0, action[12], rest[128];
	Bank_NextToken(params, idx, action, sizeof action);
	strmid(rest, params, idx, strlen(params), sizeof rest);
	if (!strcmp(action, "close", true)) { g_JblPage[playerid] = 0; return 1; }
	if (!strcmp(action, "take", true)) return Jbl_TakeInHand(playerid);
	if (!strcmp(action, "play", true)) return Jbl_Play(playerid, rest);
	if (!strcmp(action, "stop", true)) return Jbl_Stop(playerid);
	if (!strcmp(action, "drop", true)) return Jbl_Drop(playerid);
	if (!strcmp(action, "store", true)) return Jbl_Store(playerid);
	return 0;
}

// ------------------------------------------------------------
//  Barre d'objets rapides
// ------------------------------------------------------------
stock Hot_Send(playerid)
{
	if (!IsPlayerConnected(playerid) || !g_AuthLogged[playerid]) return 0;
	if (!g_InvSlotLoaded[playerid]) Inv_LoadSlots(playerid);
	new json[300], part[48], any = 0;
	json = "{\"slots\":[";
	for (new k = 0; k < INV_SLOTS; k++)
	{
		new it = g_InvSlot[playerid][k], qty = 0;
		if (it >= 0) { qty = Inv_GetQty(playerid, g_InvItems[it]); any = 1; }
		format(part, sizeof part, "%s[\"%s\",%d]", k ? (",") : (""), it >= 0 ? g_InvItems[it] : (""), qty);
		strcat(json, part);
	}
	format(part, sizeof part, "],\"show\":%d,\"jbl\":%d}", any, g_JblMode[playerid]);
	strcat(json, part);
	return ShowPlayerDialog(playerid, HOT_DIALOG, DIALOG_STYLE_MSGBOX, "@hotbar", json, "OK", "");
}

stock Hot_Use(playerid, slot)
{
	if (!g_AuthLogged[playerid]) return 0;
	if (!g_InvSlotLoaded[playerid]) Inv_LoadSlots(playerid);
	if (slot < 1 || slot > INV_SLOTS) return 0;
	new it = g_InvSlot[playerid][slot - 1];
	if (it < 0) return SendClientMessage(playerid, JBL_COL_ERR, "Emplacement vide : place un objet depuis l'inventaire.");
	new have = Inv_GetQty(playerid, g_InvItems[it]);
	if (have < 1) { Hot_Send(playerid); return SendClientMessage(playerid, JBL_COL_ERR, "Tu n'en as plus."); }
	if (!strcmp(g_InvItems[it], JBL_ITEM)) return Jbl_Use(playerid);
	new msg[96];
	if (Inv_Use(playerid, it, msg, sizeof msg))
	{
		Inv_SetQty(playerid, g_InvItems[it], have - 1);
		SendClientMessage(playerid, 0x33CC33FF, msg);
	}
	else SendClientMessage(playerid, JBL_COL_ERR, msg);
	Hot_Send(playerid);
	return 1;
}

// ------------------------------------------------------------
//  Callbacks
// ------------------------------------------------------------
stock Jbl_OnConnect(playerid)
{
	g_JblMode[playerid] = 0; g_JblObj[playerid] = 0; g_JblUrl[playerid][0] = 0;
	g_JblHear[playerid] = -1; g_JblPage[playerid] = 0;
	Jbl_StartTimer();
	return 1;
}

stock Jbl_OnDisconnect(playerid)
{
	Jbl_Store(playerid, 1);
	g_JblHear[playerid] = -1;
	return 1;
}

stock Jbl_OnSpawn(playerid)
{
	if (g_JblMode[playerid] == 1)
		SetPlayerAttachedObject(playerid, JBL_ATTACH_SLOT, JBL_MODEL, 6, 0.09, 0.02, 0.0, 0.0, 270.0, 0.0, 1.0, 1.0, 1.0);
	SetTimerEx("Hot_SendTimer", 1500, false, "i", playerid);
	return 1;
}

forward Hot_SendTimer(playerid);
public Hot_SendTimer(playerid) { g_HotPending[playerid] = 0; return Hot_Send(playerid); }

// regroupe les changements (achats, utilisation...) : un seul envoi 300 ms apres
stock Hot_SendSoon(playerid)
{
	if (playerid < 0 || playerid >= MAX_PLAYERS || g_HotPending[playerid]) return 0;
	g_HotPending[playerid] = 1;
	SetTimerEx("Hot_SendTimer", 300, false, "i", playerid);
	return 1;
}

stock Jbl_OnCommand(playerid, const cmdtext[])
{
	if (!strcmp(cmdtext, "/jblact ", true, 8)) { Jbl_Action(playerid, cmdtext[8]); return 1; }
	if (!strcmp(cmdtext, "/hotbar ", true, 8)) { Hot_Use(playerid, strval(cmdtext[8])); return 1; }
	return 0;
}
