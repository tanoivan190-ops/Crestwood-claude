// ============================================================
//  POINTS + MAPPINGS + PORTAILS (v13, 31/10/2026)
// ------------------------------------------------------------
//  Panneau admin HTML (assets/mapadmin/index.html), dialogues "@maps" :
//     9830 ouvrir / 9831 mise a jour / 9832 fermer   (contenu JSON)
//  Page -> serveur : /mapact <action> <arguments>
//
//  1) MAPPINGS : liste de tous les mappings charges (nom, objets), bouton
//     "Aller" (teleportation, le joueur reste fige jusqu'a ce que le sol soit charge)
//     et "Arrivee ici" (le point d'arrivee du mapping = ta position, sauvegarde).
//  2) PORTAILS : entree (dehors) <-> sortie (dans le mapping / l'interieur).
//     "Nouveau portail" puis, sur place : "Entree ici" / "Sortie ici".
//     Le portail marche des que les deux sont poses (icones 3D porte + main).
//  3) POINTS : banques, superettes 24/7, pompes a essence, location de
//     vehicules, sieges d'emploi. Plus AUCUN point fixe : tout est pose ici.
//
//  Commandes (admin) : /mappings  (/points, /lieux)
//  Base : scriptfiles/crestwood_points.db (points + arrivees),
//         portails dans crestwood_jobs.db (table map_portals).
// ============================================================

#define PTS_DIALOG_OPEN       9830
#define PTS_DIALOG_UPDATE     9831
#define PTS_DIALOG_CLOSE      9832
#define PTS_ADMIN_LVL         4           // niveau staff minimum (echelle 0-10)
#define PTS_MAX               240
#define PTS_MAPREG_MAX        40
#define PTS_COL_OK            0x33CC33FF
#define PTS_COL_ERR           0xFF6347FF
#define PTS_TP_WAIT_MS        7000        // fige au plus 7 s apres une teleportation (sol pas encore charge)

#define PT_BANK               1
#define PT_SHOP               2
#define PT_PUMP               3
#define PT_RENT               4
#define PT_JOB                5
#define PT_TYPES              5

new DB:g_PtDB;
new g_PtCount;
new g_PtDb[PTS_MAX];
new g_PtType[PTS_MAX];
new g_PtSub[PTS_MAX];
new g_PtName[PTS_MAX][32];
new Float:g_PtPos[PTS_MAX][4];

// mappings charges (registre rempli pendant MapObj_OnInit)
new g_MapRegCount;
new g_MapRegName[PTS_MAPREG_MAX][32];
new g_MapRegObjs[PTS_MAPREG_MAX];
new Float:g_MapRegArr[PTS_MAPREG_MAX][4];
new g_MapRegCustom[PTS_MAPREG_MAX];
new g_MapRegStart = -1;

// sieges d'emploi d'origine (pour garder le decalage du vehicule de service)
new Float:g_PtJobOrig[JOB_COUNT + 1][7];
new g_PtJobOrigDone;

new g_PtsOpen[MAX_PLAYERS];
new g_PtsView[MAX_PLAYERS];        // 0 mappings, 1 portails, 2 points
new g_PtsFilter[MAX_PLAYERS];      // type de point affiche
new g_PtsWaitUntil[MAX_PLAYERS];
new Float:g_PtsWaitPos[MAX_PLAYERS][3];
forward Pts_WaitTick(playerid);

new const g_PtTypeName[PT_TYPES + 1][24] = {"", "Banque", "Superette 24/7", "Pompe a essence", "Location de vehicules", "Siege d'emploi"};

// ------------------------------------------------------------
//  Utilitaires
// ------------------------------------------------------------
stock Pts_Clean(const in[], out[], outsize)
{
	new j = 0;
	for (new i = 0; in[i] != 0 && j < outsize - 1; i++)
	{
		new c = in[i];
		if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == ' ' || c == '-' || c == '/' || c == '.') out[j++] = c;
		else if (c == '_') out[j++] = ' ';
	}
	out[j] = 0;
	while (j > 0 && out[j - 1] == ' ') out[--j] = 0;
	return j;
}

stock Pts_Msg(playerid, ok, const text[]) { return SendClientMessage(playerid, ok ? PTS_COL_OK : PTS_COL_ERR, text); }

stock Pts_IsAdmin(playerid) { return Admin_Level(playerid) >= PTS_ADMIN_LVL; }

