// ============================================================
//  [v30] MISSIONS D'ARRIVEE (6 etapes) + permis de conduire (quiz) + points de plantation
// ------------------------------------------------------------
//  Mission 1 : superette 24/7 -> acheter a boire ou a manger -> l'utiliser (inventaire)
//  Mission 2 : mairie -> faire sa carte d'identite
//  Mission 3 : mairie -> passer le permis de conduire (quiz de 3 questions)
//  Mission 4 : signer un contrat de travail dans un metier de la ville
//  Mission 5 : se rendre sur un lieu de plantation (points poses par le staff : /plantation)
//              sans point pose, l'etabli de craft de Grove Street sert de lieu
//  Mission 6 : message de bienvenue, fin du parcours
//  Affichage : panneau d'objectifs en haut a droite du HUD (Obj_*), commande /mission
//  Les etapes deja remplies (code promo : carte + permis) sont validees toutes seules.
// ============================================================

#define MIS_STEPS             6
#define MIS_FINISHED          7
#define MIS_DIALOG_QUIZ       9895
#define MIS_LICENSE_FEE       300
#define MIS_PLANT_RANGE       30.0
#define MIS_PLANT_MAX         12

// (MEV_BUY / MEV_USE / MEV_ID / MEV_LICENSE / MEV_JOB sont definis plus haut dans basicgm1.pwn, pres de LIC_DRIVE)

new DB:g_MisDB;
new g_MisStep[MAX_PLAYERS];
new g_MisFlag[MAX_PLAYERS];          // etape 1 : bit 1 = achete, bit 2 = utilise
new g_MisLoaded[MAX_PLAYERS];
new g_MisFin[MAX_PLAYERS];
new g_MisQuiz[MAX_PLAYERS];
new g_MisQuizOk[MAX_PLAYERS];
new Float:g_MisPlant[MIS_PLANT_MAX][3];
new g_MisPlantCount;

forward Mis_Tick();

stock Mis_Exec(const q[])
{
	if (g_MisDB != DB:0) db_free_result(db_query(g_MisDB, q));
	return 1;
}

stock Missions_OnInit()
{
	g_MisDB = db_open("crestwood_missions.db");
	if (g_MisDB == DB:0) { print("[MISSIONS] ERREUR : base impossible a ouvrir."); return 0; }
	Mis_Exec("CREATE TABLE IF NOT EXISTS missions (name TEXT PRIMARY KEY COLLATE NOCASE, step INTEGER NOT NULL DEFAULT 1, flag INTEGER NOT NULL DEFAULT 0)");
	Mis_Exec("CREATE TABLE IF NOT EXISTS mis_license (name TEXT PRIMARY KEY COLLATE NOCASE, ts INTEGER NOT NULL DEFAULT 0)");
	Mis_Exec("CREATE TABLE IF NOT EXISTS mis_plants (id INTEGER PRIMARY KEY AUTOINCREMENT, x REAL, y REAL, z REAL)");
	Mis_LoadPlants();
	SetTimer("Mis_Tick", 2500, true);
	print("[MISSIONS] 6 missions d'arrivee pretes.");
	return 1;
}

stock Mis_LoadPlants()
{
	g_MisPlantCount = 0;
	if (g_MisDB == DB:0) return 0;
	new DBResult:r = db_query(g_MisDB, "SELECT x, y, z FROM mis_plants ORDER BY id LIMIT 12");
	if (r == DBResult:0) return 0;
	new f[24];
	for (new i = 0, n = db_num_rows(r); i < n && g_MisPlantCount < MIS_PLANT_MAX; i++)
	{
		db_get_field(r, 0, f, sizeof f); g_MisPlant[g_MisPlantCount][0] = floatstr(f);
		db_get_field(r, 1, f, sizeof f); g_MisPlant[g_MisPlantCount][1] = floatstr(f);
		db_get_field(r, 2, f, sizeof f); g_MisPlant[g_MisPlantCount][2] = floatstr(f);
		g_MisPlantCount++;
		db_next_row(r);
	}
	db_free_result(r);
	return 1;
}

// ---------- permis de conduire (drapeau du serveur) ----------
stock Mis_HasLicense(playerid)
{
	if (g_MisDB == DB:0) return 0;
	new name[MAX_PLAYER_NAME + 1], q[128];
	GetPlayerName(playerid, name, sizeof name);
	format(q, sizeof q, "SELECT 1 FROM mis_license WHERE name = '%s' LIMIT 1", name);
	new DBResult:r = db_query(g_MisDB, q), ok = 0;
	if (r != DBResult:0) { ok = db_num_rows(r) > 0; db_free_result(r); }
	return ok;
}

