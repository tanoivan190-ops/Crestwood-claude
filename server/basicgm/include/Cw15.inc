// ============================================================
//  CRESTWOOD RP - v15 (31/10/2026)
// ------------------------------------------------------------
//  1) ENCHERES DES MAISONS (24 h) : 500 000 -> 50 000 000 FCFA
//     - Une maison a vendre part aux encheres au premier encherisseur (page Immobilier, bouton
//       "Encherir", ou /encherir <maison> <montant>). L'enchere dure 24 h a partir de la 1re offre.
//     - Chaque nouvelle offre doit depasser la precedente d'au moins 5 % (min. 10 000 FCFA).
//     - L'argent est bloque (preleve sur le compte bancaire) ; l'ancien meilleur encherisseur est
//       rembourse tout de suite, meme hors ligne. Offre dans les 5 dernieres minutes = +5 min.
//     - 50 000 000 FCFA = achat immediat. A la fin, la maison est au gagnant (meme hors ligne).
//     - /encheres : liste des encheres en cours.  Admin : bouton "Annuler" (panneau, onglet Monde).
//  2) ZONES VERTES : zones publiques protegees (spawn, mairie, banque, hopitaux...). Armes rangees,
//     degats annules, bandeau "ZONE VERTE" sur le HUD. Admin : panneau onglet Monde / commandes.
//  3) MODE INVISIBLE (staff) : pseudo + point de la carte caches, personnage cache chez les joueurs
//     (client v15, dialogue "@hide" 9517). Les autres membres du staff le voient toujours.
//  4) STATIONS-SERVICE depuis le panneau admin (pompes = points "Pompe a essence" du panneau /points).
//  5) RP : blesse / inconscient au lieu de mourir (/reanimer pour les medecins, /hopital apres 60 s),
//     ceinture de securite (/ceinture, icone du compteur), /de et /pileface, prime de presence
//     horaire, heure du jeu = heure reelle (Abidjan).
//  6) HUD : carburant reel, zone verte et ceinture envoyes au compteur ("@hudv" 9516).
//  Donnees : scriptfiles/crestwood_v15.db (zones vertes) ; encheres dans la table houses.
// ============================================================

#define CW_HUD_DIALOG          9516
#define CW_HIDE_DIALOG         9517
#define CW_COL_OK              0x33CC33FF
#define CW_COL_ERR             0xFF6347FF
#define CW_COL_INFO            0x33CCFFFF
#define CW_COL_AUC             0xF5C451FF
#define CW_COL_GZ              0x3DDC97FF
#define CW_COL_ME              0xC2A2DAFF
#define CW_CHAT_DIST           18.0

// ---- encheres
#define AUC_MIN                500000
#define AUC_MAX                50000000
#define AUC_SECONDS            86400
#define AUC_STEP_PCT           5
#define AUC_STEP_MIN           10000
#define AUC_SNIPE              300

// ---- zones vertes
#define CW_GZ_MAX              32
#define CW_GZ_TEAM             200          // meme equipe = pas de degats entre joueurs (SA-MP)

// ---- RP
#define CW_INJ_SECONDS         150          // inconscient : transport automatique a l'hopital apres
#define CW_INJ_GIVEUP          60           // /hopital possible apres
#define CW_PAY_MINUTES         60
#define CW_PAY_AMOUNT          2500

new DB:g_CwDB;
new g_CwTickN;

// encheres : g_HouseAucBid / g_HouseAucBidder / g_HouseAucEnd sont declares dans basicgm1.pwn (module IMMOBILIER)

// zones vertes
new g_CwGzCount;
new g_CwGzId[CW_GZ_MAX];
new g_CwGzName[CW_GZ_MAX][32];
new Float:g_CwGzPos[CW_GZ_MAX][3];         // x, y, rayon
new Float:g_CwGzZ[CW_GZ_MAX];              // hauteur du sol (0 = inconnue)
new g_AucReady;
new g_CwGzZone[CW_GZ_MAX];
new g_CwInGz[MAX_PLAYERS];                 // index de zone + 1 (0 = dehors)
new g_CwGzWarn[MAX_PLAYERS];

// invisibilite
new g_CwInvis[MAX_PLAYERS];
new g_CwHideVer;
new g_CwHideSent[MAX_PLAYERS];
new g_CwHideLast[MAX_PLAYERS][96];        // derniere liste envoyee (rien n'est envoye tant qu'elle ne change pas)

// HUD
new g_CwHudFuel[MAX_PLAYERS];
new g_CwHudGz[MAX_PLAYERS];
new g_CwHudBelt[MAX_PLAYERS];
new g_CwHudVeh[MAX_PLAYERS];

// RP
new g_CwInjured[MAX_PLAYERS];              // 0 non, 1 en attente du spawn, 2 inconscient
new g_CwInjSince[MAX_PLAYERS];
new g_CwInjNoComa[MAX_PLAYERS];
new Float:g_CwInjPos[MAX_PLAYERS][4];
new g_CwInjVw[MAX_PLAYERS];
new g_CwInjInt[MAX_PLAYERS];
new g_CwBelt[MAX_PLAYERS];
new Float:g_CwVehHp[MAX_PLAYERS];
new Float:g_CwVehKmh[MAX_PLAYERS];
new g_CwPlayMin[MAX_PLAYERS];
new g_CwRealTime = 1;

forward Cw_Tick();
forward Cw_MinuteTick();
forward Cw_InjApply(playerid);

// ------------------------------------------------------------
//  Utilitaires
// ------------------------------------------------------------
stock Cw_Fmt(n, out[], size)
{
	// 1234567 -> "1 234 567"
	new tmp[16], len, j = 0;
	format(tmp, sizeof tmp, "%d", n < 0 ? -n : n);
	len = strlen(tmp);
	if (n < 0 && j < size - 1) out[j++] = '-';
	for (new i = 0; i < len && j < size - 1; i++)
	{
		out[j++] = tmp[i];
		if ((len - i - 1) % 3 == 0 && i != len - 1 && j < size - 1) out[j++] = ' ';
	}
	out[j] = 0;
	return 1;
}

stock Cw_PlayerByName(const name[])
{
	new n[MAX_PLAYER_NAME + 1];
	for (new i = 0, m = GetMaxPlayers(); i < m; i++)
	{
		if (!IsPlayerConnected(i)) continue;
		GetPlayerName(i, n, sizeof n);
		if (!strcmp(n, name, true)) return i;
	}
	return INVALID_PLAYER_ID;
}

// credite un compte bancaire, joueur connecte ou non
stock Cw_CreditByName(const name[], amount, const descr[])
{
	if (!name[0] || amount <= 0) return 0;
	new p = Cw_PlayerByName(name), q[256], date[24], h, mi, s, y, mo, d;
	if (p != INVALID_PLAYER_ID && g_BankLoaded[p])
	{
		g_BankBalance[p] += amount;
		Bank_SavePlayer(p);
		Bank_AddHistory(p, descr, amount);
		return 1;
	}
	format(q, sizeof q, "UPDATE bank_accounts SET balance = balance + %d WHERE name = '%s'", amount, name);
	db_free_result(db_query(g_BankDB, q));
	gettime(h, mi, s);
	getdate(y, mo, d);
	format(date, sizeof date, "%02d/%02d %02d:%02d", d, mo, h, mi);
	format(q, sizeof q, "INSERT INTO bank_history (name, descr, amount, date) VALUES ('%s', '%s', %d, '%s')", name, descr, amount, date);
	db_free_result(db_query(g_BankDB, q));
	return 1;
}

