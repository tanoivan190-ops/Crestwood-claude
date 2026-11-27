/* ============================================================
   CRESTWOOD RP v46 (21/11/2026)
   ------------------------------------------------------------
   1) STATS DE GROUPE (Gs_) : compteurs par joueur / faction / gang (total + aujourd'hui)
      arrestations, amendes, reanimations, incendies, verdicts, eliminations, morts, salaires,
      points de gang, gains, temps de jeu. Base crestwood_v46.db (table gs).
   2) PAGE "MON GROUPE" (page generique "groupe", tablette RP > Mon groupe, /groupe) :
      selon ce que tu es (gangster, police, EMS, pompier, justice, armee...) : tes stats, celles de
      ton groupe, les gains, le classement des membres, le journal. Gangs : coffre (depot / retrait),
      revenus de territoire toutes les 30 min, +3 points et +250 $ par rival elimine.
   3) IDENTIFIANT DU JOUR : plus de pseudo au-dessus des tetes, mais "(4821)" : un numero aleatoire
      donne par le serveur a chaque joueur, nouveau chaque jour. Envoye au client : dialogue 9518
      "@tags" -> "1|id:tag,id:tag" (playertags.cpp 32/64). Staff : /nomsstaff pour voir les pseudos.
   4) TUTORIEL PAR APPEL : au premier spawn, le telephone sonne (sonnerie), Maya de la mairie
      t'accompagne etape par etape (kit, banque, papiers, travail, vetements, garage, telephone,
      tablette, regles). GPS a chaque etape, recompense a la fin. HUD : module "tuto" (tuto.js)
      Serveur -> HUD : dialogue 9860 "@hud" {"m":"tuto",...}   HUD -> serveur : /tuto <action>
      Rejouer : /tutoriel (aussi dans /aide). Staff : /tutoreset <id>.
   Texte ASCII.
   ============================================================ */
#define C46_DB               "crestwood_v46.db"
#define C46_TAG_DIALOG       9518
#define GRP_PAGE             "groupe"
#define GANG_KILL_PTS        3
#define GANG_KILL_CASH       250
#define GANG_INCOME_EVERY    1800        // secondes
#define TT_REWARD            1500
#define TT_RANGE             6.0
#define TT_RING_S            30

#define TT_INTRO             0
#define TT_HUD               1
#define TT_KIT               2
#define TT_BANK              3
#define TT_ID                4
#define TT_JOB               5
#define TT_CLOTH             6
#define TT_GARAGE            7
#define TT_PHONE             8
#define TT_TABLET            9
#define TT_RULES             10
#define TT_END               11

new DB:g_C46DB;
new g_C46Sec;
new g_C46Day;
new g_C46Income;
new g_GsMin[MAX_PLAYERS];
new g_DtTag[MAX_PLAYERS];
new g_DtNames[MAX_PLAYERS];
new g_DtDirty;                 // [v48] opti : liste des identifiants a renvoyer (groupee, au plus toutes les 2 s)
new g_TtStep[MAX_PLAYERS] = {-1, ...};
new g_TtRing[MAX_PLAYERS];
new g_TtMissed[MAX_PLAYERS];
new g_TtNext[MAX_PLAYERS];
new g_TtDist[MAX_PLAYERS];
new g_TtLastSend[MAX_PLAYERS];
new g_TtHas[MAX_PLAYERS];
new g_TtDone[MAX_PLAYERS] = {-1, ...};
new Float:g_TtX[MAX_PLAYERS];
new Float:g_TtY[MAX_PLAYERS];
new Float:g_TtZ[MAX_PLAYERS];

forward C46_Tick();

// ------------------------------------------------------------
//  Base
// ------------------------------------------------------------
stock C46_Exec(const q[])
{
	if (g_C46DB != DB:0) db_free_result(db_query(g_C46DB, q));
	return 1;
}

stock C46_Int(DB:db, const q[])
{
	new v = 0, f[16];
	if (db == DB:0) return 0;
	new DBResult:r = db_query(db, q);
	if (r != DBResult:0)
	{
		if (db_num_rows(r) > 0)
		{
			db_get_field(r, 0, f, sizeof f);
			v = strval(f);
		}
		db_free_result(r);
	}
	return v;
}

stock Cw46_OnInit()
{
	g_C46DB = db_open(C46_DB);
	C46_Exec("CREATE TABLE IF NOT EXISTS gs (sc TEXT, sid TEXT COLLATE NOCASE, k TEXT, total INTEGER DEFAULT 0, day INTEGER DEFAULT 0, today INTEGER DEFAULT 0, PRIMARY KEY (sc, sid, k))");
	C46_Exec("CREATE TABLE IF NOT EXISTS gbank (gang INTEGER PRIMARY KEY, bal INTEGER DEFAULT 0)");
	C46_Exec("CREATE TABLE IF NOT EXISTS glog (id INTEGER PRIMARY KEY AUTOINCREMENT, gang INTEGER, ts INTEGER, text TEXT)");
	C46_Exec("CREATE TABLE IF NOT EXISTS dtag (name TEXT PRIMARY KEY COLLATE NOCASE, day INTEGER, tag INTEGER)");
	C46_Exec("CREATE TABLE IF NOT EXISTS tuto (name TEXT PRIMARY KEY COLLATE NOCASE, done INTEGER DEFAULT 0, ts INTEGER)");
	g_C46Day = Gs_Day();
	g_C46Income = gettime();
	SetTimer("C46_Tick", 1000, true);
	print("[v46] stats de groupe, identifiants du jour, tutoriel : OK");
	return 1;
}

stock Cw46_OnConnect(playerid)
{
	g_GsMin[playerid] = 0;
	g_DtTag[playerid] = 0;
	g_DtNames[playerid] = 0;
	g_TtStep[playerid] = -1;
	g_TtRing[playerid] = 0;
	g_TtMissed[playerid] = 0;
	g_TtNext[playerid] = 0;
	g_TtHas[playerid] = 0;
	g_TtDone[playerid] = -1;
	return 1;
}

