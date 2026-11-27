/* ============================================================
   CRESTWOOD RP v23 (17/11/2026) - systemes du cahier d'Anzo
   ------------------------------------------------------------
   Toutes les pages de ce module s'affichent DANS le HUD (hud/cw23.js) : aucune WebView en plus,
   ouverture instantanee. Serveur -> HUD : dialogue 9860 "@hud" {"m":"mdt"|"army"|"asm"|"arena"|"craft"|...}
   HUD -> serveur : /c23 <module> <action> [args]

   1) MDT (police, FBI, armee) : /mdt
      recherche d'un citoyen (avis de recherche, amendes, vehicules et plaques), recherche de plaque,
      amendes, localisation (suspect recherche ou amende impayee), radio inter-factions (/fr, aussi EMS
      et gouvernement), cameras de la ville, radar de vitesse qui flashe (/radar [limite]).
      Citoyens : /amendes, /payeramende <n> | tout.
   2) ARMEE : ravitaillement en munitions. Point A (aeroport LS) /charger dans un vehicule -> base militaire
      (docks) /livrer -> stock de l'armee. /fournir police|fbi|gov <qte> donne des munitions aux factions,
      /armurerie (forces de l'ordre / gouvernement) remplit l'arme en main depuis le stock de sa faction.
      Un gangster qui vole un vehicule charge l'amene au point de recel (/livrer) -> materiaux. /armee = stocks.
   3) EMS : /civiere <id> (le patient suit l'ambulancier, monte dans l'ambulance avec lui),
      /chirurgie <id> (changement de sexe, 5 000 $ payes par le patient qui accepte avec /accepterchirurgie).
   4) JOB ASSEMBLEUR DE VOITURES (usine de Willowfield) : /assembler (1 ou 2 joueurs), /piece puis /poser
      x6, couleur au hasard, /spray a l'entrepot, peindre (+10 % / 5 s par bombe, 2 bombes = 2x plus vite).
      15 000 $ au niveau 1, +1 000 $ par niveau.
   5) ARENE PVP (mapping prive, monde 777, aeroport de LV) : /arene, /quitterarene, classement.
   6) CRAFT (etabli de Grove Street) : /craft -> munitions, armes, drogue avec des materiaux.
   7) RECYCLAGE (casse de Willowfield) : /ramasser un sac poubelle, /deposer a moins de 10 m du point
      -> argent + materiaux.
   Texte ASCII. Inclus apres <Cw22>.
   ============================================================ */
#define C23_DIALOG            9860
#define C23_COL               0x6AA5FFFF
#define C23_COL_RADIO         0x8FD3FFFF
#define C23_RADARS            8
#define C23_CHAT              8
#define C23_ARENA_VW          777
#define C23_BAGS              8
#define C23_ATTACH            7           // index d'objet attache (piece / sac)

// ---------- points ----------
#define C23_DEPOT_X           1890.0      // A : depot de munitions (aeroport LS, zone cargo)
#define C23_DEPOT_Y           -2400.0
#define C23_DEPOT_Z           13.55
#define C23_BASE_X            2700.0      // B : base militaire (mapping Armee LS des docks)
#define C23_BASE_Y            -2450.0
#define C23_BASE_Z            13.6
#define C23_RECEL_X           2460.0      // point de recel (Grove)
#define C23_RECEL_Y           -1650.0
#define C23_RECEL_Z           13.45
#define C23_ASM_X             2186.0      // usine d'assemblage (Willowfield)
#define C23_ASM_Y             -2262.0
#define C23_ASM_Z             13.55
#define C23_BENCH_X           2496.0      // etabli de craft (Grove Street)
#define C23_BENCH_Y           -1688.0
#define C23_BENCH_Z           13.4
#define C23_RECY_X            2183.0      // casse / recyclage (Willowfield)
#define C23_RECY_Y            -1983.0
#define C23_RECY_Z            13.55
#define C23_ARENA_X           1477.0      // arene : piste de l'aeroport de LV
#define C23_ARENA_Y           1450.0
#define C23_ARENA_Z           10.82

new const Float:g_C23Cams[][6] = {
	{1500.0, -1700.0, 40.0, 1480.0, -1740.0, 13.0},   // Pershing Square
	{2480.0, -1660.0, 30.0, 2495.0, -1687.0, 13.0},   // Grove Street
	{1780.0, -1880.0, 30.0, 1760.0, -1900.0, 13.0},   // Unity Station
	{1180.0, -1300.0, 30.0, 1172.0, -1323.0, 15.0},   // Hopital All Saints
	{2700.0, -2400.0, 40.0, 2750.0, -2440.0, 13.0},   // Docks / base militaire
	{1540.0, -1640.0, 35.0, 1554.0, -1675.0, 16.0}    // Commissariat
};
new const g_C23CamName[][] = { "Pershing Square", "Grove Street", "Unity Station", "Hopital All Saints", "Docks - base militaire", "Commissariat" };

new const g_C23AsmModels[] = { 560, 562, 411, 541, 415, 429 };
new const g_C23ColName[][] = { "Rouge", "Bleu", "Noir", "Blanc", "Vert", "Jaune" };
new const g_C23ColId[] = { 3, 79, 0, 1, 16, 6 };
new const g_C23PartName[][] = { "Moteur", "Roue avant gauche", "Roue avant droite", "Roue arriere gauche", "Roue arriere droite", "Capot" };

// ---------- etat ----------
new DB:g_C23DB;
new g_C23Mats[MAX_PLAYERS], g_C23Drug[MAX_PLAYERS], g_C23Loaded[MAX_PLAYERS];
new g_C23Cam[MAX_PLAYERS];
new g_C23ChatF[C23_CHAT][16], g_C23ChatN[C23_CHAT][MAX_PLAYER_NAME + 1], g_C23ChatT[C23_CHAT][96], g_C23ChatHead;
new Float:g_C23Radar[C23_RADARS][3], g_C23RadarLim[C23_RADARS], g_C23RadarOwner[C23_RADARS] = {INVALID_PLAYER_ID, ...}, g_C23RadarEnd[C23_RADARS], g_C23RadarObj[C23_RADARS];
new g_C23Flash[MAX_PLAYERS];
new g_C23Stock[FACTION_TYPE_MAX + 1];
new g_C23Cargo[MAX_VEHICLES + 1];
new g_C23CargoBy[MAX_VEHICLES + 1];
new g_C23LoadCd[MAX_PLAYERS];
new g_C23Stretch[MAX_PLAYERS] = {INVALID_PLAYER_ID, ...};   // medic -> patient
new g_C23Surg[MAX_PLAYERS] = {INVALID_PLAYER_ID, ...};      // patient -> medic qui propose
// assembleur
new g_AsmCar = INVALID_VEHICLE_ID, g_AsmTeam[2] = {INVALID_PLAYER_ID, INVALID_PLAYER_ID}, g_AsmParts, g_AsmPaint, g_AsmCol = -1;
new g_AsmCarry[MAX_PLAYERS];
// arene
new g_ArIn[MAX_PLAYERS], g_ArK[MAX_PLAYERS], g_ArD[MAX_PLAYERS];
new Float:g_ArBack[MAX_PLAYERS][4], g_ArBackInt[MAX_PLAYERS], g_ArBackVw[MAX_PLAYERS], g_ArW[MAX_PLAYERS][13][2];
new g_ArObj[MAX_PLAYERS][40], g_ArObjN[MAX_PLAYERS];
// recyclage
new g_RecyObj[C23_BAGS], Float:g_RecyPos[C23_BAGS][3], g_RecyBack[C23_BAGS];
new g_RecyCarry[MAX_PLAYERS];
forward Cw23_Tick();
forward Cw23_Fast();
forward Asm_Destroy(v);

// ============================================================
//  base
// ============================================================
stock C23_Send(playerid, const json[])
{
	return ShowPlayerDialog(playerid, C23_DIALOG, DIALOG_STYLE_MSGBOX, "@hud", json, "OK", "");
}

stock C23_Name(playerid, out[], size)
{
	GetPlayerName(playerid, out, size);
	return 1;
}

stock C23_Near(playerid, Float:x, Float:y, Float:z, Float:r)
{
	return IsPlayerInRangeOfPoint(playerid, r, x, y, z);
}

stock C23_Exec(const q[])
{
	if (g_C23DB != DB:0) db_free_result(db_query(g_C23DB, q));
	return 1;
}

stock C23_SaveMats(playerid)
{
	if (!g_C23Loaded[playerid] || g_C23DB == DB:0) return 0;
	new name[MAX_PLAYER_NAME + 1], q[180];
	C23_Name(playerid, name, sizeof name);
	format(q, sizeof q, "INSERT OR REPLACE INTO c23_players (name, mats, drug, ak, ad) VALUES ('%s', %d, %d, %d, %d)", name, g_C23Mats[playerid], g_C23Drug[playerid], g_ArK[playerid], g_ArD[playerid]);
	C23_Exec(q);
	return 1;
}

stock C23_LoadMats(playerid)
{
	g_C23Mats[playerid] = 0; g_C23Drug[playerid] = 0; g_ArK[playerid] = 0; g_ArD[playerid] = 0;
	if (g_C23DB == DB:0) return 0;
	new name[MAX_PLAYER_NAME + 1], q[140];
	C23_Name(playerid, name, sizeof name);
	format(q, sizeof q, "SELECT mats, drug, ak, ad FROM c23_players WHERE name = '%s'", name);
	new DBResult:r = db_query(g_C23DB, q);
	if (r != DBResult:0)
	{
		if (db_num_rows(r))
		{
			new f[16];
			db_get_field(r, 0, f, sizeof f); g_C23Mats[playerid] = strval(f);
			db_get_field(r, 1, f, sizeof f); g_C23Drug[playerid] = strval(f);
			db_get_field(r, 2, f, sizeof f); g_ArK[playerid] = strval(f);
			db_get_field(r, 3, f, sizeof f); g_ArD[playerid] = strval(f);
		}
		db_free_result(r);
	}
	g_C23Loaded[playerid] = 1;
	return 1;
}

stock C23_SaveStock(t)
{
	new q[96];
	format(q, sizeof q, "INSERT OR REPLACE INTO c23_stock (type, qty) VALUES (%d, %d)", t, g_C23Stock[t]);
	return C23_Exec(q);
}

// [v24] stock de munitions des factions (utilise par l'armurerie des vestiaires)
stock C23_StockOf(t) return (t >= 0 && t <= FACTION_TYPE_MAX && (t == FACTION_POLICE || t == FACTION_FEDERAL || t == FACTION_ARMY || t == FACTION_GOVERNMENT)) ? g_C23Stock[t] : -1;
stock C23_TakeStock(t, q)
{
	if (t < 0 || t > FACTION_TYPE_MAX || g_C23Stock[t] < q) return 0;
	g_C23Stock[t] -= q;
	C23_SaveStock(t);
	return 1;
}