stock Cw_IsStaff(playerid) { return Admin_Level(playerid) >= 1; }

// ------------------------------------------------------------
//  Init / connexion
// ------------------------------------------------------------
stock Cw_OnInit()
{
	g_CwDB = db_open("crestwood_v15.db");
	if (g_CwDB == DB:0) print("[V15] ERREUR : impossible d'ouvrir crestwood_v15.db");
	else
	{
		db_free_result(db_query(g_CwDB, "CREATE TABLE IF NOT EXISTS greenzones (id INTEGER PRIMARY KEY AUTOINCREMENT, name TEXT, x REAL, y REAL, r REAL, z REAL DEFAULT 0)"));
		db_free_result(db_query(g_CwDB, "CREATE TABLE IF NOT EXISTS meta (k TEXT PRIMARY KEY, v INTEGER)"));
		new DBResult:res = db_query(g_CwDB, "SELECT v FROM meta WHERE k = 'gz_seeded'");
		new seeded = (res != DBResult:0 && db_num_rows(res) > 0);
		if (res != DBResult:0) db_free_result(res);
		if (!seeded)
		{
			// zones publiques par defaut (modifiables dans le panneau admin, onglet Monde)
			db_free_result(db_query(g_CwDB, "INSERT INTO greenzones (name, x, y, r) VALUES ('Place du spawn', 1912.0, -1292.0, 55.0)"));
			db_free_result(db_query(g_CwDB, "INSERT INTO greenzones (name, x, y, r) VALUES ('Mairie de Los Santos', 1481.0, -1771.0, 60.0)"));
			db_free_result(db_query(g_CwDB, "INSERT INTO greenzones (name, x, y, r) VALUES ('Banque de Los Santos', 1461.0, -1024.0, 40.0)"));
			db_free_result(db_query(g_CwDB, "INSERT INTO greenzones (name, x, y, r) VALUES ('Hopital All Saints', 1178.0, -1323.0, 45.0)"));
			db_free_result(db_query(g_CwDB, "INSERT INTO greenzones (name, x, y, r) VALUES ('Hopital County', 2034.0, -1405.0, 45.0)"));
			db_free_result(db_query(g_CwDB, "INSERT OR REPLACE INTO meta (k, v) VALUES ('gz_seeded', 1)"));
		}
		res = db_query(g_CwDB, "SELECT v FROM meta WHERE k = 'realtime'");
		if (res != DBResult:0)
		{
			if (db_num_rows(res) > 0) { new f[8]; db_get_field(res, 0, f, sizeof f); g_CwRealTime = strval(f); }
			db_free_result(res);
		}
	}
	Gz_Load();
	Auc_InitDB();
	SetTimer("Cw_Tick", 1000, true);
	SetTimer("Cw_MinuteTick", 60000, true);
	Cw_MinuteTick();
	printf("[V15] %d zone(s) verte(s), encheres pretes, heure reelle %s.", g_CwGzCount, g_CwRealTime ? ("ON") : ("OFF"));
	return 1;
}

stock Cw_OnConnect(playerid)
{
	g_CwInGz[playerid] = 0; g_CwGzWarn[playerid] = 0;
	g_CwInvis[playerid] = 0; g_CwHideSent[playerid] = -1; g_CwHideLast[playerid][0] = 0;
	g_CwHudFuel[playerid] = -1; g_CwHudGz[playerid] = -1; g_CwHudBelt[playerid] = -1; g_CwHudVeh[playerid] = -1;
	g_CwInjured[playerid] = 0; g_CwInjNoComa[playerid] = 0;
	g_CwBelt[playerid] = 0; g_CwVehHp[playerid] = 0.0; g_CwVehKmh[playerid] = 0.0;
	g_CwPlayMin[playerid] = 0;
	for (new z = 0; z < g_CwGzCount; z++) GangZoneShowForPlayer(playerid, g_CwGzZone[z], 0x3DDC9744);
	return 1;
}

stock Cw_OnDisconnect(playerid)
{
	if (g_CwInvis[playerid]) { g_CwInvis[playerid] = 0; g_CwHideVer++; }
	g_CwInjured[playerid] = 0;
	return 1;
}

// ------------------------------------------------------------
//  HUD (compteur : carburant, ceinture ; bandeau zone verte)
// ------------------------------------------------------------
stock Cw_HudPush(playerid, force = 0)
{
	new v = GetPlayerVehicleID(playerid), fuel = 100, gz = g_CwInGz[playerid] ? 1 : 0, belt = (v && g_CwBelt[playerid]) ? 1 : 0;
	if (v > 0 && v <= MAX_VEHICLES) fuel = floatround(g_CwFuel[v], floatround_ceil);
	if (fuel < 0) fuel = 0;
	if (fuel > 100) fuel = 100;
	if (!force && fuel == g_CwHudFuel[playerid] && gz == g_CwHudGz[playerid] && belt == g_CwHudBelt[playerid] && (v ? 1 : 0) == g_CwHudVeh[playerid]) return 0;
	new json[160], name[40];
	if (gz) { format(name, sizeof name, "%s", g_CwGzName[g_CwInGz[playerid] - 1]); }
	format(json, sizeof json, "{\"fuel\":%d,\"gz\":%d,\"gzn\":\"%s\",\"belt\":%d,\"veh\":%d}", fuel, gz, name, belt, v ? 1 : 0);
	g_CwHudFuel[playerid] = fuel; g_CwHudGz[playerid] = gz; g_CwHudBelt[playerid] = belt; g_CwHudVeh[playerid] = v ? 1 : 0;
	return ShowPlayerDialog(playerid, CW_HUD_DIALOG, DIALOG_STYLE_MSGBOX, "@hudv", json, "OK", "");
}

// ------------------------------------------------------------
//  Zones vertes
// ------------------------------------------------------------
stock Gz_Load()
{
	for (new z = 0; z < g_CwGzCount; z++) GangZoneDestroy(g_CwGzZone[z]);
	g_CwGzCount = 0;
	if (g_CwDB == DB:0) return 0;
	new DBResult:res = db_query(g_CwDB, "SELECT id, name, x, y, r, z FROM greenzones ORDER BY id");
	if (res == DBResult:0) return 0;
	new rows = db_num_rows(res), f[40];
	for (new r = 0; r < rows && g_CwGzCount < CW_GZ_MAX; r++)
	{
		new z = g_CwGzCount++;
		db_get_field(res, 0, f, sizeof f); g_CwGzId[z] = strval(f);
		db_get_field(res, 1, g_CwGzName[z], 32);
		db_get_field(res, 2, f, sizeof f); g_CwGzPos[z][0] = floatstr(f);
		db_get_field(res, 3, f, sizeof f); g_CwGzPos[z][1] = floatstr(f);
		db_get_field(res, 4, f, sizeof f); g_CwGzPos[z][2] = floatstr(f);
		db_get_field(res, 5, f, sizeof f); g_CwGzZ[z] = floatstr(f);
		new Float:r0 = g_CwGzPos[z][2];
		g_CwGzZone[z] = GangZoneCreate(g_CwGzPos[z][0] - r0, g_CwGzPos[z][1] - r0, g_CwGzPos[z][0] + r0, g_CwGzPos[z][1] + r0);
		db_next_row(res);
	}
	db_free_result(res);
	for (new i = 0, m = GetMaxPlayers(); i < m; i++)
	{
		if (!IsPlayerConnected(i)) continue;
		for (new z = 0; z < g_CwGzCount; z++) GangZoneShowForPlayer(i, g_CwGzZone[z], 0x3DDC9744);
		g_CwInGz[i] = 0;
		SetPlayerTeam(i, NO_TEAM);
	}
	return 1;
}

