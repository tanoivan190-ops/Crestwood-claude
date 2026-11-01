/* ============================================================
   CODES PROMO (tablette RP > Codes promo) - Crestwood RP
   ------------------------------------------------------------
   Les admins (niveau PROMO_ADMIN_LVL+) creent les codes dans la tablette :
       code + createur (nom du joueur qui touche la commission) + forfait.
   Un joueur ne peut utiliser qu'UN seul code promo (une fois).
   Forfaits :
       1 Economique : Tampa 3 jours   + 300 000 FCFA   -> createur 20 000 par utilisation
       2 Medium     : Sabre 12 jours  + 5 000 000 FCFA -> createur 35 000 par utilisation
       3 Premium    : NRG-500 1 mois  + 300 000 FCFA   -> createur 50 000 par utilisation
   Le vehicule est range dans le garage du joueur et disparait a la fin de la location.
   Argent : sur le compte en banque (en liquide si la banque n'est pas chargee).
   Commission : versee tout de suite si le createur est connecte, sinon a sa prochaine connexion.
   Journal : createur / utilisateur / forfait (visible par les admins, et par le createur pour ses codes).
   Base : crestwood_promo.db
   Page -> serveur : /rpact view promo | promouse <CODE> | promoadd <CODE> <createur> <1-3> | promodel <CODE>
   ============================================================ */
#define PROMO_ADMIN_LVL       4
#define PROMO_PACKS           3
#define PROMO_CHECK_MS        300000      // verification des locations expirees (5 min)

new const g_PromoName[PROMO_PACKS + 1][] = {"", "Economique", "Medium", "Premium"};
new const g_PromoModel[PROMO_PACKS + 1] = {0, 549, 475, 522};            // Tampa, Sabre, NRG-500
new const g_PromoDays[PROMO_PACKS + 1] = {0, 3, 12, 30};
new const g_PromoMoney[PROMO_PACKS + 1] = {0, 300000, 5000000, 300000};
new const g_PromoCommission[PROMO_PACKS + 1] = {0, 20000, 35000, 50000};

new DB:g_PromoDB;
new g_PromoPaid[MAX_PLAYERS];
forward Promo_Check();

stock Promo_OnInit()
{
	g_PromoDB = db_open("crestwood_promo.db");
	if (g_PromoDB == DB:0) { print("[PROMO] ERREUR : impossible d'ouvrir crestwood_promo.db"); return 0; }
	db_free_result(db_query(g_PromoDB, "CREATE TABLE IF NOT EXISTS promo_codes (code TEXT PRIMARY KEY COLLATE NOCASE, pack INTEGER, creator TEXT COLLATE NOCASE, admin TEXT, ts INTEGER, uses INTEGER DEFAULT 0, earned INTEGER DEFAULT 0)"));
	db_free_result(db_query(g_PromoDB, "CREATE TABLE IF NOT EXISTS promo_uses (id INTEGER PRIMARY KEY AUTOINCREMENT, code TEXT COLLATE NOCASE, user TEXT COLLATE NOCASE UNIQUE, creator TEXT COLLATE NOCASE, pack INTEGER, ts INTEGER)"));
	db_free_result(db_query(g_PromoDB, "CREATE TABLE IF NOT EXISTS promo_pending (creator TEXT PRIMARY KEY COLLATE NOCASE, amount INTEGER)"));
	db_free_result(db_query(g_PromoDB, "CREATE TABLE IF NOT EXISTS promo_rentals (vid INTEGER PRIMARY KEY, user TEXT COLLATE NOCASE, model INTEGER, expires INTEGER)"));
	SetTimer("Promo_Check", PROMO_CHECK_MS, true);
	Promo_Check();
	print("[PROMO] Codes promo prets.");
	return 1;
}

stock Promo_OnConnect(playerid)
{
	g_PromoPaid[playerid] = 0;
	return 1;
}

