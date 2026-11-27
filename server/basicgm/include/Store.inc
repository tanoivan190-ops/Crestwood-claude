// ============================================================
//  [v30] CRESTWOOD STORE (boutique premium) - page assets/store/index.html
// ------------------------------------------------------------
//  Monnaie : CrestCoins (CC), achetes avec Wave (FCFA) puis verifies par le staff.
//  serveur -> client : ShowPlayerDialog(9960 ouvrir / 9961 mise a jour / 9962 fermer, "@store", JSON)
//      {"name":"Prenom Nom","cc":3500,"money":120000,"orders":1,"waves":0}  (+ "ok":1,"msg":"..." en mise a jour)
//  client -> serveur (commandes cachees, envoyees par l'appli) :
//      /storeact buy <article> [extra]     extra = modele (vehicule) ou texte (service)
//      /storeact wave <pack> <reference>   reference de la transaction Wave
//      /storeact refresh | close
//  Joueur : /store (aussi dans la roue : "Crestwood Store")
//  Staff (niveau STORE_STAFF_LVL) :
//      /ccwave                  references Wave en attente
//      /ccvalider <n>           valide la reference n -> le joueur recoit ses CC (meme hors ligne)
//      /ccrefuser <n>           refuse la reference n
//      /ccservices              services a faire (nom, plaque, numero, avertissement)
//      /commandefaite <n>       service fait
//      /rembourser <n>          annule le service n et rend les CC
//      /donnercc <id> <nombre>  (niveau STORE_GIVE_LVL) donne / retire (nombre negatif) des CC
//  Livre tout seul : argent, vehicules (garage), permis, kits (sac).
//  Donnees : scriptfiles/crestwood_store.db
// ============================================================

#define STORE_DIALOG_OPEN     9960
#define STORE_DIALOG_UPDATE   9961
#define STORE_DIALOG_CLOSE    9962
#define STORE_STAFF_LVL       6
#define STORE_GIVE_LVL        8
#define STORE_COOLDOWN_MS     800
#define STORE_COL             0xE6E6E6FF
#define STORE_COL_OK          0x33CC33FF
#define STORE_COL_ERR         0xFF4444FF

#define ST_COINS              0           // pack Wave (FCFA -> CC)
#define ST_MONEY              1           // CC -> argent
#define ST_VEH                2           // CC -> vehicule au choix dans la categorie
#define ST_LIC                3           // argent -> permis
#define ST_KIT                4           // CC -> objets dans le sac
#define ST_ORDER              5           // CC -> service fait par le staff

#define ST_CUR_CC             0
#define ST_CUR_USD            1
#define ST_CUR_FCFA           2

