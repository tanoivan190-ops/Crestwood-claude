/* ============================================================
   VIE RP (v19, 07/11/2026) - Crestwood RP
   ------------------------------------------------------------
   1) Notifications "boite d'aide" (style GTA SA / BMX) : Cw_Notify(playerid, type, titre, texte)
      -> dialogue 9644 "@note" (HUD : hud.js SA HELP). Codes couleur GTA acceptes : ~g~ ~r~ ~y~ ~b~ ~p~ ~w~
   2) Faim / soif : baissent avec le temps, affichees autour de la minimap (envoyees dans "@hudv").
      Hamburger (+35 faim), eau Kirene (+40 soif), Vody (+10 soif), /servir des Burger (+35 faim).
      A 0 : le personnage perd de la sante (jamais en dessous de VIE_MIN_HP).
   3) Experience & niveau : 1 point d'XP toutes les VIE_XP_MINUTES minutes de jeu, niveau = score (TAB),
      prime en banque a chaque niveau. /niveau
   4) Apparence RP : /apparence <texte> (gardee), /examiner <id> (age, origine, apparence)
   5) Loto de Crestwood : /loto <1-50> dans une superette 24/7 (1 000 $, 5 tickets par tirage),
      tirage toutes les heures pile, cagnotte qui grossit tant que personne ne gagne. /loto = infos
   Base : scriptfiles/crestwood_vie.db (tables vie, loto_tickets, vie_meta)
   Inclus apres JobPro (basicgm1.pwn). Les crochets sont appeles depuis les callbacks du GM.
   ============================================================ */
#define VIE_COL_OK            0x5CC85FFF
#define VIE_COL_INFO          0x6F9CFFFF
#define VIE_COL_ERR           0xFF5A6AFF
#define VIE_COL_LOTO          0xF3C75CFF

#define VIE_THIRST_EVERY      1           // -1 soif toutes les X minutes de jeu (100 -> 0 en 100 min)
#define VIE_HUNGER_EVERY      2           // -1 faim toutes les X minutes (100 -> 0 en 200 min)
#define VIE_STARVE_HP         4.0         // sante perdue par minute a 0 de faim ou de soif
#define VIE_MIN_HP            15.0        // la faim ne tue jamais : sante minimum
#define VIE_WARN_AT           20          // alerte a 20 %

#define VIE_XP_MINUTES        30          // 1 XP toutes les 30 minutes de jeu
#define VIE_XP_PER_LEVEL      3           // XP pour passer au niveau suivant = niveau x 3
#define VIE_LEVEL_BONUS       5000        // prime = nouveau niveau x 5 000 $

#define VIE_DESC_LEN          120
#define VIE_EXAM_DIST         6.0

#define VIE_LOTO_PRICE        1000
#define VIE_LOTO_MAX_NUM      50
#define VIE_LOTO_MAX_TICKETS  5
#define VIE_LOTO_BASE         100000      // cagnotte de depart
#define VIE_LOTO_SHARE        80          // % du prix du ticket ajoute a la cagnotte

new DB:g_VieDB;
new g_VieLoaded[MAX_PLAYERS];
new g_VieHunger[MAX_PLAYERS];
new g_VieThirst[MAX_PLAYERS];
new g_VieXp[MAX_PLAYERS];
new g_VieLvl[MAX_PLAYERS];
new g_VieMin[MAX_PLAYERS];                  // minutes de jeu au total
new g_VieTickN[MAX_PLAYERS];
new g_VieWarned[MAX_PLAYERS];               // bit 1 faim, bit 2 soif
new g_VieDesc[MAX_PLAYERS][VIE_DESC_LEN + 1];
new g_VieLotoJackpot;
new g_VieLotoDraw;
new g_VieLotoHour = -1;
forward Vie_Tick();

// ------------------------------------------------------------
//  Notification facon boite d'aide de GTA SA
// ------------------------------------------------------------
stock Cw_Notify(playerid, const type[], const title[], const text[], duration = 6000)
{
	if (!IsPlayerConnected(playerid)) return 0;
	new json[380], t[48], m[260];
	Admin_Clean(title, t, sizeof t);
	Admin_Clean(text, m, sizeof m);
	format(json, sizeof json, "{\"type\":\"%s\",\"title\":\"%s\",\"message\":\"%s\",\"duration\":%d}", type, t, m, duration);
	return ShowPlayerDialog(playerid, NOTE_DIALOG, DIALOG_STYLE_MSGBOX, "@note", json, "OK", "");
}