stock Gz_At(playerid)
{
	if (GetPlayerVirtualWorld(playerid) != 0 || GetPlayerInterior(playerid) != 0) return -1;
	new Float:x, Float:y, Float:z;
	GetPlayerPos(playerid, x, y, z);
	for (new i = 0; i < g_CwGzCount; i++)
	{
		new Float:dx = x - g_CwGzPos[i][0], Float:dy = y - g_CwGzPos[i][1];
		if (dx * dx + dy * dy <= g_CwGzPos[i][2] * g_CwGzPos[i][2]) return i;
	}
	return -1;
}

stock Gz_Exempt(playerid)
{
	new t = Fac_Type(playerid);
	return (t == FACTION_POLICE || t == FACTION_FEDERAL || t == FACTION_ARMY);
}

stock Gz_Update(playerid)
{
	new z = Gz_At(playerid), msg[128];
	if (z >= 0)
	{
		if (g_CwInGz[playerid] != z + 1)
		{
			if (!g_CwInGz[playerid])
			{
				format(msg, sizeof msg, "Zone verte : %s. Armes rangees, aucune violence ici.", g_CwGzName[z]);
				SendClientMessage(playerid, CW_COL_GZ, msg);
			}
			g_CwInGz[playerid] = z + 1;
			SetPlayerTeam(playerid, CW_GZ_TEAM);
		}
		if (!Gz_Exempt(playerid) && GetPlayerWeapon(playerid) != 0) SetPlayerArmedWeapon(playerid, 0);
	}
	else if (g_CwInGz[playerid])
	{
		g_CwInGz[playerid] = 0;
		SetPlayerTeam(playerid, NO_TEAM);
		SendClientMessage(playerid, 0xAAAAAAFF, "Tu quittes la zone verte.");
	}
	return 1;
}

stock Gz_Add(playerid, Float:radius, const rawname[])
{
	new name[32], q[200];
	Pts_Clean(rawname, name, sizeof name);
	if (!name[0]) format(name, sizeof name, "Zone %d", g_CwGzCount + 1);
	if (radius < 10.0) radius = 10.0;
	if (radius > 250.0) radius = 250.0;
	if (g_CwGzCount >= CW_GZ_MAX || g_CwDB == DB:0) return 0;
	new Float:x, Float:y, Float:z;
	GetPlayerPos(playerid, x, y, z);
	format(q, sizeof q, "INSERT INTO greenzones (name, x, y, r, z) VALUES ('%s', %f, %f, %f, %f)", name, x, y, radius, z);
	db_free_result(db_query(g_CwDB, q));
	Gz_Load();
	return 1;
}

stock Gz_Del(id)
{
	new q[80], found = 0;
	for (new z = 0; z < g_CwGzCount; z++) if (g_CwGzId[z] == id) found = 1;
	if (!found || g_CwDB == DB:0) return 0;
	format(q, sizeof q, "DELETE FROM greenzones WHERE id = %d", id);
	db_free_result(db_query(g_CwDB, q));
	Gz_Load();
	return 1;
}

public OnPlayerTakeDamage(playerid, issuerid, Float:amount, weaponid, bodypart)
{
	// zone verte : les degats d'un joueur sont annules
	if (issuerid != INVALID_PLAYER_ID && (g_CwInGz[playerid] || (IsPlayerConnected(issuerid) && g_CwInGz[issuerid])))
	{
		new Float:hp;
		GetPlayerHealth(playerid, hp);
		hp += amount;
		if (hp > 100.0) hp = 100.0;
		SetPlayerHealth(playerid, hp);
		if (IsPlayerConnected(issuerid) && gettime() - g_CwGzWarn[issuerid] > 4)
		{
			g_CwGzWarn[issuerid] = gettime();
			SendClientMessage(issuerid, CW_COL_ERR, "Zone verte : impossible de blesser quelqu'un ici.");
		}
	}
	return 1;
}

// ------------------------------------------------------------
//  Invisibilite du staff
// ------------------------------------------------------------
stock Invis_Set(playerid, on)
{
	g_CwInvis[playerid] = on ? 1 : 0;
	for (new i = 0, m = GetMaxPlayers(); i < m; i++)
	{
		if (!IsPlayerConnected(i) || i == playerid) continue;
		new see = !on || Cw_IsStaff(i);
		ShowPlayerNameTagForPlayer(i, playerid, see);
		SetPlayerMarkerForPlayer(i, playerid, see ? GetPlayerColor(playerid) : (GetPlayerColor(playerid) & 0xFFFFFF00));
	}
	g_CwHideVer++;
	Invis_SyncAll();
	return 1;
}

stock Invis_List(forplayer, out[], size)
{
	out[0] = 0;
	if (Cw_IsStaff(forplayer)) return 0;     // le staff voit les admins invisibles
	new part[8], n = 0;
	for (new i = 0, m = GetMaxPlayers(); i < m; i++)
	{
		if (!IsPlayerConnected(i) || !g_CwInvis[i] || i == forplayer) continue;
		format(part, sizeof part, "%s%d", n ? (",") : (""), i);
		strcat(out, part, size);
		n++;
	}
	return n;
}

stock Invis_SyncAll()
{
	new list[160];
	for (new i = 0, m = GetMaxPlayers(); i < m; i++)
	{
		if (!IsPlayerConnected(i) || !g_AuthLogged[i] || g_CwHideSent[i] == g_CwHideVer) continue;
		g_CwHideSent[i] = g_CwHideVer;
		Invis_List(i, list, sizeof list);
		if (strcmp(list, g_CwHideLast[i]) || (list[0] == 0) != (g_CwHideLast[i][0] == 0))
		{
			format(g_CwHideLast[i], 96, "%s", list);
			ShowPlayerDialog(i, CW_HIDE_DIALOG, DIALOG_STYLE_MSGBOX, "@hide", list[0] ? list : (" "), "OK", "");
		}
		// joueurs arrives apres l'activation : pseudo / point caches aussi
		for (new a = 0; a < m; a++)
		{
			if (a == i || !IsPlayerConnected(a) || !g_CwInvis[a] || Cw_IsStaff(i)) continue;
			ShowPlayerNameTagForPlayer(i, a, false);
			SetPlayerMarkerForPlayer(i, a, GetPlayerColor(a) & 0xFFFFFF00);
		}
	}
	return 1;
}

// ------------------------------------------------------------
//  Encheres des maisons
// ------------------------------------------------------------
stock Auc_InitDB()
{
	if (g_GarDB == DB:0) return 0;
	new DBResult:res = db_query(g_GarDB, "PRAGMA table_info(houses)"), has = 0, f[32];
	if (res != DBResult:0)
	{
		new rows = db_num_rows(res);
		for (new r = 0; r < rows; r++)
		{
			db_get_field(res, 1, f, sizeof f);
			if (!strcmp(f, "auc_bid", true)) has = 1;
			db_next_row(res);
		}
		db_free_result(res);
	}
	if (!has)
	{
		db_free_result(db_query(g_GarDB, "ALTER TABLE houses ADD COLUMN auc_bid INTEGER DEFAULT 0"));
		db_free_result(db_query(g_GarDB, "ALTER TABLE houses ADD COLUMN auc_bidder TEXT DEFAULT ''"));
		db_free_result(db_query(g_GarDB, "ALTER TABLE houses ADD COLUMN auc_end INTEGER DEFAULT 0"));
		print("[V15] Table houses : colonnes d'encheres ajoutees.");
	}
	g_AucReady = 1;
	return Auc_LoadAll();
}