// nom propre pour SQL / JSON : lettres, chiffres, _ seulement
stock Promo_Clean(const in[], out[], size)
{
	new j = 0;
	for (new i = 0; in[i] != 0 && j < size - 1; i++)
	{
		new c = in[i];
		if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '_') out[j++] = c;
	}
	out[j] = 0;
	return j;
}

stock Promo_Upper(s[])
{
	for (new i = 0; s[i] != 0; i++) if (s[i] >= 'a' && s[i] <= 'z') s[i] -= 32;
	return 1;
}

// argent sur le compte en banque si possible, sinon en liquide
stock Promo_Give(playerid, amount, const why[])
{
	if (amount <= 0) return 0;
	if (g_BankLoaded[playerid])
	{
		g_BankBalance[playerid] += amount;
		Bank_AddHistory(playerid, why, amount);
		Bank_SavePlayer(playerid);
	}
	else GivePlayerMoney(playerid, amount);
	return 1;
}

// commissions en attente (createur deconnecte au moment de l'utilisation)
stock Promo_PayPending(playerid)
{
	if (g_PromoDB == DB:0 || g_PromoPaid[playerid] || !g_AuthLogged[playerid]) return 0;
	g_PromoPaid[playerid] = 1;
	new name[MAX_PLAYER_NAME + 1], q[160], f[16];
	GetPlayerName(playerid, name, sizeof name);
	format(q, sizeof q, "SELECT amount FROM promo_pending WHERE creator = '%s'", name);
	new DBResult:r = db_query(g_PromoDB, q), amount = 0;
	if (r != DBResult:0)
	{
		if (db_num_rows(r) > 0)
		{
			db_get_field(r, 0, f, sizeof f);
			amount = strval(f);
		}
		db_free_result(r);
	}
	if (amount <= 0) return 0;
	format(q, sizeof q, "DELETE FROM promo_pending WHERE creator = '%s'", name);
	db_free_result(db_query(g_PromoDB, q));
	Promo_Give(playerid, amount, "Commissions codes promo");
	new m[128];
	format(m, sizeof m, "Codes promo : %d FCFA de commissions t'ont ete verses pendant ton absence.", amount);
	SendClientMessage(playerid, 0x33CC33FF, m);
	return 1;
}

stock Promo_OnSpawn(playerid)
{
	return Promo_PayPending(playerid);
}

// locations expirees : vehicule retire du jeu et du garage
public Promo_Check()
{
	if (g_PromoDB == DB:0) return 0;
	new q[128], f[16], ids[64], n = 0;
	format(q, sizeof q, "SELECT vid FROM promo_rentals WHERE expires <= %d LIMIT 64", gettime());
	new DBResult:r = db_query(g_PromoDB, q);
	if (r == DBResult:0) return 0;
	for (new i = 0, rows = db_num_rows(r); i < rows && n < sizeof(ids); i++)
	{
		db_get_field(r, 0, f, sizeof f);
		ids[n++] = strval(f);
		db_next_row(r);
	}
	db_free_result(r);
	for (new k = 0; k < n; k++)
	{
		new v = Gar_FindOut(ids[k]);
		if (v)
		{
			new owner = g_GarVehOwner[v];
			for (new p = 0; p < MAX_PLAYERS; p++) if (IsPlayerConnected(p) && GetPlayerVehicleID(p) == v) RemovePlayerFromVehicle(p);
			Gar_StoreVehicle(v);
			if (owner != INVALID_PLAYER_ID && IsPlayerConnected(owner)) SendClientMessage(owner, 0xFFB454FF, "Code promo : la location de ton vehicule est terminee, il a ete rendu.");
		}
		format(q, sizeof q, "DELETE FROM vehicles WHERE id = %d", ids[k]);
		if (g_GarDB != DB:0) db_free_result(db_query(g_GarDB, q));
		format(q, sizeof q, "DELETE FROM promo_rentals WHERE vid = %d", ids[k]);
		db_free_result(db_query(g_PromoDB, q));
	}
	return 1;
}