stock Cw_NotifyAll(const type[], const title[], const text[], duration = 7000)
{
	for (new p = 0, m = GetMaxPlayers(); p < m; p++)
	{
		if (IsPlayerConnected(p) && g_AuthLogged[p]) Cw_Notify(p, type, title, text, duration);
	}
	return 1;
}

// texte libre -> SQL sur (les ' sont doubles) ; out doit faire 2x la taille
stock Vie_Sql(const in[], out[], size)
{
	new j = 0;
	for (new i = 0; in[i] != 0 && j < size - 2; i++)
	{
		if (in[i] == '\'') { out[j++] = '\''; out[j++] = '\''; }
		else out[j++] = in[i];
	}
	out[j] = 0;
	return j;
}

// texte affichable : pas de caracteres de controle, ni " \ % ~ { }
stock Vie_CleanText(const in[], out[], size)
{
	new j = 0;
	for (new i = 0; in[i] != 0 && j < size - 1; i++)
	{
		new c = in[i];
		if (c < 32 || c == '"' || c == '\\' || c == '%' || c == '~' || c == '{' || c == '}' || c == '|') continue;
		out[j++] = c;
	}
	while (j > 0 && out[j - 1] == ' ') j--;
	out[j] = 0;
	return j;
}

// ------------------------------------------------------------
//  Base
// ------------------------------------------------------------
stock Vie_MetaGet(const key[], def)
{
	if (g_VieDB == DB:0) return def;
	new q[96], f[24], v = def;
	format(q, sizeof q, "SELECT v FROM vie_meta WHERE k = '%s'", key);
	new DBResult:r = db_query(g_VieDB, q);
	if (r == DBResult:0) return def;
	if (db_num_rows(r) > 0) { db_get_field(r, 0, f, sizeof f); v = strval(f); }
	db_free_result(r);
	return v;
}

stock Vie_MetaSet(const key[], value)
{
	if (g_VieDB == DB:0) return 0;
	new q[120];
	format(q, sizeof q, "INSERT OR REPLACE INTO vie_meta (k, v) VALUES ('%s', %d)", key, value);
	db_free_result(db_query(g_VieDB, q));
	return 1;
}

stock Vie_OnInit()
{
	g_VieDB = db_open("crestwood_vie.db");
	if (g_VieDB == DB:0) { print("[VIE] ERREUR : impossible d'ouvrir crestwood_vie.db"); return 0; }
	db_free_result(db_query(g_VieDB, "CREATE TABLE IF NOT EXISTS vie (name TEXT PRIMARY KEY COLLATE NOCASE, hunger INTEGER DEFAULT 100, thirst INTEGER DEFAULT 100, xp INTEGER DEFAULT 0, lvl INTEGER DEFAULT 1, minutes INTEGER DEFAULT 0, descr TEXT DEFAULT '')"));
	db_free_result(db_query(g_VieDB, "CREATE TABLE IF NOT EXISTS loto_tickets (id INTEGER PRIMARY KEY AUTOINCREMENT, draw INTEGER, name TEXT COLLATE NOCASE, num INTEGER)"));
	db_free_result(db_query(g_VieDB, "CREATE TABLE IF NOT EXISTS vie_meta (k TEXT PRIMARY KEY, v INTEGER)"));
	g_VieLotoJackpot = Vie_MetaGet("loto_jackpot", VIE_LOTO_BASE);
	if (g_VieLotoJackpot < VIE_LOTO_BASE) g_VieLotoJackpot = VIE_LOTO_BASE;
	g_VieLotoDraw = Vie_MetaGet("loto_draw", 1);
	new h, mi, s;
	gettime(h, mi, s);
	g_VieLotoHour = h;                      // pas de tirage au demarrage du serveur
	SetTimer("Vie_Tick", 60000, true);
	print("[VIE] Faim/soif, niveaux, apparence et loto prets.");
	return 1;
}