stock Mis_GiveLicense(playerid)
{
	new name[MAX_PLAYER_NAME + 1], q[128];
	GetPlayerName(playerid, name, sizeof name);
	format(q, sizeof q, "INSERT OR REPLACE INTO mis_license (name, ts) VALUES ('%s', %d)", name, gettime());
	Mis_Exec(q);
	return 1;
}

// ---------- sauvegarde / chargement ----------
stock Mis_Save(playerid)
{
	if (!g_MisLoaded[playerid]) return 0;
	new name[MAX_PLAYER_NAME + 1], q[160];
	GetPlayerName(playerid, name, sizeof name);
	format(q, sizeof q, "INSERT OR REPLACE INTO missions (name, step, flag) VALUES ('%s', %d, %d)", name, g_MisStep[playerid], g_MisFlag[playerid]);
	return Mis_Exec(q);
}

stock Mis_Load(playerid)
{
	g_MisStep[playerid] = 1; g_MisFlag[playerid] = 0; g_MisLoaded[playerid] = 1;
	if (g_MisDB == DB:0) return 0;
	new name[MAX_PLAYER_NAME + 1], q[128], f[12];
	GetPlayerName(playerid, name, sizeof name);
	format(q, sizeof q, "SELECT step, flag FROM missions WHERE name = '%s' LIMIT 1", name);
	new DBResult:r = db_query(g_MisDB, q);
	if (r == DBResult:0) return 0;
	if (db_num_rows(r) > 0)
	{
		db_get_field(r, 0, f, sizeof f); g_MisStep[playerid] = strval(f);
		db_get_field(r, 1, f, sizeof f); g_MisFlag[playerid] = strval(f);
		if (g_MisStep[playerid] < 1 || g_MisStep[playerid] > MIS_FINISHED) g_MisStep[playerid] = 1;
	}
	db_free_result(r);
	return 1;
}

stock Missions_OnConnect(playerid)
{
	g_MisLoaded[playerid] = 0; g_MisStep[playerid] = 1; g_MisFlag[playerid] = 0; g_MisFin[playerid] = 0;
	g_MisQuiz[playerid] = 0; g_MisQuizOk[playerid] = 0;
	return 1;
}

// ---------- lieu de plantation le plus proche (ou etabli de craft par defaut) ----------
stock Mis_NearestPlant(playerid, &Float:px, &Float:py, &Float:pz)
{
	new Float:x, Float:y, Float:z;
	GetPlayerPos(playerid, x, y, z);
	if (g_MisPlantCount <= 0)
	{
		px = C23_BENCH_X; py = C23_BENCH_Y; pz = C23_BENCH_Z;
		return 0;
	}
	new best = 0, Float:bd = 99999999.0;
	for (new i = 0; i < g_MisPlantCount; i++)
	{
		new Float:dx = x - g_MisPlant[i][0], Float:dy = y - g_MisPlant[i][1], Float:d = dx * dx + dy * dy;
		if (d < bd) { bd = d; best = i; }
	}
	px = g_MisPlant[best][0]; py = g_MisPlant[best][1]; pz = g_MisPlant[best][2];
	return 1;
}

// ---------- GPS vers l'objectif de l'etape ----------
stock Mis_Gps(playerid)
{
	new step = g_MisStep[playerid];
	if (step == 1)
	{
		if (g_MisFlag[playerid] & 1) return 0;
		new n = Shop_Nearest(playerid);
		if (n != -1) Gps_Set(playerid, g_Shops[n][shX], g_Shops[n][shY]);
	}
	else if (step == 2 || step == 3) Gps_Set(playerid, 1481.0, -1745.0);
	else if (step == 4)
	{
		new best = 0, Float:bd = 99999999.0, Float:x, Float:y, Float:z;
		GetPlayerPos(playerid, x, y, z);
		for (new j = 1; j <= JOB_COUNT; j++)
		{
			if (!g_JobPlaced[j]) continue;
			new Float:dx = x - g_Jobs[j][jbX], Float:dy = y - g_Jobs[j][jbY], Float:d = dx * dx + dy * dy;
			if (d < bd) { bd = d; best = j; }
		}
		if (best > 0) Gps_Set(playerid, g_Jobs[best][jbX], g_Jobs[best][jbY]);
	}
	else if (step == 5)
	{
		new Float:px, Float:py, Float:pz;
		Mis_NearestPlant(playerid, px, py, pz);
		Gps_Set(playerid, px, py);
	}
	return 1;
}