stock Cw46_OnDisconnect(playerid)
{
	Gs_Flush(playerid);
	new had = g_DtTag[playerid];
	g_DtTag[playerid] = 0;
	g_TtStep[playerid] = -1;
	g_TtRing[playerid] = 0;
	if (had) SetTimer("Dt_Later", 500, false);
	return 1;
}

forward Dt_Later();
public Dt_Later() return g_DtDirty = 1, 1;

stock Cw46_OnSpawn(playerid)
{
	if (!g_AuthLogged[playerid] || IsPlayerNPC(playerid)) return 0;
	if (!g_DtTag[playerid]) Dt_Assign(playerid, 1);
	else Dt_SendTo(playerid);
	if (g_TtDone[playerid] < 0) g_TtDone[playerid] = Tt_IsDone(playerid);
	if (!g_TtDone[playerid] && g_TtStep[playerid] < 0 && !g_TtRing[playerid] && !g_TtNext[playerid]) g_TtNext[playerid] = gettime() + 25;
	return 1;
}

// ------------------------------------------------------------
//  1) Stats
// ------------------------------------------------------------
stock Gs_Day() return gettime() / 86400;

stock Gs_Raw(const sc[], const sid[], const k[], n)
{
	if (g_C46DB == DB:0 || n == 0) return 0;
	new q[320], d = Gs_Day();
	format(q, sizeof q, "INSERT OR IGNORE INTO gs (sc, sid, k, total, day, today) VALUES ('%s', '%s', '%s', 0, %d, 0)", sc, sid, k, d);
	C46_Exec(q);
	format(q, sizeof q, "UPDATE gs SET total = total + %d, today = CASE WHEN day = %d THEN today + %d ELSE %d END, day = %d WHERE sc = '%s' AND sid = '%s' AND k = '%s'", n, d, n, n, d, sc, sid, k);
	C46_Exec(q);
	return 1;
}

stock Gs_AddNoTx(playerid, const k[], n)
{
	new nm[MAX_PLAYER_NAME + 1], s[12];
	GetPlayerName(playerid, nm, sizeof nm);
	Gs_Raw("p", nm, k, n);
	new fac = Fac_Get(playerid);
	if (fac != FACTION_NONE) { format(s, sizeof s, "%d", fac); Gs_Raw("f", s, k, n); }
	new g = g_GangOf[playerid];
	if (g >= 1 && g <= GANG_COUNT) { format(s, sizeof s, "%d", g); Gs_Raw("g", s, k, n); }
	return 1;
}

stock Gs_Add(playerid, const k[], n)
{
	if (playerid < 0 || playerid >= MAX_PLAYERS || !IsPlayerConnected(playerid) || !g_AuthLogged[playerid] || n == 0) return 0;
	C46_Exec("BEGIN");
	Gs_AddNoTx(playerid, k, n);
	C46_Exec("COMMIT");
	return 1;
}

stock Gs_Flush(playerid)
{
	if (g_GsMin[playerid] <= 0) return 0;
	new n = g_GsMin[playerid];
	g_GsMin[playerid] = 0;
	return Gs_Add(playerid, "min", n);
}

stock Gs_FlushAll()
{
	C46_Exec("BEGIN");
	for (new p = 0, m = GetMaxPlayers(); p < m; p++)
	{
		if (!IsPlayerConnected(p) || !g_AuthLogged[p] || g_GsMin[p] <= 0) continue;
		Gs_AddNoTx(p, "min", g_GsMin[p]);
		g_GsMin[p] = 0;
	}
	C46_Exec("COMMIT");
	return 1;
}

stock Gs_Get(const sc[], const sid[], const k[])
{
	new q[200];
	format(q, sizeof q, "SELECT total FROM gs WHERE sc = '%s' AND sid = '%s' AND k = '%s'", sc, sid, k);
	return C46_Int(g_C46DB, q);
}

// {"k":[total,aujourd'hui],...}
stock Gs_Json(const sc[], const sid[], out[], size)
{
	new q[200], f[24], k[12], part[48], n = 0, d = Gs_Day();
	format(out, size, "{");
	format(q, sizeof q, "SELECT k, total, today, day FROM gs WHERE sc = '%s' AND sid = '%s' LIMIT 24", sc, sid);
	new DBResult:r = (g_C46DB != DB:0) ? db_query(g_C46DB, q) : DBResult:0;
	if (r != DBResult:0)
	{
		for (new rows = db_num_rows(r), i = 0; i < rows; i++)
		{
			db_get_field(r, 0, k, sizeof k);
			db_get_field(r, 1, f, sizeof f);
			new tot = strval(f);
			db_get_field(r, 3, f, sizeof f);
			new td = 0;
			if (strval(f) == d) { db_get_field(r, 2, f, sizeof f); td = strval(f); }
			format(part, sizeof part, "%s\"%s\":[%d,%d]", n ? (",") : (""), k, tot, td);
			strcat(out, part, size);
			n++;
			db_next_row(r);
		}
		db_free_result(r);
	}
	strcat(out, "}", size);
	return n;
}

// points d'accroche (appeles par le gamemode)
stock Cw46_OnArrest(playerid) return Gs_Add(playerid, "arr", 1);
stock Cw46_OnVerdict(playerid) return Gs_Add(playerid, "ver", 1);
stock Cw46_OnRevive(playerid) return Gs_Add(playerid, "rea", 1);
stock Cw46_OnFire(playerid) return Gs_Add(playerid, "feu", 1);
stock Cw46_OnSalary(playerid, pay) return Gs_Add(playerid, "sal", pay);
stock Cw46_OnFine(playerid, cost)
{
	if (!IsPlayerConnected(playerid)) return 0;
	C46_Exec("BEGIN");
	Gs_AddNoTx(playerid, "amd", 1);
	Gs_AddNoTx(playerid, "amdv", cost);
	C46_Exec("COMMIT");
	return 1;
}

