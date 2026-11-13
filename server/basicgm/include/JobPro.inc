// ============================================================
//  [v18] TABLETTE JOB + EMPLOIS CREES PAR LE STAFF  (include/JobPro.inc)
// ------------------------------------------------------------
//  TABLETTE JOB : objet "tablette_job" (24/7, 2 500 $). S'ouvre depuis l'inventaire (Utiliser),
//    la roue du HUD (Tablette job) ou /tablette. Panneau natif Android (NativeJobTablet) :
//      EMPLOIS     : les 5 metiers de la ville + les emplois crees par le staff (GPS, signer, commencer)
//      MON TRAVAIL : contrat, niveau, gains, prise de service ; tournee staff en cours
//      QUETES      : les quetes des PNJ (etat, record, GPS vers le PNJ)
//    Serveur -> client : dialogue 9890 (ouvrir) / 9891 (mise a jour) / 9892 (fermer), titre "@tablet", JSON.
//    Client -> serveur : "/tabact <action> [id]" (le serveur re-verifie tout).
//
//  EMPLOIS DU STAFF (panneau admin > Monde > Emplois, niveau 4+) : on cree un emploi (type + nom),
//    on pose le siege a sa position, on ajoute les points de tournee un par un en se deplacant,
//    on regle la paie par arret et le vehicule de service. Sauvegarde : crestwood_jobs.db (table cjobs).
//    Types : 0 Livraison (vehicule, 5 arrets au hasard), 1 Transport (vehicule, ligne dans l'ordre),
//            2 A pied (collecte dans l'ordre), 3 Patrouille (vehicule, 5 points au hasard).
//    Joueur : au siege (icone mallette) -> /emploi ou tablette "Commencer" ; /finemploi pour arreter.
//    Paie par arret + 10 % par niveau (1 niveau tous les 5 services, max 5) + prime de fin = 2 arrets.
// ============================================================

#define TAB_DIALOG_OPEN       9890
#define TAB_DIALOG_UPDATE     9891
#define TAB_DIALOG_CLOSE      9892
#define TAB_ITEM              "tablette_job"
#define TAB_PRICE             2500

#define CJ_MAX                24
#define CJ_PTS                16
#define CJ_TYPES              4
#define CJT_DELIVERY          0
#define CJT_LINE              1
#define CJT_FOOT              2
#define CJT_PATROL            3
#define CJ_HQ_RANGE           4.0
#define CJ_AWAY_MAX           60          // secondes hors du vehicule de service
#define CJ_PAY_MIN            20
#define CJ_PAY_MAX            2000
#define CJ_COL                0x3B82FFFF
#define CJ_COL_OK             0x3DDC97FF
#define CJ_COL_ERR            0xFF6B72FF

new const g_CjTypeName[CJ_TYPES][12] = { "Livraison", "Transport", "A pied", "Patrouille" };

new g_CjCount;
new g_CjDb[CJ_MAX];
new g_CjName[CJ_MAX][24];
new g_CjType[CJ_MAX];
new g_CjModel[CJ_MAX];
new g_CjPay[CJ_MAX];
new g_CjOn[CJ_MAX];
new g_CjHqSet[CJ_MAX];
new Float:g_CjHq[CJ_MAX][4];
new Float:g_CjPt[CJ_MAX][CJ_PTS][3];
new g_CjPts[CJ_MAX];
new g_CjPickup[CJ_MAX];
new Text3D:g_CjLabel[CJ_MAX];

new g_CjRun[MAX_PLAYERS];                 // index + 1 de l'emploi en cours (0 = aucun)
new g_CjOrder[MAX_PLAYERS][CJ_PTS];
new g_CjStep[MAX_PLAYERS];
new g_CjTotal[MAX_PLAYERS];
new g_CjVeh[MAX_PLAYERS];
new g_CjShift[MAX_PLAYERS];
new g_CjStartAt[MAX_PLAYERS];
new g_CjAway[MAX_PLAYERS];
new g_CjLevel[MAX_PLAYERS];
new g_TabOpen[MAX_PLAYERS];
forward Cj_Tick();
forward Tab_OpenLater(playerid);

stock Cj_Busy(playerid) return g_CjRun[playerid] != 0;

stock Cj_Find(dbid)
{
	for (new i = 0; i < g_CjCount; i++) if (g_CjDb[i] == dbid) return i;
	return -1;
}

// nom propre (lettres, chiffres, espace, . - ') sans guillemets ni apostrophe SQL
stock Cj_Clean(const src[], out[], size)
{
	new n = 0;
	for (new k = 0; src[k] && n < size - 1; k++)
	{
		new c = src[k];
		if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == ' ' || c == '.' || c == '-') out[n++] = c;
		else if (c == '_') out[n++] = ' ';
	}
	out[n] = 0;
	while (n > 0 && out[n - 1] == ' ') out[--n] = 0;
	return n;
}

stock Cj_IsFoot(i) return (g_CjType[i] == CJT_FOOT || g_CjModel[i] <= 0);

// ---------------- base de donnees ----------------
stock Cj_PtsToStr(i, out[], size)
{
	out[0] = 0;
	new part[40];
	for (new p = 0; p < g_CjPts[i]; p++)
	{
		format(part, sizeof part, "%s%.1f,%.1f,%.1f", p ? (";") : (""), g_CjPt[i][p][0], g_CjPt[i][p][1], g_CjPt[i][p][2]);
		strcat(out, part, size);
	}
	return 1;
}