// le joueur utilise un code
stock Promo_Use(playerid, const raw[])
{
	if (!g_AuthLogged[playerid]) return 0;
	if (g_PromoDB == DB:0 || g_GarDB == DB:0) return Rp_Reply(playerid, 0, "Codes promo indisponibles pour le moment.");
	new code[24], name[MAX_PLAYER_NAME + 1], q[220], f[32];
	Promo_Clean(raw, code, sizeof code);
	Promo_Upper(code);
	if (strlen(code) < 3) return Rp_Reply(playerid, 0, "Entre un code promo valide.");
	GetPlayerName(playerid, name, sizeof name);
	format(q, sizeof q, "SELECT code FROM promo_uses WHERE user = '%s'", name);
	new DBResult:r = db_query(g_PromoDB, q), used = 0;
	if (r != DBResult:0) { used = db_num_rows(r) > 0; db_free_result(r); }
	if (used) return Rp_Reply(playerid, 0, "Tu as deja utilise un code promo.");
	format(q, sizeof q, "SELECT pack, creator FROM promo_codes WHERE code = '%s'", code);
	r = db_query(g_PromoDB, q);
	new pack = 0, creator[MAX_PLAYER_NAME + 1];
	if (r != DBResult:0)
	{
		if (db_num_rows(r) > 0) { db_get_field(r, 0, f, sizeof f); pack = strval(f); db_get_field(r, 1, creator, sizeof creator); }
		db_free_result(r);
	}
	if (pack < 1 || pack > PROMO_PACKS) return Rp_Reply(playerid, 0, "Ce code promo n'existe pas.");
	if (!strcmp(creator, name, true)) return Rp_Reply(playerid, 0, "Tu ne peux pas utiliser ton propre code.");
	// 1) utilisation (UNIQUE sur user : protege contre le double clic)
	format(q, sizeof q, "INSERT INTO promo_uses (code, user, creator, pack, ts) VALUES ('%s', '%s', '%s', %d, %d)", code, name, creator, pack, gettime());
	db_free_result(db_query(g_PromoDB, q));
	format(q, sizeof q, "SELECT 1 FROM promo_uses WHERE user = '%s' AND code = '%s'", name, code);
	r = db_query(g_PromoDB, q); used = 0;
	if (r != DBResult:0) { used = db_num_rows(r) > 0; db_free_result(r); }
	if (!used) return Rp_Reply(playerid, 0, "Code promo deja utilise.");
	// 2) vehicule en location dans le garage
	format(q, sizeof q, "INSERT INTO vehicles (owner, model, c1, c2, price) VALUES ('%s', %d, 1, 1, 0)", name, g_PromoModel[pack]);
	db_free_result(db_query(g_GarDB, q));
	r = db_query(g_GarDB, "SELECT last_insert_rowid()");
	new vid = 0;
	if (r != DBResult:0)
	{
		if (db_num_rows(r) > 0)
		{
			db_get_field(r, 0, f, sizeof f);
			vid = strval(f);
		}
		db_free_result(r);
	}
	if (vid > 0)
	{
		format(q, sizeof q, "INSERT OR REPLACE INTO promo_rentals (vid, user, model, expires) VALUES (%d, '%s', %d, %d)", vid, name, g_PromoModel[pack], gettime() + g_PromoDays[pack] * 86400);
		db_free_result(db_query(g_PromoDB, q));
	}
	// 3) argent
	new why[48];
	format(why, sizeof why, "Code promo %s", g_PromoName[pack]);
	Promo_Give(playerid, g_PromoMoney[pack], why);
	// 4) commission du createur
	new com = g_PromoCommission[pack];
	format(q, sizeof q, "UPDATE promo_codes SET uses = uses + 1, earned = earned + %d WHERE code = '%s'", com, code);
	db_free_result(db_query(g_PromoDB, q));
	new c = Rp_FindByName(creator), m[144];
	if (c != INVALID_PLAYER_ID && g_AuthLogged[c])
	{
		Promo_Give(c, com, "Commission code promo");
		format(m, sizeof m, "Code promo %s : %s l'a utilise (%s). +%d FCFA.", code, name, g_PromoName[pack], com);
		SendClientMessage(c, 0x33CC33FF, m);
	}
	else
	{
		// (vieux SQLite de SA-MP : pas de "ON CONFLICT DO UPDATE")
		format(q, sizeof q, "INSERT OR IGNORE INTO promo_pending (creator, amount) VALUES ('%s', 0)", creator);
		db_free_result(db_query(g_PromoDB, q));
		format(q, sizeof q, "UPDATE promo_pending SET amount = amount + %d WHERE creator = '%s'", com, creator);
		db_free_result(db_query(g_PromoDB, q));
	}
	format(m, sizeof m, "Forfait %s active : %s %d jours (dans ton garage) + %d FCFA en banque.", g_PromoName[pack],
		pack == 1 ? ("Tampa") : (pack == 2 ? ("Sabre") : ("NRG-500")), g_PromoDays[pack], g_PromoMoney[pack]);
	SendClientMessage(playerid, 0x33CC33FF, m);
	return Rp_Reply(playerid, 1, "Code promo active !");
}