stock Cw46_OnDeath(playerid, killerid)
{
	if (!IsPlayerConnected(playerid) || !g_AuthLogged[playerid]) return 0;
	if (GetPlayerVirtualWorld(playerid) != 0) return 0;   // arenes / evenements : pas compte
	Gs_Add(playerid, "mort", 1);
	if (killerid == INVALID_PLAYER_ID || killerid == playerid || killerid < 0 || killerid >= MAX_PLAYERS) return 1;
	if (!IsPlayerConnected(killerid) || !g_AuthLogged[killerid]) return 1;
	Gs_Add(killerid, "kill", 1);
	new kg = g_GangOf[killerid], vg = g_GangOf[playerid];
	if (kg >= 1 && kg <= GANG_COUNT && vg >= 1 && vg <= GANG_COUNT && kg != vg)
	{
		g_GangPts[kg] += GANG_KILL_PTS;
		Gang_SavePts(kg);
		Gb_Add(kg, GANG_KILL_CASH);
		C46_Exec("BEGIN");
		Gs_AddNoTx(killerid, "cap", GANG_KILL_PTS);
		Gs_AddNoTx(killerid, "gain", GANG_KILL_CASH);
		C46_Exec("COMMIT");
		new m[120], kn[MAX_PLAYER_NAME + 1];
		Pg_Name(killerid, kn, sizeof kn);
		format(m, sizeof m, "%s a elimine un membre des %s : +%d points, +%d $", kn, g_GangName[vg], GANG_KILL_PTS, GANG_KILL_CASH);
		Gb_Log(kg, m);
		Cw_Notify(killerid, "success", "Gang", "Rival elimine : +3 points et +250 $ au coffre du gang.", 3500);
	}
	return 1;
}

// ------------------------------------------------------------
//  Coffre des gangs
// ------------------------------------------------------------
stock Gb_Bal(g)
{
	new q[80];
	format(q, sizeof q, "SELECT bal FROM gbank WHERE gang = %d", g);
	return C46_Int(g_C46DB, q);
}

stock Gb_Add(g, n)
{
	new q[140], b = Gb_Bal(g) + n;
	if (b < 0) b = 0;
	format(q, sizeof q, "INSERT OR REPLACE INTO gbank (gang, bal) VALUES (%d, %d)", g, b);
	C46_Exec(q);
	return b;
}

stock Gb_Log(g, const text[])
{
	new t[110], q[240];
	Law_Clean(text, t, sizeof t);
	format(q, sizeof q, "INSERT INTO glog (gang, ts, text) VALUES (%d, %d, '%s')", g, gettime(), t);
	return C46_Exec(q);
}

stock Gang_Income()
{
	new on[GANG_COUNT + 1], s[8], m[120];
	for (new p = 0, mx = GetMaxPlayers(); p < mx; p++)
		if (IsPlayerConnected(p) && g_AuthLogged[p] && g_GangOf[p] >= 1 && g_GangOf[p] <= GANG_COUNT) on[g_GangOf[p]]++;
	for (new g = 1; g <= GANG_COUNT; g++)
	{
		if (!on[g]) continue;
		new inc = 200 + g_GangPts[g] / 4 + on[g] * 50;
		if (inc > 3000) inc = 3000;
		Gb_Add(g, inc);
		format(s, sizeof s, "%d", g);
		Gs_Raw("g", s, "gain", inc);
		format(m, sizeof m, "Revenus du territoire : +%d $ au coffre (%d membre(s) en ville)", inc, on[g]);
		Gb_Log(g, m);
		for (new p = 0, mx = GetMaxPlayers(); p < mx; p++)
		{
			if (!IsPlayerConnected(p) || g_GangOf[p] != g) continue;
			format(m, sizeof m, "Revenus du territoire : ~g~+%d $~w~ au coffre du gang.", inc);
			Cw_Notify(p, "success", g_GangName[g], m, 5000);
		}
	}
	return 1;
}

// ------------------------------------------------------------
//  2) Page "Mon groupe"
// ------------------------------------------------------------
stock Grp_MainKey(playerid, out[], size)
{
	new t = Fac_Type(playerid);
	if (g_GangOf[playerid] >= 1 && g_GangOf[playerid] <= GANG_COUNT) return format(out, size, "cap"), 1;
	if (t == FACTION_POLICE || t == FACTION_FEDERAL) return format(out, size, "arr"), 1;
	if (t == FACTION_MEDIC) return format(out, size, "rea"), 1;
	if (t == FACTION_FIRE) return format(out, size, "feu"), 1;
	if (t == FACTION_JUSTICE) return format(out, size, "ver"), 1;
	if (t == FACTION_HITMAN || t == FACTION_TERRORIST || t == FACTION_ARMY) return format(out, size, "kill"), 1;
	return format(out, size, "min"), 1;
}

// top 5 des membres (noms tires de la table du groupe, valeurs dans gs)
stock Grp_Top(DB:db, const q[], const key[], out[], size)
{
	new names[40][MAX_PLAYER_NAME + 1], vals[40], n = 0;
	new DBResult:r = (db != DB:0) ? db_query(db, q) : DBResult:0;
	if (r != DBResult:0)
	{
		for (new rows = db_num_rows(r); n < rows && n < 40; n++)
		{
			db_get_field(r, 0, names[n], MAX_PLAYER_NAME + 1);
			db_next_row(r);
		}
		db_free_result(r);
	}
	for (new i = 0; i < n; i++) vals[i] = Gs_Get("p", names[i], key);
	out[0] = 0;
	new part[64], shown = 0;
	for (new pass = 0; pass < 5 && pass < n; pass++)
	{
		new best = -1;
		for (new i = 0; i < n; i++) if (vals[i] >= 0 && (best < 0 || vals[i] > vals[best])) best = i;
		if (best < 0) break;
		new nm[MAX_PLAYER_NAME + 1];
		format(nm, sizeof nm, "%s", names[best]);
		for (new c = 0; nm[c]; c++) if (nm[c] == '_') nm[c] = ' ';
		format(part, sizeof part, "%s[\"%s\",%d,%d]", shown ? (",") : (""), nm, vals[best], (Cw_PlayerByName(names[best]) != INVALID_PLAYER_ID) ? 1 : 0);
		strcat(out, part, size);
		vals[best] = -1;
		shown++;
	}
	return shown;
}