// ------------------------------------------------------------
//  Registre des mappings (appele autour de chaque MapImp_* / MapObj_*)
// ------------------------------------------------------------
stock MapReg_Begin(const name[])
{
	if (g_MapRegCount >= PTS_MAPREG_MAX) { g_MapRegStart = -1; return 0; }
	format(g_MapRegName[g_MapRegCount], 32, "%s", name);
	g_MapRegStart = g_MsCount;
	return 1;
}

stock MapReg_End()
{
	if (g_MapRegStart < 0) return 0;
	new a = g_MapRegStart, b = g_MsCount, r = g_MapRegCount;
	g_MapRegStart = -1;
	if (b <= a) return 0;
	new Float:minx = 99999.0, Float:miny = 99999.0, Float:minz = 99999.0, Float:maxx = -99999.0, Float:maxy = -99999.0, Float:maxz = -99999.0;
	for (new i = a; i < b; i++)
	{
		if (g_MsPos[i][0] < minx) minx = g_MsPos[i][0];
		if (g_MsPos[i][0] > maxx) maxx = g_MsPos[i][0];
		if (g_MsPos[i][1] < miny) miny = g_MsPos[i][1];
		if (g_MsPos[i][1] > maxy) maxy = g_MsPos[i][1];
		if (g_MsPos[i][2] < minz) minz = g_MsPos[i][2];
		if (g_MsPos[i][2] > maxz) maxz = g_MsPos[i][2];
	}
	new Float:cx = (minx + maxx) / 2.0, Float:cy = (miny + maxy) / 2.0, Float:z = -99999.0;
	if (minz < -20.0)
	{
		// interieur souterrain : on arrive sur le sol (objet le plus bas pres du centre)
		z = 99999.0;
		for (new i = a; i < b; i++)
		{
			new Float:dx = g_MsPos[i][0] - cx, Float:dy = g_MsPos[i][1] - cy;
			if (dx * dx + dy * dy < 100.0 && g_MsPos[i][2] < z) z = g_MsPos[i][2];
		}
		if (z > 9999.0) z = minz;
		z += 1.2;
	}
	else
	{
		// mapping en plein air : au-dessus de ce qui est au centre (on ne tombe jamais sous la carte)
		for (new i = a; i < b; i++)
		{
			new Float:dx = g_MsPos[i][0] - cx, Float:dy = g_MsPos[i][1] - cy;
			if (dx * dx + dy * dy < 16.0 && g_MsPos[i][2] > z) z = g_MsPos[i][2];
		}
		if (z < -9999.0) z = maxz;
		z += 1.5;
	}
	g_MapRegObjs[r] = b - a;
	g_MapRegArr[r][0] = cx; g_MapRegArr[r][1] = cy; g_MapRegArr[r][2] = z; g_MapRegArr[r][3] = 0.0;
	g_MapRegCustom[r] = 0;
	g_MapRegCount++;
	return 1;
}

// ------------------------------------------------------------
//  Base de donnees
// ------------------------------------------------------------
stock Pts_OpenDB()
{
	if (g_PtDB != DB:0) return 1;
	g_PtDB = db_open("crestwood_points.db");
	if (g_PtDB == DB:0) { print("[POINTS] ERREUR : impossible d'ouvrir crestwood_points.db"); return 0; }
	db_free_result(db_query(g_PtDB, "CREATE TABLE IF NOT EXISTS cw_points (id INTEGER PRIMARY KEY AUTOINCREMENT, type INTEGER NOT NULL, sub INTEGER NOT NULL DEFAULT 0, name TEXT NOT NULL DEFAULT '', x REAL, y REAL, z REAL, a REAL DEFAULT 0)"));
	db_free_result(db_query(g_PtDB, "CREATE TABLE IF NOT EXISTS map_arrivals (name TEXT PRIMARY KEY, x REAL, y REAL, z REAL, a REAL DEFAULT 0)"));
	return 1;
}

