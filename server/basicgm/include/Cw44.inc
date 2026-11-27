/* ============================================================
   CRESTWOOD RP v44 (21/11/2026) - NOUVEAUX SYSTEMES AVEC PAGES
   ------------------------------------------------------------
   Pages generiques (SAMP.java, "@page <nom>", 9970-9972) : assets/casino, assets/bourse, assets/event
   page -> serveur : /pageact <page> <action> [args]
   1) CASINO (civil) : machine a sous, roulette, blackjack. Mises 100 a 50 000 $ en liquide.
      Points : Alhambra (Idlewood) par defaut + /casinoadd, /casinodel (staff). Marqueur 3D + main bleue.
      /casino : ouvre la page pres d'un casino, sinon GPS vers le plus proche.
   2) BOURSE (civil) : 8 entreprises de Crestwood, cours mis a jour toutes les 5 minutes (+ actualites
      qui font monter / chuter un titre). Achat / vente avec le compte bancaire (frais 1 %).
      Portefeuille sauvegarde. /bourse (tablette RP > Bourse).
   3) EVENEMENTS (staff + joueurs) : le staff cree un evenement depuis la page (Rassemblement, Derby,
      Survie) avec une recompense ; les joueurs s'inscrivent (2 min), puis depart automatique.
      Derby : voitures en monde prive sur l'aeroport de LS, derniere voiture en etat gagne.
      Survie : a pied, armes fournies, dernier debout gagne. Rassemblement : tout le monde rejoint
      le staff. /evenements (tablette RP > Evenements).
   4) METIERS : 4 nouveaux metiers dans le systeme d'emplois existant (page Emplois) :
      Agent de proprete (balayeuse), Routier (camion), Convoyeur de fonds (fourgon blinde), Facteur (scooter).
      Le staff place leur siege avec le panneau /points, comme les autres metiers.
   Texte ASCII.
   ============================================================ */
#define C44_COL_INFO          0x33CCFFFF
#define C44_COL_OK            0x33CC33FF
#define C44_COL_ERR           0xFF6347FF

new DB:g_C44DB;

stock C44_Exec(const q[])
{
	if (g_C44DB != DB:0) db_free_result(db_query(g_C44DB, q));
	return 1;
}