stock Grp_Log(DB:db, const q[], out[], size)
{
	new f[16], t[120], part[160], n = 0;
	out[0] = 0;
	new DBResult:r = (db != DB:0) ? db_query(db, q) : DBResult:0;
	if (r != DBResult:0)
	{
		for (new rows = db_num_rows(r); n < rows; n++)
		{
			db_get_field(r, 0, f, sizeof f);
			db_get_field(r, 1, t, sizeof t);
			new e[120];
			Pg_Esc(t, e, sizeof e);
			format(part, sizeof part, "%s[%d,\"%s\"]", n ? (",") : (""), strval(f), e);
			if (strlen(out) + strlen(part) >= size - 2) break;
			strcat(out, part, size);
			db_next_row(r);
		}
		db_free_result(r);
	}
	return n;
}

stock Grp_Open(playerid, dlg = PAGE_DLG_OPEN, ok = -1, const msg[] = "")
{
	if (!g_AuthLogged[playerid]) return 0;
	Gs_Flush(playerid);
	static json[4000];
	new me[MAX_PLAYER_NAME + 1], meRaw[MAX_PLAYER_NAME + 1], m2[110], st[600], part[700], key[8], q[200];
	Pg_Name(playerid, me, sizeof me);
	GetPlayerName(playerid, meRaw, sizeof meRaw);
	Pg_Esc(msg, m2, sizeof m2);
	Grp_MainKey(playerid, key, sizeof key);
	Gs_Json("p", meRaw, st, sizeof st);
	format(json, sizeof json, "{\"ok\":%d,\"msg\":\"%s\",\"key\":\"%s\",\"me\":{\"n\":\"%s\",\"tag\":%d,\"cash\":%d,\"bank\":%d,\"st\":%s}",
		ok, m2, key, me, g_DtTag[playerid], GetPlayerMoney(playerid), g_BankBalance[playerid], st);

	new g = g_GangOf[playerid];
	if (g >= 1 && g <= GANG_COUNT)
	{
		new on = 0, place = 1, sid[8];
		for (new p = 0, mx = GetMaxPlayers(); p < mx; p++) if (IsPlayerConnected(p) && g_AuthLogged[p] && g_GangOf[p] == g) on++;
		for (new o = 1; o <= GANG_COUNT; o++) if (o != g && g_GangPts[o] > g_GangPts[g]) place++;
		format(sid, sizeof sid, "%d", g);
		Gs_Json("g", sid, st, sizeof st);
		format(part, sizeof part, ",\"g\":{\"id\":%d,\"n\":\"%s\",\"tag\":\"%s\",\"c\":\"%s\",\"rk\":%d,\"rn\":\"%s\",\"pts\":%d,\"place\":%d,\"season\":%d,\"m\":%d,\"on\":%d,\"bal\":%d,\"mg\":%d,\"st\":%s",
			g, g_GangName[g], g_GangTag[g], g_GangColor[g], g_GangRank[playerid], g_GangRankName[g_GangRank[playerid]], g_GangPts[g], place, g_GangSeason,
			Gang_MemberCount(g), on, Gb_Bal(g), g_GangRank[playerid] >= 3 ? 1 : 0, st);
		strcat(json, part, sizeof json);
		new lst[600];
		format(q, sizeof q, "SELECT name FROM gang_members WHERE gang = %d LIMIT 40", g);
		Grp_Top(g_AdminDB, q, "cap", lst, sizeof lst);
		strcat(json, ",\"top\":[", sizeof json); strcat(json, lst, sizeof json);
		format(q, sizeof q, "SELECT ts, text FROM glog WHERE gang = %d ORDER BY id DESC LIMIT 6", g);
		Grp_Log(g_C46DB, q, lst, sizeof lst);
		strcat(json, "],\"log\":[", sizeof json); strcat(json, lst, sizeof json);
		strcat(json, "]}", sizeof json);
	}

	new fac = Fac_Get(playerid);
	if (fac != FACTION_NONE)
	{
		new fn[60], sh[24], rn[32], tn[16], on = 0, sid[8], fk[8];
		Law_Clean(g_FxName[fac], fn, sizeof fn);
		Law_Clean(g_FxShort[fac], sh, sizeof sh);
		Fac_RankName(fac, g_FacRank[playerid], rn, sizeof rn);
		new t = g_FxType[fac];
		if (t >= 0 && t <= FACTION_TYPE_MAX) format(tn, sizeof tn, "%s", g_FxTypeNames[t]); else format(tn, sizeof tn, "none");
		for (new p = 0, mx = GetMaxPlayers(); p < mx; p++) if (IsPlayerConnected(p) && g_AuthLogged[p] && Fac_Get(p) == fac) on++;
		format(q, sizeof q, "SELECT COUNT(*) FROM faction_members WHERE faction = %d", fac);
		new members = C46_Int(g_FacDB, q);
		format(sid, sizeof sid, "%d", fac);
		Gs_Json("f", sid, st, sizeof st);
		format(part, sizeof part, ",\"f\":{\"id\":%d,\"n\":\"%s\",\"s\":\"%s\",\"t\":\"%s\",\"c\":\"%06x\",\"rk\":%d,\"rn\":\"%s\",\"rc\":%d,\"m\":%d,\"on\":%d,\"bal\":%d,\"mg\":%d,\"st\":%s",
			fac, fn, sh, tn, (g_FxColor[fac] >>> 8) & 0xFFFFFF, g_FacRank[playerid], rn, Fac_RankCount(fac), members, on, Fp_Bal(fac), Fp_IsMg(playerid), st);
		strcat(json, part, sizeof json);
		new lst[600];
		if (t == FACTION_POLICE || t == FACTION_FEDERAL) format(fk, sizeof fk, "arr");
		else if (t == FACTION_MEDIC) format(fk, sizeof fk, "rea");
		else if (t == FACTION_FIRE) format(fk, sizeof fk, "feu");
		else if (t == FACTION_JUSTICE) format(fk, sizeof fk, "ver");
		else if (t == FACTION_HITMAN || t == FACTION_TERRORIST || t == FACTION_ARMY) format(fk, sizeof fk, "kill");
		else format(fk, sizeof fk, "min");
		format(q, sizeof q, "SELECT name FROM faction_members WHERE faction = %d LIMIT 40", fac);
		Grp_Top(g_FacDB, q, fk, lst, sizeof lst);
		format(part, sizeof part, ",\"fk\":\"%s\",\"top\":[", fk);
		strcat(json, part, sizeof json); strcat(json, lst, sizeof json);
		format(q, sizeof q, "SELECT ts, text FROM faction_log WHERE fac = %d ORDER BY id DESC LIMIT 6", fac);
		Grp_Log(g_FacDB, q, lst, sizeof lst);
		strcat(json, "],\"log\":[", sizeof json); strcat(json, lst, sizeof json);
		strcat(json, "]}", sizeof json);
	}
	strcat(json, "}", sizeof json);
	return Pg_Send(playerid, dlg, GRP_PAGE, json), 1;
}