stock Cj_StrToPts(i, const s[])
{
	g_CjPts[i] = 0;
	new tok[20], tn = 0, axis = 0, Float:v[3];
	for (new k = 0; ; k++)
	{
		new c = s[k];
		if (c == ',' || c == ';' || c == 0)
		{
			tok[tn] = 0;
			if (tn > 0 && axis < 3) v[axis++] = floatstr(tok);
			tn = 0;
			if ((c == ';' || c == 0) && axis == 3 && g_CjPts[i] < CJ_PTS)
			{
				g_CjPt[i][g_CjPts[i]][0] = v[0]; g_CjPt[i][g_CjPts[i]][1] = v[1]; g_CjPt[i][g_CjPts[i]][2] = v[2];
				g_CjPts[i]++;
			}
			if (c == ';') axis = 0;
			if (c == 0) break;
		}
		else if (tn < sizeof(tok) - 1) tok[tn++] = c;
	}
	return g_CjPts[i];
}

stock Cj_Save(i)
{
	new pts[CJ_PTS * 34], q[800];
	Cj_PtsToStr(i, pts, sizeof pts);
	format(q, sizeof q, "UPDATE cjobs SET name = '%s', type = %d, model = %d, pay = %d, hx = %.2f, hy = %.2f, hz = %.2f, ha = %.1f, hq = %d, pts = '%s', active = %d WHERE id = %d",
		g_CjName[i], g_CjType[i], g_CjModel[i], g_CjPay[i], g_CjHq[i][0], g_CjHq[i][1], g_CjHq[i][2], g_CjHq[i][3], g_CjHqSet[i], pts, g_CjOn[i], g_CjDb[i]);
	db_free_result(db_query(g_JobDB, q));
	return 1;
}

stock Cj_Visual(i)
{
	if (g_CjPickup[i] > 0) { DestroyPickup(g_CjPickup[i]); g_CjPickup[i] = 0; }
	if (g_CjLabel[i] != Text3D:INVALID_3DTEXT_ID) { Delete3DTextLabel(g_CjLabel[i]); g_CjLabel[i] = Text3D:INVALID_3DTEXT_ID; }
	if (!g_CjHqSet[i] || !g_CjOn[i]) return 0;
	g_CjPickup[i] = CreatePickup(1210, 1, g_CjHq[i][0], g_CjHq[i][1], g_CjHq[i][2], 0);
	new t[160];
	format(t, sizeof t, "{3B82FF}EMPLOI · %s\n{F3F4F6}%s · %d $ / arret\n{9AA0A8}/emploi ou tablette job pour commencer", g_CjName[i], g_CjTypeName[g_CjType[i]], g_CjPay[i]);
	g_CjLabel[i] = Create3DTextLabel(t, 0xFFFFFFFF, g_CjHq[i][0], g_CjHq[i][1], g_CjHq[i][2] + 0.6, 18.0, 0, 1);
	return 1;
}

stock Cj_Load()
{
	for (new i = 0; i < g_CjCount; i++)
	{
		if (g_CjPickup[i] > 0) DestroyPickup(g_CjPickup[i]);
		if (g_CjLabel[i] != Text3D:INVALID_3DTEXT_ID) Delete3DTextLabel(g_CjLabel[i]);
	}
	g_CjCount = 0;
	new DBResult:r = db_query(g_JobDB, "SELECT id, name, type, model, pay, hx, hy, hz, ha, hq, pts, active FROM cjobs ORDER BY id LIMIT 24");
	if (r == DBResult:0) return 0;
	new f[CJ_PTS * 34];
	for (new row = 0; row < db_num_rows(r) && g_CjCount < CJ_MAX; row++)
	{
		new i = g_CjCount;
		db_get_field(r, 0, f, sizeof f); g_CjDb[i] = strval(f);
		db_get_field(r, 1, f, sizeof f); Cj_Clean(f, g_CjName[i], 24);
		db_get_field(r, 2, f, sizeof f); g_CjType[i] = strval(f);
		if (g_CjType[i] < 0 || g_CjType[i] >= CJ_TYPES) g_CjType[i] = 0;
		db_get_field(r, 3, f, sizeof f); g_CjModel[i] = strval(f);
		db_get_field(r, 4, f, sizeof f); g_CjPay[i] = strval(f);
		db_get_field(r, 5, f, sizeof f); g_CjHq[i][0] = floatstr(f);
		db_get_field(r, 6, f, sizeof f); g_CjHq[i][1] = floatstr(f);
		db_get_field(r, 7, f, sizeof f); g_CjHq[i][2] = floatstr(f);
		db_get_field(r, 8, f, sizeof f); g_CjHq[i][3] = floatstr(f);
		db_get_field(r, 9, f, sizeof f); g_CjHqSet[i] = strval(f);
		db_get_field(r, 10, f, sizeof f); Cj_StrToPts(i, f);
		db_get_field(r, 11, f, sizeof f); g_CjOn[i] = strval(f);
		g_CjPickup[i] = 0;
		g_CjLabel[i] = Text3D:INVALID_3DTEXT_ID;
		g_CjCount++;
		Cj_Visual(i);
		db_next_row(r);
	}
	db_free_result(r);
	printf("[EMPLOIS STAFF] %d emploi(s) charge(s).", g_CjCount);
	return 1;
}