stock Promo_Add(playerid, const rawCode[], const params[])
{
	if (Admin_Level(playerid) < PROMO_ADMIN_LVL) return Rp_Reply(playerid, 0, "Reserve aux administrateurs.");
	if (g_PromoDB == DB:0) return Rp_Reply(playerid, 0, "Base des codes promo indisponible.");
	new code[24], idx = 0, tok[32], creator[MAX_PLAYER_NAME + 1], admin[MAX_PLAYER_NAME + 1], q[240];
	Promo_Clean(rawCode, code, sizeof code);
	Promo_Upper(code);
	if (strlen(code) < 3 || strlen(code) > 15) return Rp_Reply(playerid, 0, "Code : 3 a 15 lettres / chiffres.");
	Bank_NextToken(params, idx, tok, sizeof tok);
	Promo_Clean(tok, creator, sizeof creator);
	Bank_NextToken(params, idx, tok, sizeof tok);
	new pack = strval(tok);
	if (strlen(creator) < 3) return Rp_Reply(playerid, 0, "Indique le nom du createur (Prenom_Nom).");
	if (pack < 1 || pack > PROMO_PACKS) return Rp_Reply(playerid, 0, "Forfait invalide.");
	format(q, sizeof q, "SELECT 1 FROM promo_codes WHERE code = '%s'", code);
	new DBResult:r = db_query(g_PromoDB, q), exists = 0;
	if (r != DBResult:0) { exists = db_num_rows(r) > 0; db_free_result(r); }
	if (exists) return Rp_Reply(playerid, 0, "Ce code existe deja.");
	GetPlayerName(playerid, admin, sizeof admin);
	format(q, sizeof q, "INSERT INTO promo_codes (code, pack, creator, admin, ts) VALUES ('%s', %d, '%s', '%s', %d)", code, pack, creator, admin, gettime());
	db_free_result(db_query(g_PromoDB, q));
	new m[96];
	format(m, sizeof m, "Code %s cree (%s) pour %s.", code, g_PromoName[pack], creator);
	return Rp_Reply(playerid, 1, m);
}

stock Promo_Del(playerid, const rawCode[])
{
	if (Admin_Level(playerid) < PROMO_ADMIN_LVL) return Rp_Reply(playerid, 0, "Reserve aux administrateurs.");
	new code[24], q[120];
	Promo_Clean(rawCode, code, sizeof code);
	format(q, sizeof q, "DELETE FROM promo_codes WHERE code = '%s'", code);
	db_free_result(db_query(g_PromoDB, q));
	return Rp_Reply(playerid, 1, "Code supprime (les utilisations deja faites restent).");
}