// appele a la fin de House_Load (les index des maisons changent)
stock Auc_LoadAll()
{
	for (new i = 0; i < HOUSE_MAX; i++) { g_HouseAucBid[i] = 0; g_HouseAucBidder[i][0] = 0; g_HouseAucEnd[i] = 0; }
	if (g_GarDB == DB:0 || !g_AucReady) return 0;
	new DBResult:res = db_query(g_GarDB, "SELECT id, auc_bid, auc_bidder, auc_end FROM houses WHERE auc_bid > 0");
	if (res == DBResult:0) return 0;
	new rows = db_num_rows(res), f[32];
	for (new r = 0; r < rows; r++)
	{
		db_get_field(res, 0, f, sizeof f);
		new s = House_SlotById(strval(f));
		if (s >= 0)
		{
			db_get_field(res, 1, f, sizeof f); g_HouseAucBid[s] = strval(f);
			db_get_field(res, 2, g_HouseAucBidder[s], MAX_PLAYER_NAME + 1);
			db_get_field(res, 3, f, sizeof f); g_HouseAucEnd[s] = strval(f);
		}
		db_next_row(res);
	}
	db_free_result(res);
	return 1;
}

stock Auc_Save(i)
{
	new q[200];
	format(q, sizeof q, "UPDATE houses SET auc_bid = %d, auc_bidder = '%s', auc_end = %d WHERE id = %d", g_HouseAucBid[i], g_HouseAucBidder[i], g_HouseAucEnd[i], g_HouseId[i]);
	if (g_GarDB != DB:0) db_free_result(db_query(g_GarDB, q));
	return 1;
}

stock Auc_Opening(i)
{
	new p = g_HousePrice[i];
	if (p < AUC_MIN) p = AUC_MIN;
	if (p > AUC_MAX) p = AUC_MAX;
	return p;
}

stock Auc_MinNext(i)
{
	if (g_HouseAucBid[i] <= 0) return Auc_Opening(i);
	new step = g_HouseAucBid[i] / 100 * AUC_STEP_PCT;
	if (step < AUC_STEP_MIN) step = AUC_STEP_MIN;
	new m = g_HouseAucBid[i] + step;
	if (m > AUC_MAX) m = AUC_MAX;
	return m;
}

stock Auc_IsTop(playerid, i)
{
	new name[MAX_PLAYER_NAME + 1];
	GetPlayerName(playerid, name, sizeof name);
	return g_HouseAucBid[i] > 0 && g_HouseAucBidder[i][0] && !strcmp(g_HouseAucBidder[i], name, true);
}

stock Auc_CountTop(playerid)
{
	new n = 0;
	for (new i = 0; i < g_HouseCount; i++) if (Auc_IsTop(playerid, i)) n++;
	return n;
}

stock Auc_LeftText(secs, out[], size)
{
	if (secs < 0) secs = 0;
	if (secs >= 3600) format(out, size, "%dh%02d", secs / 3600, (secs % 3600) / 60);
	else format(out, size, "%d min", (secs + 59) / 60);
	return 1;
}

// retourne un message (ok = 1 si l'offre est acceptee)
stock Auc_Bid(playerid, i, amount, msg[], msgsize)
{
	new now = gettime(), a[20], b[20], left[16], line[160];
	if (i < 0 || i >= g_HouseCount) { format(msg, msgsize, "Maison introuvable."); return 0; }
	if (g_HouseOwner[i][0]) { format(msg, msgsize, "Cette maison n'est pas a vendre."); return 0; }
	if (!g_BankLoaded[playerid]) { format(msg, msgsize, "Compte bancaire indisponible."); return 0; }
	if (Auc_IsTop(playerid, i)) { format(msg, msgsize, "Tu as deja la meilleure offre sur cette maison."); return 0; }
	if (House_CountMine(playerid) + Auc_CountTop(playerid) >= HOUSE_MAX_OWNED) { format(msg, msgsize, "Tu as deja le maximum de maisons (ou d'encheres gagnantes)."); return 0; }
	new minNext = Auc_MinNext(i);
	if (amount < minNext) { Cw_Fmt(minNext, a, sizeof a); format(msg, msgsize, "Offre minimum : %s FCFA.", a); return 0; }
	if (amount > AUC_MAX) amount = AUC_MAX;
	if (g_BankBalance[playerid] < amount) { format(msg, msgsize, "Solde bancaire insuffisant."); return 0; }

	// argent bloque
	g_BankBalance[playerid] -= amount;
	format(line, sizeof line, "Enchere maison #%d", g_HouseId[i]);
	Bank_AddHistory(playerid, line, -amount);
	Bank_SavePlayer(playerid);

	// ancien encherisseur rembourse
	if (g_HouseAucBid[i] > 0 && g_HouseAucBidder[i][0])
	{
		format(line, sizeof line, "Enchere depassee maison #%d (rembourse)", g_HouseId[i]);
		Cw_CreditByName(g_HouseAucBidder[i], g_HouseAucBid[i], line);
		new prev = Cw_PlayerByName(g_HouseAucBidder[i]);
		if (prev != INVALID_PLAYER_ID)
		{
			Cw_Fmt(amount, a, sizeof a);
			format(line, sizeof line, "[ENCHERES] Ton offre sur la maison #%d est depassee (%s FCFA). Tu as ete rembourse.", g_HouseId[i], a);
			SendClientMessage(prev, CW_COL_AUC, line);
		}
	}
	if (g_HouseAucBid[i] <= 0 || g_HouseAucEnd[i] <= now) g_HouseAucEnd[i] = now + AUC_SECONDS;
	if (g_HouseAucEnd[i] - now < AUC_SNIPE) g_HouseAucEnd[i] = now + AUC_SNIPE;
	g_HouseAucBid[i] = amount;
	GetPlayerName(playerid, g_HouseAucBidder[i], MAX_PLAYER_NAME + 1);
	Auc_Save(i);

	Cw_Fmt(amount, a, sizeof a);
	if (amount >= AUC_MAX)
	{
		Auc_Finish(i);
		format(msg, msgsize, "Achat immediat : la maison #%d est a toi !", g_HouseId[i]);
		return 1;
	}
	Auc_LeftText(g_HouseAucEnd[i] - now, left, sizeof left);
	format(line, sizeof line, "[ENCHERES] Maison #%d : nouvelle offre de %s FCFA (fin dans %s). /encheres", g_HouseId[i], a, left);
	SendClientMessageToAll(CW_COL_AUC, line);
	Cw_Fmt(Auc_MinNext(i), b, sizeof b);
	format(msg, msgsize, "Offre de %s FCFA enregistree. Prochaine offre min : %s.", a, b);
	return 1;
}