stock Vie_OnExit()
{
	for (new p = 0, m = GetMaxPlayers(); p < m; p++) if (IsPlayerConnected(p)) Vie_Save(p);
	if (g_VieDB != DB:0) { db_close(g_VieDB); g_VieDB = DB:0; }
	return 1;
}

stock Vie_OnConnect(playerid)
{
	g_VieLoaded[playerid] = 0;
	g_VieHunger[playerid] = 100; g_VieThirst[playerid] = 100;
	g_VieXp[playerid] = 0; g_VieLvl[playerid] = 1; g_VieMin[playerid] = 0;
	g_VieTickN[playerid] = 0; g_VieWarned[playerid] = 0;
	g_VieDesc[playerid][0] = 0;
	return 1;
}

stock Vie_OnDisconnect(playerid)
{
	Vie_Save(playerid);
	g_VieLoaded[playerid] = 0;
	return 1;
}

stock Vie_Load(playerid)
{
	if (g_VieLoaded[playerid] || g_VieDB == DB:0 || !g_AuthLogged[playerid]) return 0;
	new name[MAX_PLAYER_NAME + 1], q[200], f[24];
	GetPlayerName(playerid, name, sizeof name);
	format(q, sizeof q, "SELECT hunger, thirst, xp, lvl, minutes, descr FROM vie WHERE name = '%s'", name);
	new DBResult:r = db_query(g_VieDB, q);
	new found = 0;
	if (r != DBResult:0)
	{
		if (db_num_rows(r) > 0)
		{
			found = 1;
			db_get_field(r, 0, f, sizeof f); g_VieHunger[playerid] = strval(f);
			db_get_field(r, 1, f, sizeof f); g_VieThirst[playerid] = strval(f);
			db_get_field(r, 2, f, sizeof f); g_VieXp[playerid] = strval(f);
			db_get_field(r, 3, f, sizeof f); g_VieLvl[playerid] = strval(f);
			db_get_field(r, 4, f, sizeof f); g_VieMin[playerid] = strval(f);
			db_get_field(r, 5, g_VieDesc[playerid], VIE_DESC_LEN + 1);
		}
		db_free_result(r);
	}
	if (!found)
	{
		format(q, sizeof q, "INSERT OR IGNORE INTO vie (name) VALUES ('%s')", name);
		db_free_result(db_query(g_VieDB, q));
	}
	if (g_VieHunger[playerid] < 0 || g_VieHunger[playerid] > 100) g_VieHunger[playerid] = 100;
	if (g_VieThirst[playerid] < 0 || g_VieThirst[playerid] > 100) g_VieThirst[playerid] = 100;
	if (g_VieLvl[playerid] < 1) g_VieLvl[playerid] = 1;
	g_VieLoaded[playerid] = 1;
	SetPlayerScore(playerid, g_VieLvl[playerid]);
	return 1;
}

stock Vie_Save(playerid)
{
	if (!g_VieLoaded[playerid] || g_VieDB == DB:0) return 0;
	new name[MAX_PLAYER_NAME + 1], q[420], d[(VIE_DESC_LEN + 1) * 2];
	GetPlayerName(playerid, name, sizeof name);
	Vie_Sql(g_VieDesc[playerid], d, sizeof d);
	format(q, sizeof q, "UPDATE vie SET hunger = %d, thirst = %d, xp = %d, lvl = %d, minutes = %d, descr = '%s' WHERE name = '%s'",
		g_VieHunger[playerid], g_VieThirst[playerid], g_VieXp[playerid], g_VieLvl[playerid], g_VieMin[playerid], d, name);
	db_free_result(db_query(g_VieDB, q));
	return 1;
}

stock Vie_OnSpawn(playerid)
{
	if (!g_VieLoaded[playerid]) Vie_Load(playerid);
	else SetPlayerScore(playerid, g_VieLvl[playerid]);
	return 1;
}

// lus par Cw_HudPush (module Cw15) pour les jauges de la minimap
stock Vie_Hunger(playerid) { return g_VieLoaded[playerid] ? g_VieHunger[playerid] : 100; }
stock Vie_Thirst(playerid) { return g_VieLoaded[playerid] ? g_VieThirst[playerid] : 100; }