stock Cw23_OnInit()
{
	g_C23DB = db_open("crestwood_v23.db");
	if (g_C23DB != DB:0)
	{
		C23_Exec("CREATE TABLE IF NOT EXISTS c23_players (name TEXT PRIMARY KEY COLLATE NOCASE, mats INTEGER, drug INTEGER, ak INTEGER, ad INTEGER)");
		C23_Exec("CREATE TABLE IF NOT EXISTS c23_fines (id INTEGER PRIMARY KEY AUTOINCREMENT, name TEXT COLLATE NOCASE, amount INTEGER, reason TEXT, author TEXT, ts INTEGER, paid INTEGER DEFAULT 0)");
		C23_Exec("CREATE TABLE IF NOT EXISTS c23_stock (type INTEGER PRIMARY KEY, qty INTEGER)");
		new DBResult:r = db_query(g_C23DB, "SELECT type, qty FROM c23_stock"), f[16];
		if (r != DBResult:0)
		{
			for (new i = 0, n = db_num_rows(r); i < n; i++)
			{
				db_get_field(r, 0, f, sizeof f);
				new t = strval(f);
				db_get_field(r, 1, f, sizeof f);
				if (t >= 0 && t <= FACTION_TYPE_MAX) g_C23Stock[t] = strval(f);
				db_next_row(r);
			}
			db_free_result(r);
		}
	}
	for (new k = 0; k < C23_RADARS; k++) g_C23RadarObj[k] = INVALID_OBJECT_ID;
	for (new b = 0; b < C23_BAGS; b++) Recy_Spawn(b);
	Create3DTextLabel("Depot de munitions de l'armee\n{9CC1FF}/charger dans un vehicule", 0xFFFFFFFF, C23_DEPOT_X, C23_DEPOT_Y, C23_DEPOT_Z + 0.6, 25.0, 0, 1);
	Create3DTextLabel("Base militaire\n{9CC1FF}/livrer  /fournir  /armee", 0xFFFFFFFF, C23_BASE_X, C23_BASE_Y, C23_BASE_Z + 0.6, 25.0, 0, 1);
	Create3DTextLabel("Usine d'assemblage\n{9CC1FF}/assembler", 0xFFFFFFFF, C23_ASM_X, C23_ASM_Y, C23_ASM_Z + 0.6, 20.0, 0, 1);
	Create3DTextLabel("Pieces\n{9CC1FF}/piece", 0xFFFFFFFF, C23_ASM_X - 10.0, C23_ASM_Y - 8.0, C23_ASM_Z + 0.6, 15.0, 0, 1);
	Create3DTextLabel("Peinture\n{9CC1FF}/spray", 0xFFFFFFFF, C23_ASM_X + 10.0, C23_ASM_Y - 8.0, C23_ASM_Z + 0.6, 15.0, 0, 1);
	Create3DTextLabel("Etabli\n{9CC1FF}/craft", 0xFFFFFFFF, C23_BENCH_X, C23_BENCH_Y, C23_BENCH_Z + 0.6, 15.0, 0, 1);
	Create3DTextLabel("Recyclage\n{9CC1FF}/deposer tes sacs ici", 0xFFFFFFFF, C23_RECY_X, C23_RECY_Y, C23_RECY_Z + 0.6, 20.0, 0, 1);
	SetTimer("Cw23_Tick", 1000, true);
	SetTimer("Cw23_Fast", 350, true);
	print("[CW23] MDT, armee, EMS, assembleur, arene, craft, recyclage prets.");
	return 1;
}

stock Cw23_OnConnect(playerid)
{
	g_C23Loaded[playerid] = 0; g_C23Cam[playerid] = 0; g_C23Flash[playerid] = 0; g_C23LoadCd[playerid] = 0;
	g_C23Stretch[playerid] = INVALID_PLAYER_ID; g_C23Surg[playerid] = INVALID_PLAYER_ID;
	g_AsmCarry[playerid] = 0; g_RecyCarry[playerid] = 0; g_ArIn[playerid] = 0; g_ArObjN[playerid] = 0;
	return 1;
}

stock Cw23_OnDisconnect(playerid)
{
	if (g_ArIn[playerid]) Arena_Leave(playerid, 0);
	C23_SaveMats(playerid);
	for (new p = 0, m = GetMaxPlayers(); p < m; p++) if (g_C23Stretch[p] == playerid) Ems_Release(p);
	if (g_C23Stretch[playerid] != INVALID_PLAYER_ID) Ems_Release(playerid);
	for (new k = 0; k < 2; k++) if (g_AsmTeam[k] == playerid) g_AsmTeam[k] = INVALID_PLAYER_ID;
	if (g_AsmCar != INVALID_VEHICLE_ID && g_AsmTeam[0] == INVALID_PLAYER_ID && g_AsmTeam[1] == INVALID_PLAYER_ID) Asm_Reset();
	for (new k = 0; k < C23_RADARS; k++) if (g_C23RadarOwner[k] == playerid) Radar_Remove(k);
	return 1;
}

stock Cw23_OnDeath(playerid, killerid)
{
	if (g_ArIn[playerid])
	{
		g_ArD[playerid]++;
		if (killerid != INVALID_PLAYER_ID && killerid != playerid && g_ArIn[killerid])
		{
			g_ArK[killerid]++;
			new m[96], a[MAX_PLAYER_NAME + 1], b[MAX_PLAYER_NAME + 1];
			C23_Name(killerid, a, sizeof a); C23_Name(playerid, b, sizeof b);
			format(m, sizeof m, "[Arene] %s a elimine %s (%d)", a, b, g_ArK[killerid]);
			Arena_Say(m);
		}
		return 1;
	}
	if (g_C23Stretch[playerid] != INVALID_PLAYER_ID) Ems_Release(playerid);
	if (g_AsmCarry[playerid]) { g_AsmCarry[playerid] = 0; RemovePlayerAttachedObject(playerid, C23_ATTACH); }
	if (g_RecyCarry[playerid]) { g_RecyCarry[playerid] = 0; RemovePlayerAttachedObject(playerid, C23_ATTACH); }
	return 0;
}

stock Cw23_InArena(playerid) return g_ArIn[playerid];

stock Cw23_OnSpawn(playerid)
{
	if (!g_C23Loaded[playerid] && g_AuthLogged[playerid]) C23_LoadMats(playerid);
	if (g_ArIn[playerid]) Arena_Place(playerid);
	return 1;
}

stock Cw23_OnStateChange(playerid, newstate)
{
	if (newstate != PLAYER_STATE_DRIVER) return 1;
	new v = GetPlayerVehicleID(playerid);
	if (v <= 0 || v > MAX_VEHICLES || g_C23Cargo[v] <= 0) return 1;
	if (Fac_Type(playerid) == FACTION_ARMY) return Cw_Notify(playerid, "info", "Armee", "Cargaison de munitions a bord : ~b~/livrer~w~ a la base militaire (GPS).", 6000), Gps_Set(playerid, C23_BASE_X, C23_BASE_Y), 1;
	if (Fac_IsLaw(playerid)) return 1;
	Gps_Set(playerid, C23_RECEL_X, C23_RECEL_Y);
	Cw_Notify(playerid, "warning", "Cargaison volee", "Munitions militaires a bord ! Va au point de recel (GPS) et ~r~/livrer~w~ pour les materiaux.", 7000);
	new m[120], z[32];
	C23_ZoneName(playerid, z, sizeof z);
	format(m, sizeof m, "[ARMEE] Un vehicule de munitions a ete vole (%s) !", z);
	C23_ToType(FACTION_ARMY, 0xFF6347FF, m);
	C23_ToType(FACTION_POLICE, 0xFF6347FF, m);
	return 1;
}

stock C23_ZoneName(playerid, out[], size)
{
	new Float:x, Float:y, Float:z;
	GetPlayerPos(playerid, x, y, z);
	format(out, size, "%d, %d", floatround(x), floatround(y));
	return 1;
}

stock C23_ToType(t, color, const text[])
{
	for (new p = 0, m = GetMaxPlayers(); p < m; p++) if (IsPlayerConnected(p) && g_AuthLogged[p] && Fac_Type(p) == t) SendClientMessage(p, color, text);
	return 1;
}

// ============================================================
//  1) MDT
// ============================================================
stock Mdt_Can(playerid) return Fac_IsLaw(playerid);
stock Radio_Can(playerid)
{
	new t = Fac_Type(playerid);
	return (Fac_IsLaw(playerid) || t == FACTION_MEDIC || t == FACTION_GOVERNMENT);
}

stock Mdt_Home(playerid)
{
	new json[3600], part[200], name[MAX_PLAYER_NAME + 1], tn[16], n = 0;
	Law_TypeName(Fac_Type(playerid), tn, sizeof tn);
	C23_Name(playerid, name, sizeof name);
	new radar = 0;
	for (new k = 0; k < C23_RADARS; k++) if (g_C23RadarOwner[k] == playerid) radar = g_C23RadarLim[k];
	// [v25] tableau de bord : grade, unites en service, compteurs
	new rk[32], radars = 0, fines = 0, online = 0, f[16];
	Fac_RankName(Fac_Get(playerid), g_FacRank[playerid], rk, sizeof rk);
	for (new k = 0; k < C23_RADARS; k++) if (g_C23RadarOwner[k] != INVALID_PLAYER_ID) radars++;
	if (g_C23DB != DB:0)
	{
		new DBResult:r = db_query(g_C23DB, "SELECT COUNT(*) FROM c23_fines WHERE paid = 0");
		if (r != DBResult:0) { db_get_field(r, 0, f, sizeof f); fines = strval(f); db_free_result(r); }
	}
	for (new i = 0, m = GetMaxPlayers(); i < m; i++) if (IsPlayerConnected(i) && !IsPlayerNPC(i)) online++;
	format(json, sizeof json, "{\"m\":\"mdt\",\"v\":\"home\",\"tn\":\"%s\",\"me\":\"%s\",\"id\":%d,\"rank\":\"%s\",\"fines\":%d,\"radars\":%d,\"online\":%d,\"radar\":%d,\"cam\":%d,\"units\":[",
		tn, name, playerid, rk, fines, radars, online, radar, g_C23Cam[playerid]);
	for (new i = 0, m = GetMaxPlayers(); i < m && n < 10; i++)
	{
		if (!IsPlayerConnected(i) || !g_AuthLogged[i] || !Fac_IsLaw(i)) continue;
		new utn[16], un[MAX_PLAYER_NAME + 1];
		Law_TypeName(Fac_Type(i), utn, sizeof utn);
		C23_Name(i, un, sizeof un);
		format(part, sizeof part, "%s[%d,\"%s\",\"%s\",%d]", n ? (",") : (""), i, un, utn, IsPlayerInAnyVehicle(i) ? 1 : 0);
		strcat(json, part, sizeof json);
		n++;
	}
	n = 0;
	strcat(json, "],\"wanted\":[", sizeof json);
	for (new i = 0, m = GetMaxPlayers(); i < m && n < 8; i++)
	{
		if (!IsPlayerConnected(i) || GetPlayerWantedLevel(i) <= 0) continue;
		C23_Name(i, name, sizeof name);
		format(part, sizeof part, "%s[%d,\"%s\",%d,\"%s\"]", n ? (",") : (""), i, name, GetPlayerWantedLevel(i), g_LawReason[i]);
		strcat(json, part, sizeof json);
		n++;
	}
	strcat(json, "],\"chat\":[", sizeof json);
	n = 0;
	for (new k = 0; k < C23_CHAT; k++)
	{
		new i = (g_C23ChatHead + k) % C23_CHAT;
		if (!g_C23ChatT[i][0]) continue;
		format(part, sizeof part, "%s[\"%s\",\"%s\",\"%s\"]", n ? (",") : (""), g_C23ChatF[i], g_C23ChatN[i], g_C23ChatT[i]);
		if (strlen(json) + strlen(part) > sizeof(json) - 400) break;
		strcat(json, part, sizeof json);
		n++;
	}
	strcat(json, "],\"cams\":[", sizeof json);
	for (new c = 0; c < sizeof(g_C23CamName); c++)
	{
		format(part, sizeof part, "%s\"%s\"", c ? (",") : (""), g_C23CamName[c]);
		strcat(json, part, sizeof json);
	}
	strcat(json, "]}", sizeof json);
	return C23_Send(playerid, json);
}