stock C44_Int(const q[])
{
	new v = 0, f[20];
	if (g_C44DB == DB:0) return 0;
	new DBResult:r = db_query(g_C44DB, q);
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

// marqueur 3D avec un nom (le client lit "n")
stock Mk_AddN(type, Float:x, Float:y, Float:z, const name[])
{
	new part[110], nm[32];
	Pg_Esc(name, nm, sizeof nm);
	format(part, sizeof part, "%s{\"t\":%d,\"x\":%.2f,\"y\":%.2f,\"z\":%.2f,\"n\":\"%s\"}", g_MkN ? (",") : (""), type, x, y, z, nm);
	if (strlen(g_MkBuf) + strlen(part) > sizeof(g_MkBuf) - 4) { Mk_Flush(); format(part, sizeof part, "{\"t\":%d,\"x\":%.2f,\"y\":%.2f,\"z\":%.2f,\"n\":\"%s\"}", type, x, y, z, nm); }
	strcat(g_MkBuf, part, sizeof g_MkBuf);
	g_MkN++;
	return 1;
}

stock C44_Name(playerid, out[], size)
{
	GetPlayerName(playerid, out, size);
	return 1;
}

// ============================================================
//  1) CASINO
// ============================================================
#define CAS_MAX               12
#define CAS_RANGE             2.0
#define CAS_MIN_BET           100
#define CAS_MAX_BET           50000
#define CAS_MARK              20

new g_CasCount, g_CasId[CAS_MAX], Float:g_CasPos[CAS_MAX][3], g_CasName[CAS_MAX][28];
new Text3D:g_CasLabel[CAS_MAX] = {Text3D:INVALID_3DTEXT_ID, ...};
new g_CasLast[MAX_PLAYERS];
// blackjack
new g_BjOn[MAX_PLAYERS], g_BjBet[MAX_PLAYERS];
new g_BjP[MAX_PLAYERS][10], g_BjPN[MAX_PLAYERS];
new g_BjD[MAX_PLAYERS][10], g_BjDN[MAX_PLAYERS];
// derniere partie (pour l'affichage)
new g_CasRes[MAX_PLAYERS][160];

stock Cas_Build(i)
{
	if (g_CasLabel[i] != Text3D:INVALID_3DTEXT_ID) Delete3DTextLabel(g_CasLabel[i]);
	new t[96];
	format(t, sizeof t, "{FFD54A}%s\n{FFFFFF}Machines a sous - Roulette - Blackjack", g_CasName[i]);
	g_CasLabel[i] = Create3DTextLabel(t, 0xFFFFFFFF, g_CasPos[i][0], g_CasPos[i][1], g_CasPos[i][2] + 0.6, 10.0, 0, 1);
	return 1;
}

stock Cas_Load()
{
	for (new i = 0; i < g_CasCount; i++) if (g_CasLabel[i] != Text3D:INVALID_3DTEXT_ID) { Delete3DTextLabel(g_CasLabel[i]); g_CasLabel[i] = Text3D:INVALID_3DTEXT_ID; }
	g_CasCount = 0;
	if (g_C44DB == DB:0) return 0;
	if (!C44_Int("SELECT COUNT(*) FROM casino_points"))
		C44_Exec("INSERT INTO casino_points (name, x, y, z) VALUES ('Casino Alhambra', 1836.7, -1682.4, 13.3)");
	new DBResult:r = db_query(g_C44DB, "SELECT id, name, x, y, z FROM casino_points ORDER BY id");
	if (r == DBResult:0) return 0;
	new f[32];
	for (new n = db_num_rows(r), i = 0; i < n && g_CasCount < CAS_MAX; i++)
	{
		new k = g_CasCount;
		db_get_field(r, 0, f, sizeof f); g_CasId[k] = strval(f);
		db_get_field(r, 1, g_CasName[k], 28);
		db_get_field(r, 2, f, sizeof f); g_CasPos[k][0] = floatstr(f);
		db_get_field(r, 3, f, sizeof f); g_CasPos[k][1] = floatstr(f);
		db_get_field(r, 4, f, sizeof f); g_CasPos[k][2] = floatstr(f);
		Cas_Build(k);
		g_CasCount++;
		db_next_row(r);
	}
	db_free_result(r);
	return g_CasCount;
}

stock Cas_Near(playerid, Float:range)
{
	if (GetPlayerVirtualWorld(playerid) != 0) return -1;
	for (new i = 0; i < g_CasCount; i++) if (IsPlayerInRangeOfPoint(playerid, range, g_CasPos[i][0], g_CasPos[i][1], g_CasPos[i][2])) return i;
	return -1;
}

stock Cas_Nearest(playerid)
{
	new Float:x, Float:y, Float:z, best = -1, Float:bd = 1.0e9;
	GetPlayerPos(playerid, x, y, z);
	for (new i = 0; i < g_CasCount; i++)
	{
		new Float:d = (x - g_CasPos[i][0]) * (x - g_CasPos[i][0]) + (y - g_CasPos[i][1]) * (y - g_CasPos[i][1]);
		if (d < bd) { bd = d; best = i; }
	}
	return best;
}

// valeur d'une main de blackjack (as = 1 ou 11)
stock Bj_Value(const cards[], n)
{
	new v = 0, aces = 0;
	for (new i = 0; i < n; i++)
	{
		new c = cards[i];
		if (c == 1) { aces++; v += 11; }
		else if (c >= 10) v += 10;
		else v += c;
	}
	while (v > 21 && aces > 0) { v -= 10; aces--; }
	return v;
}

stock Bj_Cards(const cards[], n, out[], size, hideSecond = 0)
{
	out[0] = 0;
	new part[8];
	for (new i = 0; i < n; i++)
	{
		format(part, sizeof part, "%s%d", i ? (",") : (""), (hideSecond && i == 1) ? 0 : cards[i]);
		strcat(out, part, size);
	}
	return 1;
}

stock Cas_Send(playerid, dlg = PAGE_DLG_UPDATE, ok = -1, const msg[] = "")
{
	new json[900], pc[40], dc[40], ci = Cas_Near(playerid, CAS_RANGE + 6.0);
	Bj_Cards(g_BjP[playerid], g_BjPN[playerid], pc, sizeof pc);
	Bj_Cards(g_BjD[playerid], g_BjDN[playerid], dc, sizeof dc, g_BjOn[playerid]);
	format(json, sizeof json, "{\"v\":\"casino\",\"name\":\"%s\",\"cash\":%d,\"min\":%d,\"max\":%d,\"bj\":{\"on\":%d,\"bet\":%d,\"p\":[%s],\"d\":[%s],\"pv\":%d,\"dv\":%d},\"res\":{%s}}",
		(ci >= 0) ? g_CasName[ci] : ("Casino"), GetPlayerMoney(playerid), CAS_MIN_BET, CAS_MAX_BET,
		g_BjOn[playerid], g_BjBet[playerid], pc, dc, Bj_Value(g_BjP[playerid], g_BjPN[playerid]),
		g_BjOn[playerid] ? ((g_BjD[playerid][0] == 1) ? 11 : ((g_BjD[playerid][0] >= 10) ? 10 : g_BjD[playerid][0])) : Bj_Value(g_BjD[playerid], g_BjDN[playerid]),
		g_CasRes[playerid]);
	Pg_AddMsg(json, sizeof json, ok, msg);
	return Pg_Send(playerid, dlg, "casino", json);
}

stock Cas_Open(playerid)
{
	if (Cas_Near(playerid, CAS_RANGE + 1.5) < 0)
	{
		new i = Cas_Nearest(playerid);
		if (i < 0) return Cw_Notify(playerid, "denied", "Casino", "Aucun casino en ville pour le moment.", 4000), 1;
		Gps_Set(playerid, g_CasPos[i][0], g_CasPos[i][1]);
		return Cw_Notify(playerid, "info", "Casino", "GPS vers le casino le plus proche. Marche sur le point pour jouer.", 5000), 1;
	}
	g_CasRes[playerid][0] = 0;
	return Cas_Send(playerid, PAGE_DLG_OPEN);
}

stock Cas_TakeBet(playerid, bet, msg[], msize)
{
	if (bet < CAS_MIN_BET || bet > CAS_MAX_BET) return format(msg, msize, "Mise de %d a %d $.", CAS_MIN_BET, CAS_MAX_BET), 0;
	if (GetPlayerMoney(playerid) < bet) return format(msg, msize, "Pas assez de liquide."), 0;
	GivePlayerMoney(playerid, -bet);
	return 1;
}

// symboles : 0 cerise, 1 citron, 2 cloche, 3 BAR, 4 diamant, 5 sept
stock Slot_Symbol()
{
	new r = random(100);
	if (r < 30) return 0;
	if (r < 55) return 1;
	if (r < 73) return 2;
	if (r < 87) return 3;
	if (r < 96) return 4;
	return 5;
}

stock Cas_Slot(playerid, bet)
{
	new msg[80];
	if (!Cas_TakeBet(playerid, bet, msg, sizeof msg)) return Cas_Send(playerid, PAGE_DLG_UPDATE, 0, msg);
	new a = Slot_Symbol(), b = Slot_Symbol(), c = Slot_Symbol(), mult = 0;
	if (a == b && b == c)
	{
		switch (a)
		{
			case 5: mult = 25;
			case 4: mult = 12;
			case 3: mult = 8;
			default: mult = 5;
		}
	}
	else
	{
		new cher = 0;
		if (a == 0) cher++;
		if (b == 0) cher++;
		if (c == 0) cher++;
		if (cher >= 2) mult = 2;
	}
	new win = bet * mult;
	if (win > 0) GivePlayerMoney(playerid, win);
	format(g_CasRes[playerid], 160, "\"g\":\"slot\",\"r\":[%d,%d,%d],\"bet\":%d,\"win\":%d", a, b, c, bet, win);
	if (win >= 100000)
	{
		new n[MAX_PLAYER_NAME + 1], m[128];
		Fx_RPName(playerid, n, sizeof n);
		format(m, sizeof m, "%s decroche le JACKPOT au casino : ~y~%d $~w~ !", n, win);
		Cw_NotifyAll("success", "Casino", m, 6000);
	}
	return Cas_Send(playerid, PAGE_DLG_UPDATE, win > 0 ? 1 : -1, win > 0 ? ("Gagne !") : (""));
}

stock Roul_IsRed(n)
{
	switch (n)
	{
		case 1, 3, 5, 7, 9, 12, 14, 16, 18, 19, 21, 23, 25, 27, 30, 32, 34, 36: return 1;
	}
	return 0;
}

stock Cas_Roulette(playerid, bet, const choice[])
{
	new msg[80];
	new kind = 0, num = -1;
	if (!strcmp(choice, "rouge", true)) kind = 1;
	else if (!strcmp(choice, "noir", true)) kind = 2;
	else if (!strcmp(choice, "pair", true)) kind = 3;
	else if (!strcmp(choice, "impair", true)) kind = 4;
	else if (choice[0] == 'n' && choice[1] >= '0' && choice[1] <= '9') { kind = 5; num = strval(choice[1]); }
	if (!kind || (kind == 5 && (num < 0 || num > 36))) return Cas_Send(playerid, PAGE_DLG_UPDATE, 0, "Choix invalide.");
	if (!Cas_TakeBet(playerid, bet, msg, sizeof msg)) return Cas_Send(playerid, PAGE_DLG_UPDATE, 0, msg);
	new n = random(37), win = 0;
	switch (kind)
	{
		case 1: if (n != 0 && Roul_IsRed(n)) win = bet * 2;
		case 2: if (n != 0 && !Roul_IsRed(n)) win = bet * 2;
		case 3: if (n != 0 && n % 2 == 0) win = bet * 2;
		case 4: if (n % 2 == 1) win = bet * 2;
		case 5: if (n == num) win = bet * 36;
	}
	if (win > 0) GivePlayerMoney(playerid, win);
	format(g_CasRes[playerid], 160, "\"g\":\"roul\",\"n\":%d,\"red\":%d,\"bet\":%d,\"win\":%d", n, (n != 0 && Roul_IsRed(n)) ? 1 : 0, bet, win);
	return Cas_Send(playerid, PAGE_DLG_UPDATE, win > 0 ? 1 : -1, win > 0 ? ("Gagne !") : (""));
}

stock Bj_Draw() return 1 + random(13);

stock Bj_Finish(playerid)
{
	new pv = Bj_Value(g_BjP[playerid], g_BjPN[playerid]);
	// le croupier tire jusqu'a 17
	while (Bj_Value(g_BjD[playerid], g_BjDN[playerid]) < 17 && g_BjDN[playerid] < 9) g_BjD[playerid][g_BjDN[playerid]++] = Bj_Draw();
	new dv = Bj_Value(g_BjD[playerid], g_BjDN[playerid]), bet = g_BjBet[playerid], win = 0, res[16];
	if (pv > 21) format(res, sizeof res, "perdu");
	else if (pv == 21 && g_BjPN[playerid] == 2 && !(dv == 21 && g_BjDN[playerid] == 2)) { win = bet + bet * 3 / 2; format(res, sizeof res, "blackjack"); }
	else if (dv > 21 || pv > dv) { win = bet * 2; format(res, sizeof res, "gagne"); }
	else if (pv == dv) { win = bet; format(res, sizeof res, "egalite"); }
	else format(res, sizeof res, "perdu");
	if (win > 0) GivePlayerMoney(playerid, win);
	g_BjOn[playerid] = 0;
	format(g_CasRes[playerid], 160, "\"g\":\"bj\",\"res\":\"%s\",\"bet\":%d,\"win\":%d", res, bet, win);
	return Cas_Send(playerid, PAGE_DLG_UPDATE, win > bet ? 1 : -1, "");
}

stock Cas_Bj(playerid, const act[], bet)
{
	new msg[80];
	if (!strcmp(act, "bjdeal"))
	{
		if (g_BjOn[playerid]) return Cas_Send(playerid, PAGE_DLG_UPDATE, 0, "Termine la main en cours.");
		if (!Cas_TakeBet(playerid, bet, msg, sizeof msg)) return Cas_Send(playerid, PAGE_DLG_UPDATE, 0, msg);
		g_BjOn[playerid] = 1;
		g_BjBet[playerid] = bet;
		g_BjPN[playerid] = 2; g_BjP[playerid][0] = Bj_Draw(); g_BjP[playerid][1] = Bj_Draw();
		g_BjDN[playerid] = 2; g_BjD[playerid][0] = Bj_Draw(); g_BjD[playerid][1] = Bj_Draw();
		g_CasRes[playerid][0] = 0;
		if (Bj_Value(g_BjP[playerid], 2) == 21) return Bj_Finish(playerid);
		return Cas_Send(playerid);
	}
	if (!g_BjOn[playerid]) return Cas_Send(playerid, PAGE_DLG_UPDATE, 0, "Distribue d'abord une main.");
	if (!strcmp(act, "bjhit"))
	{
		if (g_BjPN[playerid] < 9) g_BjP[playerid][g_BjPN[playerid]++] = Bj_Draw();
		if (Bj_Value(g_BjP[playerid], g_BjPN[playerid]) >= 21) return Bj_Finish(playerid);
		return Cas_Send(playerid);
	}
	if (!strcmp(act, "bjdouble"))
	{
		if (g_BjPN[playerid] != 2) return Cas_Send(playerid, PAGE_DLG_UPDATE, 0, "Doubler : seulement avec 2 cartes.");
		if (GetPlayerMoney(playerid) < g_BjBet[playerid]) return Cas_Send(playerid, PAGE_DLG_UPDATE, 0, "Pas assez de liquide pour doubler.");
		GivePlayerMoney(playerid, -g_BjBet[playerid]);
		g_BjBet[playerid] *= 2;
		g_BjP[playerid][g_BjPN[playerid]++] = Bj_Draw();
		return Bj_Finish(playerid);
	}
	return Bj_Finish(playerid);   // bjstand
}

stock Cas_Action(playerid, const act[], const args[])
{
	if (Cas_Near(playerid, CAS_RANGE + 6.0) < 0) { Pg_Send(playerid, PAGE_DLG_CLOSE, "casino", "{}"); return Cw_Notify(playerid, "denied", "Casino", "Tu t'es eloigne du casino.", 3000), 1; }
	new now = GetTickCount();
	if (g_CasLast[playerid] && now - g_CasLast[playerid] < 700) return 1;
	g_CasLast[playerid] = (now == 0) ? 1 : now;
	new pos = 0, t1[16], t2[16];
	Pg_Tok(args, pos, t1, sizeof t1);
	Pg_Tok(args, pos, t2, sizeof t2);
	if (!strcmp(act, "slot")) return Cas_Slot(playerid, strval(t1));
	if (!strcmp(act, "roul")) return Cas_Roulette(playerid, strval(t1), t2);
	if (!strcmp(act, "bjdeal") || !strcmp(act, "bjhit") || !strcmp(act, "bjstand") || !strcmp(act, "bjdouble")) return Cas_Bj(playerid, act, strval(t1));
	return Cas_Send(playerid);
}

// ============================================================
//  2) BOURSE
// ============================================================
#define ST_COUNT              8
#define ST_HIST               24
#define ST_TICK_S             300
#define ST_FEE_PCT            1
#define ST_MAX_QTY            100000

new const g_BsName[ST_COUNT][28] =
{
	"Crestwood Petroleum", "Wave Telecom", "Atlas Banque", "Ocean Docks Logistique",
	"Vinewood Studios", "Grove Motors", "Santa Maria Resorts", "LS Construction"
};
new const g_BsTick[ST_COUNT][6] = {"CWP", "WAVE", "ATLS", "ODL", "VNW", "GRVM", "SMR", "LSC"};
new const g_BsBase[ST_COUNT] = {420, 180, 650, 260, 95, 310, 140, 220};
new const g_BsVol[ST_COUNT] = {30, 45, 20, 30, 70, 40, 50, 35};   // volatilite (pour mille)
new g_BsPrice[ST_COUNT], g_BsPrev[ST_COUNT];
new g_BsHistV[ST_COUNT][ST_HIST];
new g_BsNext;
new g_BsNews[120];
new g_BsLast[MAX_PLAYERS];

stock St_Load()
{
	for (new i = 0; i < ST_COUNT; i++)
	{
		new q[96];
		format(q, sizeof q, "SELECT price FROM stock_price WHERE id = %d", i);
		new p = C44_Int(q);
		if (p <= 0) p = g_BsBase[i];
		g_BsPrice[i] = p;
		g_BsPrev[i] = p;
		for (new h = 0; h < ST_HIST; h++) g_BsHistV[i][h] = p;
	}
	g_BsNext = gettime() + ST_TICK_S;
	format(g_BsNews, sizeof g_BsNews, "Ouverture de la Bourse de Crestwood.");
	return 1;
}

stock St_Save(i)
{
	new q[96];
	format(q, sizeof q, "INSERT OR REPLACE INTO stock_price (id, price) VALUES (%d, %d)", i, g_BsPrice[i]);
	return C44_Exec(q);
}

stock St_Tick()
{
	new newsCo = (random(100) < 30) ? random(ST_COUNT) : -1, newsUp = random(2);
	for (new i = 0; i < ST_COUNT; i++)
	{
		g_BsPrev[i] = g_BsPrice[i];
		new vol = g_BsVol[i];
		new r = random(vol * 2 + 1) - vol;                        // -vol .. +vol (pour mille)
		// rappel vers le prix de base (le cours ne part pas a l'infini)
		if (g_BsPrice[i] > g_BsBase[i] * 3) r -= vol / 2;
		else if (g_BsPrice[i] < g_BsBase[i] / 3) r += vol / 2;
		else r += 1;                                               // leger biais haussier
		if (i == newsCo) r += newsUp ? (60 + random(80)) : -(60 + random(80));
		new p = g_BsPrice[i] + g_BsPrice[i] * r / 1000;
		if (p == g_BsPrice[i]) p += (r > 0) ? 1 : ((r < 0) ? -1 : 0);
		if (p < 5) p = 5;
		g_BsPrice[i] = p;
		for (new h = 0; h < ST_HIST - 1; h++) g_BsHistV[i][h] = g_BsHistV[i][h + 1];
		g_BsHistV[i][ST_HIST - 1] = p;
		St_Save(i);
	}
	if (newsCo >= 0)
	{
		if (newsUp) format(g_BsNews, sizeof g_BsNews, "%s annonce des resultats record : le titre %s s'envole.", g_BsName[newsCo], g_BsTick[newsCo]);
		else format(g_BsNews, sizeof g_BsNews, "Scandale chez %s : le titre %s chute.", g_BsName[newsCo], g_BsTick[newsCo]);
	}
	else format(g_BsNews, sizeof g_BsNews, "Seance calme a la Bourse de Crestwood.");
	for (new p = 0, m = GetMaxPlayers(); p < m; p++) if (IsPlayerConnected(p) && Pg_IsOpen(p, "bourse")) St_Send(p);
	return 1;
}

stock St_Own(playerid, i, &avg)
{
	new name[MAX_PLAYER_NAME + 1], q[160], qty = 0, f[20];
	avg = 0;
	GetPlayerName(playerid, name, sizeof name);
	format(q, sizeof q, "SELECT qty, avg FROM stock_own WHERE name = '%s' AND id = %d", name, i);
	new DBResult:r = db_query(g_C44DB, q);
	if (r != DBResult:0)
	{
		if (db_num_rows(r) > 0)
		{
			db_get_field(r, 0, f, sizeof f); qty = strval(f);
			db_get_field(r, 1, f, sizeof f); avg = strval(f);
		}
		db_free_result(r);
	}
	return qty;
}

stock St_SetOwn(playerid, i, qty, avg)
{
	new name[MAX_PLAYER_NAME + 1], q[200];
	GetPlayerName(playerid, name, sizeof name);
	if (qty <= 0) format(q, sizeof q, "DELETE FROM stock_own WHERE name = '%s' AND id = %d", name, i);
	else format(q, sizeof q, "INSERT OR REPLACE INTO stock_own (name, id, qty, avg) VALUES ('%s', %d, %d, %d)", name, i, qty, avg);
	return C44_Exec(q);
}

stock St_Send(playerid, dlg = PAGE_DLG_UPDATE, ok = -1, const msg[] = "")
{
	static json[3000];
	new part[300], hist[160], h[8], avg, total = 0;
	format(json, sizeof json, "{\"v\":\"bourse\",\"bank\":%d,\"next\":%d,\"fee\":%d,\"news\":\"%s\",\"cos\":[", g_BankBalance[playerid], g_BsNext - gettime(), ST_FEE_PCT, g_BsNews);
	for (new i = 0; i < ST_COUNT; i++)
	{
		hist[0] = 0;
		for (new k = 0; k < ST_HIST; k++) { format(h, sizeof h, "%s%d", k ? (",") : (""), g_BsHistV[i][k]); strcat(hist, h, sizeof hist); }
		new own = St_Own(playerid, i, avg);
		total += own * g_BsPrice[i];
		format(part, sizeof part, "%s[%d,\"%s\",\"%s\",%d,%d,[%s],%d,%d]", i ? (",") : (""), i, g_BsName[i], g_BsTick[i], g_BsPrice[i], g_BsPrev[i], hist, own, avg);
		strcat(json, part, sizeof json);
	}
	format(part, sizeof part, "],\"pf\":%d}", total);
	strcat(json, part, sizeof json);
	Pg_AddMsg(json, sizeof json, ok, msg);
	return Pg_Send(playerid, dlg, "bourse", json);
}

stock St_Open(playerid)
{
	if (!g_BankLoaded[playerid]) return Cw_Notify(playerid, "denied", "Bourse", "Compte bancaire indisponible pour le moment.", 3000), 1;
	return St_Send(playerid, PAGE_DLG_OPEN);
}

stock St_Action(playerid, const act[], const args[])
{
	new now = GetTickCount();
	if (g_BsLast[playerid] && now - g_BsLast[playerid] < 600) return 1;
	g_BsLast[playerid] = (now == 0) ? 1 : now;
	if (!strcmp(act, "refresh")) return St_Send(playerid);
	new pos = 0, t1[8], t2[12];
	Pg_Tok(args, pos, t1, sizeof t1);
	Pg_Tok(args, pos, t2, sizeof t2);
	new i = strval(t1), qty = strval(t2), m[128], a[16], avg;
	if (i < 0 || i >= ST_COUNT || qty <= 0 || qty > ST_MAX_QTY) return St_Send(playerid, PAGE_DLG_UPDATE, 0, "Quantite invalide.");
	new own = St_Own(playerid, i, avg);
	if (!strcmp(act, "buy"))
	{
		new cost = g_BsPrice[i] * qty, fee = cost * ST_FEE_PCT / 100;
		if (cost <= 0 || cost > 500000000) return St_Send(playerid, PAGE_DLG_UPDATE, 0, "Ordre trop gros.");
		if (g_BankBalance[playerid] < cost + fee) return St_Send(playerid, PAGE_DLG_UPDATE, 0, "Solde bancaire insuffisant.");
		g_BankBalance[playerid] -= cost + fee;
		Bank_AddHistory(playerid, "Bourse : achat", -(cost + fee));
		Bank_SavePlayer(playerid);
		new navg = (own * avg + cost) / (own + qty);
		St_SetOwn(playerid, i, own + qty, navg);
		Cw_Fmt(cost + fee, a, sizeof a);
		format(m, sizeof m, "Achat de %d %s pour %s $ (frais compris).", qty, g_BsTick[i], a);
		return St_Send(playerid, PAGE_DLG_UPDATE, 1, m);
	}
	if (!strcmp(act, "sell"))
	{
		if (qty > own) return St_Send(playerid, PAGE_DLG_UPDATE, 0, "Tu n'as pas autant d'actions.");
		new gain = g_BsPrice[i] * qty, fee = gain * ST_FEE_PCT / 100;
		if (g_BankBalance[playerid] > BANK_MAX_MONEY - gain) return St_Send(playerid, PAGE_DLG_UPDATE, 0, "Compte plein.");
		g_BankBalance[playerid] += gain - fee;
		Bank_AddHistory(playerid, "Bourse : vente", gain - fee);
		Bank_SavePlayer(playerid);
		St_SetOwn(playerid, i, own - qty, avg);
		Cw_Fmt(gain - fee, a, sizeof a);
		format(m, sizeof m, "Vente de %d %s : +%s $ (frais deduits).", qty, g_BsTick[i], a);
		return St_Send(playerid, PAGE_DLG_UPDATE, 1, m);
	}
	return St_Send(playerid);
}

// ============================================================
//  3) EVENEMENTS
// ============================================================
#define EV_NONE               0
#define EV_MEET               1
#define EV_DERBY              2
#define EV_SURV               3
#define EV_JOIN_S             120
#define EV_MAX_S              600
#define EV_WORLD              6100
#define EV_X                  1750.0     // aeroport de Los Santos (piste et herbe, tout plat)
#define EV_Y                  (-2500.0)
#define EV_Z                  13.6
#define EV_RADIUS             70.0
#define EV_PRIZE_MAX          1000000

new g_EvState, g_EvType, g_EvPrize, g_EvBy[MAX_PLAYER_NAME + 1], g_EvEndAt, g_EvStartAt;
new Float:g_EvMeet[3], g_EvMeetInt, g_EvMeetVW;
new g_EvIn[MAX_PLAYERS];          // 0 non, 1 inscrit, 2 en jeu, 3 elimine
new g_EvVeh[MAX_PLAYERS];
new Float:g_EvBack[MAX_PLAYERS][4], g_EvBackInt[MAX_PLAYERS], g_EvBackVW[MAX_PLAYERS];
new g_EvOut[MAX_PLAYERS];
new g_EvReturn[MAX_PLAYERS];      // a remettre a sa place au prochain spawn
new g_EvLast[MAX_PLAYERS];

stock Ev_TypeName(t, out[], size)
{
	switch (t)
	{
		case EV_MEET:  format(out, size, "Rassemblement");
		case EV_DERBY: format(out, size, "Derby");
		case EV_SURV:  format(out, size, "Survie");
		default:       format(out, size, "Aucun");
	}
	return 1;
}

stock Ev_Count(st)
{
	new n = 0;
	for (new p = 0, m = GetMaxPlayers(); p < m; p++) if (IsPlayerConnected(p) && g_EvIn[p] == st) n++;
	return n;
}

stock Ev_Send(playerid, dlg = PAGE_DLG_UPDATE, ok = -1, const msg[] = "")
{
	new json[1400], tn[20], list[700], part[40], n = 0, now = gettime();
	Ev_TypeName(g_EvType, tn, sizeof tn);
	list[0] = 0;
	for (new p = 0, m = GetMaxPlayers(); p < m && n < 20; p++)
	{
		if (!IsPlayerConnected(p) || g_EvIn[p] == 0) continue;
		new nm[MAX_PLAYER_NAME + 1];
		GetPlayerName(p, nm, sizeof nm);
		format(part, sizeof part, "%s[\"%s\",%d]", n ? (",") : (""), nm, g_EvIn[p]);
		strcat(list, part, sizeof list);
		n++;
	}
	new left = (g_EvState == 1) ? (g_EvStartAt - now) : ((g_EvState == 2) ? (g_EvEndAt - now) : 0);
	if (left < 0) left = 0;
	format(json, sizeof json, "{\"v\":\"event\",\"st\":%d,\"type\":%d,\"name\":\"%s\",\"prize\":%d,\"by\":\"%s\",\"left\":%d,\"me\":%d,\"adm\":%d,\"list\":[%s]}",
		g_EvState, g_EvType, tn, g_EvPrize, g_EvBy, left, g_EvIn[playerid], (Admin_Level(playerid) >= 1) ? 1 : 0, list);
	Pg_AddMsg(json, sizeof json, ok, msg);
	return Pg_Send(playerid, dlg, "event", json);
}

stock Ev_PushAll()
{
	for (new p = 0, m = GetMaxPlayers(); p < m; p++) if (IsPlayerConnected(p) && Pg_IsOpen(p, "event")) Ev_Send(p);
	return 1;
}

stock Ev_Restore(p)
{
	if (g_EvVeh[p]) { DestroyVehicle(g_EvVeh[p]); g_EvVeh[p] = 0; }
	if (!IsPlayerConnected(p)) return 0;
	if (g_EvIn[p] >= 2)
	{
		ResetPlayerWeapons(p);
		SetPlayerVirtualWorld(p, g_EvBackVW[p]);
		SetPlayerInterior(p, g_EvBackInt[p]);
		SetPlayerPos(p, g_EvBack[p][0], g_EvBack[p][1], g_EvBack[p][2]);
		SetPlayerFacingAngle(p, g_EvBack[p][3]);
		SetCameraBehindPlayer(p);
		SetPlayerHealth(p, 100.0);
	}
	g_EvIn[p] = 0;
	g_EvOut[p] = 0;
	return 1;
}

stock Ev_End(winner)
{
	new tn[20], m[160];
	Ev_TypeName(g_EvType, tn, sizeof tn);
	if (winner != INVALID_PLAYER_ID && IsPlayerConnected(winner))
	{
		new n[MAX_PLAYER_NAME + 1], a[16];
		Fx_RPName(winner, n, sizeof n);
		Cw_Fmt(g_EvPrize, a, sizeof a);
		if (g_EvPrize > 0) Frp_PayBank(winner, g_EvPrize, "Gain d'evenement");
		format(m, sizeof m, "%s remporte l'evenement %s ! Recompense : ~g~%s $~w~.", n, tn, a);
		Cw_NotifyAll("success", "Evenement", m, 8000);
	}
	else if (g_EvType != EV_MEET)
	{
		format(m, sizeof m, "L'evenement %s est termine sans gagnant.", tn);
		Cw_NotifyAll("info", "Evenement", m, 6000);
	}
	for (new p = 0, mx = GetMaxPlayers(); p < mx; p++) if (g_EvIn[p]) Ev_Restore(p);
	g_EvState = 0;
	g_EvType = EV_NONE;
	g_EvPrize = 0;
	Ev_PushAll();
	return 1;
}

stock Ev_Place(p, k, n)
{
	new Float:a = 360.0 * float(k) / float(n < 1 ? 1 : n);
	new Float:x = EV_X + 22.0 * floatsin(a, degrees), Float:y = EV_Y + 22.0 * floatcos(a, degrees);
	GetPlayerPos(p, g_EvBack[p][0], g_EvBack[p][1], g_EvBack[p][2]);
	GetPlayerFacingAngle(p, g_EvBack[p][3]);
	g_EvBackInt[p] = GetPlayerInterior(p);
	g_EvBackVW[p] = GetPlayerVirtualWorld(p);
	if (IsPlayerInAnyVehicle(p)) RemovePlayerFromVehicle(p);
	g_EvIn[p] = 2;
	g_EvOut[p] = 0;
	SetPlayerInterior(p, 0);
	SetPlayerVirtualWorld(p, EV_WORLD);
	SetPlayerHealth(p, 100.0);
	SetPlayerArmour(p, 0.0);
	if (g_EvType == EV_DERBY)
	{
		static const models[] = {504, 503, 502, 494, 541, 415, 411, 560};
		new v = CreateVehicle(models[random(sizeof models)], x, y, EV_Z + 0.6, a + 180.0, -1, -1, -1);
		if (v != INVALID_VEHICLE_ID)
		{
			SetVehicleVirtualWorld(v, EV_WORLD);
			new engine, lights, alarm, doors, bonnet, boot, objective;
			GetVehicleParamsEx(v, engine, lights, alarm, doors, bonnet, boot, objective);
			SetVehicleParamsEx(v, VEHICLE_PARAMS_ON, lights, alarm, VEHICLE_PARAMS_OFF, bonnet, boot, objective);
			g_EvVeh[p] = v;
			SetPlayerPos(p, x, y, EV_Z + 1.0);
			PutPlayerInVehicle(p, v, 0);
		}
	}
	else
	{
		SetPlayerPos(p, x, y, EV_Z + 0.5);
		SetPlayerFacingAngle(p, a + 180.0);
		ResetPlayerWeapons(p);
		GivePlayerWeapon(p, 24, 120);
		GivePlayerWeapon(p, 25, 40);
		SetPlayerArmour(p, 50.0);
	}
	SetCameraBehindPlayer(p);
	return 1;
}

stock Ev_Start()
{
	new tn[20], m[160], n = Ev_Count(1);
	Ev_TypeName(g_EvType, tn, sizeof tn);
	if (g_EvType == EV_MEET)
	{
		for (new p = 0, mx = GetMaxPlayers(); p < mx; p++)
		{
			if (!IsPlayerConnected(p) || g_EvIn[p] != 1) continue;
			if (IsPlayerInAnyVehicle(p)) RemovePlayerFromVehicle(p);
			SetPlayerInterior(p, g_EvMeetInt);
			SetPlayerVirtualWorld(p, g_EvMeetVW);
			SetPlayerPos(p, g_EvMeet[0] + float(random(6)) - 3.0, g_EvMeet[1] + float(random(6)) - 3.0, g_EvMeet[2]);
			g_EvIn[p] = 0;
		}
		Cw_NotifyAll("info", "Evenement", "Le rassemblement commence : les inscrits ont rejoint le staff.", 6000);
		g_EvState = 0; g_EvType = EV_NONE; g_EvPrize = 0;
		return Ev_PushAll();
	}
	if (n < 2)
	{
		Cw_NotifyAll("denied", "Evenement", "Evenement annule : pas assez d'inscrits (2 minimum).", 6000);
		for (new p = 0, mx = GetMaxPlayers(); p < mx; p++) g_EvIn[p] = 0;
		g_EvState = 0; g_EvType = EV_NONE;
		return Ev_PushAll();
	}
	new k = 0;
	for (new p = 0, mx = GetMaxPlayers(); p < mx; p++) if (IsPlayerConnected(p) && g_EvIn[p] == 1) Ev_Place(p, k++, n);
	g_EvState = 2;
	g_EvEndAt = gettime() + EV_MAX_S;
	format(m, sizeof m, "%s : %d participants, c'est parti ! Dernier en jeu = gagnant.", tn, n);
	for (new p = 0, mx = GetMaxPlayers(); p < mx; p++) if (g_EvIn[p] == 2) Cw_Notify(p, "warning", "Evenement", m, 6000);
	return Ev_PushAll();
}

stock Ev_Eliminate(p, const why[])
{
	if (g_EvIn[p] != 2) return 0;
	new m[128], n[MAX_PLAYER_NAME + 1];
	Fx_RPName(p, n, sizeof n);
	format(m, sizeof m, "%s est elimine (%s). Restants : %d", n, why, Ev_Count(2) - 1);
	for (new i = 0, mx = GetMaxPlayers(); i < mx; i++) if (IsPlayerConnected(i) && g_EvIn[i] >= 2) SendClientMessage(i, 0xFFB020FF, m);
	g_EvIn[p] = 3;
	Ev_Restore(p);
	return 1;
}

stock Ev_Tick()
{
	new now = gettime();
	if (g_EvState == 1 && now >= g_EvStartAt) return Ev_Start();
	if (g_EvState != 2) return 0;
	for (new p = 0, mx = GetMaxPlayers(); p < mx; p++)
	{
		if (!IsPlayerConnected(p) || g_EvIn[p] != 2) continue;
		new Float:x, Float:y, Float:z;
		GetPlayerPos(p, x, y, z);
		if (Frp_Dist2(x, y, EV_X, EV_Y) > EV_RADIUS || GetPlayerVirtualWorld(p) != EV_WORLD) { Ev_Eliminate(p, "sorti de l'arene"); continue; }
		if (g_EvType == EV_DERBY)
		{
			new v = g_EvVeh[p];
			new Float:hp = 0.0;
			if (v) GetVehicleHealth(v, hp);
			if (!v || hp < 300.0) { Ev_Eliminate(p, "voiture detruite"); continue; }
			if (GetPlayerVehicleID(p) != v) { if (++g_EvOut[p] >= 8) Ev_Eliminate(p, "hors de sa voiture"); }
			else g_EvOut[p] = 0;
		}
	}
	new left = Ev_Count(2);
	if (left <= 1 || now >= g_EvEndAt)
	{
		new w = INVALID_PLAYER_ID;
		if (left == 1) for (new p = 0, mx = GetMaxPlayers(); p < mx; p++) if (IsPlayerConnected(p) && g_EvIn[p] == 2) { w = p; break; }
		return Ev_End(w);
	}
	return 1;
}

stock Ev_Create(playerid, type, prize)
{
	if (Admin_Level(playerid) < 1) return Ev_Send(playerid, PAGE_DLG_UPDATE, 0, "Reserve au staff.");
	if (g_EvState) return Ev_Send(playerid, PAGE_DLG_UPDATE, 0, "Un evenement est deja en cours.");
	if (type < EV_MEET || type > EV_SURV) return Ev_Send(playerid, PAGE_DLG_UPDATE, 0, "Type invalide.");
	if (prize < 0) prize = 0;
	if (prize > EV_PRIZE_MAX) prize = EV_PRIZE_MAX;
	g_EvState = 1;
	g_EvType = type;
	g_EvPrize = (type == EV_MEET) ? 0 : prize;
	GetPlayerName(playerid, g_EvBy, sizeof g_EvBy);
	g_EvStartAt = gettime() + EV_JOIN_S;
	GetPlayerPos(playerid, g_EvMeet[0], g_EvMeet[1], g_EvMeet[2]);
	g_EvMeetInt = GetPlayerInterior(playerid);
	g_EvMeetVW = GetPlayerVirtualWorld(playerid);
	for (new p = 0, mx = GetMaxPlayers(); p < mx; p++) g_EvIn[p] = 0;
	new tn[20], m[180], a[16];
	Ev_TypeName(type, tn, sizeof tn);
	Cw_Fmt(g_EvPrize, a, sizeof a);
	if (g_EvPrize > 0) format(m, sizeof m, "Evenement ~y~%s~w~ dans 2 minutes, recompense ~g~%s $~w~. Inscris-toi : tablette RP > Evenements.", tn, a);
	else format(m, sizeof m, "Evenement ~y~%s~w~ dans 2 minutes. Inscris-toi : tablette RP > Evenements.", tn);
	Cw_NotifyAll("warning", "Evenement", m, 10000);
	Ev_PushAll();
	return 1;
}

stock Ev_Action(playerid, const act[], const args[])
{
	new now = GetTickCount();
	if (g_EvLast[playerid] && now - g_EvLast[playerid] < 500) return 1;
	g_EvLast[playerid] = (now == 0) ? 1 : now;
	new pos = 0, t1[8], t2[12];
	Pg_Tok(args, pos, t1, sizeof t1);
	Pg_Tok(args, pos, t2, sizeof t2);
	if (!strcmp(act, "join"))
	{
		if (g_EvState != 1) return Ev_Send(playerid, PAGE_DLG_UPDATE, 0, "Pas d'inscription en cours.");
		if (g_PolCuffed[playerid] || g_PolJailUntil[playerid] > gettime()) return Ev_Send(playerid, PAGE_DLG_UPDATE, 0, "Impossible pour le moment.");
		g_EvIn[playerid] = 1;
		Ev_PushAll();
		return Ev_Send(playerid, PAGE_DLG_UPDATE, 1, "Inscrit ! Reste connecte, depart automatique.");
	}
	if (!strcmp(act, "leave"))
	{
		if (g_EvIn[playerid] == 1) g_EvIn[playerid] = 0;
		else if (g_EvIn[playerid] == 2) Ev_Eliminate(playerid, "abandon");
		Ev_PushAll();
		return 1;
	}
	if (!strcmp(act, "create")) return Ev_Create(playerid, strval(t1), strval(t2));
	if (!strcmp(act, "start"))
	{
		if (Admin_Level(playerid) < 1 || g_EvState != 1) return 1;
		return Ev_Start();
	}
	if (!strcmp(act, "stop"))
	{
		if (Admin_Level(playerid) < 1 || !g_EvState) return 1;
		return Ev_End(INVALID_PLAYER_ID);
	}
	return Ev_Send(playerid);
}

// ============================================================
//  branchements
// ============================================================
forward C44_Tick();
public C44_Tick()
{
	if (gettime() >= g_BsNext) { g_BsNext = gettime() + ST_TICK_S; St_Tick(); }
	if (g_EvState) Ev_Tick();
	return 1;
}

stock Cw44_OnInit()
{
	g_C44DB = db_open("crestwood_v44.db");
	if (g_C44DB != DB:0)
	{
		C44_Exec("CREATE TABLE IF NOT EXISTS casino_points (id INTEGER PRIMARY KEY AUTOINCREMENT, name TEXT, x REAL, y REAL, z REAL)");
		C44_Exec("CREATE TABLE IF NOT EXISTS stock_price (id INTEGER PRIMARY KEY, price INTEGER)");
		C44_Exec("CREATE TABLE IF NOT EXISTS stock_own (name TEXT COLLATE NOCASE, id INTEGER, qty INTEGER, avg INTEGER, PRIMARY KEY (name, id))");
	}
	else print("[V44] ERREUR : impossible d'ouvrir crestwood_v44.db");
	Cas_Load();
	St_Load();
	SetTimer("C44_Tick", 1000, true);
	return 1;
}

stock Cw44_OnConnect(playerid)
{
	g_CasLast[playerid] = 0; g_BjOn[playerid] = 0; g_BjPN[playerid] = 0; g_BjDN[playerid] = 0; g_CasRes[playerid][0] = 0;
	g_BsLast[playerid] = 0;
	g_EvIn[playerid] = 0; g_EvVeh[playerid] = 0; g_EvOut[playerid] = 0; g_EvReturn[playerid] = 0; g_EvLast[playerid] = 0;
	return 1;
}

stock Cw44_OnDisconnect(playerid)
{
	// une main de blackjack en cours est rendue au joueur (pas perdue a cause d'une deconnexion)
	if (g_BjOn[playerid]) { GivePlayerMoney(playerid, g_BjBet[playerid]); g_BjOn[playerid] = 0; }
	if (g_EvVeh[playerid]) { DestroyVehicle(g_EvVeh[playerid]); g_EvVeh[playerid] = 0; }
	g_EvIn[playerid] = 0;
	return 1;
}

// mort pendant un evenement : pas de penalite, retour a sa place au respawn
stock Cw44_OnDeath(playerid)
{
	if (g_EvIn[playerid] != 2) return 0;
	g_EvReturn[playerid] = 1;
	if (g_EvVeh[playerid]) { DestroyVehicle(g_EvVeh[playerid]); g_EvVeh[playerid] = 0; }
	new m[128], n[MAX_PLAYER_NAME + 1];
	Fx_RPName(playerid, n, sizeof n);
	format(m, sizeof m, "%s est elimine. Restants : %d", n, Ev_Count(2) - 1);
	for (new i = 0, mx = GetMaxPlayers(); i < mx; i++) if (IsPlayerConnected(i) && g_EvIn[i] >= 2) SendClientMessage(i, 0xFFB020FF, m);
	g_EvIn[playerid] = 3;
	return 1;
}

stock Cw44_OnSpawn(playerid)
{
	if (!g_EvReturn[playerid]) return 0;
	g_EvReturn[playerid] = 0;
	ResetPlayerWeapons(playerid);
	SetPlayerVirtualWorld(playerid, g_EvBackVW[playerid]);
	SetPlayerInterior(playerid, g_EvBackInt[playerid]);
	SetPlayerPos(playerid, g_EvBack[playerid][0], g_EvBack[playerid][1], g_EvBack[playerid][2]);
	SetPlayerHealth(playerid, 100.0);
	g_EvIn[playerid] = 0;
	return 1;
}

stock Cw44_PushMarks()
{
	for (new i = 0; i < g_CasCount; i++) Mk_AddN(CAS_MARK, g_CasPos[i][0], g_CasPos[i][1], g_CasPos[i][2] - 0.95, g_CasName[i]);
	return 1;
}

// main bleue : casino (220)
stock Cw44_InteractKey(playerid)
{
	if (!g_AuthLogged[playerid]) return 0;
	if (Cas_Near(playerid, CAS_RANGE) >= 0) return 220;
	return 0;
}

stock Cw44_PageAction(playerid, const page[], const act[], const args[])
{
	if (!strcmp(page, "casino", true)) return Cas_Action(playerid, act, args), 1;
	if (!strcmp(page, "bourse", true)) return St_Action(playerid, act, args), 1;
	if (!strcmp(page, "event", true)) return Ev_Action(playerid, act, args), 1;
	return 0;
}

stock Cw44_OnCommand(playerid, const cmdtext[])
{
	new idx;
	if (Fx_IsCmd(cmdtext, "/casino", idx)) return Cas_Open(playerid), 1;
	if (Fx_IsCmd(cmdtext, "/bourse", idx)) return St_Open(playerid), 1;
	if (Fx_IsCmd(cmdtext, "/evenements", idx) || Fx_IsCmd(cmdtext, "/event", idx)) return Ev_Send(playerid, PAGE_DLG_OPEN), 1;
	if (Fx_IsCmd(cmdtext, "/casinoadd", idx))
	{
		if (!Frp_Staff(playerid)) return 0;
		new nm[28], sn[28], q[200], Float:x, Float:y, Float:z;
		Frp_Rest(cmdtext, idx, nm, sizeof nm);
		Law_Clean(nm, sn, sizeof sn);
		if (strlen(sn) < 3) format(sn, sizeof sn, "Casino");
		GetPlayerPos(playerid, x, y, z);
		format(q, sizeof q, "INSERT INTO casino_points (name, x, y, z) VALUES ('%s', %f, %f, %f)", sn, x, y, z);
		C44_Exec(q);
		Cas_Load();
		Marks_SendAll();
		return Cw_Notify(playerid, "success", "Casino", "Point de casino ajoute ici.", 3000), 1;
	}
	if (Fx_IsCmd(cmdtext, "/casinodel", idx))
	{
		if (!Frp_Staff(playerid)) return 0;
		new i = Cas_Near(playerid, 6.0), q[80];
		if (i < 0) return Cw_Notify(playerid, "denied", "Casino", "Place-toi sur le point a supprimer.", 3000), 1;
		format(q, sizeof q, "DELETE FROM casino_points WHERE id = %d", g_CasId[i]);
		C44_Exec(q);
		Cas_Load();
		Marks_SendAll();
		return Cw_Notify(playerid, "success", "Casino", "Point supprime.", 3000), 1;
	}
	return 0;
}