stock Grp_Action(playerid, const act[], const arg[])
{
	if (!strcmp(act, "refresh", true)) return Grp_Open(playerid, PAGE_DLG_UPDATE);
	if (!strcmp(act, "fac", true))
	{
		Pg_Send(playerid, PAGE_DLG_CLOSE, GRP_PAGE, "{}");
		return Fp_Open(playerid), 1;
	}
	if (!strcmp(act, "tuto", true))
	{
		Pg_Send(playerid, PAGE_DLG_CLOSE, GRP_PAGE, "{}");
		return Tt_Ring(playerid, 1), 1;
	}
	new g = g_GangOf[playerid];
	if (!strcmp(act, "dep", true) || !strcmp(act, "ret", true))
	{
		if (g < 1 || g > GANG_COUNT) return Grp_Open(playerid, PAGE_DLG_UPDATE, 0, "Tu n'es dans aucun gang.");
		new v = strval(arg), m[120], a[16], nm[MAX_PLAYER_NAME + 1];
		if (v <= 0 || v > 5000000) return Grp_Open(playerid, PAGE_DLG_UPDATE, 0, "Montant invalide.");
		Pg_Name(playerid, nm, sizeof nm);
		Cw_Fmt(v, a, sizeof a);
		if (!strcmp(act, "dep", true))
		{
			if (GetPlayerMoney(playerid) < v) return Grp_Open(playerid, PAGE_DLG_UPDATE, 0, "Tu n'as pas assez de liquide.");
			GivePlayerMoney(playerid, -v);
			Gb_Add(g, v);
			Gs_Add(playerid, "dep", v);
			format(m, sizeof m, "%s a depose %s $ au coffre", nm, a);
		}
		else
		{
			if (g_GangRank[playerid] < 3) return Grp_Open(playerid, PAGE_DLG_UPDATE, 0, "Retrait reserve aux Lieutenants et au Chef.");
			if (Gb_Bal(g) < v) return Grp_Open(playerid, PAGE_DLG_UPDATE, 0, "Le coffre n'a pas assez d'argent.");
			Gb_Add(g, -v);
			GivePlayerMoney(playerid, v);
			format(m, sizeof m, "%s a retire %s $ du coffre", nm, a);
		}
		Gb_Log(g, m);
		return Grp_Open(playerid, PAGE_DLG_UPDATE, 1, m);
	}
	return 1;
}

// ------------------------------------------------------------
//  3) Identifiant du jour au-dessus des tetes
// ------------------------------------------------------------
stock Dt_Used(tag, except)
{
	for (new p = 0, m = GetMaxPlayers(); p < m; p++) if (p != except && IsPlayerConnected(p) && g_DtTag[p] == tag) return 1;
	return 0;
}

stock Dt_Assign(playerid, broadcast)
{
	new nm[MAX_PLAYER_NAME + 1], q[200], f[16], d = Gs_Day(), tag = 0;
	GetPlayerName(playerid, nm, sizeof nm);
	format(q, sizeof q, "SELECT day, tag FROM dtag WHERE name = '%s'", nm);
	new DBResult:r = (g_C46DB != DB:0) ? db_query(g_C46DB, q) : DBResult:0;
	if (r != DBResult:0)
	{
		if (db_num_rows(r) > 0)
		{
			db_get_field(r, 0, f, sizeof f);
			if (strval(f) == d) { db_get_field(r, 1, f, sizeof f); tag = strval(f); }
		}
		db_free_result(r);
	}
	if (tag < 1000 || tag > 9999 || Dt_Used(tag, playerid))
	{
		for (new tries = 0; tries < 40; tries++)
		{
			tag = 1000 + random(9000);
			if (!Dt_Used(tag, playerid)) break;
		}
		format(q, sizeof q, "INSERT OR REPLACE INTO dtag (name, day, tag) VALUES ('%s', %d, %d)", nm, d, tag);
		C46_Exec(q);
	}
	g_DtTag[playerid] = tag;
	new m[110];
	format(m, sizeof m, "Aujourd'hui, au-dessus de ta tete : ~b~(%04d)~w~. Nouveau numero demain.", tag);
	Cw_Notify(playerid, "info", "Identifiant du jour", m, 6000);
	if (broadcast) g_DtDirty = 1;   // [v48] envoi groupe par C46_Tick (evite N x N dialogues quand tout le monde se reconnecte)
	return tag;
}