// ------------------------------------------------------------
//  Faim / soif
// ------------------------------------------------------------
stock Vie_Active(p)
{
	if (!IsPlayerConnected(p) || !g_AuthLogged[p] || g_IdPending[p] || !g_VieLoaded[p]) return 0;
	if (GetPlayerState(p) == PLAYER_STATE_ONFOOT || GetPlayerState(p) == PLAYER_STATE_DRIVER || GetPlayerState(p) == PLAYER_STATE_PASSENGER) return 1;
	return 0;
}

stock Vie_Eat(playerid, amount)
{
	if (!g_VieLoaded[playerid]) Vie_Load(playerid);
	g_VieHunger[playerid] += amount;
	if (g_VieHunger[playerid] > 100) g_VieHunger[playerid] = 100;
	if (g_VieHunger[playerid] > VIE_WARN_AT) g_VieWarned[playerid] &= ~1;
	Cw_HudPush(playerid, 1);
	return 1;
}

stock Vie_Drink(playerid, amount)
{
	if (!g_VieLoaded[playerid]) Vie_Load(playerid);
	g_VieThirst[playerid] += amount;
	if (g_VieThirst[playerid] > 100) g_VieThirst[playerid] = 100;
	if (g_VieThirst[playerid] > VIE_WARN_AT) g_VieWarned[playerid] &= ~2;
	Cw_HudPush(playerid, 1);
	return 1;
}

stock Vie_NeedsTick(p)
{
	g_VieTickN[p]++;
	if (g_VieTickN[p] % VIE_THIRST_EVERY == 0 && g_VieThirst[p] > 0) g_VieThirst[p]--;
	if (g_VieTickN[p] % VIE_HUNGER_EVERY == 0 && g_VieHunger[p] > 0) g_VieHunger[p]--;
	if (g_VieHunger[p] <= VIE_WARN_AT && g_VieHunger[p] > 0 && !(g_VieWarned[p] & 1))
	{
		g_VieWarned[p] |= 1;
		Cw_Notify(p, "warning", "Faim", "Ton ventre gargouille. Mange quelque chose : ~y~hamburger~w~ dans les superettes 24/7 (/boutique).");
	}
	if (g_VieThirst[p] <= VIE_WARN_AT && g_VieThirst[p] > 0 && !(g_VieWarned[p] & 2))
	{
		g_VieWarned[p] |= 2;
		Cw_Notify(p, "warning", "Soif", "Tu as la gorge seche. Bois de l'~b~eau Kirene~w~ (superettes 24/7, /boutique).");
	}
	if (g_VieHunger[p] == 0 || g_VieThirst[p] == 0)
	{
		new Float:hp;
		GetPlayerHealth(p, hp);
		if (hp > VIE_MIN_HP)
		{
			hp -= VIE_STARVE_HP;
			if (hp < VIE_MIN_HP) hp = VIE_MIN_HP;
			SetPlayerHealth(p, hp);
		}
		if (g_VieTickN[p] % 5 == 0)
		{
			if (g_VieHunger[p] == 0 && g_VieThirst[p] == 0) Cw_Notify(p, "denied", "Epuisement", "Tu meurs de ~r~faim~w~ et de ~r~soif~w~ : ta sante baisse. Mange et bois vite !");
			else if (g_VieHunger[p] == 0) Cw_Notify(p, "denied", "Faim", "Tu es affame : ta ~r~sante baisse~w~. Mange un hamburger.");
			else Cw_Notify(p, "denied", "Soif", "Tu es deshydrate : ta ~r~sante baisse~w~. Bois de l'eau.");
		}
	}
	return 1;
}

// ------------------------------------------------------------
//  Experience & niveau
// ------------------------------------------------------------
stock Vie_XpTick(p)
{
	g_VieMin[p]++;
	if (g_VieMin[p] % VIE_XP_MINUTES != 0) return 0;
	g_VieXp[p]++;
	new need = g_VieLvl[p] * VIE_XP_PER_LEVEL, m[160], a[16];
	if (g_VieXp[p] < need) return 1;
	g_VieXp[p] = 0;
	g_VieLvl[p]++;
	SetPlayerScore(p, g_VieLvl[p]);
	new bonus = g_VieLvl[p] * VIE_LEVEL_BONUS;
	if (g_BankLoaded[p])
	{
		g_BankBalance[p] += bonus;
		Bank_AddHistory(p, "Prime de niveau", bonus);
		Bank_SavePlayer(p);
	}
	else GivePlayerMoney(p, bonus);
	Cw_Fmt(bonus, a, sizeof a);
	format(m, sizeof m, "Tu passes au ~g~niveau %d~w~ ! Prime de ~y~%s $~w~ versee sur ton compte.", g_VieLvl[p], a);
	Cw_Notify(p, "success", "Niveau superieur", m, 8000);
	format(m, sizeof m, "[NIVEAU] Felicitations : niveau %d atteint (+%s $ en banque).", g_VieLvl[p], a);
	SendClientMessage(p, VIE_COL_OK, m);
	PlayerPlaySound(p, 1058, 0.0, 0.0, 0.0);
	Vie_Save(p);
	return 1;
}