stock Cj_OnInit()
{
	if (g_JobDB == DB:0) return 0;
	db_free_result(db_query(g_JobDB, "CREATE TABLE IF NOT EXISTS cjobs (id INTEGER PRIMARY KEY AUTOINCREMENT, name TEXT, type INTEGER DEFAULT 0, model INTEGER DEFAULT 0, pay INTEGER DEFAULT 120, hx REAL DEFAULT 0, hy REAL DEFAULT 0, hz REAL DEFAULT 0, ha REAL DEFAULT 0, hq INTEGER DEFAULT 0, pts TEXT DEFAULT '', active INTEGER DEFAULT 1)"));
	db_free_result(db_query(g_JobDB, "CREATE TABLE IF NOT EXISTS cjob_stats (name TEXT COLLATE NOCASE, job INTEGER, runs INTEGER DEFAULT 0, earned INTEGER DEFAULT 0, PRIMARY KEY (name, job))"));
	for (new i = 0; i < CJ_MAX; i++) g_CjLabel[i] = Text3D:INVALID_3DTEXT_ID;
	Cj_Load();
	SetTimer("Cj_Tick", 1000, true);
	return 1;
}

stock Cj_Stats(playerid, dbid, &runs, &earned)
{
	runs = 0; earned = 0;
	new name[MAX_PLAYER_NAME + 1], q[160], f[16];
	GetPlayerName(playerid, name, sizeof name);
	format(q, sizeof q, "SELECT runs, earned FROM cjob_stats WHERE name = '%s' AND job = %d", name, dbid);
	new DBResult:r = db_query(g_JobDB, q);
	if (r == DBResult:0) return 0;
	if (db_num_rows(r) > 0) { db_get_field(r, 0, f, sizeof f); runs = strval(f); db_get_field(r, 1, f, sizeof f); earned = strval(f); }
	db_free_result(r);
	return 1;
}

stock Cj_AddStats(playerid, dbid, money)
{
	new name[MAX_PLAYER_NAME + 1], q[220];
	GetPlayerName(playerid, name, sizeof name);
	format(q, sizeof q, "INSERT OR IGNORE INTO cjob_stats (name, job, runs, earned) VALUES ('%s', %d, 0, 0)", name, dbid);
	db_free_result(db_query(g_JobDB, q));
	format(q, sizeof q, "UPDATE cjob_stats SET runs = runs + 1, earned = earned + %d WHERE name = '%s' AND job = %d", money, name, dbid);
	db_free_result(db_query(g_JobDB, q));
	return 1;
}

// ---------------- tournee ----------------
stock Cj_NearHQ(playerid)
{
	for (new i = 0; i < g_CjCount; i++)
		if (g_CjOn[i] && g_CjHqSet[i] && IsPlayerInRangeOfPoint(playerid, CJ_HQ_RANGE, g_CjHq[i][0], g_CjHq[i][1], g_CjHq[i][2])) return i;
	return -1;
}

stock Cj_Objectives(playerid)
{
	new i = g_CjRun[playerid] - 1;
	if (i < 0) return 0;
	new sub[64], line[64];
	format(sub, sizeof sub, "%s · arret %d/%d · %d $", g_CjTypeName[g_CjType[i]], g_CjStep[playerid] + 1 > g_CjTotal[playerid] ? g_CjTotal[playerid] : g_CjStep[playerid] + 1, g_CjTotal[playerid], g_CjShift[playerid]);
	Obj_Begin(g_CjName[i], sub);
	if (!Cj_IsFoot(i))
	{
		new inVeh = (g_CjVeh[playerid] != INVALID_VEHICLE_ID && IsPlayerInVehicle(playerid, g_CjVeh[playerid]));
		Obj_Line("Monte dans le vehicule de service", inVeh ? 2 : 1);
	}
	new shown = 0;
	for (new s = 0; s < g_CjTotal[playerid] && shown < 6; s++)
	{
		if (s < g_CjStep[playerid] - 1) continue;
		format(line, sizeof line, "%s %d", (g_CjType[i] == CJT_PATROL) ? ("Point de patrouille") : ((g_CjType[i] == CJT_LINE) ? ("Arret") : ((g_CjType[i] == CJT_FOOT) ? ("Collecte") : ("Livraison"))), s + 1);
		Obj_Line(line, (s < g_CjStep[playerid]) ? 2 : ((s == g_CjStep[playerid]) ? 1 : 0));
		shown++;
	}
	Obj_Line("Prime de fin de service", 0);
	return Obj_Send(playerid);
}

stock Cj_ShowPoint(playerid)
{
	new i = g_CjRun[playerid] - 1, s = g_CjStep[playerid];
	if (i < 0 || s >= g_CjTotal[playerid]) return 0;
	new p = g_CjOrder[playerid][s], last = (s + 1 >= g_CjTotal[playerid]);
	new np = last ? p : g_CjOrder[playerid][s + 1];
	SetPlayerRaceCheckpoint(playerid, last ? 1 : 0, g_CjPt[i][p][0], g_CjPt[i][p][1], g_CjPt[i][p][2], g_CjPt[i][np][0], g_CjPt[i][np][1], g_CjPt[i][np][2], Cj_IsFoot(i) ? 2.0 : 6.0);
	Gps_Set(playerid, g_CjPt[i][p][0], g_CjPt[i][p][1]);
	return 1;
}