stock Dt_Build(forid, out[], size)
{
	new part[16], n = 0;
	format(out, size, "%d|", g_DtNames[forid] ? 0 : 1);
	for (new p = 0, m = GetMaxPlayers(); p < m; p++)
	{
		if (!IsPlayerConnected(p) || !g_DtTag[p]) continue;
		format(part, sizeof part, "%s%d:%d", n ? (",") : (""), p, g_DtTag[p]);
		if (strlen(out) + strlen(part) >= size - 1) break;
		strcat(out, part, size);
		n++;
	}
	return n;
}

stock Dt_SendTo(playerid)
{
	if (!IsPlayerConnected(playerid) || !g_AuthLogged[playerid]) return 0;
	new list[2048];
	Dt_Build(playerid, list, sizeof list);
	return ShowPlayerDialog(playerid, C46_TAG_DIALOG, DIALOG_STYLE_MSGBOX, "@tags", list, "OK", "");
}

stock Dt_Broadcast()
{
	for (new p = 0, m = GetMaxPlayers(); p < m; p++) if (IsPlayerConnected(p) && g_AuthLogged[p] && !IsPlayerNPC(p)) Dt_SendTo(p);
	return 1;
}

stock Dt_NewDay()
{
	for (new p = 0, m = GetMaxPlayers(); p < m; p++) if (IsPlayerConnected(p)) g_DtTag[p] = 0;
	for (new p = 0, m = GetMaxPlayers(); p < m; p++) if (IsPlayerConnected(p) && g_AuthLogged[p] && !IsPlayerNPC(p)) Dt_Assign(p, 0);
	return g_DtDirty = 1, 1;
}

// joueur par identifiant du jour (-1 si personne)
stock Dt_Find(tag)
{
	for (new p = 0, m = GetMaxPlayers(); p < m; p++) if (IsPlayerConnected(p) && g_DtTag[p] == tag) return p;
	return INVALID_PLAYER_ID;
}

// ------------------------------------------------------------
//  4) Tutoriel par appel
// ------------------------------------------------------------
stock Tt_IsDone(playerid)
{
	new nm[MAX_PLAYER_NAME + 1], q[140];
	GetPlayerName(playerid, nm, sizeof nm);
	format(q, sizeof q, "SELECT done FROM tuto WHERE name = '%s'", nm);
	return C46_Int(g_C46DB, q);
}

stock Tt_SetDone(playerid)
{
	new nm[MAX_PLAYER_NAME + 1], q[180];
	GetPlayerName(playerid, nm, sizeof nm);
	format(q, sizeof q, "INSERT OR REPLACE INTO tuto (name, done, ts) VALUES ('%s', 1, %d)", nm, gettime());
	C46_Exec(q);
	g_TtDone[playerid] = 1;
	return 1;
}

stock Tt_Hud(playerid, const st[], const extra[] = "")
{
	new j[300];
	format(j, sizeof j, "{\"m\":\"tuto\",\"st\":\"%s\",\"step\":%d,\"has\":%d,\"dist\":%d,\"miss\":%d%s}",
		st, g_TtStep[playerid], g_TtHas[playerid], g_TtDist[playerid], g_TtMissed[playerid], extra);
	g_TtLastSend[playerid] = gettime();
	return ShowPlayerDialog(playerid, C23_DIALOG, DIALOG_STYLE_MSGBOX, "@hud", j, "OK", "");
}

stock Tt_CanRing(playerid)
{
	if (!IsPlayerConnected(playerid) || !g_AuthLogged[playerid]) return 0;
	if (g_CreaOn[playerid] || g_LgOn[playerid]) return 0;
	new s = GetPlayerState(playerid);
	if (s == PLAYER_STATE_NONE || s == PLAYER_STATE_WASTED || s == PLAYER_STATE_SPECTATING) return 0;
	return 1;
}

stock Tt_Ring(playerid, force = 0)
{
	if (g_TtStep[playerid] >= 0 && !force) return 0;
	if (!Tt_CanRing(playerid)) { g_TtNext[playerid] = gettime() + 10; return 0; }
	g_TtNext[playerid] = 0;
	g_TtStep[playerid] = -1;
	g_TtHas[playerid] = 0;
	g_TtRing[playerid] = TT_RING_S;
	return Tt_Hud(playerid, "ring");
}

stock Tt_Nearest(playerid, Float:x, Float:y, Float:z, &Float:bd, &Float:ox, &Float:oy, &Float:oz)
{
	new Float:px, Float:py, Float:pz;
	GetPlayerPos(playerid, px, py, pz);
	new Float:d = (px - x) * (px - x) + (py - y) * (py - y) + 0.0 * pz;
	if (bd < 0.0 || d < bd) { bd = d; ox = x; oy = y; oz = z; return 1; }
	return 0;
}

stock Tt_Target(playerid, step)
{
	new Float:bd = -1.0, Float:ox, Float:oy, Float:oz;
	switch (step)
	{
		case TT_BANK: for (new i = 0; i < g_BankCount; i++) Tt_Nearest(playerid, g_BankPos[i][0], g_BankPos[i][1], g_BankPos[i][2], bd, ox, oy, oz);
		case TT_ID: for (new i = 0; i < g_NpcCount; i++) if (g_NpcRole[i] == NPC_ROLE_GOV && g_NpcVw[i] == 0) Tt_Nearest(playerid, g_NpcPos[i][0], g_NpcPos[i][1], g_NpcPos[i][2], bd, ox, oy, oz);
		case TT_JOB: for (new j = 1; j <= JOB_COUNT; j++) if (g_JobPlaced[j]) Tt_Nearest(playerid, g_Jobs[j][jbX], g_Jobs[j][jbY], g_Jobs[j][jbZ], bd, ox, oy, oz);
		case TT_CLOTH: for (new i = 0; i < g_VetCount; i++) Tt_Nearest(playerid, g_VetPos[i][0], g_VetPos[i][1], g_VetPos[i][2], bd, ox, oy, oz);
		case TT_GARAGE: for (new i = 0; i < g_GarCount; i++) Tt_Nearest(playerid, g_GarPos[i][0], g_GarPos[i][1], g_GarPos[i][2], bd, ox, oy, oz);
		default: return 0;
	}
	if (bd < 0.0) return 0;
	g_TtX[playerid] = ox; g_TtY[playerid] = oy; g_TtZ[playerid] = oz;
	return 1;
}