// ---------- panneau d'objectifs ----------
stock Missions_Show(playerid)
{
	if (!g_AuthLogged[playerid]) return 0;
	if (!g_MisLoaded[playerid]) Mis_Load(playerid);
	new step = g_MisStep[playerid], sub[48], fl = g_MisFlag[playerid];
	if (step >= MIS_FINISHED) return 0;
	format(sub, sizeof sub, "Mission %d sur %d", step, MIS_STEPS);
	Obj_Begin("Bienvenue a Crestwood", sub);
	switch (step)
	{
		case 1:
		{
			Obj_Line("Va a la superette 24/7", (fl & 1) ? 2 : 1);
			Obj_Line("Achete a boire ou a manger", (fl & 1) ? 2 : 0);
			Obj_Line("Utilise-le (touche inventaire)", (fl & 2) ? 2 : ((fl & 1) ? 1 : 0));
		}
		case 2:
		{
			Obj_Line("Va a la mairie (GPS : /mission)", 1);
			Obj_Line("Parle a l'agent de l'etat civil", 0);
			Obj_Line("Fais ta carte d'identite", 0);
		}
		case 3:
		{
			Obj_Line("Va a la mairie (GPS : /mission)", 1);
			Obj_Line("Parle a l'agent, choisis le permis", 0);
			Obj_Line("Reussis le quiz (3 questions)", 0);
		}
		case 4:
		{
			Obj_Line("Rends-toi dans un emploi de la ville", 1);
			Obj_Line("Signe ton contrat au siege", 0);
		}
		case 5:
		{
			Obj_Line("Rends-toi sur un lieu de plantation", 1);
			Obj_Line("Reste discret...", 0);
		}
		default:
		{
			Obj_Line("Bonne arrivee dans Crestwood RP", 1);
			Obj_Line("Impose-toi et vis ton RP", 1);
		}
	}
	return Obj_Send(playerid);
}

// ---------- validation de l'etape en cours ----------
stock Mis_StepDone(playerid)
{
	new step = g_MisStep[playerid];
	switch (step)
	{
		case 1: return ((g_MisFlag[playerid] & 3) == 3) ? 1 : 0;
		case 2:
		{
			new age, origin[32];
			return Ui_LoadId(playerid, age, origin, sizeof origin);
		}
		case 3: return Mis_HasLicense(playerid);
		case 4: return (g_JobOf[playerid] != JOB_NONE) ? 1 : 0;
		case 5: return (g_MisFlag[playerid] & 4) ? 1 : 0;
		case 6: return 1;
	}
	return 0;
}

stock Mis_Advance(playerid)
{
	new step = g_MisStep[playerid], t[48], m[160];
	if (step >= MIS_FINISHED) return 0;
	g_MisStep[playerid] = step + 1;
	g_MisFlag[playerid] = 0;
	Mis_Save(playerid);
	if (step == MIS_STEPS)
	{
		SendClientMessage(playerid, 0x3B82FFFF, "Bonne arrivee dans Crestwood RP ! Impose-toi et vis ton RP.");
		Cw_Notify(playerid, "success", "Crestwood RP", "Bienvenue a Crestwood RP !", 3500);
		Obj_Hide(playerid);
		return 1;
	}
	format(t, sizeof t, "Mission %d terminee", step);
	format(m, sizeof m, "Suite : %d/%d (~b~/mission~w~)", step + 1, MIS_STEPS);
	Cw_Notify(playerid, "success", t, m, 3500);
	Mis_Gps(playerid);
	return 1;
}

// Valide toutes les etapes deja remplies (promo : carte + permis), puis affiche
stock Missions_Check(playerid)
{
	if (!g_AuthLogged[playerid]) return 0;
	if (!g_MisLoaded[playerid]) Mis_Load(playerid);
	new guard = 0;
	while (g_MisStep[playerid] < MIS_FINISHED && g_MisStep[playerid] != 6 && Mis_StepDone(playerid) && guard < 8)
	{
		Mis_Advance(playerid);
		guard++;
	}
	// etape 6 : message de bienvenue affiche un moment, puis fin du parcours
	if (g_MisStep[playerid] == 6 && !g_MisFin[playerid])
	{
		g_MisFin[playerid] = 1;
		SetTimerEx("Mis_Finish", 30000, false, "i", playerid);
	}
	Missions_Show(playerid);
	return 1;
}