// --- catalogue (memes identifiants et prix que assets/store/store.js) ---
new const g_StId[][12] =
{
	"cc_500", "cc_1100", "cc_3000", "cc_6500", "cc_14000",
	"mo_100k", "mo_500k", "mo_1m", "mo_5m",
	"pv_super", "pv_air", "pv_sea", "pv_sport", "pv_prest", "pv_tuning", "pv_4x4", "pv_moto", "pv_low",
	"li_truck", "li_boat", "li_gun",
	"kt_surv", "kt_heal", "kt_drink",
	"sv_name", "sv_plate", "sv_phone", "sv_warn"
};
new const g_StName[][28] =
{
	"500 CC", "1 100 CC", "3 000 CC", "6 500 CC", "14 000 CC",
	"100 000 $", "500 000 $", "1 000 000 $", "5 000 000 $",
	"Supercar", "Aerien", "Nautique", "Sport", "Prestige", "Tuning", "Tout-terrain", "Moto", "Lowrider",
	"Permis poids lourds", "Permis navire", "Port d'arme",
	"Kit de survie", "Kit de soins", "Pack boissons",
	"Changement de nom", "Plaque perso", "Numero perso", "Retrait d'avertissement"
};
new const g_StType[] =
{
	ST_COINS, ST_COINS, ST_COINS, ST_COINS, ST_COINS,
	ST_MONEY, ST_MONEY, ST_MONEY, ST_MONEY,
	ST_VEH, ST_VEH, ST_VEH, ST_VEH, ST_VEH, ST_VEH, ST_VEH, ST_VEH, ST_VEH,
	ST_LIC, ST_LIC, ST_LIC,
	ST_KIT, ST_KIT, ST_KIT,
	ST_ORDER, ST_ORDER, ST_ORDER, ST_ORDER
};
new const g_StCur[] =
{
	ST_CUR_FCFA, ST_CUR_FCFA, ST_CUR_FCFA, ST_CUR_FCFA, ST_CUR_FCFA,
	ST_CUR_CC, ST_CUR_CC, ST_CUR_CC, ST_CUR_CC,
	ST_CUR_CC, ST_CUR_CC, ST_CUR_CC, ST_CUR_CC, ST_CUR_CC, ST_CUR_CC, ST_CUR_CC, ST_CUR_CC, ST_CUR_CC,
	ST_CUR_USD, ST_CUR_USD, ST_CUR_USD,
	ST_CUR_CC, ST_CUR_CC, ST_CUR_CC,
	ST_CUR_CC, ST_CUR_CC, ST_CUR_CC, ST_CUR_CC
};
new const g_StPrice[] =
{
	1000, 2000, 5000, 10000, 20000,
	100, 450, 850, 4000,
	4500, 6000, 3500, 3200, 3000, 2800, 2200, 1900, 1600,
	60000, 80000, 150000,
	120, 90, 80,
	500, 350, 400, 800
};
// parametre : CC recus (packs) / $ recus (argent) / categorie (vehicule) / permis / kit / 0
new const g_StParam[] =
{
	500, 1100, 3000, 6500, 14000,
	100000, 500000, 1000000, 5000000,
	0, 1, 2, 3, 4, 5, 6, 7, 8,
	LIC_TRUCK, LIC_BOAT, LIC_WEAPON,
	0, 1, 2,
	0, 0, 0, 0
};
new const g_StVeh[9][4] =
{
	{411, 451, 541, 0},     // Supercar : Infernus, Turismo, Bullet
	{487, 469, 593, 0},     // Aerien : Maverick, Sparrow, Dodo
	{493, 446, 452, 0},     // Nautique : Jetmax, Squalo, Speeder
	{429, 415, 477, 0},     // Sport : Banshee, Cheetah, ZR-350
	{409, 580, 507, 0},     // Prestige : Stretch, Stafford, Elegant
	{562, 560, 559, 565},   // Tuning : Elegy, Sultan, Jester, Flash
	{495, 579, 500, 489},   // Tout-terrain : Sandking, Huntley, Mesa, Rancher
	{522, 521, 461, 468},   // Moto : NRG-500, FCR-900, PCJ-600, Sanchez
	{536, 567, 412, 575}    // Lowrider : Blade, Savanna, Voodoo, Broadway
};

new DB:g_StoreDB;
new g_StoreOpen[MAX_PLAYERS];
new g_StoreLast[MAX_PLAYERS];
new PlayerText:g_StPv[MAX_PLAYERS] = {PlayerText:INVALID_TEXT_DRAW, ...};   // [v34] apercu 3D du vehicule

// [v34] apercu du vehicule dans la boutique : textdraw "model preview" placee sous la case transparente de la page.
// Le CLIENT la dessine avec les donnees du jeu (un vehicule modde s'affiche donc tel qu'il est en jeu).
stock Store_PvHide(playerid)
{
	if (g_StPv[playerid] != PlayerText:INVALID_TEXT_DRAW)
	{
		PlayerTextDrawDestroy(playerid, g_StPv[playerid]);
		g_StPv[playerid] = PlayerText:INVALID_TEXT_DRAW;
	}
	return 1;
}

stock Store_PvShow(playerid, model, px, py, pw, ph, rot, col)
{
	Store_PvHide(playerid);
	if (model < 400 || model > 611) return 0;
	if (px < 0) px = 0; if (py < 0) py = 0;
	if (pw < 20) pw = 20; if (ph < 20) ph = 20;
	if (px > 980) px = 980; if (py > 980) py = 980;
	if (pw > 1000) pw = 1000; if (ph > 1000) ph = 1000;
	new PlayerText:t = CreatePlayerTextDraw(playerid, float(px) * 0.640, float(py) * 0.448, "_");
	if (t == PlayerText:INVALID_TEXT_DRAW) return 0;
	PlayerTextDrawFont(playerid, t, TEXT_DRAW_FONT_MODEL_PREVIEW);
	PlayerTextDrawColor(playerid, t, 0xFFFFFFFF);
	PlayerTextDrawUseBox(playerid, t, 1);
	PlayerTextDrawBoxColor(playerid, t, 0x00000000);
	PlayerTextDrawBackgroundColor(playerid, t, 0x15171CFF);
	PlayerTextDrawTextSize(playerid, t, float(pw) * 0.640, float(ph) * 0.448);
	PlayerTextDrawSetPreviewModel(playerid, t, model);
	PlayerTextDrawSetPreviewRot(playerid, t, -14.0, 0.0, 35.0 + float(rot), 0.95);
	PlayerTextDrawSetPreviewVehCol(playerid, t, col, col);
	PlayerTextDrawShow(playerid, t);
	g_StPv[playerid] = t;
	return 1;
}