stock Cj_End(playerid, const reason[], success)
{
	if (!g_CjRun[playerid]) return 0;
	DisablePlayerRaceCheckpoint(playerid);
	Gps_Clear(playerid);
	Obj_Hide(playerid);
	if (g_CjVeh[playerid] != INVALID_VEHICLE_ID) { DestroyVehicle(g_CjVeh[playerid]); g_CjVeh[playerid] = INVALID_VEHICLE_ID; }
	if (reason[0] && IsPlayerConnected(playerid)) SendClientMessage(playerid, success ? CJ_COL_OK : CJ_COL_ERR, reason);
	if (!success && IsPlayerConnected(playerid)) GameTextForPlayer(playerid, "~r~SERVICE TERMINE", 2500, 3);
	g_CjRun[playerid] = 0;
	g_CjStep[playerid] = 0;
	g_CjTotal[playerid] = 0;
	g_CjAway[playerid] = 0;
	return 1;
}

stock Cj_Start(playerid, i, msg[], msgsize)
{
	if (i < 0 || i >= g_CjCount || !g_CjOn[i]) { format(msg, msgsize, "Cet emploi n'est pas disponible."); return 0; }
	if (!g_CjHqSet[i] || g_CjPts[i] < 2) { format(msg, msgsize, "Emploi pas encore pret (siege ou tournee a poser par le staff)."); return 0; }
	if (g_CjRun[playerid]) { format(msg, msgsize, "Tu es deja en service : /finemploi pour arreter."); return 0; }
	if (g_Q[playerid] != Q_NONE) { format(msg, msgsize, "Termine (ou /abandonner) ta quete en cours d'abord."); return 0; }
	if (g_JobOn[playerid]) { format(msg, msgsize, "Tu es deja en service dans ton metier."); return 0; }
	if (!IsPlayerInRangeOfPoint(playerid, CJ_HQ_RANGE * 2.0, g_CjHq[i][0], g_CjHq[i][1], g_CjHq[i][2]))
	{
		Gps_Set(playerid, g_CjHq[i][0], g_CjHq[i][1]);
		format(msg, msgsize, "Va au siege de %s pour commencer (GPS active).", g_CjName[i]);
		return 0;
	}
	// tournee : ordre fixe (transport, a pied) ou 5 points tires au hasard (livraison, patrouille)
	new n = g_CjPts[i];
	for (new k = 0; k < n; k++) g_CjOrder[playerid][k] = k;
	if (g_CjType[i] == CJT_DELIVERY || g_CjType[i] == CJT_PATROL)
	{
		for (new k = n - 1; k > 0; k--) { new j = random(k + 1), t = g_CjOrder[playerid][k]; g_CjOrder[playerid][k] = g_CjOrder[playerid][j]; g_CjOrder[playerid][j] = t; }
		if (n > 5) n = 5;
	}
	new runs, earned;
	Cj_Stats(playerid, g_CjDb[i], runs, earned);
	g_CjLevel[playerid] = runs / 5;
	if (g_CjLevel[playerid] > 5) g_CjLevel[playerid] = 5;
	g_CjRun[playerid] = i + 1;
	g_CjStep[playerid] = 0;
	g_CjTotal[playerid] = n;
	g_CjShift[playerid] = 0;
	g_CjAway[playerid] = 0;
	g_CjStartAt[playerid] = gettime();
	g_CjVeh[playerid] = INVALID_VEHICLE_ID;
	if (!Cj_IsFoot(i))
	{
		new Float:a = g_CjHq[i][3], Float:x = g_CjHq[i][0] + 4.0 * floatsin(-(a + 90.0), degrees), Float:y = g_CjHq[i][1] + 4.0 * floatcos(-(a + 90.0), degrees);
		new v = CreateVehicle(g_CjModel[i], x, y, g_CjHq[i][2] + 0.6, a, 1, 1, -1);
		if (v == INVALID_VEHICLE_ID || v <= 0 || v > MAX_VEHICLES) { g_CjRun[playerid] = 0; format(msg, msgsize, "Vehicule de service indisponible, reessaie."); return 0; }
		SetVehicleVirtualWorld(v, GetPlayerVirtualWorld(playerid));
		g_CwFuel[v] = 100.0;
		SetVehicleNumberPlate(v, "CW-JOB");
		SetVehicleParamsEx(v, VEHICLE_PARAMS_OFF, VEHICLE_PARAMS_OFF, VEHICLE_PARAMS_OFF, VEHICLE_PARAMS_OFF, VEHICLE_PARAMS_OFF, VEHICLE_PARAMS_OFF, VEHICLE_PARAMS_OFF);   // [v20] moteur coupe
		g_CjVeh[playerid] = v;
		PutPlayerInVehicle(playerid, v, 0);
		Veh_EngineHint(playerid);
	}
	Cj_ShowPoint(playerid);
	Cj_Objectives(playerid);
	new m[144];
	format(m, sizeof m, "Emploi : service %s commence (%d arrets, niveau %d : +%d %%). /finemploi pour arreter.", g_CjName[i], n, g_CjLevel[playerid] + 1, g_CjLevel[playerid] * 10);
	SendClientMessage(playerid, CJ_COL, m);
	format(msg, msgsize, "Service commence : suis le GPS.");
	return 1;
}