// Recharge tous les points et remplit les tableaux des systemes (banque, 24/7, essence, location, emplois)
stock Pts_LoadAll()
{
	if (!g_PtJobOrigDone)
	{
		for (new j = 1; j <= JOB_COUNT; j++)
		{
			g_PtJobOrig[j][0] = g_Jobs[j][jbX]; g_PtJobOrig[j][1] = g_Jobs[j][jbY]; g_PtJobOrig[j][2] = g_Jobs[j][jbZ];
			g_PtJobOrig[j][3] = g_Jobs[j][jbVX]; g_PtJobOrig[j][4] = g_Jobs[j][jbVY]; g_PtJobOrig[j][5] = g_Jobs[j][jbVZ]; g_PtJobOrig[j][6] = g_Jobs[j][jbVA];
		}
		g_PtJobOrigDone = 1;
	}
	g_PtCount = 0;
	g_BankCount = 0; g_ShopCount = 0; g_SvcPumpCount = 0; g_SvcRentCount = 0;
	for (new j = 1; j <= JOB_COUNT; j++) g_JobPlaced[j] = 0;
	if (!Pts_OpenDB()) return 0;
	new DBResult:res = db_query(g_PtDB, "SELECT id, type, sub, name, x, y, z, a FROM cw_points ORDER BY id");
	if (res == DBResult:0) return 0;
	new rows = db_num_rows(res), f[40];
	for (new r = 0; r < rows && g_PtCount < PTS_MAX; r++)
	{
		new p = g_PtCount;
		db_get_field(res, 1, f, sizeof f);
		if (strval(f) < 1 || strval(f) > PT_TYPES) { db_next_row(res); continue; }
		g_PtCount++;
		g_PtType[p] = strval(f);
		db_get_field(res, 0, f, sizeof f); g_PtDb[p] = strval(f);
		db_get_field(res, 2, f, sizeof f); g_PtSub[p] = strval(f);
		db_get_field(res, 3, g_PtName[p], 32);
		for (new k = 0; k < 4; k++) { db_get_field(res, 4 + k, f, sizeof f); g_PtPos[p][k] = floatstr(f); }
		Pts_Apply(p);
		db_next_row(res);
	}
	db_free_result(res);
	return 1;
}

stock Pts_Apply(p)
{
	new Float:x = g_PtPos[p][0], Float:y = g_PtPos[p][1], Float:z = g_PtPos[p][2];
	switch (g_PtType[p])
	{
		case PT_BANK:
		{
			if (g_BankCount >= PT_MAX_BANK) return 0;
			new i = g_BankCount++;
			g_BankPos[i][0] = x; g_BankPos[i][1] = y; g_BankPos[i][2] = z;
		}
		case PT_SHOP:
		{
			if (g_ShopCount >= PT_MAX_SHOP) return 0;
			new i = g_ShopCount++;
			if (g_PtName[p][0]) format(g_Shops[i][shName], 32, "%s", g_PtName[p]);
			else format(g_Shops[i][shName], 32, "Superette 24/7");
			g_Shops[i][shX] = x; g_Shops[i][shY] = y; g_Shops[i][shZ] = z;
		}
		case PT_PUMP:
		{
			if (g_SvcPumpCount >= SVC_MAX_PUMPS) return 0;
			new i = g_SvcPumpCount++;
			g_SvcPumps[i][0] = x; g_SvcPumps[i][1] = y; g_SvcPumps[i][2] = z;
		}
		case PT_RENT:
		{
			if (g_SvcRentCount >= SVC_MAX_RENT) return 0;
			new i = g_SvcRentCount++;
			g_SvcRentPts[i][0] = x; g_SvcRentPts[i][1] = y; g_SvcRentPts[i][2] = z;
		}
		case PT_JOB:
		{
			new j = g_PtSub[p];
			if (j < 1 || j > JOB_COUNT) return 0;
			g_JobPlaced[j] = 1;
			g_Jobs[j][jbX] = x; g_Jobs[j][jbY] = y; g_Jobs[j][jbZ] = z;
			// le vehicule de service garde le meme decalage par rapport au siege
			g_Jobs[j][jbVX] = x + (g_PtJobOrig[j][3] - g_PtJobOrig[j][0]);
			g_Jobs[j][jbVY] = y + (g_PtJobOrig[j][4] - g_PtJobOrig[j][1]);
			g_Jobs[j][jbVZ] = z + (g_PtJobOrig[j][5] - g_PtJobOrig[j][2]);
			g_Jobs[j][jbVA] = g_PtJobOrig[j][6];
		}
	}
	return 1;
}