stock Auc_Finish(i)
{
	if (g_HouseAucBid[i] <= 0 || !g_HouseAucBidder[i][0]) return 0;
	new a[20], line[170];
	format(g_HouseOwner[i], MAX_PLAYER_NAME + 1, "%s", g_HouseAucBidder[i]);
	g_HousePrice[i] = g_HouseAucBid[i];
	g_HouseLock[i] = 1;
	g_HouseSafe[i] = 0;
	House_SaveSlot(i);
	Cw_Fmt(g_HouseAucBid[i], a, sizeof a);
	new w = Cw_PlayerByName(g_HouseAucBidder[i]);
	if (w != INVALID_PLAYER_ID)
	{
		format(line, sizeof line, "[ENCHERES] Felicitations ! Tu remportes la maison #%d pour %s FCFA. /maison pour t'y rendre.", g_HouseId[i], a);
		SendClientMessage(w, CW_COL_OK, line);
	}
	new rp[MAX_PLAYER_NAME + 1];
	format(rp, sizeof rp, "%s", g_HouseAucBidder[i]);
	for (new k = 0; rp[k]; k++) if (rp[k] == '_') rp[k] = ' ';
	format(line, sizeof line, "[ENCHERES] La maison #%d est adjugee a %s pour %s FCFA.", g_HouseId[i], rp, a);
	SendClientMessageToAll(CW_COL_AUC, line);
	g_HouseAucBid[i] = 0; g_HouseAucBidder[i][0] = 0; g_HouseAucEnd[i] = 0;
	Auc_Save(i);
	Marks_SendAll();
	return 1;
}

stock Auc_Cancel(i)
{
	if (i < 0 || i >= g_HouseCount || g_HouseAucBid[i] <= 0) return 0;
	new line[96];
	format(line, sizeof line, "Enchere annulee maison #%d (rembourse)", g_HouseId[i]);
	Cw_CreditByName(g_HouseAucBidder[i], g_HouseAucBid[i], line);
	new p = Cw_PlayerByName(g_HouseAucBidder[i]);
	if (p != INVALID_PLAYER_ID)
	{
		format(line, sizeof line, "[ENCHERES] L'enchere sur la maison #%d a ete annulee : tu es rembourse.", g_HouseId[i]);
		SendClientMessage(p, CW_COL_AUC, line);
	}
	g_HouseAucBid[i] = 0; g_HouseAucBidder[i][0] = 0; g_HouseAucEnd[i] = 0;
	Auc_Save(i);
	return 1;
}

stock Auc_Check()
{
	new now = gettime();
	for (new i = 0; i < g_HouseCount; i++)
		if (g_HouseAucBid[i] > 0 && g_HouseAucEnd[i] > 0 && now >= g_HouseAucEnd[i]) Auc_Finish(i);
	return 1;
}

// champs ajoutes a l'objet "h" du JSON de la page Immobilier
stock Auc_JsonFields(playerid, i, out[], size)
{
	new rp[MAX_PLAYER_NAME + 1];
	format(rp, sizeof rp, "%s", g_HouseAucBidder[i]);
	format(out, size, ",\"ab\":%d,\"abn\":\"%s\",\"aend\":%d,\"amin\":%d,\"atop\":%d,\"amax\":%d,\"aopen\":%d",
		g_HouseAucBid[i], rp, g_HouseAucEnd[i], Auc_MinNext(i), Auc_IsTop(playerid, i), AUC_MAX, Auc_Opening(i));
	return 1;
}

// ------------------------------------------------------------
//  RP : blesse / inconscient
// ------------------------------------------------------------
stock Cw_OnDeath(playerid, killerid)
{
	#pragma unused killerid
	if (g_CwInjured[playerid] == 2 || g_CwInjNoComa[playerid])
	{
		// mort pendant qu'il etait deja inconscient : spawn normal (hopital)
		g_CwInjured[playerid] = 0;
		g_CwInjNoComa[playerid] = 0;
		return 1;
	}
	GetPlayerPos(playerid, g_CwInjPos[playerid][0], g_CwInjPos[playerid][1], g_CwInjPos[playerid][2]);
	GetPlayerFacingAngle(playerid, g_CwInjPos[playerid][3]);
	g_CwInjVw[playerid] = GetPlayerVirtualWorld(playerid);
	g_CwInjInt[playerid] = GetPlayerInterior(playerid);
	g_CwInjured[playerid] = 1;
	return 1;
}

stock Cw_OnSpawn(playerid)
{
	if (g_CwInGz[playerid]) SetPlayerTeam(playerid, CW_GZ_TEAM);
	g_CwHudFuel[playerid] = -1;   // HUD recharge : on renvoie tout
	if (g_CwInjured[playerid] == 1) SetTimerEx("Cw_InjApply", 700, false, "i", playerid);
	return 1;
}

public Cw_InjApply(playerid)
{
	if (!IsPlayerConnected(playerid) || g_CwInjured[playerid] != 1) return 0;
	g_CwInjured[playerid] = 2;
	g_CwInjSince[playerid] = gettime();
	SetPlayerVirtualWorld(playerid, g_CwInjVw[playerid]);
	SetPlayerInterior(playerid, g_CwInjInt[playerid]);
	SetPlayerPos(playerid, g_CwInjPos[playerid][0], g_CwInjPos[playerid][1], g_CwInjPos[playerid][2]);
	SetPlayerFacingAngle(playerid, g_CwInjPos[playerid][3]);
	SetPlayerHealth(playerid, 25.0);
	TogglePlayerControllable(playerid, 0);
	ApplyAnimation(playerid, "PED", "KO_skid_front", 4.1, 0, 1, 1, 1, 0, 1);
	SetCameraBehindPlayer(playerid);
	SendClientMessage(playerid, CW_COL_ERR, "Tu es inconscient. Les medecins sont prevenus (/reanimer). Apres 60 s : /hopital.");
	GameTextForPlayer(playerid, "~r~INCONSCIENT", 3500, 3);
	return 1;
}

stock Cw_Revive(playerid, Float:hp = 50.0)
{
	if (g_CwInjured[playerid] != 2) return 0;
	g_CwInjured[playerid] = 0;
	ClearAnimations(playerid, 1);
	TogglePlayerControllable(playerid, 1);
	SetPlayerHealth(playerid, hp);
	return 1;
}

stock Cw_ToHospital(playerid, const why[])
{
	if (g_CwInjured[playerid] != 2) return 0;
	g_CwInjured[playerid] = 0;
	g_CwInjNoComa[playerid] = 0;
	ClearAnimations(playerid, 1);
	TogglePlayerControllable(playerid, 1);
	SendClientMessage(playerid, CW_COL_INFO, why);
	SpawnPlayer(playerid);          // point de spawn normal (hopital / maison)
	return 1;
}

stock Cw_InjTick(playerid, now)
{
	if (g_CwInjured[playerid] != 2) return 0;
	if (now - g_CwInjSince[playerid] >= CW_INJ_SECONDS) return Cw_ToHospital(playerid, "Les secours t'ont transporte a l'hopital.");
	if ((now - g_CwInjSince[playerid]) % 4 == 0) ApplyAnimation(playerid, "PED", "KO_skid_front", 4.1, 0, 1, 1, 1, 0, 1);
	return 1;
}