stock Cj_Arrive(playerid)
{
	new i = g_CjRun[playerid] - 1;
	if (i < 0) return 0;
	if (!Cj_IsFoot(i) && (g_CjVeh[playerid] == INVALID_VEHICLE_ID || !IsPlayerInVehicle(playerid, g_CjVeh[playerid])))
	{
		GameTextForPlayer(playerid, "~w~Avec le ~b~vehicule de service~w~ !", 2000, 3);
		return 0;
	}
	if (Cj_IsFoot(i)) ApplyAnimation(playerid, "CARRY", "putdwn", 4.1, 0, 0, 0, 0, 0, 1);
	new pay = g_CjPay[i] * (100 + g_CjLevel[playerid] * 10) / 100;
	GivePlayerMoney(playerid, pay);
	g_CjShift[playerid] += pay;
	PlayerPlaySound(playerid, 1058, 0.0, 0.0, 0.0);
	new gt[48];
	format(gt, sizeof gt, "~b~+%d $", pay);
	GameTextForPlayer(playerid, gt, 1500, 1);
	g_CjStep[playerid]++;
	if (g_CjStep[playerid] >= g_CjTotal[playerid])
	{
		new bonus = pay * 2;
		GivePlayerMoney(playerid, bonus);
		g_CjShift[playerid] += bonus;
		Cj_AddStats(playerid, g_CjDb[i], g_CjShift[playerid]);
		new m[144];
		format(m, sizeof m, "Emploi : service %s termine en %d:%02d. Total %d $ (prime %d). Bon travail !", g_CjName[i], (gettime() - g_CjStartAt[playerid]) / 60, (gettime() - g_CjStartAt[playerid]) % 60, g_CjShift[playerid], bonus);
		GameTextForPlayer(playerid, "~b~SERVICE TERMINE", 2500, 3);
		return Cj_End(playerid, m, 1);
	}
	Cj_ShowPoint(playerid);
	Cj_Objectives(playerid);
	return 1;
}

public Cj_Tick()
{
	for (new p = 0; p < MAX_PLAYERS; p++)
	{
		if (!g_CjRun[p] || !IsPlayerConnected(p)) continue;
		new i = g_CjRun[p] - 1;
		if (Cj_IsFoot(i)) continue;
		if (g_CjVeh[p] != INVALID_VEHICLE_ID && IsPlayerInVehicle(p, g_CjVeh[p])) { g_CjAway[p] = 0; continue; }
		g_CjAway[p]++;
		if (g_CjAway[p] == 20 || g_CjAway[p] == 45)
		{
			new m[96];
			format(m, sizeof m, "Emploi : remonte dans le vehicule de service (%d s).", CJ_AWAY_MAX - g_CjAway[p]);
			SendClientMessage(p, 0xFFB454FF, m);
		}
		if (g_CjAway[p] >= CJ_AWAY_MAX) Cj_End(p, "Emploi : vehicule de service abandonne, service arrete.", 0);
	}
	return 1;
}

// ---------------- callbacks ----------------
stock Cj_OnConnect(playerid)
{
	g_CjRun[playerid] = 0;
	g_CjVeh[playerid] = INVALID_VEHICLE_ID;
	g_TabOpen[playerid] = 0;
	return 1;
}
stock Cj_OnDisconnect(playerid) { Cj_End(playerid, "", 0); g_TabOpen[playerid] = 0; return 1; }
stock Cj_OnDeath(playerid) { if (g_CjRun[playerid]) Cj_End(playerid, "Emploi : tu es tombe, service arrete.", 0); return 1; }
stock Cj_OnVehicleDeath(vehicleid)
{
	for (new p = 0; p < MAX_PLAYERS; p++)
	{
		if (g_CjRun[p] && g_CjVeh[p] == vehicleid)
		{
			g_CjVeh[p] = INVALID_VEHICLE_ID;
			Cj_End(p, "Emploi : le vehicule de service est detruit, service arrete.", 0);
		}
	}
	return 1;
}
stock Cj_OnRaceCheckpoint(playerid)
{
	if (!g_CjRun[playerid]) return 0;
	new i = g_CjRun[playerid] - 1, s = g_CjStep[playerid];
	if (s >= g_CjTotal[playerid]) return 1;
	new p = g_CjOrder[playerid][s];
	if (IsPlayerInRangeOfPoint(playerid, 12.0, g_CjPt[i][p][0], g_CjPt[i][p][1], g_CjPt[i][p][2])) Cj_Arrive(playerid);
	return 1;
}

// ---------------- tablette ----------------
stock Tab_Has(playerid) return (Inv_GetQty(playerid, TAB_ITEM) > 0 || Admin_Level(playerid) >= 1);