stock Pts_LoadArrivals()
{
	if (!Pts_OpenDB()) return 0;
	new DBResult:res = db_query(g_PtDB, "SELECT name, x, y, z, a FROM map_arrivals");
	if (res == DBResult:0) return 0;
	new rows = db_num_rows(res), name[40], f[24];
	for (new r = 0; r < rows; r++)
	{
		db_get_field(res, 0, name, sizeof name);
		for (new m = 0; m < g_MapRegCount; m++)
		{
			if (strcmp(name, g_MapRegName[m], true) != 0) continue;
			for (new k = 0; k < 4; k++) { db_get_field(res, 1 + k, f, sizeof f); g_MapRegArr[m][k] = floatstr(f); }
			g_MapRegCustom[m] = 1;
		}
		db_next_row(res);
	}
	db_free_result(res);
	return 1;
}

// ------------------------------------------------------------
//  Teleportation : fige le joueur jusqu'a ce que le mapping autour soit charge
// ------------------------------------------------------------
stock Ms_NearReady(playerid, Float:x, Float:y, Float:z)
{
	if (playerid < 0 || playerid >= MS_PLAYERS || !g_MsBuilt) return 1;
	for (new zn = 0; zn < g_MsZoneCount; zn++)
	{
		new Float:dx = x - g_MsZoneC[zn][0], Float:dy = y - g_MsZoneC[zn][1], Float:dz = z - g_MsZoneC[zn][2];
		new Float:d = floatsqroot(dx * dx + dy * dy + dz * dz) - g_MsZoneR[zn];
		if (d < 25.0 && g_MsPState[playerid][zn] != 2) return 0;
	}
	return 1;
}

stock Pts_WaitStream(playerid, Float:x, Float:y, Float:z)
{
	g_PtsWaitUntil[playerid] = GetTickCount() + PTS_TP_WAIT_MS;
	g_PtsWaitPos[playerid][0] = x; g_PtsWaitPos[playerid][1] = y; g_PtsWaitPos[playerid][2] = z;
	SetTimerEx("Pts_WaitTick", 600, false, "i", playerid);
	return 1;
}

public Pts_WaitTick(playerid)
{
	if (!IsPlayerConnected(playerid)) return 0;
	if (Ms_NearReady(playerid, g_PtsWaitPos[playerid][0], g_PtsWaitPos[playerid][1], g_PtsWaitPos[playerid][2]) || GetTickCount() >= g_PtsWaitUntil[playerid])
	{
		SetPlayerPos(playerid, g_PtsWaitPos[playerid][0], g_PtsWaitPos[playerid][1], g_PtsWaitPos[playerid][2]);   // repose sur le sol charge
		TogglePlayerControllable(playerid, 1);
		return 1;
	}
	SetTimerEx("Pts_WaitTick", 300, false, "i", playerid);
	return 1;
}

stock Pts_Teleport(playerid, Float:x, Float:y, Float:z, Float:a)
{
	if (IsPlayerInAnyVehicle(playerid)) RemovePlayerFromVehicle(playerid);
	Entry_Teleport(playerid, x, y, z, a);
	return 1;
}

// ------------------------------------------------------------
//  Panneau
// ------------------------------------------------------------
stock Pts_JsonStr(const in[], out[], outsize)
{
	new j = 0;
	for (new i = 0; in[i] != 0 && j < outsize - 2; i++)
	{
		if (in[i] == '"' || in[i] == '\\') out[j++] = '\\';
		out[j++] = in[i];
	}
	out[j] = 0;
	return j;
}