// ------------------------------------------------------------
//  RP : ceinture (choc sans ceinture = blessure)
// ------------------------------------------------------------
stock Cw_BeltTick(playerid)
{
	new v = GetPlayerVehicleID(playerid);
	if (!v) { g_CwBelt[playerid] = 0; g_CwVehHp[playerid] = 0.0; g_CwVehKmh[playerid] = 0.0; return 0; }
	new Float:hp, Float:vx, Float:vy, Float:vz;
	GetVehicleHealth(v, hp);
	GetVehicleVelocity(v, vx, vy, vz);
	new Float:kmh = floatsqroot(vx * vx + vy * vy + vz * vz) * 180.0;
	if (g_CwVehHp[playerid] > 0.0)
	{
		new Float:drop = g_CwVehHp[playerid] - hp;
		if (drop >= 90.0 && g_CwVehKmh[playerid] >= 45.0 && !Svc_IsBike(GetVehicleModel(v)))
		{
			new Float:php, Float:dmg = g_CwBelt[playerid] ? (drop / 40.0) : (drop / 7.0);
			if (dmg > 45.0) dmg = 45.0;
			GetPlayerHealth(playerid, php);
			if (php - dmg < 5.0) dmg = php - 5.0;
			if (dmg > 0.5) SetPlayerHealth(playerid, php - dmg);
			if (!g_CwBelt[playerid])
			{
				GameTextForPlayer(playerid, "~r~CHOC !~n~~w~pas de ceinture", 2500, 3);
				SendClientMessage(playerid, CW_COL_ERR, "Choc violent : tu n'avais pas ta ceinture (/ceinture).");
			}
		}
	}
	g_CwVehHp[playerid] = hp;
	g_CwVehKmh[playerid] = kmh;
	return 1;
}

// ------------------------------------------------------------
//  Timers
// ------------------------------------------------------------
public Cw_Tick()
{
	new now = gettime();
	g_CwTickN++;
	for (new p = 0, m = GetMaxPlayers(); p < m; p++)
	{
		if (!IsPlayerConnected(p) || !g_AuthLogged[p]) continue;
		Gz_Update(p);
		Cw_BeltTick(p);
		Cw_InjTick(p, now);
		Cw_HudPush(p);
	}
	if (g_CwTickN % 5 == 0) Invis_SyncAll();
	if (g_CwTickN % 30 == 0) Auc_Check();
	return 1;
}

public Cw_MinuteTick()
{
	new h, mi, s, line[128], a[16];
	gettime(h, mi, s);
	if (g_CwRealTime) SetWorldTime(h);
	for (new p = 0, m = GetMaxPlayers(); p < m; p++)
	{
		if (!IsPlayerConnected(p) || !g_AuthLogged[p]) continue;
		if (g_CwRealTime) SetPlayerTime(p, h, mi);
		g_CwPlayMin[p]++;
		if (g_CwPlayMin[p] % CW_PAY_MINUTES == 0 && g_BankLoaded[p])
		{
			g_BankBalance[p] += CW_PAY_AMOUNT;
			Bank_AddHistory(p, "Prime de presence", CW_PAY_AMOUNT);
			Bank_SavePlayer(p);
			Cw_Fmt(CW_PAY_AMOUNT, a, sizeof a);
			format(line, sizeof line, "[PAYE] Prime de presence : +%s FCFA sur ton compte bancaire. Merci de faire vivre Crestwood !", a);
			SendClientMessage(p, CW_COL_OK, line);
		}
	}
	return 1;
}

// ------------------------------------------------------------
//  Panneau admin : actions + donnees "Monde" ({"x":"world"})
// ------------------------------------------------------------
stock Cw_ToggleRealTime()
{
	g_CwRealTime = !g_CwRealTime;
	if (g_CwDB != DB:0)
	{
		new q[80];
		format(q, sizeof q, "INSERT OR REPLACE INTO meta (k, v) VALUES ('realtime', %d)", g_CwRealTime);
		db_free_result(db_query(g_CwDB, q));
	}
	if (g_CwRealTime) Cw_MinuteTick();
	return g_CwRealTime;
}

stock Cw_AdminNeed(const action[])
{
	if (!strcmp(action, "invis", true) || !strcmp(action, "worldinfo", true)) return 1;
	if (!strcmp(action, "gztp", true) || !strcmp(action, "pumpadd", true) || !strcmp(action, "pumpdel", true) || !strcmp(action, "pumptp", true)) return 4;
	if (!strcmp(action, "rtime", true)) return 5;
	if (!strcmp(action, "gzadd", true) || !strcmp(action, "gzdel", true) || !strcmp(action, "aucstop", true)) return 6;
	return 0;
}

stock Cw_AdminSendWorld(playerid)
{
	new json[3900], part[120], now = gettime();
	format(json, sizeof json, "{\"x\":\"world\",\"invis\":%d,\"rt\":%d,\"gz\":[", g_CwInvis[playerid], g_CwRealTime);
	for (new z = 0; z < g_CwGzCount; z++)
	{
		format(part, sizeof part, "%s[%d,\"%s\",%d,%d,%d]", z ? (",") : (""), g_CwGzId[z], g_CwGzName[z], floatround(g_CwGzPos[z][0]), floatround(g_CwGzPos[z][1]), floatround(g_CwGzPos[z][2]));
		strcat(json, part, sizeof json);
	}
	strcat(json, "],\"pumps\":[", sizeof json);
	new n = 0;
	for (new p = 0; p < g_PtCount && n < 45; p++)
	{
		if (g_PtType[p] != PT_PUMP) continue;
		format(part, sizeof part, "%s[%d,%d,%d,%d]", n ? (",") : (""), g_PtDb[p], floatround(g_PtPos[p][0]), floatround(g_PtPos[p][1]), floatround(g_PtPos[p][2]));
		if (strlen(json) + strlen(part) > 2900) break;
		strcat(json, part, sizeof json);
		n++;
	}
	strcat(json, "],\"auc\":[", sizeof json);
	n = 0;
	for (new i = 0; i < g_HouseCount; i++)
	{
		if (g_HouseAucBid[i] <= 0) continue;
		format(part, sizeof part, "%s[%d,%d,\"%s\",%d]", n ? (",") : (""), g_HouseId[i], g_HouseAucBid[i], g_HouseAucBidder[i], g_HouseAucEnd[i] - now);
		if (strlen(json) + strlen(part) > 3800) break;
		strcat(json, part, sizeof json);
		n++;
	}
	strcat(json, "]}", sizeof json);
	return ShowPlayerDialog(playerid, ADMIN_DIALOG_UPDATE, DIALOG_STYLE_MSGBOX, "@admin", json, "OK", "");
}