stock Tab_Send(playerid, dialog, const msg[], ok)
{
	new json[3600], part[150], name[MAX_PLAYER_NAME + 1], Float:x, Float:y, Float:z;
	GetPlayerName(playerid, name, sizeof name);
	for (new k = 0; name[k]; k++) if (name[k] == '_') name[k] = ' ';
	GetPlayerPos(playerid, x, y, z);
	new job = g_JobOf[playerid], cj = g_CjRun[playerid] - 1, cm[96], jn[24], cjn[24];
	Admin_Clean(msg, cm, sizeof cm);
	if (Job_Valid(job)) format(jn, sizeof jn, "%s", g_Jobs[job][jbName]); else format(jn, sizeof jn, "Aucun");
	if (cj >= 0) format(cjn, sizeof cjn, "%s", g_CjName[cj]); else cjn[0] = 0;
	format(json, sizeof json, "{\"me\":{\"n\":\"%s\",\"j\":%d,\"jn\":\"%s\",\"r\":%d,\"e\":%d,\"on\":%d,\"sh\":%d,\"cj\":%d,\"cjn\":\"%s\",\"cs\":%d,\"ct\":%d,\"cm\":%d,\"cash\":%d,\"q\":%d},\"msg\":\"%s\",\"ok\":%d,\"b\":[",
		name, job, jn, g_JobRoutes[playerid], g_JobEarned[playerid], g_JobOn[playerid], g_JobShift[playerid],
		cj >= 0 ? g_CjDb[cj] : 0, cjn, g_CjStep[playerid], g_CjTotal[playerid], g_CjShift[playerid], GetPlayerMoney(playerid), g_Q[playerid], cm, ok);
	for (new j = 1; j <= JOB_COUNT; j++)
	{
		new d = -1;
		if (g_JobPlaced[j]) d = floatround(floatsqroot((x - g_Jobs[j][jbX]) * (x - g_Jobs[j][jbX]) + (y - g_Jobs[j][jbY]) * (y - g_Jobs[j][jbY])));
		format(part, sizeof part, "%s[%d,\"%s\",%d,%d,%d]", (j > 1) ? (",") : (""), j, g_Jobs[j][jbName], g_Jobs[j][jbModel], g_Jobs[j][jbBonus], d);
		strcat(json, part, sizeof json);
	}
	strcat(json, "],\"s\":[", sizeof json);
	new n = 0;
	for (new i = 0; i < g_CjCount; i++)
	{
		if (!g_CjOn[i] || !g_CjHqSet[i] || g_CjPts[i] < 2) continue;
		new d = floatround(floatsqroot((x - g_CjHq[i][0]) * (x - g_CjHq[i][0]) + (y - g_CjHq[i][1]) * (y - g_CjHq[i][1])));
		new runs, earned;
		Cj_Stats(playerid, g_CjDb[i], runs, earned);
		format(part, sizeof part, "%s[%d,\"%s\",%d,%d,%d,%d,%d,%d]", n ? (",") : (""), g_CjDb[i], g_CjName[i], g_CjType[i], Cj_IsFoot(i) ? 0 : g_CjModel[i], g_CjPay[i], d,
			(g_CjType[i] == CJT_DELIVERY || g_CjType[i] == CJT_PATROL) ? ((g_CjPts[i] > 5) ? 5 : g_CjPts[i]) : g_CjPts[i], runs);
		if (strlen(json) + strlen(part) > sizeof(json) - 200) break;
		strcat(json, part, sizeof json);
		n++;
	}
	strcat(json, "],\"qs\":[", sizeof json);
	for (new q = 1; q < Q_COUNT; q++)
	{
		new best, done = Npc_QuestDone(playerid, q, best);
		if (q == Q_PILOT && Gov_HasLicense(playerid, LIC_PLANE) && !done) done = 1;
		format(part, sizeof part, "%s[%d,\"%s\",%d,%d]", (q > 1) ? (",") : (""), q, g_QuestName[q], done, best);
		strcat(json, part, sizeof json);
	}
	strcat(json, "]}", sizeof json);
	if (dialog == TAB_DIALOG_OPEN) g_TabOpen[playerid] = 1;
	return ShowPlayerDialog(playerid, dialog, DIALOG_STYLE_MSGBOX, "@tablet", json, "OK", "");
}

stock Tab_Open(playerid)
{
	if (!Tab_Has(playerid))
	{
		new m[128];
		format(m, sizeof m, "Tablette job : achete-la dans un 24/7 (%d $), puis ouvre-la depuis l'inventaire ou la roue.", TAB_PRICE);
		return SendClientMessage(playerid, CJ_COL_ERR, m);
	}
	return Tab_Send(playerid, TAB_DIALOG_OPEN, "", 1);
}

public Tab_OpenLater(playerid) { if (IsPlayerConnected(playerid)) Tab_Open(playerid); return 1; }

stock Tab_Close(playerid)
{
	g_TabOpen[playerid] = 0;
	return ShowPlayerDialog(playerid, TAB_DIALOG_CLOSE, DIALOG_STYLE_MSGBOX, "@tablet", "{}", "OK", "");
}

stock Tab_GpsRole(playerid, role)
{
	new best = -1, Float:bd = 999999999.0, Float:x, Float:y, Float:z;
	GetPlayerPos(playerid, x, y, z);
	for (new i = 0; i < g_NpcCount; i++)
	{
		if (g_NpcRole[i] != role || g_NpcVw[i] != 0) continue;
		new Float:dx = x - g_NpcPos[i][0], Float:dy = y - g_NpcPos[i][1], Float:d = dx * dx + dy * dy;
		if (d < bd) { bd = d; best = i; }
	}
	if (best < 0) return 0;
	Gps_Set(playerid, g_NpcPos[best][0], g_NpcPos[best][1]);
	return 1;
}