forward Mis_Finish(playerid);
public Mis_Finish(playerid)
{
	if (!IsPlayerConnected(playerid) || !g_AuthLogged[playerid]) return 0;
	if (g_MisStep[playerid] == 6) Mis_Advance(playerid);
	return 1;
}

stock Missions_Event(playerid, ev)
{
	if (!g_AuthLogged[playerid]) return 0;
	if (!g_MisLoaded[playerid]) Mis_Load(playerid);
	new step = g_MisStep[playerid];
	if (step >= MIS_FINISHED) return 0;
	if (step == 1 && ev == MEV_BUY) g_MisFlag[playerid] |= 1;
	else if (step == 1 && ev == MEV_USE && (g_MisFlag[playerid] & 1)) g_MisFlag[playerid] |= 2;
	Mis_Save(playerid);
	Missions_Check(playerid);
	return 1;
}

stock Missions_OnSpawn(playerid)
{
	SetTimerEx("Mis_ShowLater", 6000, false, "i", playerid);
	return 1;
}

forward Mis_ShowLater(playerid);
public Mis_ShowLater(playerid)
{
	if (!IsPlayerConnected(playerid)) return 0;
	return Missions_Check(playerid);
}

// Boucle : etape 5 (arrivee sur un lieu de plantation) + reaffichage du panneau
public Mis_Tick()
{
	for (new p = 0, m = GetMaxPlayers(); p < m; p++)
	{
		if (!IsPlayerConnected(p) || !g_AuthLogged[p] || !g_MisLoaded[p]) continue;
		if (g_MisStep[p] >= MIS_FINISHED) continue;
		if (g_MisStep[p] == 5 && GetPlayerState(p) != PLAYER_STATE_SPECTATING)
		{
			new Float:px, Float:py, Float:pz;
			Mis_NearestPlant(p, px, py, pz);
			if (IsPlayerInRangeOfPoint(p, MIS_PLANT_RANGE, px, py, pz))
			{
				g_MisFlag[p] |= 4;
				Mis_Save(p);
				Missions_Check(p);
				continue;
			}
		}
		if (!g_JobOn[p] && !g_ObjShown[p]) Missions_Show(p);
	}
	return 1;
}

// ---------- quiz du permis de conduire ----------
stock Mis_StartLicenseQuiz(playerid)
{
	if (Mis_HasLicense(playerid)) return Cw_Notify(playerid, "info", "Permis de conduire", "Tu as deja ton permis de conduire.", 4000), 1;
	if (GetPlayerMoney(playerid) < MIS_LICENSE_FEE && !(g_BankLoaded[playerid] && g_BankBalance[playerid] >= MIS_LICENSE_FEE))
	{
		new m[96];
		format(m, sizeof m, "Le permis coute %d $ (liquide ou banque).", MIS_LICENSE_FEE);
		return Cw_Notify(playerid, "denied", "Permis de conduire", m, 5000), 1;
	}
	if (!Npc_Pay(playerid, MIS_LICENSE_FEE, "Permis de conduire")) return 1;
	g_MisQuiz[playerid] = 0;
	g_MisQuizOk[playerid] = 0;
	SendClientMessage(playerid, 0x3B82FFFF, "Examinateur : 3 questions, il faut 2 bonnes reponses sur 3.");
	return Mis_QuizQuestion(playerid);
}

stock Mis_QuizQuestion(playerid)
{
	new cap[64];
	format(cap, sizeof cap, "{3B82FF}Permis de conduire {9AA0A8}- question %d/3", g_MisQuiz[playerid] + 1);
	switch (g_MisQuiz[playerid])
	{
		case 0: return ShowPlayerDialog(playerid, MIS_DIALOG_QUIZ, DIALOG_STYLE_TABLIST_HEADERS, cap, "{9AA0A8}Au feu rouge, je :\nPasse si la route semble libre\nMarrete completement et j'attends le vert\nKlaxonne pour faire avancer les autres", "Repondre", "");
		case 1: return ShowPlayerDialog(playerid, MIS_DIALOG_QUIZ, DIALOG_STYLE_TABLIST_HEADERS, cap, "{9AA0A8}A un carrefour sans panneau, un vehicule arrive de ma droite :\nJe lui cede le passage\nJ'accelere pour passer avant lui\nJe m'arrete au milieu du carrefour", "Repondre", "");
		default: return ShowPlayerDialog(playerid, MIS_DIALOG_QUIZ, DIALOG_STYLE_TABLIST_HEADERS, cap, "{9AA0A8}En ville, la vitesse maximale est de :\n50 km/h\n90 km/h\n130 km/h", "Repondre", "");
	}
	return 0;
}