// appele en tete de AdmX_Action : 1 = traite
stock Cw_AdminAction(playerid, const action[], const tid[], target, const arg[], msg[], msgsize, &ok)
{
	if (!strcmp(action, "time", true)) { g_CwRealTime = 0; return 0; }     // heure fixee a la main : plus d'heure reelle
	if (!strcmp(action, "heal", true) && target != INVALID_PLAYER_ID) { Cw_Revive(target, 100.0); return 0; }
	if (!Cw_AdminNeed(action)) return 0;

	if (!strcmp(action, "worldinfo", true)) { Cw_AdminSendWorld(playerid); format(msg, msgsize, "Monde actualise."); ok = 1; return 1; }
	if (!strcmp(action, "invis", true))
	{
		Invis_Set(playerid, !g_CwInvis[playerid]);
		format(msg, msgsize, g_CwInvis[playerid] ? ("Mode invisible ACTIVE : les joueurs ne te voient plus (le staff si).") : ("Mode invisible coupe : tu es de nouveau visible."));
		ok = 1;
	}
	else if (!strcmp(action, "gzadd", true))
	{
		new idx = 0, r[12], name[40];
		Bank_NextToken(arg, idx, r, sizeof r);
		while (arg[idx] == ' ') idx++;
		format(name, sizeof name, "%s", arg[idx]);
		if (strval(r) <= 0) format(msg, msgsize, "Rayon invalide (10 a 250 m).");
		else if (GetPlayerVirtualWorld(playerid) != 0 || GetPlayerInterior(playerid) != 0) format(msg, msgsize, "Place-toi dehors (monde 0).");
		else if (!Gz_Add(playerid, float(strval(r)), name)) format(msg, msgsize, "Impossible (maximum %d zones).", CW_GZ_MAX);
		else { format(msg, msgsize, "Zone verte creee ici (rayon %d m).", strval(r)); ok = 1; }
	}
	else if (!strcmp(action, "gzdel", true))
	{
		if (Gz_Del(strval(tid))) { format(msg, msgsize, "Zone verte supprimee."); ok = 1; }
		else format(msg, msgsize, "Zone introuvable.");
	}
	else if (!strcmp(action, "gztp", true))
	{
		new id = strval(tid);
		for (new z = 0; z < g_CwGzCount; z++) if (g_CwGzId[z] == id)
		{
			SetPlayerInterior(playerid, 0); SetPlayerVirtualWorld(playerid, 0);
			Pts_Teleport(playerid, g_CwGzPos[z][0], g_CwGzPos[z][1], g_CwGzZ[z] > 0.0 ? (g_CwGzZ[z] + 1.0) : 35.0, 0.0);
			format(msg, msgsize, "Teleporte : %s.", g_CwGzName[z]); ok = 1;
		}
		if (!ok) format(msg, msgsize, "Zone introuvable.");
	}
	else if (!strcmp(action, "pumpadd", true))
	{
		new Float:x, Float:y, Float:z, Float:a, q[200];
		GetPlayerPos(playerid, x, y, z);
		GetPlayerFacingAngle(playerid, a);
		if (GetPlayerVirtualWorld(playerid) != 0 || GetPlayerInterior(playerid) != 0) format(msg, msgsize, "Place-toi dehors, a cote de la pompe.");
		else if (!Pts_OpenDB()) format(msg, msgsize, "Base des points indisponible.");
		else
		{
			format(q, sizeof q, "INSERT INTO cw_points (type, sub, name, x, y, z, a) VALUES (%d, 0, 'Station-service', %f, %f, %f, %f)", PT_PUMP, x, y, z, a);
			db_free_result(db_query(g_PtDB, q));
			Pts_ReloadPoints();
			format(msg, msgsize, "Pompe a essence posee ici (%d pompe(s) au total).", g_SvcPumpCount);
			ok = 1;
		}
	}
	else if (!strcmp(action, "pumpdel", true) || !strcmp(action, "pumptp", true))
	{
		new db = strval(tid), p = Pts_FindPoint(db);
		if (p == -1 || g_PtType[p] != PT_PUMP) format(msg, msgsize, "Pompe introuvable.");
		else if (!strcmp(action, "pumptp", true))
		{
			SetPlayerInterior(playerid, 0); SetPlayerVirtualWorld(playerid, 0);
			Pts_Teleport(playerid, g_PtPos[p][0] + 1.5, g_PtPos[p][1], g_PtPos[p][2] + 0.5, g_PtPos[p][3]);
			format(msg, msgsize, "Teleporte a la pompe."); ok = 1;
		}
		else
		{
			new q[64];
			format(q, sizeof q, "DELETE FROM cw_points WHERE id = %d", db);
			db_free_result(db_query(g_PtDB, q));
			Pts_ReloadPoints();
			format(msg, msgsize, "Pompe supprimee."); ok = 1;
		}
	}
	else if (!strcmp(action, "rtime", true))
	{
		Cw_ToggleRealTime();
		format(msg, msgsize, g_CwRealTime ? ("Heure reelle activee (heure d'Abidjan).") : ("Heure reelle coupee."));
		ok = 1;
	}
	else if (!strcmp(action, "aucstop", true))
	{
		new s = House_SlotById(strval(tid));
		if (s < 0 || !Auc_Cancel(s)) format(msg, msgsize, "Aucune enchere sur cette maison.");
		else { format(msg, msgsize, "Enchere annulee, encherisseur rembourse."); ok = 1; }
	}
	Cw_AdminSendWorld(playerid);
	return 1;
}

// ------------------------------------------------------------
//  Commandes
// ------------------------------------------------------------
stock Cw_Arg(const cmdtext[], const cmd[])
{
	new l = strlen(cmd);
	if (strcmp(cmdtext, cmd, true, l)) return -1;
	if (cmdtext[l] == 0) return l;
	if (cmdtext[l] != ' ') return -1;
	l++;
	while (cmdtext[l] == ' ') l++;
	return l;
}