stock Pts_Send(playerid, dialogid, ok = 1, const msg[] = "")
{
	new json[3800], part[180], esc[80];
	new view = g_PtsView[playerid];
	Pts_JsonStr(msg, esc, sizeof esc);
	format(json, sizeof json, "{\"view\":%d,\"ok\":%d,\"msg\":\"%s\",\"filter\":%d,\"list\":[", view, ok, esc, g_PtsFilter[playerid]);
	new n = 0;
	if (view == 0)
	{
		for (new m = 0; m < g_MapRegCount; m++)
		{
			Pts_JsonStr(g_MapRegName[m], esc, sizeof esc);
			format(part, sizeof part, "%s[%d,\"%s\",%d,%d]", n ? (",") : (""), m, esc, g_MapRegObjs[m], g_MapRegCustom[m]);
			if (strlen(json) + strlen(part) > sizeof(json) - 400) break;
			strcat(json, part); n++;
		}
	}
	else if (view == 1)
	{
		for (new e = 0; e < g_EntryCount; e++)
		{
			Pts_JsonStr(g_Entries[e][enName], esc, sizeof esc);
			format(part, sizeof part, "%s[%d,\"%s\",%d,%d]", n ? (",") : (""), g_Entries[e][enDb], esc, Entry_HasOut(e), Entry_HasIn(e));
			if (strlen(json) + strlen(part) > sizeof(json) - 400) break;
			strcat(json, part); n++;
		}
	}
	else
	{
		for (new p = 0; p < g_PtCount; p++)
		{
			if (g_PtsFilter[playerid] && g_PtType[p] != g_PtsFilter[playerid]) continue;
			new label[40];
			if (g_PtType[p] == PT_JOB && g_PtSub[p] >= 1 && g_PtSub[p] <= JOB_COUNT) format(label, sizeof label, "%s", g_Jobs[g_PtSub[p]][jbName]);
			else if (g_PtName[p][0]) format(label, sizeof label, "%s", g_PtName[p]);
			else format(label, sizeof label, "%s", g_PtTypeName[g_PtType[p]]);
			Pts_JsonStr(label, esc, sizeof esc);
			format(part, sizeof part, "%s[%d,%d,\"%s\",%.0f,%.0f]", n ? (",") : (""), g_PtDb[p], g_PtType[p], esc, g_PtPos[p][0], g_PtPos[p][1]);
			if (strlen(json) + strlen(part) > sizeof(json) - 400) break;
			strcat(json, part); n++;
		}
	}
	strcat(json, "],\"jobs\":[");
	for (new j = 1; j <= JOB_COUNT; j++)
	{
		format(part, sizeof part, "%s[%d,\"%s\",%d]", (j > 1) ? (",") : (""), j, g_Jobs[j][jbName], g_JobPlaced[j]);
		strcat(json, part);
	}
	format(part, sizeof part, "],\"counts\":[%d,%d,%d,%d,%d,%d]}", g_PtCount, g_BankCount, g_ShopCount, g_SvcPumpCount, g_SvcRentCount, Pts_JobCount());
	strcat(json, part);
	return ShowPlayerDialog(playerid, dialogid, DIALOG_STYLE_MSGBOX, "@maps", json, "OK", "");
}

stock Pts_JobCount()
{
	new n = 0;
	for (new j = 1; j <= JOB_COUNT; j++) if (g_JobPlaced[j]) n++;
	return n;
}

stock Pts_Open(playerid)
{
	if (!Pts_IsAdmin(playerid)) { Pts_Msg(playerid, 0, "Reserve aux administrateurs."); return 0; }
	g_PtsOpen[playerid] = 1;
	return Pts_Send(playerid, PTS_DIALOG_OPEN);
}

stock Pts_Refresh(playerid, ok, const msg[])
{
	if (!g_PtsOpen[playerid]) return Pts_Msg(playerid, ok, msg);
	return Pts_Send(playerid, PTS_DIALOG_UPDATE, ok, msg);
}

stock Pts_FindPortal(db)
{
	for (new e = 0; e < g_EntryCount; e++) if (g_Entries[e][enDb] == db) return e;
	return -1;
}

stock Pts_FindPoint(db)
{
	for (new p = 0; p < g_PtCount; p++) if (g_PtDb[p] == db) return p;
	return -1;
}

stock Pts_ReloadPoints()
{
	Pts_LoadAll();
	Marks_SendAll();
	return 1;
}

stock Pts_ReloadPortals()
{
	for (new e = 0; e < g_EntryCount; e++) Entry_Destroy(e);
	Entry_LoadDB();
	for (new e = 0; e < g_EntryCount; e++) Entry_Build(e);
	Marks_SendAll();
	return 1;
}