stock Store_Find(const id[])
{
	for (new i = 0; i < sizeof g_StId; i++) if (!strcmp(id, g_StId[i], true)) return i;
	return -1;
}

stock Store_OnInit()
{
	g_StoreDB = db_open("crestwood_store.db");
	if (g_StoreDB == DB:0) { print("[STORE] ERREUR : crestwood_store.db"); return 0; }
	db_free_result(db_query(g_StoreDB, "CREATE TABLE IF NOT EXISTS store_cc (name TEXT PRIMARY KEY COLLATE NOCASE, cc INTEGER NOT NULL DEFAULT 0)"));
	db_free_result(db_query(g_StoreDB, "CREATE TABLE IF NOT EXISTS store_wave (id INTEGER PRIMARY KEY AUTOINCREMENT, name TEXT COLLATE NOCASE, pack TEXT, fcfa INTEGER, cc INTEGER, ref TEXT, ts INTEGER, state INTEGER NOT NULL DEFAULT 0, staff TEXT NOT NULL DEFAULT '')"));
	db_free_result(db_query(g_StoreDB, "CREATE TABLE IF NOT EXISTS store_orders (id INTEGER PRIMARY KEY AUTOINCREMENT, name TEXT COLLATE NOCASE, item TEXT, extra TEXT, price INTEGER, ts INTEGER, state INTEGER NOT NULL DEFAULT 0, staff TEXT NOT NULL DEFAULT '')"));
	db_free_result(db_query(g_StoreDB, "CREATE TABLE IF NOT EXISTS store_log (id INTEGER PRIMARY KEY AUTOINCREMENT, name TEXT COLLATE NOCASE, item TEXT, extra TEXT, price INTEGER, cur INTEGER, ts INTEGER)"));
	print("[STORE] Crestwood Store pret.");
	return 1;
}

stock Store_OnExit()
{
	if (g_StoreDB != DB:0) db_close(g_StoreDB);
	g_StoreDB = DB:0;
	return 1;
}

stock Store_OnConnect(playerid)
{
	g_StoreOpen[playerid] = 0;
	g_StPv[playerid] = PlayerText:INVALID_TEXT_DRAW;
	g_StoreLast[playerid] = 0;
	return 1;
}

stock Store_OnDisconnect(playerid)
{
	g_StoreOpen[playerid] = 0;
	g_StPv[playerid] = PlayerText:INVALID_TEXT_DRAW;   // detruite avec le joueur
	return 1;
}

// --- CrestCoins (par nom : marche aussi pour un joueur hors ligne) ---
stock Store_CCByName(const name[])
{
	if (g_StoreDB == DB:0) return 0;
	new q[140], f[16], cc = 0;
	format(q, sizeof q, "SELECT cc FROM store_cc WHERE name = '%s'", name);
	new DBResult:r = db_query(g_StoreDB, q);
	if (r != DBResult:0)
	{
		if (db_num_rows(r) > 0) { db_get_field(r, 0, f, sizeof f); cc = strval(f); }
		db_free_result(r);
	}
	return cc;
}

stock Store_AddCCByName(const name[], amount)
{
	if (g_StoreDB == DB:0) return 0;
	new q[200];
	format(q, sizeof q, "INSERT OR IGNORE INTO store_cc (name, cc) VALUES ('%s', 0)", name);
	db_free_result(db_query(g_StoreDB, q));
	format(q, sizeof q, "UPDATE store_cc SET cc = MAX(0, cc + %d) WHERE name = '%s'", amount, name);
	db_free_result(db_query(g_StoreDB, q));
	return 1;
}

stock Store_CC(playerid)
{
	new name[MAX_PLAYER_NAME + 1];
	GetPlayerName(playerid, name, sizeof name);
	return Store_CCByName(name);
}