stock Cw_OnCommand(playerid, const cmdtext[])
{
	if (!g_AuthLogged[playerid]) return 0;
	new a, msg[170], name[MAX_PLAYER_NAME + 1];
	RpX_Name(playerid, name, sizeof name);

	// ---- encheres
	if (!strcmp(cmdtext, "/encheres", true))
	{
		new n = 0, now = gettime(), b[20], left[16], rp[MAX_PLAYER_NAME + 1];
		SendClientMessage(playerid, CW_COL_AUC, "===== Encheres immobilieres (24 h, 500 000 a 50 000 000 FCFA) =====");
		for (new i = 0; i < g_HouseCount; i++)
		{
			if (g_HouseOwner[i][0]) continue;
			if (g_HouseAucBid[i] > 0)
			{
				Cw_Fmt(g_HouseAucBid[i], b, sizeof b);
				Auc_LeftText(g_HouseAucEnd[i] - now, left, sizeof left);
				format(rp, sizeof rp, "%s", g_HouseAucBidder[i]);
				for (new k = 0; rp[k]; k++) if (rp[k] == '_') rp[k] = ' ';
				format(msg, sizeof msg, "  Maison #%d : %s FCFA par %s - fin dans %s", g_HouseId[i], b, rp, left);
			}
			else
			{
				Cw_Fmt(Auc_Opening(i), b, sizeof b);
				format(msg, sizeof msg, "  Maison #%d : aucune offre - mise de depart %s FCFA", g_HouseId[i], b);
			}
			SendClientMessage(playerid, 0xFFFFFFFF, msg);
			if (++n >= 12) break;
		}
		if (!n) SendClientMessage(playerid, 0xFFFFFFFF, "  Aucune maison a vendre pour le moment.");
		SendClientMessage(playerid, CW_COL_AUC, "Encherir : devant la maison (main \"Maison\") ou /encherir <maison> <montant>.");
		return 1;
	}
	if ((a = Cw_Arg(cmdtext, "/encherir")) != -1)
	{
		new idx = a, t1[12], t2[16];
		Bank_NextToken(cmdtext, idx, t1, sizeof t1);
		Bank_NextToken(cmdtext, idx, t2, sizeof t2);
		new s = House_SlotById(strval(t1));
		if (!t1[0] || !t2[0]) return SendClientMessage(playerid, CW_COL_INFO, "/encherir <numero de maison> <montant>   (liste : /encheres)"), 1;
		new ok = Auc_Bid(playerid, s, strval(t2), msg, sizeof msg);
		SendClientMessage(playerid, ok ? CW_COL_OK : CW_COL_ERR, msg);
		return 1;
	}
	// ---- zones vertes
	if (!strcmp(cmdtext, "/zonesvertes", true) || !strcmp(cmdtext, "/zoneverte", true))
	{
		SendClientMessage(playerid, CW_COL_GZ, "===== Zones vertes (armes rangees, aucune violence) =====");
		for (new z = 0; z < g_CwGzCount; z++)
		{
			format(msg, sizeof msg, "  #%d %s (rayon %d m)", g_CwGzId[z], g_CwGzName[z], floatround(g_CwGzPos[z][2]));
			SendClientMessage(playerid, 0xFFFFFFFF, msg);
		}
		if (Admin_Level(playerid) >= 6) SendClientMessage(playerid, 0xAAAAAAFF, "Staff : /creerzoneverte <rayon> <nom>  /supprzoneverte <id>  (ou panneau admin, onglet Monde)");
		return 1;
	}
	if ((a = Cw_Arg(cmdtext, "/creerzoneverte")) != -1)
	{
		if (Admin_Level(playerid) < 6) return 0;
		new idx = a, r[12];
		Bank_NextToken(cmdtext, idx, r, sizeof r);
		while (cmdtext[idx] == ' ') idx++;
		if (strval(r) <= 0) return SendClientMessage(playerid, CW_COL_INFO, "/creerzoneverte <rayon 10-250> <nom>"), 1;
		if (!Gz_Add(playerid, float(strval(r)), cmdtext[idx])) return SendClientMessage(playerid, CW_COL_ERR, "Impossible de creer la zone."), 1;
		return SendClientMessage(playerid, CW_COL_OK, "Zone verte creee a ta position."), 1;
	}
	if ((a = Cw_Arg(cmdtext, "/supprzoneverte")) != -1)
	{
		if (Admin_Level(playerid) < 6) return 0;
		if (!Gz_Del(strval(cmdtext[a]))) return SendClientMessage(playerid, CW_COL_ERR, "Zone introuvable (/zonesvertes)."), 1;
		return SendClientMessage(playerid, CW_COL_OK, "Zone verte supprimee."), 1;
	}
	// ---- invisibilite (staff)
	if (!strcmp(cmdtext, "/invisible", true) || !strcmp(cmdtext, "/invis", true))
	{
		if (Admin_Level(playerid) < 1) return 0;
		Invis_Set(playerid, !g_CwInvis[playerid]);
		SendClientMessage(playerid, CW_COL_OK, g_CwInvis[playerid] ? ("Mode invisible ACTIVE.") : ("Mode invisible coupe."));
		return 1;
	}
	// ---- ceinture
	if (!strcmp(cmdtext, "/ceinture", true) || !strcmp(cmdtext, "/belt", true))
	{
		if (!IsPlayerInAnyVehicle(playerid)) return SendClientMessage(playerid, CW_COL_ERR, "Tu dois etre dans un vehicule."), 1;
		g_CwBelt[playerid] = !g_CwBelt[playerid];
		format(msg, sizeof msg, "* %s %s sa ceinture de securite.", name, g_CwBelt[playerid] ? ("attache") : ("detache"));
		RpX_Near(playerid, 10.0, CW_COL_ME, msg);
		Cw_HudPush(playerid, 1);
		return 1;
	}
	// ---- des / pile ou face
	if ((a = Cw_Arg(cmdtext, "/de")) != -1)
	{
		new faces = cmdtext[a] ? strval(cmdtext[a]) : 6;
		if (faces < 2 || faces > 100) faces = 6;
		format(msg, sizeof msg, "* %s lance un de a %d faces et obtient %d.", name, faces, random(faces) + 1);
		RpX_Near(playerid, CW_CHAT_DIST, CW_COL_ME, msg);
		return 1;
	}
	if (!strcmp(cmdtext, "/pileface", true) || !strcmp(cmdtext, "/piece", true))
	{
		format(msg, sizeof msg, "* %s lance une piece : %s.", name, random(2) ? ("PILE") : ("FACE"));
		RpX_Near(playerid, CW_CHAT_DIST, CW_COL_ME, msg);
		return 1;
	}
	// ---- blesses
	if ((a = Cw_Arg(cmdtext, "/reanimer")) != -1)
	{
		new t = cmdtext[a] ? strval(cmdtext[a]) : INVALID_PLAYER_ID;
		if (Fac_Type(playerid) != FACTION_MEDIC && Admin_Level(playerid) < 2) return SendClientMessage(playerid, CW_COL_ERR, "Reserve aux medecins."), 1;
		if (!cmdtext[a] || !IsPlayerConnected(t) || t == playerid) return SendClientMessage(playerid, CW_COL_INFO, "/reanimer <id>"), 1;
		if (g_CwInjured[t] != 2) return SendClientMessage(playerid, CW_COL_ERR, "Cette personne n'est pas inconsciente."), 1;
		new Float:x, Float:y, Float:z;
		GetPlayerPos(t, x, y, z);
		if (!IsPlayerInRangeOfPoint(playerid, 3.5, x, y, z) && Admin_Level(playerid) < 2) return SendClientMessage(playerid, CW_COL_ERR, "Approche-toi du blesse."), 1;
		ApplyAnimation(playerid, "MEDIC", "CPR", 4.1, 0, 0, 0, 0, 0, 1);
		Cw_Revive(t, 50.0);
		new tn[MAX_PLAYER_NAME + 1];
		RpX_Name(t, tn, sizeof tn);
		format(msg, sizeof msg, "* %s pratique les premiers secours sur %s, qui reprend connaissance.", name, tn);
		RpX_Near(playerid, CW_CHAT_DIST, CW_COL_ME, msg);
		if (Fac_Type(playerid) == FACTION_MEDIC && g_BankLoaded[playerid])
		{
			g_BankBalance[playerid] += 750;
			Bank_AddHistory(playerid, "Intervention medicale", 750);
			Bank_SavePlayer(playerid);
			SendClientMessage(playerid, CW_COL_OK, "Intervention : +750 FCFA sur ton compte.");
		}
		return 1;
	}
	if (!strcmp(cmdtext, "/hopital", true))
	{
		if (g_CwInjured[playerid] != 2) return SendClientMessage(playerid, CW_COL_ERR, "Tu n'es pas inconscient."), 1;
		new left = CW_INJ_GIVEUP - (gettime() - g_CwInjSince[playerid]);
		if (left > 0) { format(msg, sizeof msg, "Attends encore %d s (un medecin arrive peut-etre).", left); return SendClientMessage(playerid, CW_COL_ERR, msg), 1; }
		Cw_ToHospital(playerid, "Tu es transporte a l'hopital.");
		return 1;
	}
	if (!strcmp(cmdtext, "/blesses", true))
	{
		if (Fac_Type(playerid) != FACTION_MEDIC && Admin_Level(playerid) < 1) return 0;
		new n = 0, tn[MAX_PLAYER_NAME + 1];
		for (new i = 0, m = GetMaxPlayers(); i < m; i++)
		{
			if (!IsPlayerConnected(i) || g_CwInjured[i] != 2) continue;
			RpX_Name(i, tn, sizeof tn);
			format(msg, sizeof msg, "  %s (%d) - inconscient depuis %d s", tn, i, gettime() - g_CwInjSince[i]);
			SendClientMessage(playerid, 0xFFFFFFFF, msg);
			if (!n) Gps_Set(playerid, g_CwInjPos[i][0], g_CwInjPos[i][1]);
			n++;
		}
		if (!n) SendClientMessage(playerid, CW_COL_INFO, "Aucun blesse pour le moment.");
		else SendClientMessage(playerid, CW_COL_INFO, "GPS : le premier blesse est marque sur ta carte.");
		return 1;
	}
	if (!strcmp(cmdtext, "/heurereelle", true))
	{
		if (Admin_Level(playerid) < 5) return 0;
		Cw_ToggleRealTime();
		return SendClientMessage(playerid, CW_COL_OK, g_CwRealTime ? ("Heure reelle activee (heure d'Abidjan).") : ("Heure reelle coupee.")), 1;
	}
	return 0;
}