// age d'apres la date de naissance de la carte d'identite (jj/mm/aaaa)
stock Vie_Age(playerid)
{
	new b[12];
	format(b, sizeof b, "%s", g_IdBirth[playerid]);
	if (strlen(b) < 10) return 0;
	new d = (b[0] - '0') * 10 + (b[1] - '0');
	new mo = (b[3] - '0') * 10 + (b[4] - '0');
	new y = (b[6] - '0') * 1000 + (b[7] - '0') * 100 + (b[8] - '0') * 10 + (b[9] - '0');
	new cy, cm, cd;
	getdate(cy, cm, cd);
	new age = cy - y;
	if (cm < mo || (cm == mo && cd < d)) age--;
	if (age < 0 || age > 120) return 0;
	return age;
}

// ------------------------------------------------------------
//  Loto
// ------------------------------------------------------------
stock Vie_LotoCount(const name[])
{
	if (g_VieDB == DB:0) return 0;
	new q[160], f[12], n = 0;
	format(q, sizeof q, "SELECT COUNT(*) FROM loto_tickets WHERE draw = %d AND name = '%s'", g_VieLotoDraw, name);
	new DBResult:r = db_query(g_VieDB, q);
	if (r == DBResult:0) return 0;
	if (db_num_rows(r) > 0) { db_get_field(r, 0, f, sizeof f); n = strval(f); }
	db_free_result(r);
	return n;
}

stock Vie_LotoBuy(playerid, num)
{
	if (g_VieDB == DB:0) return SendClientMessage(playerid, VIE_COL_ERR, "Le loto est indisponible pour le moment."), 1;
	if (Shop_Find(playerid) == -1) return Cw_Notify(playerid, "denied", "Loto", "Les tickets de loto s'achetent au comptoir d'une ~y~superette 24/7~w~ (/boutique pour le GPS)."), 1;
	if (num < 1 || num > VIE_LOTO_MAX_NUM) return SendClientMessage(playerid, VIE_COL_ERR, "Choisis un numero entre 1 et 50 : /loto <numero>"), 1;
	new name[MAX_PLAYER_NAME + 1];
	GetPlayerName(playerid, name, sizeof name);
	if (Vie_LotoCount(name) >= VIE_LOTO_MAX_TICKETS) return Cw_Notify(playerid, "warning", "Loto", "Tu as deja 5 tickets pour ce tirage. Attends le prochain."), 1;
	if (GetPlayerMoney(playerid) < VIE_LOTO_PRICE) return Cw_Notify(playerid, "denied", "Loto", "Un ticket coute ~y~1 000 $~w~ en liquide."), 1;
	GivePlayerMoney(playerid, -VIE_LOTO_PRICE);
	new q[200], m[180], a[16];
	format(q, sizeof q, "INSERT INTO loto_tickets (draw, name, num) VALUES (%d, '%s', %d)", g_VieLotoDraw, name, num);
	db_free_result(db_query(g_VieDB, q));
	g_VieLotoJackpot += VIE_LOTO_PRICE * VIE_LOTO_SHARE / 100;
	Vie_MetaSet("loto_jackpot", g_VieLotoJackpot);
	new h, mi, s;
	gettime(h, mi, s);
	Cw_Fmt(g_VieLotoJackpot, a, sizeof a);
	format(m, sizeof m, "Ticket n~y~%d~w~ valide pour le tirage de ~b~%02dh00~w~. Cagnotte : ~g~%s $~w~. Bonne chance !", num, (h + 1) % 24, a);
	Cw_Notify(playerid, "success", "Loto de Crestwood", m);
	ApplyAnimation(playerid, "DEALER", "shop_pay", 4.1, 0, 0, 0, 0, 0, 1);
	return 1;
}