stock Tab_Action(playerid, const params[])
{
	if (!Tab_Has(playerid)) return Tab_Close(playerid);
	new idx = 0, act[12], sId[12], msg[128], ok = 0;
	Bank_NextToken(params, idx, act, sizeof act);
	Bank_NextToken(params, idx, sId, sizeof sId);
	new id = strval(sId);
	if (!strcmp(act, "close", true)) { g_TabOpen[playerid] = 0; return 1; }
	else if (!strcmp(act, "refresh", true)) { ok = 1; }
	else if (!strcmp(act, "bgps", true))
	{
		if (!Job_Valid(id) || !g_JobPlaced[id]) format(msg, sizeof msg, "Ce siege n'est pas encore place.");
		else { Gps_Set(playerid, g_Jobs[id][jbX], g_Jobs[id][jbY]); format(msg, sizeof msg, "GPS : siege %s.", g_Jobs[id][jbName]); ok = 1; }
	}
	else if (!strcmp(act, "bsign", true)) ok = Job_Sign(playerid, id, msg, sizeof msg);
	else if (!strcmp(act, "bresign", true)) ok = Job_Resign(playerid, msg, sizeof msg);
	else if (!strcmp(act, "bstart", true))
	{
		ok = Job_Start(playerid, msg, sizeof msg);
		if (ok) { Tab_Close(playerid); if (msg[0]) SendClientMessage(playerid, CJ_COL_OK, msg); return 1; }
	}
	else if (!strcmp(act, "sgps", true))
	{
		new i = Cj_Find(id);
		if (i < 0 || !g_CjHqSet[i]) format(msg, sizeof msg, "Emploi introuvable.");
		else { Gps_Set(playerid, g_CjHq[i][0], g_CjHq[i][1]); format(msg, sizeof msg, "GPS : siege %s.", g_CjName[i]); ok = 1; }
	}
	else if (!strcmp(act, "sstart", true))
	{
		ok = Cj_Start(playerid, Cj_Find(id), msg, sizeof msg);
		if (ok) { Tab_Close(playerid); return 1; }
	}
	else if (!strcmp(act, "sstop", true))
	{
		if (!g_CjRun[playerid]) format(msg, sizeof msg, "Aucun service en cours.");
		else { Cj_End(playerid, "Emploi : service arrete.", 0); format(msg, sizeof msg, "Service arrete."); ok = 1; }
	}
	else if (!strcmp(act, "qgps", true))
	{
		new role = (id == Q_SCOOTER) ? NPC_ROLE_GUIDE : ((id == Q_PILOT) ? NPC_ROLE_PILOT : NPC_ROLE_COURIER);
		if (id < 1 || id >= Q_COUNT) format(msg, sizeof msg, "Quete inconnue.");
		else if (Tab_GpsRole(playerid, role)) { format(msg, sizeof msg, "GPS : %s.", g_QuestName[id]); ok = 1; }
		else format(msg, sizeof msg, "Aucun PNJ pour cette quete pour l'instant.");
	}
	else return 1;
	return Tab_Send(playerid, g_TabOpen[playerid] ? TAB_DIALOG_UPDATE : TAB_DIALOG_OPEN, msg, ok);
}

stock Cj_OnCommand(playerid, const cmdtext[])
{
	if (!strcmp(cmdtext, "/tablette", true) || !strcmp(cmdtext, "/tab", true)) return Tab_Open(playerid), 1;
	if (!strcmp(cmdtext, "/tabact ", true, 8)) return Tab_Action(playerid, cmdtext[8]), 1;
	if (!strcmp(cmdtext, "/emploi", true))
	{
		new i = Cj_NearHQ(playerid), msg[128];
		if (i < 0) return SendClientMessage(playerid, CJ_COL_ERR, "Emploi : va au siege d'un emploi (icone mallette) ou ouvre ta tablette job."), 1;
		if (!Cj_Start(playerid, i, msg, sizeof msg)) SendClientMessage(playerid, CJ_COL_ERR, msg);
		return 1;
	}
	if (!strcmp(cmdtext, "/finemploi", true))
	{
		if (!g_CjRun[playerid]) return SendClientMessage(playerid, CJ_COL_ERR, "Emploi : aucun service en cours."), 1;
		return Cj_End(playerid, "Emploi : service arrete.", 0), 1;
	}
	return 0;
}

// ---------------- panneau admin (onglet Monde) ----------------
stock Cj_AdminNeed(const action[])
{
	if (!strcmp(action, "cjtp", true)) return 2;
	if (!strcmp(action, "cjadd", true) || !strcmp(action, "cjdel", true) || !strcmp(action, "cjhq", true) || !strcmp(action, "cjpt", true) ||
		!strcmp(action, "cjclr", true) || !strcmp(action, "cjpay", true) || !strcmp(action, "cjveh", true) || !strcmp(action, "cjtype", true) ||
		!strcmp(action, "cjname", true) || !strcmp(action, "cjtog", true) || !strcmp(action, "cjundo", true)) return 4;
	return 0;
}

stock Cj_AdminSend(playerid)
{
	new json[2600], part[120];
	format(json, sizeof json, "{\"x\":\"cjobs\",\"l\":[");
	for (new i = 0; i < g_CjCount; i++)
	{
		format(part, sizeof part, "%s[%d,\"%s\",%d,%d,%d,%d,%d,%d]", i ? (",") : (""), g_CjDb[i], g_CjName[i], g_CjType[i], g_CjModel[i], g_CjPay[i], g_CjHqSet[i], g_CjPts[i], g_CjOn[i]);
		if (strlen(json) + strlen(part) > sizeof(json) - 4) break;
		strcat(json, part, sizeof json);
	}
	strcat(json, "]}", sizeof json);
	return ShowPlayerDialog(playerid, ADMIN_DIALOG_UPDATE, DIALOG_STYLE_MSGBOX, "@admin", json, "OK", "");
}