stock Store_AddCC(playerid, amount)
{
	new name[MAX_PLAYER_NAME + 1];
	GetPlayerName(playerid, name, sizeof name);
	return Store_AddCCByName(name, amount);
}

stock Store_Count(const table[], const name[])
{
	new q[160], f[12], n = 0;
	format(q, sizeof q, "SELECT COUNT(*) FROM %s WHERE name = '%s' AND state = 0", table, name);
	new DBResult:r = db_query(g_StoreDB, q);
	if (r != DBResult:0)
	{
		if (db_num_rows(r) > 0) { db_get_field(r, 0, f, sizeof f); n = strval(f); }
		db_free_result(r);
	}
	return n;
}

// --- page ---
stock Store_Send(playerid, dialogid, ok = 1, const msg[] = "")
{
	new name[MAX_PLAYER_NAME + 1], nice[MAX_PLAYER_NAME + 1], m[150], json[400];
	GetPlayerName(playerid, name, sizeof name);
	format(nice, sizeof nice, "%s", name);
	for (new i = 0; nice[i]; i++) if (nice[i] == '_') nice[i] = ' ';
	Admin_Clean(msg, m, sizeof m);
	format(json, sizeof json, "{\"name\":\"%s\",\"cc\":%d,\"money\":%d,\"orders\":%d,\"waves\":%d,\"ok\":%d,\"msg\":\"%s\"}",
		nice, Store_CC(playerid), GetPlayerMoney(playerid), Store_Count("store_orders", name), Store_Count("store_wave", name), ok, m);
	return ShowPlayerDialog(playerid, dialogid, DIALOG_STYLE_MSGBOX, "@store", json, "OK", "");
}

stock Store_Open(playerid)
{
	if (!g_AuthLogged[playerid]) return 0;
	g_StoreOpen[playerid] = 1;
	return Store_Send(playerid, STORE_DIALOG_OPEN);
}

stock Store_Reply(playerid, ok, const msg[])
{
	if (g_StoreOpen[playerid]) Store_Send(playerid, STORE_DIALOG_UPDATE, ok, msg);
	else SendClientMessage(playerid, ok ? STORE_COL_OK : STORE_COL_ERR, msg);
	return 1;
}

stock Store_StaffAlert(const text[])
{
	for (new p = 0, m = GetMaxPlayers(); p < m; p++)
	{
		if (IsPlayerConnected(p) && g_AuthLogged[p] && Admin_Level(p) >= STORE_STAFF_LVL) SendClientMessage(p, STORE_COL, text);
	}
	printf("%s", text);
	return 1;
}

stock Store_Log(playerid, const item[], const extra[], price, cur)
{
	new name[MAX_PLAYER_NAME + 1], q[300];
	GetPlayerName(playerid, name, sizeof name);
	format(q, sizeof q, "INSERT INTO store_log (name, item, extra, price, cur, ts) VALUES ('%s', '%s', '%s', %d, %d, %d)", name, item, extra, price, cur, gettime());
	db_free_result(db_query(g_StoreDB, q));
	return 1;
}

// texte libre du joueur : seulement lettres, chiffres et _ (pas de ' dans les requetes)
stock Store_CleanArg(const in[], out[], outsize, maxlen)
{
	new j = 0;
	for (new i = 0; in[i] && j < outsize - 1 && j < maxlen; i++)
	{
		new c = in[i];
		if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '_' || c == '-') out[j++] = c;
	}
	out[j] = 0;
	return j;
}