stock Vie_LotoInfo(playerid)
{
	new name[MAX_PLAYER_NAME + 1], m[170], a[16], h, mi, s;
	GetPlayerName(playerid, name, sizeof name);
	gettime(h, mi, s);
	Cw_Fmt(g_VieLotoJackpot, a, sizeof a);
	SendClientMessage(playerid, VIE_COL_LOTO, "===== LOTO DE CRESTWOOD =====");
	format(m, sizeof m, "Cagnotte : %s $ | Prochain tirage : %02dh00 (dans %d min) | Tes tickets : %d/%d", a, (h + 1) % 24, 60 - mi, Vie_LotoCount(name), VIE_LOTO_MAX_TICKETS);
	SendClientMessage(playerid, VIE_COL_LOTO, m);
	SendClientMessage(playerid, VIE_COL_LOTO, "Acheter : /loto <1-50> au comptoir d'une superette 24/7 (1 000 $ le ticket).");
	if (g_VieDB != DB:0)
	{
		new q[160], f[8], line[90], n = 0;
		format(q, sizeof q, "SELECT num FROM loto_tickets WHERE draw = %d AND name = '%s' LIMIT %d", g_VieLotoDraw, name, VIE_LOTO_MAX_TICKETS);
		new DBResult:r = db_query(g_VieDB, q);
		if (r != DBResult:0)
		{
			format(line, sizeof line, "Tes numeros :");
			for (new i = 0, rows = db_num_rows(r); i < rows; i++)
			{
				db_get_field(r, 0, f, sizeof f);
				strcat(line, " ", sizeof line);
				strcat(line, f, sizeof line);
				n++;
				db_next_row(r);
			}
			db_free_result(r);
			if (n) SendClientMessage(playerid, VIE_COL_LOTO, line);
		}
	}
	return 1;
}

stock Vie_LotoDraw()
{
	if (g_VieDB == DB:0) return 0;
	new win = random(VIE_LOTO_MAX_NUM) + 1, q[200], names[16][MAX_PLAYER_NAME + 1], n = 0, total = 0, f[12];
	format(q, sizeof q, "SELECT COUNT(*) FROM loto_tickets WHERE draw = %d", g_VieLotoDraw);
	new DBResult:r = db_query(g_VieDB, q);
	if (r != DBResult:0) { if (db_num_rows(r) > 0) { db_get_field(r, 0, f, sizeof f); total = strval(f); } db_free_result(r); }
	format(q, sizeof q, "SELECT name FROM loto_tickets WHERE draw = %d AND num = %d LIMIT 16", g_VieLotoDraw, win);
	r = db_query(g_VieDB, q);
	if (r != DBResult:0)
	{
		for (new i = 0, rows = db_num_rows(r); i < rows && n < sizeof(names); i++)
		{
			db_get_field(r, 0, names[n], MAX_PLAYER_NAME + 1);
			n++;
			db_next_row(r);
		}
		db_free_result(r);
	}
	new m[200], a[16];
	if (n > 0)
	{
		new share = g_VieLotoJackpot / n;
		Cw_Fmt(share, a, sizeof a);
		for (new i = 0; i < n; i++)
		{
			Cw_CreditByName(names[i], share, "Gain au loto");
			new p = Cw_PlayerByName(names[i]);
			if (p != INVALID_PLAYER_ID)
			{
				format(m, sizeof m, "~g~JACKPOT !~w~ Ton numero %d est sorti : ~y~%s $~w~ verses sur ton compte en banque.", win, a);
				Cw_Notify(p, "success", "Loto de Crestwood", m, 10000);
				PlayerPlaySound(p, 1149, 0.0, 0.0, 0.0);
			}
		}
		format(m, sizeof m, "[LOTO] Numero gagnant : %d. %d gagnant(s) se partagent la cagnotte (%s $ chacun) !", win, n, a);
		SendClientMessageToAll(VIE_COL_LOTO, m);
		g_VieLotoJackpot = VIE_LOTO_BASE;
	}
	else
	{
		Cw_Fmt(g_VieLotoJackpot, a, sizeof a);
		format(m, sizeof m, "[LOTO] Numero gagnant : %d. Personne n'a gagne (%d tickets) : la cagnotte de %s $ est remise en jeu !", win, total, a);
		SendClientMessageToAll(VIE_COL_LOTO, m);
		if (total > 0)
		{
			format(m, sizeof m, "Numero gagnant : ~y~%d~w~. Pas de gagnant : la cagnotte de ~g~%s $~w~ reste en jeu. /loto", win, a);
			Cw_NotifyAll("system", "Loto de Crestwood", m);
		}
	}
	g_VieLotoDraw++;
	Vie_MetaSet("loto_draw", g_VieLotoDraw);
	Vie_MetaSet("loto_jackpot", g_VieLotoJackpot);
	format(q, sizeof q, "DELETE FROM loto_tickets WHERE draw < %d", g_VieLotoDraw);
	db_free_result(db_query(g_VieDB, q));
	return 1;
}