stock Tt_Enter(playerid, step, okprev = 0)
{
	for (;;)
	{
		if (step >= TT_END) return Tt_Finish(playerid);
		g_TtHas[playerid] = 0;
		g_TtDist[playerid] = 0;
		if (step == TT_KIT)
		{
			if (Kit_Claimed(playerid)) { step++; continue; }
			// la fenetre du kit est montree APRES la carte d'appel (le dernier dialogue envoye doit etre le kit)
			g_TtStep[playerid] = step;
			Tt_Hud(playerid, "call", okprev ? (",\"ok\":1") : (""));
			return Kit_Show(playerid), 1;
		}
		if (step == TT_BANK || step == TT_ID || step == TT_JOB || step == TT_CLOTH || step == TT_GARAGE)
		{
			if (!Tt_Target(playerid, step)) { step++; continue; }
			g_TtHas[playerid] = 1;
			Gps_Set(playerid, g_TtX[playerid], g_TtY[playerid]);
			g_TtDist[playerid] = floatround(GetPlayerDistanceFromPoint(playerid, g_TtX[playerid], g_TtY[playerid], g_TtZ[playerid]));
		}
		break;
	}
	g_TtStep[playerid] = step;
	return Tt_Hud(playerid, "call", okprev ? (",\"ok\":1") : (""));
}

stock Tt_Finish(playerid)
{
	new first = !Tt_IsDone(playerid);
	g_TtStep[playerid] = TT_END;
	g_TtHas[playerid] = 0;
	Tt_Hud(playerid, "end", first ? (",\"rew\":1500") : (",\"rew\":0"));
	g_TtStep[playerid] = -1;
	Gps_Clear(playerid);
	if (first)
	{
		GivePlayerMoney(playerid, TT_REWARD);
		Cw_Notify(playerid, "success", "Tutoriel termine", "Bienvenue a Crestwood ! +1 500 $ offerts par la mairie.", 6000);
	}
	Tt_SetDone(playerid);
	return 1;
}

stock Tt_Stop(playerid)
{
	g_TtStep[playerid] = -1;
	g_TtRing[playerid] = 0;
	g_TtNext[playerid] = 0;
	g_TtHas[playerid] = 0;
	Gps_Clear(playerid);
	Tt_SetDone(playerid);
	Tt_Hud(playerid, "off");
	return Cw_Notify(playerid, "info", "Tutoriel", "Tu peux rappeler Maya quand tu veux : /aide > Tutoriel, ou la tablette RP.", 6000), 1;
}

stock Tt_Second(playerid)
{
	new now = gettime();
	if (g_TtRing[playerid] > 0)
	{
		g_TtRing[playerid]--;
		if (g_TtRing[playerid] == 0)
		{
			g_TtMissed[playerid]++;
			Tt_Hud(playerid, "missed");
			if (g_TtMissed[playerid] < 3) g_TtNext[playerid] = now + 90;
			else Cw_Notify(playerid, "info", "Appel manque", "Maya (mairie) a essaye de t'appeler. Rappelle-la : /aide > Tutoriel.", 7000);
		}
		else if (now - g_TtLastSend[playerid] >= 6) Tt_Hud(playerid, "ring");
		return 1;
	}
	if (g_TtNext[playerid] && now >= g_TtNext[playerid] && g_TtStep[playerid] < 0) return Tt_Ring(playerid), 1;
	new s = g_TtStep[playerid];
	if (s < 0) return 1;
	if (s == TT_KIT)
	{
		if (Kit_Claimed(playerid)) return Tt_Enter(playerid, s + 1, 1);
		return 1;
	}
	if (!g_TtHas[playerid]) return 1;
	new d = floatround(GetPlayerDistanceFromPoint(playerid, g_TtX[playerid], g_TtY[playerid], g_TtZ[playerid]));
	if (GetPlayerVirtualWorld(playerid) == 0 && d <= floatround(TT_RANGE)) return Tt_Enter(playerid, s + 1, 1);
	new diff = d - g_TtDist[playerid];
	if (diff < 0) diff = -diff;
	if (diff >= 6 || now - g_TtLastSend[playerid] >= 5)
	{
		g_TtDist[playerid] = d;
		Tt_Hud(playerid, "call");
	}
	return 1;
}

stock Tt_Command(playerid, const arg[])
{
	if (!strcmp(arg, "accept", true))
	{
		if (!g_TtRing[playerid]) return 1;
		g_TtRing[playerid] = 0;
		return Tt_Enter(playerid, TT_INTRO);
	}
	if (!strcmp(arg, "decline", true))
	{
		if (!g_TtRing[playerid]) return 1;
		g_TtRing[playerid] = 0;
		g_TtMissed[playerid]++;
		if (g_TtMissed[playerid] < 3) g_TtNext[playerid] = gettime() + 120;
		Tt_Hud(playerid, "off");
		return Cw_Notify(playerid, "info", "Appel refuse", "Maya rappellera dans 2 minutes (ou /aide > Tutoriel).", 5000), 1;
	}
	new s = g_TtStep[playerid];
	if (s < 0) return 1;
	if (!strcmp(arg, "next", true))
	{
		if (g_TtHas[playerid] || s == TT_KIT) return Tt_Hud(playerid, "call"), 1;   // il faut y aller (ou "passer")
		return Tt_Enter(playerid, s + 1);
	}
	if (!strcmp(arg, "skip", true)) return Tt_Enter(playerid, s + 1);
	if (!strcmp(arg, "gps", true))
	{
		if (g_TtHas[playerid]) Gps_Set(playerid, g_TtX[playerid], g_TtY[playerid]);
		if (s == TT_KIT) Kit_Show(playerid);
		return 1;
	}
	if (!strcmp(arg, "stop", true)) return Tt_Stop(playerid);
	if (!strcmp(arg, "sync", true)) return Tt_Hud(playerid, g_TtRing[playerid] ? ("ring") : ("call")), 1;
	return 1;
}