// JSON de la vue "promo" (ajoute a Rp_Send)
stock Promo_Build(playerid, out[], size)
{
	new name[MAX_PLAYER_NAME + 1], q[220], f[32], part[160], adm = (Admin_Level(playerid) >= PROMO_ADMIN_LVL) ? 1 : 0;
	GetPlayerName(playerid, name, sizeof name);
	new usedCode[24], usedPack = 0;
	usedCode[0] = 0;
	format(out, size, ",\"pr\":{\"adm\":%d", adm);
	if (g_PromoDB == DB:0) { strcat(out, ",\"off\":1}", size); return 1; }
	format(q, sizeof q, "SELECT code, pack FROM promo_uses WHERE user = '%s'", name);
	new DBResult:r = db_query(g_PromoDB, q);
	if (r != DBResult:0)
	{
		if (db_num_rows(r) > 0)
		{
			db_get_field(r, 0, usedCode, sizeof usedCode);
			db_get_field(r, 1, f, sizeof f);
			usedPack = strval(f);
		}
		db_free_result(r);
	}
	format(part, sizeof part, ",\"used\":\"%s\",\"usedPack\":%d", usedCode, usedPack);
	strcat(out, part, size);
	// codes : tous (admin) ou les miens (createur)
	if (adm) format(q, sizeof q, "SELECT code, pack, creator, uses, earned FROM promo_codes ORDER BY ts DESC LIMIT 20");
	else format(q, sizeof q, "SELECT code, pack, creator, uses, earned FROM promo_codes WHERE creator = '%s' ORDER BY ts DESC LIMIT 20", name);
	strcat(out, ",\"codes\":[", size);
	r = db_query(g_PromoDB, q);
	if (r != DBResult:0)
	{
		new c[24], cr[MAX_PLAYER_NAME + 1], pk[8], us[12], ea[16];
		for (new i = 0, rows = db_num_rows(r); i < rows; i++)
		{
			db_get_field(r, 0, c, sizeof c); db_get_field(r, 1, pk, sizeof pk); db_get_field(r, 2, cr, sizeof cr);
			db_get_field(r, 3, us, sizeof us); db_get_field(r, 4, ea, sizeof ea);
			format(part, sizeof part, "%s[\"%s\",%d,\"%s\",%d,%d]", i ? (",") : (""), c, strval(pk), cr, strval(us), strval(ea));
			if (strlen(out) + strlen(part) > size - 400) { db_free_result(r); r = DBResult:0; break; }
			strcat(out, part, size);
			db_next_row(r);
		}
		if (r != DBResult:0) db_free_result(r);
	}
	strcat(out, "]", size);
	// journal : createur / utilisateur / forfait
	if (adm) format(q, sizeof q, "SELECT ts, code, creator, user, pack FROM promo_uses ORDER BY id DESC LIMIT 12");
	else format(q, sizeof q, "SELECT ts, code, creator, user, pack FROM promo_uses WHERE creator = '%s' ORDER BY id DESC LIMIT 12", name);
	strcat(out, ",\"log\":[", size);
	r = db_query(g_PromoDB, q);
	if (r != DBResult:0)
	{
		new ts[16], c[24], cr[MAX_PLAYER_NAME + 1], us[MAX_PLAYER_NAME + 1], pk[8];
		for (new i = 0, rows = db_num_rows(r); i < rows; i++)
		{
			db_get_field(r, 0, ts, sizeof ts); db_get_field(r, 1, c, sizeof c); db_get_field(r, 2, cr, sizeof cr);
			db_get_field(r, 3, us, sizeof us); db_get_field(r, 4, pk, sizeof pk);
			format(part, sizeof part, "%s[%d,\"%s\",\"%s\",\"%s\",%d]", i ? (",") : (""), strval(ts), c, cr, us, strval(pk));
			if (strlen(out) + strlen(part) > size - 40) break;
			strcat(out, part, size);
			db_next_row(r);
		}
		db_free_result(r);
	}
	strcat(out, "]}", size);
	return 1;
}