stock Fine_Sum(const name[], &count)
{
	count = 0;
	if (g_C23DB == DB:0) return 0;
	new q[160], f[16], total = 0;
	format(q, sizeof q, "SELECT COUNT(*), SUM(amount) FROM c23_fines WHERE name = '%s' AND paid = 0", name);
	new DBResult:r = db_query(g_C23DB, q);
	if (r != DBResult:0)
	{
		db_get_field(r, 0, f, sizeof f); count = strval(f);
		db_get_field(r, 1, f, sizeof f); total = strval(f);
		db_free_result(r);
	}
	return total;
}

stock Mdt_Person(playerid, const name[], online)
{
	new json[2400], part[180], cnt, total = Fine_Sum(name, cnt), sp[MAX_PLAYER_NAME + 1];
	format(sp, sizeof sp, "%s", name);
	for (new i = 0; sp[i]; i++) if (sp[i] == '_') sp[i] = ' ';
	new lvl = (online != INVALID_PLAYER_ID) ? g_VieLvl[online] : -1, wl = (online != INVALID_PLAYER_ID) ? GetPlayerWantedLevel(online) : 0, wr[64];
	wr[0] = 0;
	if (online != INVALID_PLAYER_ID) format(wr, sizeof wr, "%s", g_LawReason[online]);
	format(json, sizeof json, "{\"m\":\"mdt\",\"v\":\"person\",\"p\":{\"name\":\"%s\",\"id\":%d,\"lvl\":%d,\"wl\":%d,\"wr\":\"%s\",\"fines\":%d,\"fsum\":%d,\"veh\":[",
		sp, (online != INVALID_PLAYER_ID) ? online : -1, lvl, wl, wr, cnt, total);
	if (g_GarDB != DB:0)
	{
		new q[180];
		format(q, sizeof q, "SELECT id, model, plate FROM vehicles WHERE owner = '%s' LIMIT 6", name);
		new DBResult:r = db_query(g_GarDB, q);
		if (r != DBResult:0)
		{
			new f[12], pl[16], ptxt[20], vn[32];
			for (new i = 0, n = db_num_rows(r); i < n; i++)
			{
				db_get_field(r, 0, f, sizeof f); new id = strval(f);
				db_get_field(r, 1, f, sizeof f); new model = strval(f);
				db_get_field(r, 2, pl, sizeof pl);
				Gar_PlateText(id, pl, ptxt, sizeof ptxt);
				Gar_VehName(model, vn, sizeof vn);
				format(part, sizeof part, "%s[\"%s\",\"%s\"]", i ? (",") : (""), ptxt, vn);
				strcat(json, part, sizeof json);
				db_next_row(r);
			}
			db_free_result(r);
		}
	}
	strcat(json, "],\"list\":[", sizeof json);
	if (g_C23DB != DB:0)
	{
		new q[200];
		format(q, sizeof q, "SELECT id, amount, reason FROM c23_fines WHERE name = '%s' AND paid = 0 ORDER BY id DESC LIMIT 5", name);
		new DBResult:r = db_query(g_C23DB, q);
		if (r != DBResult:0)
		{
			new f[12], rs[64];
			for (new i = 0, n = db_num_rows(r); i < n; i++)
			{
				db_get_field(r, 0, f, sizeof f); new id = strval(f);
				db_get_field(r, 1, f, sizeof f); new am = strval(f);
				db_get_field(r, 2, rs, sizeof rs);
				format(part, sizeof part, "%s[%d,%d,\"%s\"]", i ? (",") : (""), id, am, rs);
				strcat(json, part, sizeof json);
				db_next_row(r);
			}
			db_free_result(r);
		}
	}
	strcat(json, "]}}", sizeof json);
	return C23_Send(playerid, json);
}

stock Mdt_Find(playerid, const text[])
{
	if (strlen(text) < 2) return Cw_Notify(playerid, "warning", "MDT", "Ecris au moins 2 lettres (ou l'ID).", 3000), 1;
	new clean[MAX_PLAYER_NAME + 1], j = 0;
	for (new i = 0; text[i] && j < MAX_PLAYER_NAME; i++)
	{
		new c = text[i];
		if (c == ' ') c = '_';
		if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '_') clean[j++] = c;
	}
	clean[j] = 0;
	if (!clean[0]) return 1;
	new isnum = 1;
	for (new i = 0; clean[i]; i++) if (clean[i] < '0' || clean[i] > '9') isnum = 0;
	new name[MAX_PLAYER_NAME + 1];
	if (isnum)
	{
		new id = strval(clean);
		if (id < 0 || id >= MAX_PLAYERS || !IsPlayerConnected(id)) return Cw_Notify(playerid, "denied", "MDT", "Aucun joueur avec cet ID.", 3000), 1;
		C23_Name(id, name, sizeof name);
		return Mdt_Person(playerid, name, id);
	}
	for (new i = 0, m = GetMaxPlayers(); i < m; i++)
	{
		if (!IsPlayerConnected(i)) continue;
		C23_Name(i, name, sizeof name);
		if (strfind(name, clean, true) != -1) return Mdt_Person(playerid, name, i);
	}
	if (g_BankDB != DB:0)
	{
		new q[160];
		format(q, sizeof q, "SELECT name FROM bank_accounts WHERE name LIKE '%%%s%%' LIMIT 1", clean);
		new DBResult:r = db_query(g_BankDB, q);
		if (r != DBResult:0)
		{
			if (db_num_rows(r)) { db_get_field(r, 0, name, sizeof name); db_free_result(r); return Mdt_Person(playerid, name, INVALID_PLAYER_ID); }
			db_free_result(r);
		}
	}
	return Cw_Notify(playerid, "denied", "MDT", "Aucun citoyen trouve.", 3000), 1;
}