// ------------------------------------------------------------
//  Minuterie (1 s)
// ------------------------------------------------------------
public C46_Tick()
{
	g_C46Sec++;
	new now = gettime();
	for (new p = 0, m = GetMaxPlayers(); p < m; p++)
	{
		if (!IsPlayerConnected(p) || !g_AuthLogged[p] || IsPlayerNPC(p)) continue;
		if (g_C46Sec % 60 == 0) g_GsMin[p]++;
		if (g_TtRing[p] || g_TtNext[p] || g_TtStep[p] >= 0) Tt_Second(p);
	}
	if (g_DtDirty && g_C46Sec % 2 == 0) { g_DtDirty = 0; Dt_Broadcast(); }
	if (g_C46Sec % 600 == 0) Gs_FlushAll();
	if (g_C46Sec % 60 == 30)
	{
		new d = Gs_Day();
		if (d != g_C46Day) { g_C46Day = d; Dt_NewDay(); }
	}
	if (now - g_C46Income >= GANG_INCOME_EVERY)
	{
		g_C46Income = now;
		Gang_Income();
	}
	return 1;
}

// ------------------------------------------------------------
//  Commandes et pages
// ------------------------------------------------------------
stock Cw46_PageAction(playerid, const page[], const act[], const args[])
{
	if (!strcmp(page, GRP_PAGE, true)) return Grp_Action(playerid, act, args), 1;
	return 0;
}

stock Cw46_OnCommand(playerid, const cmdtext[])
{
	new idx;
	if (Fx_IsCmd(cmdtext, "/groupe", idx) || Fx_IsCmd(cmdtext, "/messtats", idx)) return Grp_Open(playerid), 1;
	if (!strcmp(cmdtext, "/tuto ", true, 6))
	{
		new a[12], i = 6;
		while (cmdtext[i] == ' ') i++;
		format(a, sizeof a, "%s", cmdtext[i]);
		return Tt_Command(playerid, a), 1;
	}
	if (Fx_IsCmd(cmdtext, "/tutoriel", idx)) return Tt_Ring(playerid, 1), 1;
	if (Fx_IsCmd(cmdtext, "/nomsstaff", idx))
	{
		if (!Cw_IsStaff(playerid)) return 0;
		g_DtNames[playerid] = !g_DtNames[playerid];
		Dt_SendTo(playerid);
		return Cw_Notify(playerid, "info", "Staff", g_DtNames[playerid] ? ("Pseudos visibles au-dessus des tetes (toi seul).") : ("Identifiants du jour au-dessus des tetes."), 4000), 1;
	}
	if (Fx_IsCmd(cmdtext, "/qui", idx))
	{
		// staff : qui est (4821) ?
		if (!Cw_IsStaff(playerid)) return 0;
		new tok[8];
		Frp_Tok(cmdtext, idx, tok, sizeof tok);
		new t = Dt_Find(strval(tok)), m[120], nm[MAX_PLAYER_NAME + 1];
		if (t == INVALID_PLAYER_ID) return Cw_Notify(playerid, "denied", "Staff", "Personne avec cet identifiant aujourd'hui.", 3500), 1;
		GetPlayerName(t, nm, sizeof nm);
		format(m, sizeof m, "(%04d) = %s (id %d)", g_DtTag[t], nm, t);
		return Cw_Notify(playerid, "info", "Staff", m, 6000), 1;
	}
	if (Fx_IsCmd(cmdtext, "/tutoreset", idx))
	{
		if (Admin_Level(playerid) < 2) return 0;
		new tok[8], q[160], nm[MAX_PLAYER_NAME + 1];
		Frp_Tok(cmdtext, idx, tok, sizeof tok);
		new t = strval(tok);
		if (!tok[0] || t < 0 || t >= MAX_PLAYERS || !IsPlayerConnected(t) || !g_AuthLogged[t]) return Cw_Notify(playerid, "denied", "Staff", "/tutoreset <id joueur>", 3500), 1;
		GetPlayerName(t, nm, sizeof nm);
		format(q, sizeof q, "DELETE FROM tuto WHERE name = '%s'", nm);
		C46_Exec(q);
		g_TtDone[t] = 0;
		g_TtMissed[t] = 0;
		Tt_Ring(t, 1);
		return Cw_Notify(playerid, "success", "Staff", "Tutoriel relance pour ce joueur (son telephone sonne).", 4000), 1;
	}
	if (Fx_IsCmd(cmdtext, "/gangcoffre", idx))
	{
		if (Admin_Level(playerid) < 6) return 0;
		new tg[8], tv[16], m[120];
		Frp_Tok(cmdtext, idx, tg, sizeof tg);
		Frp_Tok(cmdtext, idx, tv, sizeof tv);
		new g = strval(tg), v = strval(tv);
		if (g < 1 || g > GANG_COUNT || !tv[0]) return Cw_Notify(playerid, "denied", "Staff", "/gangcoffre <gang 1-4> <montant +/->", 3500), 1;
		new b = Gb_Add(g, v);
		format(m, sizeof m, "Coffre des %s : %d $", g_GangName[g], b);
		return Cw_Notify(playerid, "success", "Staff", m, 4000), 1;
	}
	return 0;
}