stock Pts_Action(playerid, const params[])
{
	if (!Pts_IsAdmin(playerid)) return 0;
	new idx = 0, action[12], a1[16], a2[16], rest[64], q[300], m[128], name[34];
	Bank_NextToken(params, idx, action, sizeof action);
	if (!strcmp(action, "close", true)) { g_PtsOpen[playerid] = 0; return 1; }
	new Float:x, Float:y, Float:z, Float:a;
	GetPlayerPos(playerid, x, y, z);
	GetPlayerFacingAngle(playerid, a);

	if (!strcmp(action, "view", true))
	{
		Bank_NextToken(params, idx, a1, sizeof a1);
		Bank_NextToken(params, idx, a2, sizeof a2);
		g_PtsView[playerid] = strval(a1);
		if (g_PtsView[playerid] < 0 || g_PtsView[playerid] > 2) g_PtsView[playerid] = 0;
		g_PtsFilter[playerid] = strval(a2);
		if (g_PtsFilter[playerid] < 0 || g_PtsFilter[playerid] > PT_TYPES) g_PtsFilter[playerid] = 0;
		return Pts_Send(playerid, PTS_DIALOG_UPDATE);
	}
	// ---- mappings
	if (!strcmp(action, "maptp", true))
	{
		Bank_NextToken(params, idx, a1, sizeof a1);
		new mi = strval(a1);
		if (mi < 0 || mi >= g_MapRegCount) return Pts_Refresh(playerid, 0, "Mapping inconnu.");
		Pts_Teleport(playerid, g_MapRegArr[mi][0], g_MapRegArr[mi][1], g_MapRegArr[mi][2], g_MapRegArr[mi][3]);
		format(m, sizeof m, "Teleporte : %s (le sol se charge, ne bouge pas).", g_MapRegName[mi]);
		return Pts_Refresh(playerid, 1, m);
	}
	if (!strcmp(action, "maparr", true))
	{
		Bank_NextToken(params, idx, a1, sizeof a1);
		new mi = strval(a1);
		if (mi < 0 || mi >= g_MapRegCount) return Pts_Refresh(playerid, 0, "Mapping inconnu.");
		g_MapRegArr[mi][0] = x; g_MapRegArr[mi][1] = y; g_MapRegArr[mi][2] = z; g_MapRegArr[mi][3] = a;
		g_MapRegCustom[mi] = 1;
		format(q, sizeof q, "INSERT OR REPLACE INTO map_arrivals (name, x, y, z, a) VALUES ('%s', %f, %f, %f, %f)", g_MapRegName[mi], x, y, z, a);
		db_free_result(db_query(g_PtDB, q));
		format(m, sizeof m, "Arrivee de \"%s\" = ta position.", g_MapRegName[mi]);
		return Pts_Refresh(playerid, 1, m);
	}
	// ---- portails
	if (!strcmp(action, "pnew", true))
	{
		strmid(rest, params, idx, strlen(params), sizeof rest);
		Pts_Clean(rest, name, 32);
		if (!name[0]) format(name, sizeof name, "Portail %d", g_EntryCount + 1);
		if (g_EntryCount >= PORTAL_MAX) return Pts_Refresh(playerid, 0, "Trop de portails.");
		format(q, sizeof q, "INSERT INTO map_portals (name, ox, oy, oz) VALUES ('%s', %f, %f, %f)", name, x, y, z - 0.6);
		db_free_result(db_query(g_JobDB, q));
		Pts_ReloadPortals();
		format(m, sizeof m, "Portail \"%s\" cree : ENTREE posee ici. Va dans le mapping et touche \"Sortie ici\".", name);
		return Pts_Refresh(playerid, 1, m);
	}
	if (!strcmp(action, "pout", true) || !strcmp(action, "pin", true) || !strcmp(action, "ptpout", true) || !strcmp(action, "ptpin", true) || !strcmp(action, "pdel", true))
	{
		Bank_NextToken(params, idx, a1, sizeof a1);
		new db = strval(a1), e = Pts_FindPortal(db);
		if (e == -1) return Pts_Refresh(playerid, 0, "Portail introuvable.");
		if (!strcmp(action, "pout", true))
		{
			format(q, sizeof q, "UPDATE map_portals SET ox = %f, oy = %f, oz = %f WHERE id = %d", x, y, z - 0.6, db);
			db_free_result(db_query(g_JobDB, q));
			Pts_ReloadPortals();
			return Pts_Refresh(playerid, 1, "Entree du portail posee ici.");
		}
		if (!strcmp(action, "pin", true))
		{
			format(q, sizeof q, "UPDATE map_portals SET ix = %f, iy = %f, iz = %f, ia = %f WHERE id = %d", x, y, z - 0.6, a, db);
			db_free_result(db_query(g_JobDB, q));
			Pts_ReloadPortals();
			return Pts_Refresh(playerid, 1, "Sortie du portail posee ici (on arrive ici en entrant).");
		}
		if (!strcmp(action, "ptpout", true))
		{
			if (!Entry_HasOut(e)) return Pts_Refresh(playerid, 0, "Entree pas encore posee.");
			Pts_Teleport(playerid, g_Entries[e][enOX], g_Entries[e][enOY], g_Entries[e][enOZ] + 0.6, 0.0);
			return Pts_Refresh(playerid, 1, "Teleporte a l'entree.");
		}
		if (!strcmp(action, "ptpin", true))
		{
			if (!Entry_HasIn(e)) return Pts_Refresh(playerid, 0, "Sortie pas encore posee.");
			Pts_Teleport(playerid, g_Entries[e][enIX], g_Entries[e][enIY], g_Entries[e][enIZ] + 0.6, g_Entries[e][enIA]);
			return Pts_Refresh(playerid, 1, "Teleporte a la sortie.");
		}
		format(q, sizeof q, "DELETE FROM map_portals WHERE id = %d", db);
		db_free_result(db_query(g_JobDB, q));
		Pts_ReloadPortals();
		return Pts_Refresh(playerid, 1, "Portail supprime.");
	}
	// ---- points
	if (!strcmp(action, "ptnew", true))
	{
		Bank_NextToken(params, idx, a1, sizeof a1);
		Bank_NextToken(params, idx, a2, sizeof a2);
		strmid(rest, params, idx, strlen(params), sizeof rest);
		new type = strval(a1), sub = strval(a2);
		if (type < 1 || type > PT_TYPES) return Pts_Refresh(playerid, 0, "Type de point inconnu.");
		if (type == PT_JOB)
		{
			if (sub < 1 || sub > JOB_COUNT) return Pts_Refresh(playerid, 0, "Choisis le metier.");
			format(q, sizeof q, "DELETE FROM cw_points WHERE type = %d AND sub = %d", PT_JOB, sub);   // un seul siege par metier
			db_free_result(db_query(g_PtDB, q));
		}
		else sub = 0;
		Pts_Clean(rest, name, 32);
		format(q, sizeof q, "INSERT INTO cw_points (type, sub, name, x, y, z, a) VALUES (%d, %d, '%s', %f, %f, %f, %f)", type, sub, name, x, y, z, a);
		db_free_result(db_query(g_PtDB, q));
		Pts_ReloadPoints();
		g_PtsView[playerid] = 2; g_PtsFilter[playerid] = type;
		format(m, sizeof m, "%s pose(e) ici.", g_PtTypeName[type]);
		return Pts_Refresh(playerid, 1, m);
	}
	if (!strcmp(action, "ptdel", true) || !strcmp(action, "pttp", true))
	{
		Bank_NextToken(params, idx, a1, sizeof a1);
		new db = strval(a1), p = Pts_FindPoint(db);
		if (p == -1) return Pts_Refresh(playerid, 0, "Point introuvable.");
		if (!strcmp(action, "pttp", true))
		{
			Pts_Teleport(playerid, g_PtPos[p][0] + 1.0, g_PtPos[p][1], g_PtPos[p][2] + 0.5, g_PtPos[p][3]);
			return Pts_Refresh(playerid, 1, "Teleporte au point.");
		}
		format(q, sizeof q, "DELETE FROM cw_points WHERE id = %d", db);
		db_free_result(db_query(g_PtDB, q));
		Pts_ReloadPoints();
		return Pts_Refresh(playerid, 1, "Point supprime.");
	}
	return 0;
}

// ------------------------------------------------------------
//  Callbacks
// ------------------------------------------------------------
stock Pts_OnConnect(playerid)
{
	g_PtsOpen[playerid] = 0; g_PtsView[playerid] = 0; g_PtsFilter[playerid] = 0;
	return 1;
}

stock Pts_OnCommand(playerid, const cmdtext[])
{
	if (!strcmp(cmdtext, "/mapact ", true, 8)) { Pts_Action(playerid, cmdtext[8]); return 1; }
	if (!strcmp(cmdtext, "/mappings", true) || !strcmp(cmdtext, "/points", true) || !strcmp(cmdtext, "/lieux", true))
	{
		if (!Pts_IsAdmin(playerid)) return 0;
		if (!strcmp(cmdtext, "/points", true)) { g_PtsView[playerid] = 2; g_PtsFilter[playerid] = 0; }
		else g_PtsView[playerid] = 0;
		Pts_Open(playerid);
		return 1;
	}
	return 0;
}
