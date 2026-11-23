// [v22] Boutique Crestwood : packs d'argent achetes sur le site (Wave), valides par le staff.
//  Le joueur paie sur le site ; la commande arrive dans Discord avec la commande a copier :
//      /livrer Prenom_Nom <pack> <reference>
//  Un meme code de reference ne peut etre livre qu'une seule fois. Joueur hors ligne : livre a sa connexion.
//  ATTENTION : la liste des packs (prix / argent / vehicules) doit rester identique dans le Worker (worker.js, PACKS).

#define SHOP_PACKS 13

enum E_SHOPPACK { spFcfa, spMoney, spVeh1, spVeh2 }
new const g_ShopPack[SHOP_PACKS][E_SHOPPACK] =
{
	{   500,   25000,   0,   0},
	{  1000,   55000,   0,   0},
	{  2000,  120000,   0,   0},
	{  2500,  155000,   0,   0},
	{  3200,  210000, 522,   0},   // NRG-500 (Yamaha R1)
	{  4000,  270000,   0,   0},
	{  4500,  310000,   0,   0},
	{  6000,  430000, 560,   0},   // Sultan (Subaru WRX STI)
	{  9000,  700000, 562,   0},   // Elegy (Nissan GT-R)
	{ 14500, 1150000, 429,   0},   // Banshee (Dodge Viper)
	{ 15000, 1200000, 541,   0},   // Bullet (Ford GT)
	{ 30000, 2600000, 411,   0},   // Infernus (Lamborghini Aventador)
	{ 45000, 4000000, 411, 579}    // Infernus + Huntley (Aventador + Urus)
};

stock Shop_Esc(const src[], dst[], dsize)
{
	new j = 0;
	for (new i = 0; src[i] && j < dsize - 1; i++) if ((src[i] >= 'a' && src[i] <= 'z') || (src[i] >= 'A' && src[i] <= 'Z') || (src[i] >= '0' && src[i] <= '9') || src[i] == '_' || src[i] == '-') dst[j++] = src[i];
	dst[j] = 0;
	return j;
}

stock Shop_OnInit()
{
	if (g_PromoDB == DB:0) return 0;
	db_free_result(db_query(g_PromoDB, "CREATE TABLE IF NOT EXISTS shop_orders (ref TEXT PRIMARY KEY, name TEXT, pack INTEGER, money INTEGER DEFAULT 0, paid INTEGER DEFAULT 0, ts INTEGER)"));
	return 1;
}

// verse l'argent en attente (money > 0 et paid = 0) d'un joueur en ligne
forward Shop_Deliver(playerid);
public Shop_Deliver(playerid)
{
	if (!IsPlayerConnected(playerid) || g_PromoDB == DB:0) return 0;
	new nm[MAX_PLAYER_NAME], en[MAX_PLAYER_NAME * 2], q[200], f[24], total = 0;
	GetPlayerName(playerid, nm, sizeof nm);
	Shop_Esc(nm, en, sizeof en);
	format(q, sizeof q, "SELECT SUM(money) FROM shop_orders WHERE name = '%s' COLLATE NOCASE AND paid = 0", en);
	new DBResult:r = db_query(g_PromoDB, q);
	if (r != DBResult:0) { if (db_num_rows(r) > 0) { db_get_field(r, 0, f, sizeof f); total = strval(f); } db_free_result(r); }
	if (total <= 0) return 1;
	format(q, sizeof q, "UPDATE shop_orders SET paid = 1 WHERE name = '%s' COLLATE NOCASE AND paid = 0", en);
	db_free_result(db_query(g_PromoDB, q));
	Promo_Give(playerid, total, "Boutique Crestwood");
	format(q, sizeof q, "Boutique Crestwood : %d $ ont ete verses sur ton compte. Merci !", total);
	SendClientMessage(playerid, 0x66FF66FF, q);
	return 1;
}

stock Shop_OnConnect(playerid)
{
	SetTimerEx("Shop_Deliver", 9000, false, "i", playerid);
	return 1;
}

// /livrer Prenom_Nom <pack 1..13> <reference>
stock Shop_OnCommand(playerid, const cmdtext[])
{
	if (strcmp(cmdtext, "/livrer ", true, 8)) return 0;
	if (Admin_Level(playerid) < 4) return 0;
	new nm[40], pk[8], rf[24], idx = 8;
	Bank_NextToken(cmdtext, idx, nm, sizeof nm);
	Bank_NextToken(cmdtext, idx, pk, sizeof pk);
	Bank_NextToken(cmdtext, idx, rf, sizeof rf);
	new pack = strval(pk), q[260], m[144], en[48], er[32];
	if (!nm[0] || pack < 1 || pack > SHOP_PACKS || !rf[0]) { SendClientMessage(playerid, 0xFF4444FF, "Usage : /livrer Prenom_Nom <pack 1-13> <reference>"); return 1; }
	Shop_Esc(nm, en, sizeof en);
	Shop_Esc(rf, er, sizeof er);
	if (!er[0]) { SendClientMessage(playerid, 0xFF4444FF, "Reference invalide."); return 1; }
	// deja livre ?
	format(q, sizeof q, "SELECT 1 FROM shop_orders WHERE ref = '%s'", er);
	new DBResult:r = db_query(g_PromoDB, q), dup = 0;
	if (r != DBResult:0) { dup = (db_num_rows(r) > 0); db_free_result(r); }
	if (dup) { SendClientMessage(playerid, 0xFF4444FF, "Cette commande a DEJA ete livree."); return 1; }
	new p = pack - 1;
	format(q, sizeof q, "INSERT INTO shop_orders (ref, name, pack, money, paid, ts) VALUES ('%s', '%s', %d, %d, 0, %d)", er, en, pack, g_ShopPack[p][spMoney], gettime());
	db_free_result(db_query(g_PromoDB, q));
	// vehicules definitifs dans le garage (marche meme hors ligne)
	for (new v = 0; v < 2; v++)
	{
		new model = (v == 0) ? g_ShopPack[p][spVeh1] : g_ShopPack[p][spVeh2];
		if (!model) continue;
		format(q, sizeof q, "INSERT INTO vehicles (owner, model, c1, c2, price) VALUES ('%s', %d, 1, 1, 0)", en, model);
		db_free_result(db_query(g_GarDB, q));
	}
	// argent : tout de suite si en ligne, sinon a la connexion
	new target = INVALID_PLAYER_ID, pn[MAX_PLAYER_NAME];
	for (new i = 0; i < MAX_PLAYERS; i++) if (IsPlayerConnected(i)) { GetPlayerName(i, pn, sizeof pn); if (!strcmp(pn, en, true)) { target = i; break; } }
	if (target != INVALID_PLAYER_ID)
	{
		Shop_Deliver(target);
		SendClientMessage(target, 0x66FF66FF, "Boutique Crestwood : ton pack est livre. Tes vehicules sont dans ton garage.");
		format(m, sizeof m, "Pack %d livre a %s (en ligne). Ref %s.", pack, en, er);
	}
	else format(m, sizeof m, "Pack %d enregistre pour %s (hors ligne) : argent a sa connexion, vehicules deja au garage. Ref %s.", pack, en, er);
	SendClientMessage(playerid, 0x66FF66FF, m);
	return 1;
}