// ------------------------------------------------------------
//  Minuterie (1 min)
// ------------------------------------------------------------
public Vie_Tick()
{
	for (new p = 0, m = GetMaxPlayers(); p < m; p++)
	{
		if (!Vie_Active(p)) continue;
		Vie_NeedsTick(p);
		Vie_XpTick(p);
		if (g_VieMin[p] % 5 == 0) Vie_Save(p);
	}
	new h, mi, s;
	gettime(h, mi, s);
	if (mi == 0 && h != g_VieLotoHour)
	{
		g_VieLotoHour = h;
		Vie_LotoDraw();
	}
	return 1;
}

// ------------------------------------------------------------
//  Commandes
// ------------------------------------------------------------
stock Vie_OnCommand(playerid, const cmdtext[])
{
	if (!g_AuthLogged[playerid]) return 0;
	new a, m[170];
	if (!strcmp(cmdtext, "/besoins", true) || !strcmp(cmdtext, "/faim", true) || !strcmp(cmdtext, "/soif", true))
	{
		if (!g_VieLoaded[playerid]) Vie_Load(playerid);
		format(m, sizeof m, "Faim : ~y~%d/100~w~   Soif : ~b~%d/100~w~~n~Hamburger et eau Kirene dans les superettes 24/7.", g_VieHunger[playerid], g_VieThirst[playerid]);
		Cw_Notify(playerid, "system", "Besoins", m);
		return 1;
	}
	if (!strcmp(cmdtext, "/niveau", true) || !strcmp(cmdtext, "/level", true) || !strcmp(cmdtext, "/xp", true))
	{
		if (!g_VieLoaded[playerid]) Vie_Load(playerid);
		new need = g_VieLvl[playerid] * VIE_XP_PER_LEVEL, left = VIE_XP_MINUTES - (g_VieMin[playerid] % VIE_XP_MINUTES);
		format(m, sizeof m, "Niveau ~g~%d~w~ - XP ~y~%d/%d~w~~n~Prochain point d'XP dans %d min. Temps de jeu : %dh%02d.",
			g_VieLvl[playerid], g_VieXp[playerid], need, left, g_VieMin[playerid] / 60, g_VieMin[playerid] % 60);
		Cw_Notify(playerid, "system", "Experience", m, 7000);
		return 1;
	}
	if ((a = Cw_Arg(cmdtext, "/apparence")) != -1)
	{
		if (!g_VieLoaded[playerid]) Vie_Load(playerid);
		if (!cmdtext[a])
		{
			if (g_VieDesc[playerid][0]) { format(m, sizeof m, "Ton apparence : %s", g_VieDesc[playerid]); SendClientMessage(playerid, VIE_COL_INFO, m); }
			SendClientMessage(playerid, VIE_COL_INFO, "/apparence <texte> (ex : grand, cicatrice sur la joue, veste noire)  |  /apparence effacer");
			return 1;
		}
		if (!strcmp(cmdtext[a], "effacer", true))
		{
			g_VieDesc[playerid][0] = 0;
			Vie_Save(playerid);
			return Cw_Notify(playerid, "success", "Apparence", "Ton apparence a ete effacee."), 1;
		}
		new clean[VIE_DESC_LEN + 1];
		if (Vie_CleanText(cmdtext[a], clean, sizeof clean) < 5) return SendClientMessage(playerid, VIE_COL_ERR, "Decris un peu plus ton personnage (5 caracteres minimum)."), 1;
		format(g_VieDesc[playerid], VIE_DESC_LEN + 1, "%s", clean);
		Vie_Save(playerid);
		Cw_Notify(playerid, "success", "Apparence", "Apparence enregistree. Les joueurs proches la voient avec ~b~/examiner~w~.");
		return 1;
	}
	if ((a = Cw_Arg(cmdtext, "/examiner")) != -1)
	{
		if (!cmdtext[a]) return SendClientMessage(playerid, VIE_COL_INFO, "/examiner <id> : observer un joueur proche"), 1;
		new t = strval(cmdtext[a]);
		if (t == playerid || !IsPlayerConnected(t) || !g_AuthLogged[t]) return SendClientMessage(playerid, VIE_COL_ERR, "Joueur introuvable."), 1;
		new Float:x, Float:y, Float:z;
		GetPlayerPos(t, x, y, z);
		if (!IsPlayerInRangeOfPoint(playerid, VIE_EXAM_DIST, x, y, z) || GetPlayerVirtualWorld(t) != GetPlayerVirtualWorld(playerid))
			return SendClientMessage(playerid, VIE_COL_ERR, "Approche-toi de cette personne pour l'observer."), 1;
		if (!g_VieLoaded[t]) Vie_Load(t);
		new tn[MAX_PLAYER_NAME + 1], me[MAX_PLAYER_NAME + 1], age = Vie_Age(t);
		RpX_Name(t, tn, sizeof tn);
		RpX_Name(playerid, me, sizeof me);
		SendClientMessage(playerid, VIE_COL_INFO, "----------------------------------------------");
		if (age > 0) format(m, sizeof m, "%s : %s d'environ %d ans", tn, (g_IdGender[t] == 1) ? ("une femme") : ("un homme"), age);
		else format(m, sizeof m, "%s : %s", tn, (g_IdGender[t] == 1) ? ("une femme") : ("un homme"));
		if (g_IdOrigin[t][0]) { new org[40]; format(org, sizeof org, ", origine %s.", g_IdOrigin[t]); strcat(m, org, sizeof m); }
		else strcat(m, ".", sizeof m);
		SendClientMessage(playerid, VIE_COL_INFO, m);
		if (g_VieDesc[t][0]) format(m, sizeof m, "Apparence : %s", g_VieDesc[t]);
		else format(m, sizeof m, "Apparence : rien de particulier.");
		SendClientMessage(playerid, 0xDDDDDDFF, m);
		{
			new sp[MAX_PLAYER_NAME + 1];
			if (Ville_Spouse(t, sp, sizeof sp)) { format(m, sizeof m, "Porte une alliance : marie(e) a %s.", sp); SendClientMessage(playerid, 0xF48FB1FF, m); }
		}
		SendClientMessage(playerid, VIE_COL_INFO, "----------------------------------------------");
		format(m, sizeof m, "* %s observe %s de la tete aux pieds.", me, tn);
		RpX_Near(playerid, 15.0, RPX_COL_ME, m);
		return 1;
	}
	if ((a = Cw_Arg(cmdtext, "/loto")) != -1)
	{
		if (!cmdtext[a]) return Vie_LotoInfo(playerid), 1;
		return Vie_LotoBuy(playerid, strval(cmdtext[a])), 1;
	}
	// staff : remettre faim et soif a 100 (niveau 2)
	if ((a = Cw_Arg(cmdtext, "/nourrir")) != -1)
	{
		if (Admin_Level(playerid) < 2) return 0;
		new t = cmdtext[a] ? strval(cmdtext[a]) : playerid;
		if (!IsPlayerConnected(t)) return SendClientMessage(playerid, VIE_COL_ERR, "Joueur introuvable."), 1;
		Vie_Eat(t, 100);
		Vie_Drink(t, 100);
		if (t != playerid) Cw_Notify(t, "success", "Staff", "Un membre du staff t'a rassasie (faim et soif a 100).");
		return SendClientMessage(playerid, VIE_COL_OK, "Faim et soif remises a 100."), 1;
	}
	return 0;
}