stock Store_ValidName(const s[])
{
	new len = strlen(s), us = -1;
	if (len < 5 || len > 22) return 0;
	for (new i = 0; i < len; i++)
	{
		new c = s[i];
		if (c == '_') { if (us != -1) return 0; us = i; continue; }
		if (!((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z'))) return 0;
	}
	if (us < 2 || us > len - 3) return 0;
	if (!(s[0] >= 'A' && s[0] <= 'Z') || !(s[us + 1] >= 'A' && s[us + 1] <= 'Z')) return 0;
	return 1;
}

stock Store_AllDigits(const s[], minl, maxl)
{
	new len = strlen(s);
	if (len < minl || len > maxl) return 0;
	for (new i = 0; i < len; i++) if (s[i] < '0' || s[i] > '9') return 0;
	return 1;
}

stock Store_PlateOk(const s[])
{
	new len = strlen(s);
	if (len < 2 || len > 8) return 0;
	for (new i = 0; i < len; i++)
	{
		new c = s[i];
		if (!((c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9'))) return 0;
	}
	return 1;
}

// --- achat ---
stock Store_Buy(playerid, const id[], const rawExtra[])
{
	new it = Store_Find(id);
	if (it == -1 || g_StType[it] == ST_COINS) return Store_Reply(playerid, 0, "Article introuvable.");
	new price = g_StPrice[it], cur = g_StCur[it], extra[32], msg[150];
	Store_CleanArg(rawExtra, extra, sizeof extra, 24);

	// 1) verifications propres a l'article (avant de payer)
	new model = 0;
	switch (g_StType[it])
	{
		case ST_VEH:
		{
			model = strval(extra);
			new cat = g_StParam[it], found = 0;
			for (new k = 0; k < 4; k++) if (g_StVeh[cat][k] && g_StVeh[cat][k] == model) found = 1;
			if (!found || Veh_IsBlocked(model)) return Store_Reply(playerid, 0, "Choisis un vehicule de la liste.");
		}
		case ST_LIC:
		{
			if (Gov_HasLicense(playerid, g_StParam[it]) == 1) return Store_Reply(playerid, 0, "Tu as deja ce permis.");
		}
		case ST_ORDER:
		{
			if (!strcmp(id, "sv_name") && !Store_ValidName(extra)) return Store_Reply(playerid, 0, "Nom invalide. Format : Prenom_Nom (ex. Kevin_Kouassi).");
			if (!strcmp(id, "sv_plate"))
			{
				for (new i = 0; extra[i]; i++) if (extra[i] >= 'a' && extra[i] <= 'z') extra[i] -= 32;
				if (!Store_PlateOk(extra)) return Store_Reply(playerid, 0, "Plaque invalide : 2 a 8 lettres ou chiffres.");
			}
			if (!strcmp(id, "sv_phone") && !Store_AllDigits(extra, 4, 7)) return Store_Reply(playerid, 0, "Numero invalide : 4 a 7 chiffres.");
			if (!strcmp(id, "sv_warn")) format(extra, sizeof extra, "-");
		}
	}

	// 2) paiement
	if (cur == ST_CUR_CC)
	{
		new have = Store_CC(playerid);
		if (have < price) { format(msg, sizeof msg, "Il te manque %d CC.", price - have); return Store_Reply(playerid, 0, msg); }
		Store_AddCC(playerid, -price);
	}
	else
	{
		if (GetPlayerMoney(playerid) < price) { format(msg, sizeof msg, "Il te manque %d $.", price - GetPlayerMoney(playerid)); return Store_Reply(playerid, 0, msg); }
		GivePlayerMoney(playerid, -price);
	}
	Store_Log(playerid, g_StId[it], extra, price, cur);

	// 3) livraison
	new name[MAX_PLAYER_NAME + 1], q[240];
	GetPlayerName(playerid, name, sizeof name);
	switch (g_StType[it])
	{
		case ST_MONEY:
		{
			GivePlayerMoney(playerid, g_StParam[it]);
			format(msg, sizeof msg, "%s ajoutes a ton argent liquide.", g_StName[it]);
		}
		case ST_VEH:
		{
			new vname[32];
			format(q, sizeof q, "INSERT INTO vehicles (owner, model, c1, c2, price) VALUES ('%s', %d, %d, %d, %d)", name, model, random(127), random(127), GAR_DEFAULT_PRICE);
			db_free_result(db_query(g_GarDB, q));
			Gar_VehName(model, vname, sizeof vname);
			format(msg, sizeof msg, "%s livre ! Il t'attend dans n'importe quel garage.", vname);
		}
		case ST_LIC:
		{
			format(q, sizeof q, "INSERT OR REPLACE INTO gov_licenses (name, lic, ts) VALUES ('%s', %d, %d)", name, g_StParam[it], gettime());
			db_free_result(db_query(g_JobDB, q));
			format(msg, sizeof msg, "%s ajoute a tes papiers.", g_LicName[g_StParam[it]]);
		}
		case ST_KIT:
		{
			switch (g_StParam[it])
			{
				case 0:
				{
					Inv_SetQty(playerid, "hamburger", Inv_GetQty(playerid, "hamburger") + 5);
					Inv_SetQty(playerid, "eau_kirene", Inv_GetQty(playerid, "eau_kirene") + 5);
					Inv_SetQty(playerid, "bandage", Inv_GetQty(playerid, "bandage") + 2);
				}
				case 1: Inv_SetQty(playerid, "bandage", Inv_GetQty(playerid, "bandage") + 6);
				default:
				{
					Inv_SetQty(playerid, "eau_kirene", Inv_GetQty(playerid, "eau_kirene") + 6);
					Inv_SetQty(playerid, "vody", Inv_GetQty(playerid, "vody") + 4);
				}
			}
			format(msg, sizeof msg, "%s range dans ton sac.", g_StName[it]);
		}
		case ST_ORDER:
		{
			format(q, sizeof q, "INSERT INTO store_orders (name, item, extra, price, ts) VALUES ('%s', '%s', '%s', %d, %d)", name, g_StId[it], extra, price, gettime());
			db_free_result(db_query(g_StoreDB, q));
			format(msg, sizeof msg, "Commande enregistree : %s. Un membre du staff s'en occupe.", g_StName[it]);
			new al[150];
			format(al, sizeof al, "[STORE] %s a commande \"%s\" (%s). /ccservices", name, g_StName[it], extra);
			Store_StaffAlert(al);
		}
	}
	return Store_Reply(playerid, 1, msg);
}

stock Store_Wave(playerid, const pack[], const rawRef[])
{
	new it = Store_Find(pack), ref[36], name[MAX_PLAYER_NAME + 1], q[300];
	if (it == -1 || g_StType[it] != ST_COINS) return Store_Reply(playerid, 0, "Pack introuvable.");
	if (Store_CleanArg(rawRef, ref, sizeof ref, 32) < 5) return Store_Reply(playerid, 0, "Reference Wave trop courte.");
	GetPlayerName(playerid, name, sizeof name);
	if (Store_Count("store_wave", name) >= 3) return Store_Reply(playerid, 0, "Tu as deja 3 references en attente : attends la verification du staff.");
	format(q, sizeof q, "SELECT 1 FROM store_wave WHERE ref = '%s' AND state != 2 LIMIT 1", ref);
	new DBResult:r = db_query(g_StoreDB, q), dup = 0;
	if (r != DBResult:0) { dup = db_num_rows(r) > 0; db_free_result(r); }
	if (dup) return Store_Reply(playerid, 0, "Cette reference a deja ete envoyee.");
	format(q, sizeof q, "INSERT INTO store_wave (name, pack, fcfa, cc, ref, ts) VALUES ('%s', '%s', %d, %d, '%s', %d)", name, g_StId[it], g_StPrice[it], g_StParam[it], ref, gettime());
	db_free_result(db_query(g_StoreDB, q));
	new al[150];
	format(al, sizeof al, "[STORE] %s : paiement Wave %d FCFA (%d CC), ref %s. /ccwave", name, g_StPrice[it], g_StParam[it], ref);
	Store_StaffAlert(al);
	return Store_Reply(playerid, 1, "Reference envoyee ! Tes CC arrivent des que le staff a verifie le paiement.");
}

// --- staff ---
stock Store_StaffList(playerid, waves)
{
	new q[200], line[144], f[6][32];
	if (waves) format(q, sizeof q, "SELECT id, name, fcfa, cc, ref FROM store_wave WHERE state = 0 ORDER BY id LIMIT 12");
	else format(q, sizeof q, "SELECT id, name, item, extra, price FROM store_orders WHERE state = 0 ORDER BY id LIMIT 12");
	new DBResult:r = db_query(g_StoreDB, q);
	if (r == DBResult:0) return 1;
	new rows = db_num_rows(r);
	SendClientMessage(playerid, STORE_COL, waves ? ("--- Paiements Wave a verifier (/ccvalider n, /ccrefuser n) ---") : ("--- Services a faire (/commandefaite n, /rembourser n) ---"));
	if (!rows) SendClientMessage(playerid, STORE_COL, "Rien en attente.");
	for (new i = 0; i < rows; i++)
	{
		for (new k = 0; k < 5; k++) db_get_field(r, k, f[k], 32);
		if (waves) format(line, sizeof line, "n%s  %s  %s FCFA -> %s CC  ref %s", f[0], f[1], f[2], f[3], f[4]);
		else format(line, sizeof line, "n%s  %s  %s  [%s]  %s CC", f[0], f[1], f[2], f[3], f[4]);
		SendClientMessage(playerid, 0xFFFFFFFF, line);
		db_next_row(r);
	}
	db_free_result(r);
	return 1;
}

stock Store_PlayerByName(const name[])
{
	new n[MAX_PLAYER_NAME + 1];
	for (new p = 0, m = GetMaxPlayers(); p < m; p++)
	{
		if (!IsPlayerConnected(p) || !g_AuthLogged[p]) continue;
		GetPlayerName(p, n, sizeof n);
		if (!strcmp(n, name, true)) return p;
	}
	return INVALID_PLAYER_ID;
}

// action : 1 valider / 2 refuser (wave) ; 3 fait / 4 rembourser (commande)
stock Store_StaffDo(playerid, id, action)
{
	new q[200], f[3][32], staff[MAX_PLAYER_NAME + 1], m[150];
	GetPlayerName(playerid, staff, sizeof staff);
	new wave = (action <= 2);
	if (wave) format(q, sizeof q, "SELECT name, cc, fcfa FROM store_wave WHERE id = %d AND state = 0", id);
	else format(q, sizeof q, "SELECT name, price, item FROM store_orders WHERE id = %d AND state = 0", id);
	new DBResult:r = db_query(g_StoreDB, q);
	if (r == DBResult:0) return 1;
	if (!db_num_rows(r)) { db_free_result(r); return SendClientMessage(playerid, STORE_COL_ERR, "Numero introuvable ou deja traite."), 1; }
	for (new k = 0; k < 3; k++) db_get_field(r, k, f[k], 32);
	db_free_result(r);
	new amount = strval(f[1]), target = Store_PlayerByName(f[0]);
	format(q, sizeof q, "UPDATE %s SET state = %d, staff = '%s' WHERE id = %d", wave ? ("store_wave") : ("store_orders"), (action == 1 || action == 3) ? 1 : 2, staff, id);
	db_free_result(db_query(g_StoreDB, q));
	switch (action)
	{
		case 1:
		{
			Store_AddCCByName(f[0], amount);
			format(m, sizeof m, "Wave n%d valide : %s recoit %d CC.", id, f[0], amount);
			if (target != INVALID_PLAYER_ID) { new t[120]; format(t, sizeof t, "Paiement Wave verifie : +%d CrestCoins !", amount); Cw_Notify(target, "success", "Crestwood Store", t); if (g_StoreOpen[target]) Store_Send(target, STORE_DIALOG_UPDATE, 1, t); }
		}
		case 2:
		{
			format(m, sizeof m, "Wave n%d refuse (%s).", id, f[0]);
			if (target != INVALID_PLAYER_ID) Cw_Notify(target, "denied", "Crestwood Store", "Ta reference Wave n'a pas ete validee. Contacte le staff si tu as bien paye.");
		}
		case 3:
		{
			format(m, sizeof m, "Service n%d (%s) marque comme fait.", id, f[0]);
			if (target != INVALID_PLAYER_ID) Cw_Notify(target, "success", "Crestwood Store", "Ta commande a ete livree par le staff.");
		}
		default:
		{
			Store_AddCCByName(f[0], amount);
			format(m, sizeof m, "Service n%d annule : %d CC rendus a %s.", id, amount, f[0]);
			if (target != INVALID_PLAYER_ID) { new t[120]; format(t, sizeof t, "Commande annulee : %d CC rendus.", amount); Cw_Notify(target, "info", "Crestwood Store", t); }
		}
	}
	SendClientMessage(playerid, STORE_COL_OK, m);
	printf("[STORE] %s : %s", staff, m);
	return 1;
}

stock Store_OnCommand(playerid, const cmdtext[])
{
	new a;
	if (!strcmp(cmdtext, "/storeact ", true, 10))
	{
		if (!g_AuthLogged[playerid]) return 1;
		new idx = 10, act[12], p1[24], p2[40];
		Bank_NextToken(cmdtext, idx, act, sizeof act);
		Bank_NextToken(cmdtext, idx, p1, sizeof p1);
		Bank_NextToken(cmdtext, idx, p2, sizeof p2);
		if (!strcmp(act, "close", true)) { g_StoreOpen[playerid] = 0; Store_PvHide(playerid); return 1; }
		if (!strcmp(act, "pvoff", true)) return Store_PvHide(playerid), 1;
		if (!strcmp(act, "pv", true))
		{
			// /storeact pv <modele> <x> <y> <l> <h> <rotation> <couleur>   (x.. h en pour-mille de l'ecran)
			new v[7], tok[8], j = 10;
			Bank_NextToken(cmdtext, j, tok, sizeof tok);   // "pv"
			for (new k = 0; k < 7; k++) { Bank_NextToken(cmdtext, j, tok, sizeof tok); v[k] = strval(tok); }
			if (!g_StoreOpen[playerid]) return 1;
			return Store_PvShow(playerid, v[0], v[1], v[2], v[3], v[4], v[5] % 360, v[6] & 255), 1;
		}
		if (!strcmp(act, "refresh", true)) return Store_Send(playerid, STORE_DIALOG_UPDATE), 1;
		new now = GetTickCount();
		if (g_StoreLast[playerid] && now - g_StoreLast[playerid] < STORE_COOLDOWN_MS) return 1;
		g_StoreLast[playerid] = (now == 0) ? 1 : now;
		if (!strcmp(act, "buy", true)) return Store_Buy(playerid, p1, p2), 1;
		if (!strcmp(act, "wave", true)) return Store_Wave(playerid, p1, p2), 1;
		return 1;
	}
	if (!strcmp(cmdtext, "/store", true) || !strcmp(cmdtext, "/crestwoodstore", true) || !strcmp(cmdtext, "/cc", true))
	{
		if (g_AuthLogged[playerid]) Store_Open(playerid);
		return 1;
	}
	if (!strcmp(cmdtext, "/ccwave", true)) { if (Admin_Level(playerid) < STORE_STAFF_LVL) return 0; return Store_StaffList(playerid, 1), 1; }
	if (!strcmp(cmdtext, "/ccservices", true)) { if (Admin_Level(playerid) < STORE_STAFF_LVL) return 0; return Store_StaffList(playerid, 0), 1; }
	if ((a = Cw_Arg(cmdtext, "/ccvalider")) != -1) { if (Admin_Level(playerid) < STORE_STAFF_LVL) return 0; return Store_StaffDo(playerid, strval(cmdtext[a]), 1), 1; }
	if ((a = Cw_Arg(cmdtext, "/ccrefuser")) != -1) { if (Admin_Level(playerid) < STORE_STAFF_LVL) return 0; return Store_StaffDo(playerid, strval(cmdtext[a]), 2), 1; }
	if ((a = Cw_Arg(cmdtext, "/commandefaite")) != -1) { if (Admin_Level(playerid) < STORE_STAFF_LVL) return 0; return Store_StaffDo(playerid, strval(cmdtext[a]), 3), 1; }
	if ((a = Cw_Arg(cmdtext, "/rembourser")) != -1) { if (Admin_Level(playerid) < STORE_STAFF_LVL) return 0; return Store_StaffDo(playerid, strval(cmdtext[a]), 4), 1; }
	if ((a = Cw_Arg(cmdtext, "/donnercc")) != -1)
	{
		if (Admin_Level(playerid) < STORE_GIVE_LVL) return 0;
		new idx = a, sT[8], sN[12], m[144], tn[MAX_PLAYER_NAME + 1];
		Bank_NextToken(cmdtext, idx, sT, sizeof sT);
		Bank_NextToken(cmdtext, idx, sN, sizeof sN);
		new t = strval(sT), n = strval(sN);
		if (!sT[0] || !sN[0] || !IsPlayerConnected(t) || !g_AuthLogged[t] || n == 0) return SendClientMessage(playerid, STORE_COL_ERR, "Usage : /donnercc <id joueur> <nombre> (negatif = retirer)"), 1;
		Store_AddCC(t, n);
		GetPlayerName(t, tn, sizeof tn);
		format(m, sizeof m, "%s : %s%d CC (solde %d CC).", tn, n > 0 ? ("+") : (""), n, Store_CC(t));
		SendClientMessage(playerid, STORE_COL_OK, m);
		format(m, sizeof m, "Le staff t'a %s %d CrestCoins.", n > 0 ? ("donne") : ("retire"), n > 0 ? n : -n);
		Cw_Notify(t, "info", "Crestwood Store", m);
		if (g_StoreOpen[t]) Store_Send(t, STORE_DIALOG_UPDATE, 1, "");
		new staff[MAX_PLAYER_NAME + 1]; GetPlayerName(playerid, staff, sizeof staff);
		printf("[STORE] %s /donnercc %s %d", staff, tn, n);
		return 1;
	}
	return 0;
}