stock Missions_OnDialog(playerid, dialogid, response, listitem)
{
	if (dialogid != MIS_DIALOG_QUIZ) return 0;
	if (!response) { SendClientMessage(playerid, 0xFF4444FF, "Examen abandonne (non rembourse)."); return 1; }
	new good = (g_MisQuiz[playerid] == 0) ? 1 : 0;   // ligne de la bonne reponse (0 = 1re)
	if (listitem == good) g_MisQuizOk[playerid]++;
	g_MisQuiz[playerid]++;
	if (g_MisQuiz[playerid] < 3) return Mis_QuizQuestion(playerid);
	new m[128];
	if (g_MisQuizOk[playerid] >= 2)
	{
		Mis_GiveLicense(playerid);
		GameTextForPlayer(playerid, "~g~PERMIS DE CONDUIRE OBTENU", 4000, 3);
		SendClientMessage(playerid, 0x33CC33FF, "Examinateur : \"Felicitations, voici ton permis de conduire !\"");
		Missions_Event(playerid, MEV_LICENSE);
		return 1;
	}
	format(m, sizeof m, "Examinateur : \"%d/3, ce n'est pas suffisant. Repaie l'examen pour retenter.\"", g_MisQuizOk[playerid]);
	SendClientMessage(playerid, 0xFF4444FF, m);
	return 1;
}

// ---------- commandes ----------
stock Missions_OnCommand(playerid, const cmdtext[])
{
	if (!g_AuthLogged[playerid]) return 0;
	new a;
	if (!strcmp(cmdtext, "/mission", true) || !strcmp(cmdtext, "/missions", true))
	{
		if (!g_MisLoaded[playerid]) Mis_Load(playerid);
		if (g_MisStep[playerid] >= MIS_FINISHED)
			return Cw_Notify(playerid, "info", "Missions", "Tu as termine toutes les missions d'arrivee. Bon RP !", 5000), 1;
		Mis_Gps(playerid);
		Missions_Show(playerid);
		return 1;
	}
	if ((a = Cw_Arg(cmdtext, "/plantation")) != -1)
	{
		if (Admin_Level(playerid) < 4) return 0;
		new m[128];
		if (!cmdtext[a] || !strcmp(cmdtext[a], "liste", true))
		{
			format(m, sizeof m, "Plantations : %d point(s) pose(s). /plantation ajouter | supprimer | liste", g_MisPlantCount);
			SendClientMessage(playerid, 0x33CCFFFF, m);
			for (new i = 0; i < g_MisPlantCount; i++)
			{
				format(m, sizeof m, "  #%d : %.1f, %.1f, %.1f", i + 1, g_MisPlant[i][0], g_MisPlant[i][1], g_MisPlant[i][2]);
				SendClientMessage(playerid, 0xFFFFFFFF, m);
			}
			return 1;
		}
		if (!strcmp(cmdtext[a], "ajouter", true))
		{
			if (g_MisPlantCount >= MIS_PLANT_MAX) return SendClientMessage(playerid, 0xFF4444FF, "Maximum de 12 plantations."), 1;
			new Float:x, Float:y, Float:z, q[160];
			GetPlayerPos(playerid, x, y, z);
			format(q, sizeof q, "INSERT INTO mis_plants (x, y, z) VALUES (%.3f, %.3f, %.3f)", x, y, z);
			Mis_Exec(q);
			Mis_LoadPlants();
			format(m, sizeof m, "Plantation ajoutee ici (%d au total).", g_MisPlantCount);
			SendClientMessage(playerid, 0x33CC33FF, m);
			return 1;
		}
		if (!strcmp(cmdtext[a], "supprimer", true))
		{
			Mis_Exec("DELETE FROM mis_plants");
			Mis_LoadPlants();
			SendClientMessage(playerid, 0x33CC33FF, "Toutes les plantations ont ete supprimees (l'etabli de craft sert de lieu par defaut).");
			return 1;
		}
		SendClientMessage(playerid, 0x33CCFFFF, "/plantation ajouter (a ta position) | supprimer (tout) | liste");
		return 1;
	}
	return 0;
}