// cjadd "type nom" ; cjpay/cjveh/cjtype "valeur" ; cjname "nom"
stock Cj_AdminAction(playerid, const action[], const tid[], const arg[], msg[], msgsize, &ok)
{
	if (!Cj_AdminNeed(action)) return 0;
	new i = Cj_Find(strval(tid));
	if (!strcmp(action, "cjadd", true))
	{
		new idx = 0, sType[8], nm[24];
		Bank_NextToken(arg, idx, sType, sizeof sType);
		while (arg[idx] == ' ') idx++;
		Cj_Clean(arg[idx], nm, sizeof nm);
		new type = strval(sType);
		if (type < 0 || type >= CJ_TYPES) type = 0;
		if (strlen(nm) < 2) { format(msg, msgsize, "Donne un nom a l'emploi (2 lettres min)."); return 1; }
		if (g_CjCount >= CJ_MAX) { format(msg, msgsize, "Maximum %d emplois.", CJ_MAX); return 1; }
		new q[200], Float:x, Float:y, Float:z, Float:a;
		GetPlayerPos(playerid, x, y, z);
		GetPlayerFacingAngle(playerid, a);
		new model = (type == CJT_FOOT) ? 0 : ((type == CJT_LINE) ? 431 : ((type == CJT_PATROL) ? 596 : 414));
		format(q, sizeof q, "INSERT INTO cjobs (name, type, model, pay, hx, hy, hz, ha, hq, pts, active) VALUES ('%s', %d, %d, 120, %.2f, %.2f, %.2f, %.1f, 1, '', 1)", nm, type, model, x, y, z, a);
		db_free_result(db_query(g_JobDB, q));
		Cj_Load();
		format(msg, msgsize, "Emploi \"%s\" cree, siege pose ici. Ajoute maintenant les points de tournee (+ Point ici).", nm);
		ok = 1;
		return 1;
	}
	if (i < 0) { format(msg, msgsize, "Emploi introuvable."); return 1; }
	if (!strcmp(action, "cjtp", true))
	{
		SetPlayerPos(playerid, g_CjHq[i][0] + 1.0, g_CjHq[i][1], g_CjHq[i][2] + 0.5);
		SetPlayerVirtualWorld(playerid, 0); SetPlayerInterior(playerid, 0);
		format(msg, msgsize, "Teleporte au siege de %s.", g_CjName[i]); ok = 1;
		return 1;
	}
	if (!strcmp(action, "cjdel", true))
	{
		new q[96];
		for (new p = 0; p < MAX_PLAYERS; p++) if (g_CjRun[p] == i + 1) Cj_End(p, "Emploi : cet emploi a ete supprime par le staff.", 0);
		format(q, sizeof q, "DELETE FROM cjobs WHERE id = %d", g_CjDb[i]);
		db_free_result(db_query(g_JobDB, q));
		format(msg, msgsize, "Emploi %s supprime.", g_CjName[i]);
		Cj_Load();
		ok = 1;
		return 1;
	}
	if (!strcmp(action, "cjhq", true))
	{
		GetPlayerPos(playerid, g_CjHq[i][0], g_CjHq[i][1], g_CjHq[i][2]);
		GetPlayerFacingAngle(playerid, g_CjHq[i][3]);
		g_CjHqSet[i] = 1;
		format(msg, msgsize, "Siege de %s pose ici.", g_CjName[i]);
	}
	else if (!strcmp(action, "cjpt", true))
	{
		if (g_CjPts[i] >= CJ_PTS) { format(msg, msgsize, "Maximum %d points.", CJ_PTS); return 1; }
		new Float:x, Float:y, Float:z;
		GetPlayerPos(playerid, x, y, z);
		g_CjPt[i][g_CjPts[i]][0] = x; g_CjPt[i][g_CjPts[i]][1] = y; g_CjPt[i][g_CjPts[i]][2] = z;
		g_CjPts[i]++;
		format(msg, msgsize, "Point %d ajoute a %s.", g_CjPts[i], g_CjName[i]);
	}
	else if (!strcmp(action, "cjundo", true))
	{
		if (g_CjPts[i] > 0) g_CjPts[i]--;
		format(msg, msgsize, "Dernier point retire (%d restant).", g_CjPts[i]);
	}
	else if (!strcmp(action, "cjclr", true)) { g_CjPts[i] = 0; format(msg, msgsize, "Tournee de %s effacee.", g_CjName[i]); }
	else if (!strcmp(action, "cjpay", true))
	{
		new v = strval(arg);
		if (v < CJ_PAY_MIN || v > CJ_PAY_MAX) { format(msg, msgsize, "Paie par arret : %d a %d $.", CJ_PAY_MIN, CJ_PAY_MAX); return 1; }
		g_CjPay[i] = v;
		format(msg, msgsize, "%s : %d $ par arret.", g_CjName[i], v);
	}
	else if (!strcmp(action, "cjveh", true))
	{
		new v = strval(arg);
		if (v != 0 && (v < 400 || v > 611)) { format(msg, msgsize, "Modele 400 a 611 (0 = a pied)."); return 1; }
		g_CjModel[i] = v;
		format(msg, msgsize, "%s : vehicule %d.", g_CjName[i], v);
	}
	else if (!strcmp(action, "cjtype", true))
	{
		new v = strval(arg);
		if (v < 0 || v >= CJ_TYPES) { format(msg, msgsize, "Type 0 a 3."); return 1; }
		g_CjType[i] = v;
		format(msg, msgsize, "%s : type %s.", g_CjName[i], g_CjTypeName[v]);
	}
	else if (!strcmp(action, "cjname", true))
	{
		new nm[24];
		if (Cj_Clean(arg, nm, sizeof nm) < 2) { format(msg, msgsize, "Nom trop court."); return 1; }
		format(g_CjName[i], 24, "%s", nm);
		format(msg, msgsize, "Emploi renomme : %s.", nm);
	}
	else if (!strcmp(action, "cjtog", true))
	{
		g_CjOn[i] = !g_CjOn[i];
		format(msg, msgsize, "%s %s.", g_CjName[i], g_CjOn[i] ? ("ouvert") : ("ferme"));
	}
	Cj_Save(i);
	Cj_Visual(i);
	ok = 1;
	return 1;
}