stock Mdt_Plate(playerid, const text[])
{
	if (g_GarDB == DB:0) return 1;
	new p[12], j = 0;
	for (new i = 0; text[i] && j < 10; i++)
	{
		new c = toupper(text[i]);
		if ((c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '-') p[j++] = c;
	}
	p[j] = 0;
	if (strlen(p) < 3) return Cw_Notify(playerid, "warning", "MDT", "Plaque trop courte.", 3000), 1;
	new q[200];
	if (!strcmp(p, "CW-", false, 3) && strlen(p) >= 4) format(q, sizeof q, "SELECT id, owner, model, plate FROM vehicles WHERE (id = %d AND plate = '') OR plate = '%s' LIMIT 1", strval(p[3]), p);
	else format(q, sizeof q, "SELECT id, owner, model, plate FROM vehicles WHERE plate = '%s' LIMIT 1", p);
	new DBResult:r = db_query(g_GarDB, q);
	if (r == DBResult:0) return 1;
	if (!db_num_rows(r)) { db_free_result(r); return Cw_Notify(playerid, "denied", "MDT", "Plaque inconnue au fichier.", 3000), 1; }
	new f[12], owner[MAX_PLAYER_NAME + 1], pl[16], ptxt[20], vn[32], json[300];
	db_get_field(r, 0, f, sizeof f); new id = strval(f);
	db_get_field(r, 1, owner, sizeof owner);
	db_get_field(r, 2, f, sizeof f); new model = strval(f);
	db_get_field(r, 3, pl, sizeof pl);
	db_free_result(r);
	Gar_PlateText(id, pl, ptxt, sizeof ptxt);
	Gar_VehName(model, vn, sizeof vn);
	new on = Cw_PlayerByName(owner);
	for (new i = 0; owner[i]; i++) if (owner[i] == '_') owner[i] = ' ';
	format(json, sizeof json, "{\"m\":\"mdt\",\"v\":\"plate\",\"r\":{\"plate\":\"%s\",\"veh\":\"%s\",\"owner\":\"%s\",\"id\":%d}}", ptxt, vn, owner, (on != INVALID_PLAYER_ID) ? on : -1);
	return C23_Send(playerid, json);
}

stock Fine_Give(playerid, target, amount, const reason[])
{
	if (target < 0 || target >= MAX_PLAYERS || !IsPlayerConnected(target)) return Cw_Notify(playerid, "denied", "MDT", "Joueur introuvable.", 3000), 0;
	if (amount < 50 || amount > 50000) return Cw_Notify(playerid, "denied", "MDT", "Amende : 50 a 50 000 $.", 3000), 0;
	new rs[64], name[MAX_PLAYER_NAME + 1], by[MAX_PLAYER_NAME + 1], q[300], m[180], a[16];
	Law_Clean(reason, rs, sizeof rs);
	if (strlen(rs) < 3) format(rs, sizeof rs, "Infraction");
	C23_Name(target, name, sizeof name);
	if (playerid != INVALID_PLAYER_ID) C23_Name(playerid, by, sizeof by); else format(by, sizeof by, "Radar");
	format(q, sizeof q, "INSERT INTO c23_fines (name, amount, reason, author, ts) VALUES ('%s', %d, '%s', '%s', %d)", name, amount, rs, by, gettime());
	C23_Exec(q);
	Cw_Fmt(amount, a, sizeof a);
	format(m, sizeof m, "Amende de %s $ : %s", a, rs);
	Jus_Record(name, "amende", m, by);   // [v41] casier judiciaire
	format(m, sizeof m, "Amende de ~r~%s $~w~ : %s. Paie avec ~b~/payeramende tout~w~.", a, rs);
	Cw_Notify(target, "denied", "Amende", m, 8000);
	if (playerid != INVALID_PLAYER_ID) { format(m, sizeof m, "Amende de %s $ envoyee.", a); Cw_Notify(playerid, "success", "MDT", m, 3500); }
	return 1;
}

stock Fine_List(playerid)
{
	if (g_C23DB == DB:0) return 1;
	new name[MAX_PLAYER_NAME + 1], q[200], f[12], rs[64], m[150];
	C23_Name(playerid, name, sizeof name);
	format(q, sizeof q, "SELECT id, amount, reason FROM c23_fines WHERE name = '%s' AND paid = 0 ORDER BY id LIMIT 8", name);
	new DBResult:r = db_query(g_C23DB, q);
	if (r == DBResult:0) return 1;
	new n = db_num_rows(r);
	if (!n) { db_free_result(r); return Cw_Notify(playerid, "success", "Amendes", "Aucune amende a payer.", 3500), 1; }
	SendClientMessage(playerid, C23_COL, "---- Tes amendes (/payeramende <n> ou /payeramende tout) ----");
	for (new i = 0; i < n; i++)
	{
		db_get_field(r, 0, f, sizeof f); new id = strval(f);
		db_get_field(r, 1, f, sizeof f); new am = strval(f);
		db_get_field(r, 2, rs, sizeof rs);
		format(m, sizeof m, "N.%d - %d $ - %s", id, am, rs);
		SendClientMessage(playerid, 0xFFFFFFFF, m);
		db_next_row(r);
	}
	db_free_result(r);
	return 1;
}

stock Fine_Pay(playerid, const arg[])
{
	if (g_C23DB == DB:0) return 1;
	new name[MAX_PLAYER_NAME + 1], q[200], cnt;
	C23_Name(playerid, name, sizeof name);
	if (!strcmp(arg, "tout", true))
	{
		new total = Fine_Sum(name, cnt);
		if (!cnt) return Cw_Notify(playerid, "success", "Amendes", "Aucune amende a payer.", 3500), 1;
		if (GetPlayerMoney(playerid) < total) return Cw_Notify(playerid, "denied", "Amendes", "Pas assez de liquide sur toi.", 3500), 1;
		GivePlayerMoney(playerid, -total);
		format(q, sizeof q, "UPDATE c23_fines SET paid = 1 WHERE name = '%s' AND paid = 0", name);
		C23_Exec(q);
		return Cw_Notify(playerid, "success", "Amendes", "Toutes tes amendes sont payees.", 4000), 1;
	}
	new id = strval(arg);
	format(q, sizeof q, "SELECT amount FROM c23_fines WHERE id = %d AND name = '%s' AND paid = 0", id, name);
	new DBResult:r = db_query(g_C23DB, q), f[12];
	if (r == DBResult:0) return 1;
	if (!db_num_rows(r)) { db_free_result(r); return Cw_Notify(playerid, "denied", "Amendes", "Amende introuvable (/amendes).", 3500), 1; }
	db_get_field(r, 0, f, sizeof f); db_free_result(r);
	new am = strval(f);
	if (GetPlayerMoney(playerid) < am) return Cw_Notify(playerid, "denied", "Amendes", "Pas assez de liquide sur toi.", 3500), 1;
	GivePlayerMoney(playerid, -am);
	format(q, sizeof q, "UPDATE c23_fines SET paid = 1 WHERE id = %d", id);
	C23_Exec(q);
	return Cw_Notify(playerid, "success", "Amendes", "Amende payee.", 3500), 1;
}

stock Radio_Say(playerid, const text[])
{
	if (!Radio_Can(playerid)) return Cw_Notify(playerid, "denied", "Radio", "Reserve aux forces de l'ordre, EMS et gouvernement.", 3000), 1;
	new t[96], name[MAX_PLAYER_NAME + 1], tn[16], m[180];
	Law_Clean(text, t, sizeof t);
	if (strlen(t) < 1) return 1;
	C23_Name(playerid, name, sizeof name);
	new ft = Fac_Type(playerid);
	if (ft == FACTION_MEDIC) format(tn, sizeof tn, "EMS"); else if (ft == FACTION_GOVERNMENT) format(tn, sizeof tn, "Gouv"); else Law_TypeName(ft, tn, sizeof tn);
	new i = g_C23ChatHead;
	format(g_C23ChatF[i], 16, "%s", tn);
	format(g_C23ChatN[i], MAX_PLAYER_NAME + 1, "%s", name);
	format(g_C23ChatT[i], 96, "%s", t);
	g_C23ChatHead = (g_C23ChatHead + 1) % C23_CHAT;
	format(m, sizeof m, "[Radio inter-factions] %s | %s : %s", tn, name, t);
	for (new p = 0, mx = GetMaxPlayers(); p < mx; p++) if (IsPlayerConnected(p) && Radio_Can(p)) SendClientMessage(p, C23_COL_RADIO, m);
	return 1;
}

stock Cam_Set(playerid, n)
{
	if (n < 0 || n >= sizeof(g_C23Cams)) return 1;
	if (IsPlayerInAnyVehicle(playerid)) return Cw_Notify(playerid, "denied", "Cameras", "Descends du vehicule pour regarder les cameras.", 3000), 1;
	g_C23Cam[playerid] = n + 1;
	TogglePlayerControllable(playerid, 0);
	SetPlayerCameraPos(playerid, g_C23Cams[n][0], g_C23Cams[n][1], g_C23Cams[n][2]);
	SetPlayerCameraLookAt(playerid, g_C23Cams[n][3], g_C23Cams[n][4], g_C23Cams[n][5]);
	new m[80];
	format(m, sizeof m, "~w~CAMERA : ~b~%s", g_C23CamName[n]);
	GameTextForPlayer(playerid, m, 2500, 4);
	return 1;
}

stock Cam_Off(playerid)
{
	if (!g_C23Cam[playerid]) return 1;
	g_C23Cam[playerid] = 0;
	SetCameraBehindPlayer(playerid);
	TogglePlayerControllable(playerid, 1);
	return 1;
}

stock Radar_Remove(k)
{
	if (g_C23RadarObj[k] != INVALID_OBJECT_ID) DestroyObject(g_C23RadarObj[k]);
	g_C23RadarObj[k] = INVALID_OBJECT_ID;
	g_C23RadarOwner[k] = INVALID_PLAYER_ID;
	return 1;
}

stock Radar_Toggle(playerid, limit)
{
	for (new k = 0; k < C23_RADARS; k++) if (g_C23RadarOwner[k] == playerid) { Radar_Remove(k); return Cw_Notify(playerid, "info", "Radar", "Radar retire.", 3000), 1; }
	if (IsPlayerInAnyVehicle(playerid)) return Cw_Notify(playerid, "denied", "Radar", "Pose le radar a pied, au bord de la route.", 3000), 1;
	if (limit < 30 || limit > 250) limit = 90;
	for (new k = 0; k < C23_RADARS; k++)
	{
		if (g_C23RadarOwner[k] != INVALID_PLAYER_ID) continue;
		new Float:x, Float:y, Float:z, Float:a;
		GetPlayerPos(playerid, x, y, z);
		GetPlayerFacingAngle(playerid, a);
		g_C23Radar[k][0] = x; g_C23Radar[k][1] = y; g_C23Radar[k][2] = z;
		g_C23RadarLim[k] = limit; g_C23RadarOwner[k] = playerid; g_C23RadarEnd[k] = gettime() + 1800;
		g_C23RadarObj[k] = CreateObject(1238, x, y, z - 0.7, 0.0, 0.0, a, 150.0);   // cone = radar pose
		new m[96];
		format(m, sizeof m, "Radar pose : limite ~b~%d km/h~w~ (30 min). /radar pour le retirer.", limit);
		return Cw_Notify(playerid, "success", "Radar", m, 5000), 1;
	}
	return Cw_Notify(playerid, "denied", "Radar", "Trop de radars en ville (8 max).", 3000), 1;
}

stock Radar_Check()
{
	new now = gettime();
	for (new k = 0; k < C23_RADARS; k++)
	{
		if (g_C23RadarOwner[k] == INVALID_PLAYER_ID) continue;
		if (now > g_C23RadarEnd[k]) { Radar_Remove(k); continue; }
		for (new p = 0, m = GetMaxPlayers(); p < m; p++)
		{
			if (!IsPlayerConnected(p) || GetPlayerState(p) != PLAYER_STATE_DRIVER || Fac_IsLaw(p)) continue;
			if (!IsPlayerInRangeOfPoint(p, 22.0, g_C23Radar[k][0], g_C23Radar[k][1], g_C23Radar[k][2])) continue;
			if (now - g_C23Flash[p] < 30) continue;
			new Float:vx, Float:vy, Float:vz;
			GetVehicleVelocity(GetPlayerVehicleID(p), vx, vy, vz);
			new kmh = floatround(floatsqroot(vx * vx + vy * vy + vz * vz) * 180.0);
			if (kmh <= g_C23RadarLim[k] + 5) continue;
			g_C23Flash[p] = now;
			new over = kmh - g_C23RadarLim[k], amount = 250 + over * 10, rs[64], msg[120], name[MAX_PLAYER_NAME + 1];
			if (amount > 5000) amount = 5000;
			format(msg, sizeof msg, "~w~FLASH ~r~%d km/h ~w~(limite %d)", kmh, g_C23RadarLim[k]);
			GameTextForPlayer(p, msg, 3000, 3);
			PlayerPlaySound(p, 1132, 0.0, 0.0, 0.0);
			format(rs, sizeof rs, "Radar : %d km/h (limite %d)", kmh, g_C23RadarLim[k]);
			Fine_Give(INVALID_PLAYER_ID, p, amount, rs);
			C23_Name(p, name, sizeof name);
			format(msg, sizeof msg, "[Radar] %s flashe a %d km/h (amende %d $).", name, kmh, amount);
			SendClientMessage(g_C23RadarOwner[k], C23_COL, msg);
		}
	}
	return 1;
}

stock Mdt_Action(playerid, const act[], const rest[])
{
	if (!Mdt_Can(playerid) && strcmp(act, "say", true)) return Cw_Notify(playerid, "denied", "MDT", "Reserve aux forces de l'ordre.", 3000), 1;
	new idx = 0, a1[24], a2[16];
	if (!strcmp(act, "home", true)) return Mdt_Home(playerid);
	if (!strcmp(act, "find", true)) return Mdt_Find(playerid, rest);
	if (!strcmp(act, "plate", true)) return Mdt_Plate(playerid, rest);
	if (!strcmp(act, "say", true)) { Radio_Say(playerid, rest); return Mdt_Can(playerid) ? Mdt_Home(playerid) : 1; }
	if (!strcmp(act, "cam", true)) return Cam_Set(playerid, strval(rest));
	if (!strcmp(act, "camoff", true)) return Cam_Off(playerid);
	if (!strcmp(act, "radar", true)) { Radar_Toggle(playerid, strval(rest)); return Mdt_Home(playerid); }
	Bank_NextToken(rest, idx, a1, sizeof a1);
	Bank_NextToken(rest, idx, a2, sizeof a2);
	while (rest[idx] == ' ') idx++;
	new t = strval(a1);
	if (!strcmp(act, "fine", true)) { Fine_Give(playerid, t, strval(a2), rest[idx]); return 1; }
	if (!strcmp(act, "want", true)) { Law_SetWanted(playerid, t, strval(a2), rest[idx]); return Mdt_Home(playerid); }
	if (!strcmp(act, "gps", true))
	{
		if (t < 0 || t >= MAX_PLAYERS || !IsPlayerConnected(t)) return Cw_Notify(playerid, "denied", "MDT", "Joueur hors ligne.", 3000), 1;
		new name[MAX_PLAYER_NAME + 1], cnt;
		C23_Name(t, name, sizeof name);
		Fine_Sum(name, cnt);
		if (GetPlayerWantedLevel(t) <= 0 && cnt <= 0) return Cw_Notify(playerid, "denied", "MDT", "Localisation : seulement un suspect recherche ou avec une amende impayee.", 4500), 1;
		new Float:x, Float:y, Float:z;
		GetPlayerPos(t, x, y, z);
		Gps_Set(playerid, x, y);
		return Cw_Notify(playerid, "success", "MDT", "Position envoyee au GPS.", 3000), 1;
	}
	return 1;
}

// ============================================================
//  2) ARMEE
// ============================================================
stock Army_Panel(playerid)
{
	new json[260], v = GetPlayerVehicleID(playerid);
	format(json, sizeof json, "{\"m\":\"army\",\"army\":%d,\"police\":%d,\"fbi\":%d,\"gov\":%d,\"cargo\":%d,\"me\":%d}",
		g_C23Stock[FACTION_ARMY], g_C23Stock[FACTION_POLICE], g_C23Stock[FACTION_FEDERAL], g_C23Stock[FACTION_GOVERNMENT],
		(v > 0 && v <= MAX_VEHICLES) ? g_C23Cargo[v] : 0, Fac_Type(playerid));
	return C23_Send(playerid, json);
}

stock Army_Load(playerid)
{
	if (Fac_Type(playerid) != FACTION_ARMY) return Cw_Notify(playerid, "denied", "Armee", "Reserve a l'armee.", 3000), 1;
	if (GetPlayerState(playerid) != PLAYER_STATE_DRIVER) return Cw_Notify(playerid, "denied", "Armee", "Monte au volant d'un vehicule.", 3000), 1;
	if (!C23_Near(playerid, C23_DEPOT_X, C23_DEPOT_Y, C23_DEPOT_Z, 12.0)) { Gps_Set(playerid, C23_DEPOT_X, C23_DEPOT_Y); return Cw_Notify(playerid, "warning", "Armee", "Va au depot de munitions (GPS).", 3500), 1; }
	if (gettime() < g_C23LoadCd[playerid]) return Cw_Notify(playerid, "warning", "Armee", "Le depot prepare la prochaine cargaison (5 min).", 3500), 1;
	new v = GetPlayerVehicleID(playerid);
	g_C23Cargo[v] = 500; g_C23CargoBy[v] = playerid;
	g_C23LoadCd[playerid] = gettime() + 300;
	Gps_Set(playerid, C23_BASE_X, C23_BASE_Y);
	return Cw_Notify(playerid, "success", "Armee", "500 munitions chargees. Livre-les a la base (GPS) : ~b~/livrer~w~. Attention aux gangs !", 6000), 1;
}

stock Army_Deliver(playerid)
{
	new v = GetPlayerVehicleID(playerid);
	if (v <= 0 || v > MAX_VEHICLES || g_C23Cargo[v] <= 0) return Cw_Notify(playerid, "denied", "Livraison", "Ce vehicule ne transporte rien.", 3000), 1;
	new q = g_C23Cargo[v];
	if (Fac_Type(playerid) == FACTION_ARMY)
	{
		if (!C23_Near(playerid, C23_BASE_X, C23_BASE_Y, C23_BASE_Z, 14.0)) { Gps_Set(playerid, C23_BASE_X, C23_BASE_Y); return Cw_Notify(playerid, "warning", "Armee", "Va a la base militaire (GPS).", 3500), 1; }
		g_C23Cargo[v] = 0;
		g_C23Stock[FACTION_ARMY] += q;
		C23_SaveStock(FACTION_ARMY);
		GivePlayerMoney(playerid, 1500);
		new m[120];
		format(m, sizeof m, "%d munitions livrees (stock : %d). Prime : ~g~1 500 $~w~.", q, g_C23Stock[FACTION_ARMY]);
		return Cw_Notify(playerid, "success", "Armee", m, 5000), 1;
	}
	if (Fac_IsLaw(playerid)) return Cw_Notify(playerid, "denied", "Livraison", "Ramene-la a la base militaire.", 3000), 1;
	if (!C23_Near(playerid, C23_RECEL_X, C23_RECEL_Y, C23_RECEL_Z, 12.0)) { Gps_Set(playerid, C23_RECEL_X, C23_RECEL_Y); return Cw_Notify(playerid, "warning", "Recel", "Va au point de recel (GPS).", 3500), 1; }
	g_C23Cargo[v] = 0;
	g_C23Mats[playerid] += q / 5;
	C23_SaveMats(playerid);
	new m[120];
	format(m, sizeof m, "Cargaison revendue : ~g~+%d materiaux~w~ (total %d). Va a l'etabli : ~b~/craft~w~.", q / 5, g_C23Mats[playerid]);
	Cw_Notify(playerid, "success", "Recel", m, 6000);
	C23_ToType(FACTION_ARMY, 0xFF6347FF, "[ARMEE] Une cargaison volee a ete revendue au marche noir.");
	return 1;
}

stock Army_Give(playerid, const who[], qty)
{
	if (Fac_Type(playerid) != FACTION_ARMY) return Cw_Notify(playerid, "denied", "Armee", "Reserve a l'armee.", 3000), 1;
	if (!C23_Near(playerid, C23_BASE_X, C23_BASE_Y, C23_BASE_Z, 30.0)) return Cw_Notify(playerid, "warning", "Armee", "A faire depuis la base militaire.", 3000), 1;
	new t = -1;
	if (!strcmp(who, "police", true)) t = FACTION_POLICE;
	else if (!strcmp(who, "fbi", true)) t = FACTION_FEDERAL;
	else if (!strcmp(who, "gov", true) || !strcmp(who, "gouv", true)) t = FACTION_GOVERNMENT;
	if (t == -1 || qty <= 0) return SendClientMessage(playerid, C23_COL, "/fournir police|fbi|gov <quantite>"), 1;
	if (qty > g_C23Stock[FACTION_ARMY]) return Cw_Notify(playerid, "denied", "Armee", "Stock de l'armee insuffisant.", 3000), 1;
	g_C23Stock[FACTION_ARMY] -= qty; g_C23Stock[t] += qty;
	C23_SaveStock(FACTION_ARMY); C23_SaveStock(t);
	new m[120];
	format(m, sizeof m, "[ARMEE] %d munitions fournies a votre faction (/armurerie).", qty);
	C23_ToType(t, 0x33CC33FF, m);
	Army_Panel(playerid);
	return Cw_Notify(playerid, "success", "Armee", "Munitions fournies.", 3000), 1;
}

stock Army_Armory(playerid)
{
	new t = Fac_Type(playerid);
	if (!(t == FACTION_POLICE || t == FACTION_FEDERAL || t == FACTION_GOVERNMENT || t == FACTION_ARMY)) return Cw_Notify(playerid, "denied", "Armurerie", "Reserve aux forces de l'ordre et au gouvernement.", 3000), 1;
	new w = GetPlayerWeapon(playerid);
	if (w < 22 || w > 34) return Cw_Notify(playerid, "warning", "Armurerie", "Prends une arme a feu en main d'abord.", 3000), 1;
	if (g_C23Stock[t] < 60) return Cw_Notify(playerid, "denied", "Armurerie", "Plus de munitions : demande a l'armee (/fournir).", 4000), 1;
	g_C23Stock[t] -= 60;
	C23_SaveStock(t);
	GivePlayerWeapon(playerid, w, 60);
	new m[80];
	format(m, sizeof m, "+60 munitions (stock de la faction : %d).", g_C23Stock[t]);
	return Cw_Notify(playerid, "success", "Armurerie", m, 3500), 1;
}

// ============================================================
//  3) EMS
// ============================================================
stock Ems_Release(medic)
{
	new p = g_C23Stretch[medic];
	g_C23Stretch[medic] = INVALID_PLAYER_ID;
	if (p == INVALID_PLAYER_ID || !IsPlayerConnected(p)) return 1;
	TogglePlayerControllable(p, 1);
	if (!IsPlayerInAnyVehicle(p)) ClearAnimations(p);
	Cw_Notify(p, "info", "EMS", "Tu es descendu de la civiere.", 3000);
	return 1;
}

stock Ems_Stretcher(playerid, target)
{
	if (Fac_Type(playerid) != FACTION_MEDIC) return Cw_Notify(playerid, "denied", "EMS", "Reserve aux ambulanciers.", 3000), 1;
	if (g_C23Stretch[playerid] != INVALID_PLAYER_ID) { Ems_Release(playerid); return Cw_Notify(playerid, "info", "EMS", "Patient pose.", 3000), 1; }
	if (target < 0 || target >= MAX_PLAYERS || !IsPlayerConnected(target) || target == playerid) return SendClientMessage(playerid, C23_COL, "/civiere <id> (le patient doit etre a moins de 3 m)"), 1;
	new Float:x, Float:y, Float:z;
	GetPlayerPos(target, x, y, z);
	if (!IsPlayerInRangeOfPoint(playerid, 3.0, x, y, z)) return Cw_Notify(playerid, "denied", "EMS", "Le patient est trop loin.", 3000), 1;
	g_C23Stretch[playerid] = target;
	TogglePlayerControllable(target, 0);
	ApplyAnimation(target, "CRACK", "crckidle2", 4.1, 1, 0, 0, 0, 0);
	Cw_Notify(target, "info", "EMS", "Un ambulancier t'a mis sur une civiere.", 4000);
	return Cw_Notify(playerid, "success", "EMS", "Patient sur la civiere : il te suit et monte dans l'ambulance avec toi. ~b~/civiere~w~ pour le poser.", 6000), 1;
}

stock Ems_Follow()
{
	for (new m = 0, mx = GetMaxPlayers(); m < mx; m++)
	{
		new p = g_C23Stretch[m];
		if (p == INVALID_PLAYER_ID) continue;
		if (!IsPlayerConnected(m) || !IsPlayerConnected(p)) { g_C23Stretch[m] = INVALID_PLAYER_ID; continue; }
		if (IsPlayerInAnyVehicle(m))
		{
			new v = GetPlayerVehicleID(m);
			if (!IsPlayerInVehicle(p, v)) PutPlayerInVehicle(p, v, 2);
			continue;
		}
		if (IsPlayerInAnyVehicle(p)) { RemovePlayerFromVehicle(p); continue; }
		new Float:x, Float:y, Float:z, Float:a;
		GetPlayerPos(m, x, y, z);
		GetPlayerFacingAngle(m, a);
		SetPlayerInterior(p, GetPlayerInterior(m));
		SetPlayerVirtualWorld(p, GetPlayerVirtualWorld(m));
		SetPlayerPos(p, x + 1.3 * floatsin(-a, degrees), y + 1.3 * floatcos(-a, degrees), z - 0.4);
		SetPlayerFacingAngle(p, a + 90.0);
	}
	return 1;
}

stock Ems_Surgery(playerid, target)
{
	if (Fac_Type(playerid) != FACTION_MEDIC) return Cw_Notify(playerid, "denied", "EMS", "Reserve aux medecins.", 3000), 1;
	if (target < 0 || target >= MAX_PLAYERS || !IsPlayerConnected(target) || target == playerid) return SendClientMessage(playerid, C23_COL, "/chirurgie <id> : changement de sexe (5 000 $ payes par le patient)"), 1;
	new Float:x, Float:y, Float:z;
	GetPlayerPos(target, x, y, z);
	if (!IsPlayerInRangeOfPoint(playerid, 3.0, x, y, z)) return Cw_Notify(playerid, "denied", "EMS", "Le patient est trop loin.", 3000), 1;
	g_C23Surg[target] = playerid;
	Cw_Notify(target, "info", "Chirurgie", "Un medecin te propose un changement de sexe (5 000 $). Accepte avec ~b~/accepterchirurgie~w~.", 9000);
	return Cw_Notify(playerid, "success", "Chirurgie", "Proposition envoyee au patient.", 3000), 1;
}

stock Ems_SurgeryAccept(playerid)
{
	new med = g_C23Surg[playerid];
	g_C23Surg[playerid] = INVALID_PLAYER_ID;
	if (med == INVALID_PLAYER_ID || !IsPlayerConnected(med)) return Cw_Notify(playerid, "denied", "Chirurgie", "Aucune proposition en cours.", 3000), 1;
	if (GetPlayerMoney(playerid) < 5000) return Cw_Notify(playerid, "denied", "Chirurgie", "Il te faut 5 000 $ en liquide.", 3000), 1;
	GivePlayerMoney(playerid, -5000);
	GivePlayerMoney(med, 2500);
	new skin = (g_ClothSkin[playerid] == CLOTH_SKIN_FEMALE) ? CLOTH_SKIN_MALE : CLOTH_SKIN_FEMALE;
	g_ClothSkin[playerid] = skin;
	SetPlayerSkin(playerid, skin);
	if (g_IdGender[playerid] >= 0) g_IdGender[playerid] = (skin == CLOTH_SKIN_FEMALE) ? 1 : 0;
	if (g_ClothDB != DB:0)
	{
		new name[MAX_PLAYER_NAME + 1], q[140];
		C23_Name(playerid, name, sizeof name);
		format(q, sizeof q, "UPDATE clothes SET skin = %d WHERE name = '%s'", skin, name);
		db_free_result(db_query(g_ClothDB, q));
	}
	Cw_Notify(med, "success", "Chirurgie", "Operation reussie : +2 500 $.", 4000);
	return Cw_Notify(playerid, "success", "Chirurgie", "Operation reussie. Passe au magasin de vetements pour ta nouvelle tenue.", 6000), 1;
}

// ============================================================
//  4) JOB ASSEMBLEUR DE VOITURES
// ============================================================
stock Asm_IsTeam(playerid) return (g_AsmTeam[0] == playerid || g_AsmTeam[1] == playerid);

stock Asm_Push()
{
	new json[300], a[MAX_PLAYER_NAME + 1], b[MAX_PLAYER_NAME + 1], nx[24], cl[12];
	a[0] = 0; b[0] = 0; nx[0] = 0; cl[0] = 0;
	if (g_AsmParts < 6) format(nx, sizeof nx, "%s", g_C23PartName[g_AsmParts]);
	if (g_AsmCol >= 0) format(cl, sizeof cl, "%s", g_C23ColName[g_AsmCol]);
	if (g_AsmTeam[0] != INVALID_PLAYER_ID) C23_Name(g_AsmTeam[0], a, sizeof a);
	if (g_AsmTeam[1] != INVALID_PLAYER_ID) C23_Name(g_AsmTeam[1], b, sizeof b);
	format(json, sizeof json, "{\"m\":\"asm\",\"on\":%d,\"parts\":%d,\"total\":6,\"next\":\"%s\",\"paint\":%d,\"color\":\"%s\",\"team\":[\"%s\",\"%s\"]}",
		(g_AsmCar != INVALID_VEHICLE_ID) ? 1 : 0, g_AsmParts, nx, g_AsmPaint, cl, a, b);
	for (new k = 0; k < 2; k++) if (g_AsmTeam[k] != INVALID_PLAYER_ID) C23_Send(g_AsmTeam[k], json);
	return 1;
}

stock Asm_Reset()
{
	if (g_AsmCar != INVALID_VEHICLE_ID) DestroyVehicle(g_AsmCar);
	g_AsmCar = INVALID_VEHICLE_ID; g_AsmParts = 0; g_AsmPaint = 0; g_AsmCol = -1;
	for (new k = 0; k < 2; k++)
	{
		new p = g_AsmTeam[k];
		if (p != INVALID_PLAYER_ID && IsPlayerConnected(p))
		{
			if (g_AsmCarry[p]) { g_AsmCarry[p] = 0; RemovePlayerAttachedObject(p, C23_ATTACH); }
			C23_Send(p, "{\"m\":\"asm\",\"on\":0}");
		}
		g_AsmTeam[k] = INVALID_PLAYER_ID;
	}
	return 1;
}

public Asm_Destroy(v)
{
	if (v != INVALID_VEHICLE_ID) DestroyVehicle(v);
	return 1;
}

stock Asm_Start(playerid, const arg[])
{
	if (!strcmp(arg, "quitter", true))
	{
		if (!Asm_IsTeam(playerid)) return 1;
		for (new k = 0; k < 2; k++) if (g_AsmTeam[k] == playerid) g_AsmTeam[k] = INVALID_PLAYER_ID;
		if (g_AsmCarry[playerid]) { g_AsmCarry[playerid] = 0; RemovePlayerAttachedObject(playerid, C23_ATTACH); }
		C23_Send(playerid, "{\"m\":\"asm\",\"on\":0}");
		if (g_AsmTeam[0] == INVALID_PLAYER_ID && g_AsmTeam[1] == INVALID_PLAYER_ID) Asm_Reset();
		return Cw_Notify(playerid, "info", "Assembleur", "Tu as quitte le poste.", 3000), 1;
	}
	if (!C23_Near(playerid, C23_ASM_X, C23_ASM_Y, C23_ASM_Z, 15.0)) { Gps_Set(playerid, C23_ASM_X, C23_ASM_Y); return Cw_Notify(playerid, "warning", "Assembleur", "Va a l'usine d'assemblage (GPS).", 3500), 1; }
	if (Asm_IsTeam(playerid)) return Asm_Push();
	if (g_AsmCar == INVALID_VEHICLE_ID)
	{
		new model = g_C23AsmModels[random(sizeof(g_C23AsmModels))];
		g_AsmCar = CreateVehicle(model, C23_ASM_X, C23_ASM_Y + 10.0, C23_ASM_Z + 0.4, 90.0, 1, 1, -1);
		SetVehicleParamsEx(g_AsmCar, 0, 0, 0, 1, 0, 0, 0);
		g_AsmParts = 0; g_AsmPaint = 0; g_AsmCol = -1;
		g_AsmTeam[0] = playerid; g_AsmTeam[1] = INVALID_PLAYER_ID;
		Cw_Notify(playerid, "success", "Assembleur", "Chassis en place. Prends une piece (~b~/piece~w~) et pose-la sur la voiture (~b~/poser~w~). Un collegue peut t'aider : /assembler.", 8000);
		return Asm_Push();
	}
	if (g_AsmTeam[1] == INVALID_PLAYER_ID) { g_AsmTeam[1] = playerid; Cw_Notify(playerid, "success", "Assembleur", "Tu rejoins l'equipe.", 3000); return Asm_Push(); }
	if (g_AsmTeam[0] == INVALID_PLAYER_ID) { g_AsmTeam[0] = playerid; Cw_Notify(playerid, "success", "Assembleur", "Tu rejoins l'equipe.", 3000); return Asm_Push(); }
	return Cw_Notify(playerid, "denied", "Assembleur", "Le poste est complet (2 personnes). Reviens plus tard.", 3500), 1;
}

stock Asm_Piece(playerid)
{
	if (!Asm_IsTeam(playerid) || g_AsmCar == INVALID_VEHICLE_ID) return Cw_Notify(playerid, "denied", "Assembleur", "Commence avec /assembler.", 3000), 1;
	if (g_AsmParts >= 6) return Cw_Notify(playerid, "info", "Assembleur", "Toutes les pieces sont posees : place a la peinture (/spray).", 3500), 1;
	if (g_AsmCarry[playerid]) return Cw_Notify(playerid, "warning", "Assembleur", "Tu portes deja une piece : /poser pres de la voiture.", 3000), 1;
	if (!C23_Near(playerid, C23_ASM_X - 10.0, C23_ASM_Y - 8.0, C23_ASM_Z, 4.0)) return Cw_Notify(playerid, "warning", "Assembleur", "Les pieces sont sur le rack ~b~Pieces~w~ de l'usine.", 3500), 1;
	g_AsmCarry[playerid] = 1;
	new model = (g_AsmParts >= 1 && g_AsmParts <= 4) ? 1098 : 2912;   // roue / caisse
	SetPlayerAttachedObject(playerid, C23_ATTACH, model, 5, 0.1, 0.05, 0.0, 0.0, 0.0, 0.0, 0.5, 0.5, 0.5);
	ApplyAnimation(playerid, "CARRY", "liftup", 4.1, 0, 0, 0, 0, 0);
	new m[96];
	format(m, sizeof m, "%s pris : ~b~/poser~w~ pres de la voiture.", g_C23PartName[g_AsmParts]);
	return Cw_Notify(playerid, "info", "Assembleur", m, 3500), 1;
}

stock Asm_Put(playerid)
{
	if (!g_AsmCarry[playerid] || g_AsmCar == INVALID_VEHICLE_ID) return 0;
	new Float:x, Float:y, Float:z;
	GetVehiclePos(g_AsmCar, x, y, z);
	if (!IsPlayerInRangeOfPoint(playerid, 4.5, x, y, z)) return Cw_Notify(playerid, "warning", "Assembleur", "Approche-toi de la voiture.", 3000), 1;
	g_AsmCarry[playerid] = 0;
	RemovePlayerAttachedObject(playerid, C23_ATTACH);
	ApplyAnimation(playerid, "CARRY", "putdwn", 4.1, 0, 0, 0, 0, 0);
	g_AsmParts++;
	if (g_AsmParts >= 6)
	{
		g_AsmCol = random(sizeof(g_C23ColId));
		new m[140];
		format(m, sizeof m, "Voiture assemblee ! Peins-la en ~b~%s~w~ : prends une bombe a l'entrepot (~b~/spray~w~) et vise la voiture.", g_C23ColName[g_AsmCol]);
		for (new k = 0; k < 2; k++) if (g_AsmTeam[k] != INVALID_PLAYER_ID) Cw_Notify(g_AsmTeam[k], "success", "Assembleur", m, 8000);
	}
	return Asm_Push();
}

stock Asm_Spray(playerid)
{
	if (!Asm_IsTeam(playerid) || g_AsmParts < 6) return Cw_Notify(playerid, "denied", "Assembleur", "La voiture n'est pas encore assemblee.", 3000), 1;
	if (!C23_Near(playerid, C23_ASM_X + 10.0, C23_ASM_Y - 8.0, C23_ASM_Z, 4.0)) return Cw_Notify(playerid, "warning", "Assembleur", "Les bombes sont sur le rack ~b~Peinture~w~.", 3500), 1;
	GivePlayerWeapon(playerid, 41, 2000);
	return Cw_Notify(playerid, "success", "Assembleur", "Bombe de peinture prise : vise la voiture et appuie sur tirer.", 4000), 1;
}

stock Asm_Tick()
{
	if (g_AsmCar == INVALID_VEHICLE_ID || g_AsmParts < 6 || g_AsmCol < 0) return 1;
	new Float:x, Float:y, Float:z, add = 0;
	GetVehiclePos(g_AsmCar, x, y, z);
	for (new k = 0; k < 2; k++)
	{
		new p = g_AsmTeam[k];
		if (p == INVALID_PLAYER_ID || !IsPlayerConnected(p)) continue;
		if (GetPlayerWeapon(p) != 41 || !IsPlayerInRangeOfPoint(p, 6.0, x, y, z)) continue;
		new keys, ud, lr;
		GetPlayerKeys(p, keys, ud, lr);
		if (keys & KEY_FIRE) add += 2;   // 10 % toutes les 5 s par bombe
	}
	if (!add) return 1;
	g_AsmPaint += add;
	if (g_AsmPaint < 100) return Asm_Push();
	g_AsmPaint = 100;
	ChangeVehicleColor(g_AsmCar, g_C23ColId[g_AsmCol], g_C23ColId[g_AsmCol]);
	for (new k = 0; k < 2; k++)
	{
		new p = g_AsmTeam[k];
		if (p == INVALID_PLAYER_ID || !IsPlayerConnected(p)) continue;
		new lvl = g_VieLvl[p];
		if (lvl < 1) lvl = 1;
		new pay = 15000 + (lvl - 1) * 1000, a[16], m[120];
		GivePlayerMoney(p, pay);
		Cw_Fmt(pay, a, sizeof a);
		format(m, sizeof m, "Voiture terminee et livree : ~g~+%s $~w~ (niveau %d).", a, lvl);
		Cw_Notify(p, "success", "Assembleur", m, 6000);
		C23_Send(p, "{\"m\":\"asm\",\"on\":0}");
		g_AsmTeam[k] = INVALID_PLAYER_ID;
	}
	SetTimerEx("Asm_Destroy", 8000, false, "i", g_AsmCar);
	g_AsmCar = INVALID_VEHICLE_ID; g_AsmParts = 0; g_AsmPaint = 0; g_AsmCol = -1;
	return 1;
}

// ============================================================
//  5) ARENE PVP (mapping prive : objets du joueur, monde 777)
// ============================================================
stock Arena_Say(const text[])
{
	for (new p = 0, m = GetMaxPlayers(); p < m; p++) if (IsPlayerConnected(p) && g_ArIn[p]) SendClientMessage(p, 0xFFB347FF, text);
	return 1;
}

stock Arena_Obj(playerid, model, Float:x, Float:y, Float:z, Float:rz)
{
	if (g_ArObjN[playerid] >= sizeof(g_ArObj[])) return 0;
	g_ArObj[playerid][g_ArObjN[playerid]++] = CreatePlayerObject(playerid, model, C23_ARENA_X + x, C23_ARENA_Y + y, C23_ARENA_Z + z, 0.0, 0.0, rz, 250.0);
	return 1;
}

stock Arena_Build(playerid)
{
	g_ArObjN[playerid] = 0;
	// murs : conteneurs autour d'un carre de 72 m
	for (new i = -3; i <= 2; i++)
	{
		Arena_Obj(playerid, 2934, i * 12.0 + 6.0, 36.0, 1.4, 0.0);
		Arena_Obj(playerid, 2934, i * 12.0 + 6.0, -36.0, 1.4, 0.0);
		Arena_Obj(playerid, 2934, 36.0, i * 12.0 + 6.0, 1.4, 90.0);
		Arena_Obj(playerid, 2934, -36.0, i * 12.0 + 6.0, 1.4, 90.0);
	}
	// abris au centre : caisses et conteneurs
	Arena_Obj(playerid, 2934, 0.0, 0.0, 1.4, 45.0);
	Arena_Obj(playerid, 2932, -14.0, 12.0, 1.4, 0.0);
	Arena_Obj(playerid, 2932, 14.0, -12.0, 1.4, 0.0);
	Arena_Obj(playerid, 2935, 12.0, 16.0, 1.4, 90.0);
	Arena_Obj(playerid, 2935, -12.0, -16.0, 1.4, 90.0);
	Arena_Obj(playerid, 3798, -22.0, 0.0, 0.0, 0.0);
	Arena_Obj(playerid, 3798, 22.0, 0.0, 0.0, 0.0);
	Arena_Obj(playerid, 3798, 0.0, 22.0, 0.0, 0.0);
	Arena_Obj(playerid, 3798, 0.0, -22.0, 0.0, 0.0);
	Arena_Obj(playerid, 3798, -22.0, 1.9, 0.0, 0.0);
	Arena_Obj(playerid, 3798, 22.0, -1.9, 0.0, 0.0);
	Arena_Obj(playerid, 3800, -6.0, -6.0, 0.0, 0.0);
	Arena_Obj(playerid, 3800, 6.0, 6.0, 0.0, 0.0);
	return 1;
}

stock Arena_Place(playerid)
{
	static const Float:sp[][3] = { {-28.0, -28.0, 45.0}, {28.0, 28.0, 225.0}, {-28.0, 28.0, 135.0}, {28.0, -28.0, 315.0} };
	new k = random(sizeof(sp));
	SetPlayerInterior(playerid, 0);
	SetPlayerVirtualWorld(playerid, C23_ARENA_VW);
	SetPlayerPos(playerid, C23_ARENA_X + sp[k][0], C23_ARENA_Y + sp[k][1], C23_ARENA_Z + 0.6);
	SetPlayerFacingAngle(playerid, sp[k][2]);
	SetCameraBehindPlayer(playerid);
	SetPlayerHealth(playerid, 100.0);
	SetPlayerArmour(playerid, 100.0);
	ResetPlayerWeapons(playerid);
	GivePlayerWeapon(playerid, 24, 200);
	GivePlayerWeapon(playerid, 25, 60);
	GivePlayerWeapon(playerid, 31, 300);
	return Arena_Push(playerid);
}

stock Arena_Push(playerid)
{
	new json[600], part[64], name[MAX_PLAYER_NAME + 1], used[MAX_PLAYERS], n = 0;
	format(json, sizeof json, "{\"m\":\"arena\",\"on\":%d,\"k\":%d,\"d\":%d,\"top\":[", g_ArIn[playerid], g_ArK[playerid], g_ArD[playerid]);
	for (new r = 0; r < 5; r++)
	{
		new best = -1;
		for (new p = 0, m = GetMaxPlayers(); p < m; p++)
			if (IsPlayerConnected(p) && g_ArIn[p] && !used[p] && (best == -1 || g_ArK[p] > g_ArK[best])) best = p;
		if (best == -1) break;
		used[best] = 1;
		C23_Name(best, name, sizeof name);
		format(part, sizeof part, "%s[\"%s\",%d,%d]", n ? (",") : (""), name, g_ArK[best], g_ArD[best]);
		strcat(json, part, sizeof json);
		n++;
	}
	strcat(json, "]}", sizeof json);
	return C23_Send(playerid, json);
}

stock Arena_Join(playerid)
{
	if (g_ArIn[playerid]) return Arena_Push(playerid);
	if (IsPlayerInAnyVehicle(playerid)) return Cw_Notify(playerid, "denied", "Arene", "Descends du vehicule.", 3000), 1;
	if (GetPlayerWantedLevel(playerid) > 0) return Cw_Notify(playerid, "denied", "Arene", "Impossible : tu es recherche.", 3000), 1;
	GetPlayerPos(playerid, g_ArBack[playerid][0], g_ArBack[playerid][1], g_ArBack[playerid][2]);
	GetPlayerFacingAngle(playerid, g_ArBack[playerid][3]);
	g_ArBackInt[playerid] = GetPlayerInterior(playerid);
	g_ArBackVw[playerid] = GetPlayerVirtualWorld(playerid);
	for (new s = 0; s < 13; s++) GetPlayerWeaponData(playerid, s, g_ArW[playerid][s][0], g_ArW[playerid][s][1]);
	g_ArIn[playerid] = 1;
	Arena_Build(playerid);
	Arena_Place(playerid);
	new m[96], name[MAX_PLAYER_NAME + 1];
	C23_Name(playerid, name, sizeof name);
	format(m, sizeof m, "[Arene] %s entre dans l'arene.", name);
	Arena_Say(m);
	return Cw_Notify(playerid, "info", "Arene PvP", "Bienvenue dans l'arene ! Armes fournies, reapparition automatique. ~b~/quitterarene~w~ pour sortir.", 6000), 1;
}

stock Arena_Leave(playerid, notify = 1)
{
	if (!g_ArIn[playerid]) return 1;
	g_ArIn[playerid] = 0;
	for (new i = 0; i < g_ArObjN[playerid]; i++) DestroyPlayerObject(playerid, g_ArObj[playerid][i]);
	g_ArObjN[playerid] = 0;
	C23_SaveMats(playerid);
	if (!notify) return 1;
	ResetPlayerWeapons(playerid);
	for (new s = 0; s < 13; s++) if (g_ArW[playerid][s][0] > 0 && g_ArW[playerid][s][1] > 0) GivePlayerWeapon(playerid, g_ArW[playerid][s][0], g_ArW[playerid][s][1]);
	SetPlayerInterior(playerid, g_ArBackInt[playerid]);
	SetPlayerVirtualWorld(playerid, g_ArBackVw[playerid]);
	SetPlayerPos(playerid, g_ArBack[playerid][0], g_ArBack[playerid][1], g_ArBack[playerid][2]);
	SetPlayerFacingAngle(playerid, g_ArBack[playerid][3]);
	SetCameraBehindPlayer(playerid);
	SetPlayerHealth(playerid, 100.0);
	C23_Send(playerid, "{\"m\":\"arena\",\"on\":0}");
	return Cw_Notify(playerid, "info", "Arene PvP", "Tu as quitte l'arene.", 3000), 1;
}

// ============================================================
//  6) CRAFT
// ============================================================
stock Craft_Panel(playerid)
{
	new json[200];
	format(json, sizeof json, "{\"m\":\"craft\",\"mats\":%d,\"drug\":%d,\"near\":%d}", g_C23Mats[playerid], g_C23Drug[playerid], C23_Near(playerid, C23_BENCH_X, C23_BENCH_Y, C23_BENCH_Z, 5.0));
	return C23_Send(playerid, json);
}

stock Craft_Make(playerid, const what[])
{
	if (Fac_IsLaw(playerid)) return Cw_Notify(playerid, "denied", "Craft", "Un agent ne fabrique pas d'armes.", 3000), 1;
	if (!C23_Near(playerid, C23_BENCH_X, C23_BENCH_Y, C23_BENCH_Z, 5.0)) { Gps_Set(playerid, C23_BENCH_X, C23_BENCH_Y); return Cw_Notify(playerid, "warning", "Craft", "Va a l'etabli (GPS).", 3500), 1; }
	new cost = 0, weapon = 0, ammo = 0, drug = 0;
	if (!strcmp(what, "ammo", true)) cost = 20;
	else if (!strcmp(what, "colt", true)) { cost = 150; weapon = 22; ammo = 60; }
	else if (!strcmp(what, "deagle", true)) { cost = 300; weapon = 24; ammo = 35; }
	else if (!strcmp(what, "uzi", true)) { cost = 350; weapon = 28; ammo = 120; }
	else if (!strcmp(what, "pompe", true)) { cost = 400; weapon = 25; ammo = 30; }
	else if (!strcmp(what, "ak", true)) { cost = 700; weapon = 30; ammo = 150; }
	else if (!strcmp(what, "drogue", true)) { cost = 30; drug = 1; }
	else return 1;
	if (g_C23Mats[playerid] < cost) return Cw_Notify(playerid, "denied", "Craft", "Pas assez de materiaux (recyclage, cargaisons militaires).", 4000), 1;
	if (!strcmp(what, "ammo", true))
	{
		new w = GetPlayerWeapon(playerid);
		if (w < 22 || w > 34) return Cw_Notify(playerid, "warning", "Craft", "Prends une arme a feu en main pour fabriquer ses balles.", 3500), 1;
		weapon = w; ammo = 50;
	}
	g_C23Mats[playerid] -= cost;
	if (weapon) GivePlayerWeapon(playerid, weapon, ammo);
	if (drug) g_C23Drug[playerid]++;
	C23_SaveMats(playerid);
	ApplyAnimation(playerid, "BD_FIRE", "wash_up", 4.1, 0, 0, 0, 0, 0);
	Cw_Notify(playerid, "success", "Craft", drug ? ("Drogue fabriquee : /drogue pour la consommer.") : ("Fabrication terminee."), 3500);
	return Craft_Panel(playerid);
}

stock Drug_Use(playerid)
{
	if (g_C23Drug[playerid] <= 0) return Cw_Notify(playerid, "denied", "Drogue", "Tu n'en as pas.", 3000), 1;
	g_C23Drug[playerid]--;
	C23_SaveMats(playerid);
	new Float:h, Float:ar;
	GetPlayerHealth(playerid, h);
	GetPlayerArmour(playerid, ar);
	SetPlayerHealth(playerid, (h + 25.0 > 100.0) ? 100.0 : h + 25.0);
	SetPlayerArmour(playerid, (ar + 15.0 > 100.0) ? 100.0 : ar + 15.0);
	SetPlayerDrunkLevel(playerid, 6000);
	ApplyAnimation(playerid, "SMOKING", "M_smk_drag", 4.1, 0, 0, 0, 0, 0);
	return Cw_Notify(playerid, "info", "Drogue", "+25 sante, +15 armure... la tete tourne un peu.", 4000), 1;
}

// ============================================================
//  7) RECYCLAGE
// ============================================================
stock Recy_Spawn(b)
{
	new Float:a = float(random(360)), Float:r = 12.0 + float(random(28));
	g_RecyPos[b][0] = C23_RECY_X + r * floatsin(a, degrees);
	g_RecyPos[b][1] = C23_RECY_Y + r * floatcos(a, degrees);
	g_RecyPos[b][2] = C23_RECY_Z - 0.75;
	g_RecyObj[b] = CreateObject(1265, g_RecyPos[b][0], g_RecyPos[b][1], g_RecyPos[b][2], 0.0, 0.0, float(random(360)), 80.0);   // sac poubelle
	g_RecyBack[b] = 0;
	return 1;
}

stock Recy_Pick(playerid)
{
	if (g_RecyCarry[playerid]) return Cw_Notify(playerid, "warning", "Recyclage", "Tu portes deja un sac : ~b~/deposer~w~ au point de recyclage.", 3000), 1;
	for (new b = 0; b < C23_BAGS; b++)
	{
		if (g_RecyBack[b] || !IsPlayerInRangeOfPoint(playerid, 2.5, g_RecyPos[b][0], g_RecyPos[b][1], g_RecyPos[b][2])) continue;
		DestroyObject(g_RecyObj[b]);
		g_RecyBack[b] = gettime() + 60;
		g_RecyCarry[playerid] = 1;
		SetPlayerAttachedObject(playerid, C23_ATTACH, 1265, 6, 0.25, 0.0, 0.0, 0.0, 0.0, 0.0, 0.6, 0.6, 0.6);
		return Cw_Notify(playerid, "info", "Recyclage", "Sac ramasse : porte-le au point ~b~Recyclage~w~ (moins de 10 m) et ~b~/deposer~w~.", 4000), 1;
	}
	Gps_Set(playerid, C23_RECY_X, C23_RECY_Y);
	return Cw_Notify(playerid, "warning", "Recyclage", "Aucun sac ici : il y en a autour de la casse de Willowfield (GPS).", 3500), 1;
}

stock Recy_Drop(playerid)
{
	if (!g_RecyCarry[playerid]) return 0;
	if (!C23_Near(playerid, C23_RECY_X, C23_RECY_Y, C23_RECY_Z, 10.0)) return Cw_Notify(playerid, "warning", "Recyclage", "Approche-toi du point de recyclage (10 m).", 3000), 1;
	g_RecyCarry[playerid] = 0;
	RemovePlayerAttachedObject(playerid, C23_ATTACH);
	GivePlayerMoney(playerid, 60);
	g_C23Mats[playerid] += 2;
	C23_SaveMats(playerid);
	new m[96];
	format(m, sizeof m, "Sac recycle : ~g~+60 $~w~ et ~b~+2 materiaux~w~ (total %d).", g_C23Mats[playerid]);
	return Cw_Notify(playerid, "success", "Recyclage", m, 3500), 1;
}

// ============================================================
//  minuteries
// ============================================================
public Cw23_Tick()
{
	Radar_Check();
	Asm_Tick();
	new now = gettime();
	for (new b = 0; b < C23_BAGS; b++) if (g_RecyBack[b] && now >= g_RecyBack[b]) Recy_Spawn(b);
	return 1;
}

public Cw23_Fast()
{
	Ems_Follow();
	return 1;
}

// ============================================================
//  commandes
// ============================================================
stock Cw23_OnCommand(playerid, const cmdtext[])
{
	if (!g_AuthLogged[playerid]) return 0;
	new a;
	if (!strcmp(cmdtext, "/mdt", true))
	{
		if (!Mdt_Can(playerid)) return Cw_Notify(playerid, "denied", "MDT", "Reserve aux forces de l'ordre (police, FBI, armee).", 3500), 1;
		return Mdt_Home(playerid);
	}
	if ((a = Cw_Arg(cmdtext, "/c23")) != -1)
	{
		new idx = a, mod[8], act[10];
		Bank_NextToken(cmdtext, idx, mod, sizeof mod);
		Bank_NextToken(cmdtext, idx, act, sizeof act);
		while (cmdtext[idx] == ' ') idx++;
		if (!strcmp(mod, "mdt", true)) return Mdt_Action(playerid, act, cmdtext[idx]);
		if (!strcmp(mod, "army", true))
		{
			if (!strcmp(act, "load", true)) return Army_Load(playerid);
			if (!strcmp(act, "deliver", true)) return Army_Deliver(playerid);
			if (!strcmp(act, "armory", true)) return Army_Armory(playerid);
			if (!strcmp(act, "give", true)) { new j = idx, who[8], q[10]; Bank_NextToken(cmdtext, j, who, sizeof who); Bank_NextToken(cmdtext, j, q, sizeof q); return Army_Give(playerid, who, strval(q)); }
			return Army_Panel(playerid);
		}
		if (!strcmp(mod, "asm", true))
		{
			if (!strcmp(act, "piece", true)) return Asm_Piece(playerid);
			if (!strcmp(act, "put", true)) return Asm_Put(playerid);
			if (!strcmp(act, "spray", true)) return Asm_Spray(playerid);
			if (!strcmp(act, "quit", true)) return Asm_Start(playerid, "quitter");
			return Asm_Start(playerid, "");
		}
		if (!strcmp(mod, "arena", true)) return !strcmp(act, "leave", true) ? Arena_Leave(playerid) : Arena_Join(playerid);
		if (!strcmp(mod, "craft", true)) return act[0] ? Craft_Make(playerid, act) : Craft_Panel(playerid);
		return 1;
	}
	if ((a = Cw_Arg(cmdtext, "/fr")) != -1) return Radio_Say(playerid, cmdtext[a]);
	if ((a = Cw_Arg(cmdtext, "/radar")) != -1) { if (!Mdt_Can(playerid)) return 0; return Radar_Toggle(playerid, strval(cmdtext[a])); }
	if (!strcmp(cmdtext, "/amendes", true)) return Fine_List(playerid);
	if ((a = Cw_Arg(cmdtext, "/payeramende")) != -1) return Fine_Pay(playerid, cmdtext[a]);
	if (!strcmp(cmdtext, "/charger", true)) return Army_Load(playerid);
	if (!strcmp(cmdtext, "/livrer", true)) return Army_Deliver(playerid);
	if (!strcmp(cmdtext, "/armee", true)) return Army_Panel(playerid);
	if (!strcmp(cmdtext, "/armurerie", true)) return Army_Armory(playerid);
	if ((a = Cw_Arg(cmdtext, "/fournir")) != -1)
	{
		new j = a, who[8], q[10];
		Bank_NextToken(cmdtext, j, who, sizeof who);
		Bank_NextToken(cmdtext, j, q, sizeof q);
		return Army_Give(playerid, who, strval(q));
	}
	if ((a = Cw_Arg(cmdtext, "/civiere")) != -1) return Ems_Stretcher(playerid, cmdtext[a] ? strval(cmdtext[a]) : -1);
	if ((a = Cw_Arg(cmdtext, "/chirurgie")) != -1) return Ems_Surgery(playerid, cmdtext[a] ? strval(cmdtext[a]) : -1);
	if (!strcmp(cmdtext, "/accepterchirurgie", true)) return Ems_SurgeryAccept(playerid);
	if ((a = Cw_Arg(cmdtext, "/assembler")) != -1) return Asm_Start(playerid, cmdtext[a]);
	if (!strcmp(cmdtext, "/piece", true) && Asm_IsTeam(playerid)) return Asm_Piece(playerid);   // sinon /piece = pile ou face (Cw15)
	if (!strcmp(cmdtext, "/poser", true)) { if (Asm_Put(playerid)) return 1; return 0; }
	if (!strcmp(cmdtext, "/spray", true)) return Asm_Spray(playerid);
	if (!strcmp(cmdtext, "/arene", true)) return Arena_Join(playerid);
	if (!strcmp(cmdtext, "/quitterarene", true)) return Arena_Leave(playerid);
	if (!strcmp(cmdtext, "/craft", true)) return Craft_Panel(playerid);
	if (!strcmp(cmdtext, "/drogue", true)) return Drug_Use(playerid);
	if (!strcmp(cmdtext, "/ramasser", true)) return Recy_Pick(playerid);
	if (!strcmp(cmdtext, "/deposer", true)) { if (Recy_Drop(playerid)) return 1; return 0; }
	return 0;
}
