/* ============================================================
   SYSTEME IMMOBILIER (HTML @house) - CrestwoodRP
   ============================================================ */
#define HOUSE_DIALOG_OPEN      9730
#define HOUSE_DIALOG_UPDATE    9731
#define HOUSE_DIALOG_CLOSE     9732

#define HOUSE_MAX              20
#define HOUSE_MAX_OWNED        2
#define HOUSE_SELL_PCT         60     // revente a 60% du prix d'achat

#define HOUSE_TYPE_STUDIO      1
#define HOUSE_TYPE_APPART      2
#define HOUSE_TYPE_MAISON      3
#define HOUSE_TYPE_GRANDE      4
#define HOUSE_TYPE_VILLA       5

new HousePickup[HOUSE_MAX];
new HouseBlip[HOUSE_MAX];
new HouseModel[HOUSE_MAX];          // id du modele d'objet pour la porte
new Float:HousePos[HOUSE_MAX][3];   // position de la porte
new Float:HouseIntPos[HOUSE_MAX][3];// position interieur (teleport)
new HousePrice[HOUSE_MAX];
new HouseType[HOUSE_MAX];           // 1-5
new HouseOwner[MAX_PLAYER_NAME + 1][HOUSE_MAX]; // nom du proprietaire
new HouseLocked[HOUSE_MAX];         // 0 ouvert, 1 ferme
new HouseSafe[HOUSE_MAX];           // montant dans le coffre
new HouseForSale[HOUSE_MAX];        // 0 non, 1 oui
new HouseCount[MAX_PLAYERS];        // nombre de maisons posee par joueur

new DB:HouseDB;

forward House_OnInit();
forward House_OnConnect(playerid);
forward House_OnCommand(playerid, const cmdtext[]);
forward House_OnDialogResponse(playerid, dialogid, response, listitem, inputtext[]);

// ------------------------------------------------------------
//  Base de donnees
// ------------------------------------------------------------
stock House_InitDB()
{
	HouseDB = db_open("crestwood_house.db");
	if (HouseDB == DB:0)
	{
		print("[MAISON] ERREUR : impossible d'ouvrir crestwood_house.db");
		return 0;
	}
	db_free_result(db_query(HouseDB, "CREATE TABLE IF NOT EXISTS houses (id INTEGER PRIMARY KEY, owner TEXT COLLATE NOCASE, price INTEGER, locked INTEGER DEFAULT 0, safe INTEGER DEFAULT 0, for_sale INTEGER DEFAULT 0)"));
	db_free_result(db_query(HouseDB, "CREATE TABLE IF NOT EXISTS house_safe (house_id INTEGER PRIMARY KEY, amount INTEGER DEFAULT 0)"));
	print("[MAISON] Base de donnees prete.");
	return 1;
}

// ------------------------------------------------------------
//  Initialisation des maisons (emplacements)
// ------------------------------------------------------------
stock House_Add(id, Float:x, Float:y, Float:z, Float:ix, Float:iy, Float:iz, price, type)
{
	if (id < 0 || id >= HOUSE_MAX) return 0;
	HousePos[id][0] = x; HousePos[id][1] = y; HousePos[id][2] = z;
	HouseIntPos[id][0] = ix; HouseIntPos[id][1] = iy; HouseIntPos[id][2] = iz;
	HousePrice[id] = price;
	HouseType[id] = type;
	HouseOwner[id][0] = 0;
	HouseLocked[id] = 1;
	HouseSafe[id] = 0;
	HouseForSale[id] = 1;

	// Pickup porte (1273 = maison a vendre, 1272 = vendue)
	HousePickup[id] = CreatePickup(1273, 23, x, y, z + 1.0, -1);
	// Blip carte (31 = maison a vendre, 32 = vendue)
	HouseBlip[id] = 0;
	return 1;
}

stock House_LoadFromDB()
{
	new id, owner[32], price, locked, safe, for_sale, query[256];
	new DBResult:res = db_query(HouseDB, "SELECT id, owner, price, locked, safe, for_sale FROM houses");
	new rows = db_num_rows(res);
	for (new i = 0; i < rows; i++)
	{
		db_get_field(res, 0, owner, sizeof owner); id = strval(owner);
		db_get_field(res, 1, owner, sizeof owner);
		db_get_field(res, 2, owner, sizeof owner); price = strval(owner);
		db_get_field(res, 3, owner, sizeof owner); locked = strval(owner);
		db_get_field(res, 4, owner, sizeof owner); safe = strval(owner);
		db_get_field(res, 5, owner, sizeof owner); for_sale = strval(owner);
		HouseOwner[id][0] = 0;
		strcopy(HouseOwner[id], owner);
		HousePrice[id] = price;
		HouseLocked[id] = locked;
		HouseSafe[id] = safe;
		HouseForSale[id] = for_sale;
		HouseCount[0]++; // placeholder
		db_next_row(res);
	}
	db_free_result(res);
	return rows;
}

// ------------------------------------------------------------
//  Ouverture de la page HTML immobilier
// ------------------------------------------------------------
stock House_Open(playerid)
{
	if (!IsPlayerConnected(playerid)) return 0;
	new json[2000], list[1500], name[MAX_PLAYER_NAME + 1];
	GetPlayerName(playerid, name, sizeof name);
	for (new i = 0; name[i] != 0; i++) if (name[i] == '_') name[i] = ' ';

	new cash = GetPlayerMoney(playerid);
	new bank = 0; // recuperer solde bancaire si dispo
	new mine = HouseCount[playerid];
	new max = HOUSE_MAX_OWNED;
	new sellPct = HOUSE_SELL_PCT;

	list[0] = 0;
	for (new i = 0; i < HOUSE_MAX; i++)
	{
		if (HouseOwner[i][0] == 0) continue; // pas encore init
		new dist = floatround(GetPlayerDistanceFromPoint(playerid, HousePos[i][0], HousePos[i][1], HousePos[i][2]));
		new entry[120];
		format(entry, sizeof entry, "%d,%d,%d,\"%s\",%d,%d,%d,", i, HouseType[i], HousePrice[i], HouseOwner[i], HouseLocked[i], HouseForSale[i], dist);
		if (list[0] != 0) strcat(list, ",", sizeof list);
		strcat(list, entry, sizeof list);
	}

	new h_id = -1, h_type = 0, h_price = 0, h_locked = 1, h_in = 0, h_safe = 0;
	for (new i = 0; i < HOUSE_MAX; i++)
	{
		if (strcmp(HouseOwner[i], name) == 0)
		{
			h_id = i; h_type = HouseType[i]; h_price = HousePrice[i]; h_locked = HouseLocked[i]; h_safe = HouseSafe[i]; break;
		}
	}

	format(json, sizeof json,
		"{\"v\":\"door\",\"cash\":%d,\"bank\":%d,\"mine\":%d,\"max\":%d,\"sellPct\":%d,\"ok\":1,\"msg\":\"\",\"h\":{\"id\":%d,\"t\":%d,\"price\":%d,\"owner\":\"%s\",\"me\":%d,\"lock\":%d,\"safe\":%d,\"in\":%d},\"list\":[%s]}",
		cash, bank, mine, max, sellPct,
		h_id, h_type, h_price, (h_id >= 0 ? HouseOwner[h_id] : ""), (h_id >= 0 ? 1 : 0), h_locked, h_safe, h_in, list);

	ShowPlayerDialog(playerid, HOUSE_DIALOG_OPEN, DIALOG_STYLE_MSGBOX, "@house", json, "OK", "");
	return 1;
}

// ------------------------------------------------------------
//  Commande /maison
// ------------------------------------------------------------
stock House_OnCommand(playerid, const cmdtext[])
{
	if (!strcmp(cmdtext, "/maison", true) || !strcmp(cmdtext, "/house", true))
	{
		return House_Open(playerid);
	}
	// Commande cachee envoyee par la page HTML
	if (!strcmp(cmdtext, "/houseact ", true, 9))
	{
		House_Action(playerid, cmdtext[9]);
		return 1;
	}
	return 0;
}

// ------------------------------------------------------------
//  Actions /houseact
// ------------------------------------------------------------
stock House_Action(playerid, const params[])
{
	new idx = 0, action[16], arg[64];
	Bank_NextToken(params, idx, action, sizeof action);
	Bank_NextToken(params, idx, arg, sizeof arg);

	if (!strcmp(action, "refresh", true))
	{
		House_Open(playerid);
		return 1;
	}

	new name[MAX_PLAYER_NAME + 1];
	GetPlayerName(playerid, name, sizeof name);
	for (new i = 0; name[i] != 0; i++) if (name[i] == '_') name[i] = ' ';

	if (!strcmp(action, "buy", true))
	{
		// Acheter la maison la plus proche
		new best = -1, bestDist = 999999;
		new Float:x, Float:y, Float:z;
		GetPlayerPos(playerid, x, y, z);
		for (new i = 0; i < HOUSE_MAX; i++)
		{
			if (!HouseForSale[i]) continue;
			new Float:d = GetPlayerDistanceFromPoint(playerid, HousePos[i][0], HousePos[i][1], HousePos[i][2]);
			if (d < bestDist) { bestDist = floatround(d); best = i; }
		}
		if (best == -1) { SendClientMessage(playerid, 0xFF4444FF, "Aucune maison a vendre pres de vous."); return 1; }
		if (HouseCount[playerid] >= HOUSE_MAX_OWNED) { SendClientMessage(playerid, 0xFF4444FF, "Vous possedez deja le maximum de maisons."); return 1; }
		new money = GetPlayerMoney(playerid);
		if (money < HousePrice[best]) { SendClientMessage(playerid, 0xFF4444FF, "Argent insuffisant."); return 1; }
		GivePlayerMoney(playerid, -HousePrice[best]);
		HouseOwner[best][0] = 0;
		strcopy(HouseOwner[best], name);
		HouseLocked[best] = 1;
		HouseForSale[best] = 0;
		HouseCount[playerid]++;
		DestroyPickup(HousePickup[best]); HousePickup[best] = CreatePickup(1272, 23, HousePos[best][0], HousePos[best][1], HousePos[best][2] + 1.0, -1);
		// SetBlipColor non dispo en 0.3.7
		SendClientMessage(playerid, 0x33CC33FF, "Maison achetee avec succes !");
		House_Open(playerid);
		return 1;
	}

	if (!strcmp(action, "sell", true))
	{
		for (new i = 0; i < HOUSE_MAX; i++)
		{
			if (strcmp(HouseOwner[i], name) == 0)
			{
				new sellPrice = HousePrice[i] * HOUSE_SELL_PCT / 100;
				new money = GetPlayerMoney(playerid);
				GivePlayerMoney(playerid, sellPrice);
				HouseOwner[i][0] = 0;
				HouseLocked[i] = 1;
				HouseForSale[i] = 1;
				HouseCount[playerid]--;
				DestroyPickup(HousePickup[i]); HousePickup[i] = CreatePickup(1273, 23, HousePos[i][0], HousePos[i][1], HousePos[i][2] + 1.0, -1);
				// SetBlipColor non dispo en 0.3.7
				SendClientMessage(playerid, 0x33CC33FF, "Maison vendue avec succes !");
				House_Open(playerid);
				return 1;
			}
		}
		SendClientMessage(playerid, 0xFF4444FF, "Vous ne possedez aucune maison.");
		return 1;
	}

	if (!strcmp(action, "lock", true))
	{
		for (new i = 0; i < HOUSE_MAX; i++)
		{
			if (strcmp(HouseOwner[i], name) == 0)
			{
				HouseLocked[i] = !HouseLocked[i];
				SendClientMessage(playerid, HouseLocked[i] ? 0xFF4444FF : 0x33CC33FF, HouseLocked[i] ? "Maison fermee a cle." : "Maison ouverte.");
				House_Open(playerid);
				return 1;
			}
		}
		SendClientMessage(playerid, 0xFF4444FF, "Vous ne possedez aucune maison.");
		return 1;
	}

	if (!strcmp(action, "enter", true))
	{
		for (new i = 0; i < HOUSE_MAX; i++)
		{
			if (strcmp(HouseOwner[i], name) == 0)
			{
				SetPlayerPos(playerid, HouseIntPos[i][0], HouseIntPos[i][1], HouseIntPos[i][2]);
				SendClientMessage(playerid, 0x33CC33FF, "Vous entrez dans votre maison.");
				return 1;
			}
		}
		SendClientMessage(playerid, 0xFF4444FF, "Vous ne possedez aucune maison.");
		return 1;
	}

	if (!strcmp(action, "gps", true))
	{
		new target = strval(arg);
		if (target >= 0 && target < HOUSE_MAX)
		{
			// Utiliser le systeme GPS existant
			SendClientMessage(playerid, 0x00FF00FF, "GPS defini sur votre maison.");
		}
		return 1;
	}

	if (!strcmp(action, "deposit", true))
	{
		new amount = strval(arg);
		for (new i = 0; i < HOUSE_MAX; i++)
		{
			if (strcmp(HouseOwner[i], name) == 0)
			{
				new money = GetPlayerMoney(playerid);
				if (money < amount) { SendClientMessage(playerid, 0xFF4444FF, "Argent insuffisant."); return 1; }
				GivePlayerMoney(playerid, -amount);
				HouseSafe[i] += amount;
				SendClientMessage(playerid, 0x33CC33FF, "Argent depose dans le coffre.");
				House_Open(playerid);
				return 1;
			}
		}
		SendClientMessage(playerid, 0xFF4444FF, "Vous ne possedez aucune maison.");
		return 1;
	}

	if (!strcmp(action, "withdraw", true))
	{
		new amount = strval(arg);
		for (new i = 0; i < HOUSE_MAX; i++)
		{
			if (strcmp(HouseOwner[i], name) == 0)
			{
				if (HouseSafe[i] < amount) { SendClientMessage(playerid, 0xFF4444FF, "Coffre insuffisamment garni."); return 1; }
				GivePlayerMoney(playerid, amount);
				HouseSafe[i] -= amount;
				SendClientMessage(playerid, 0x33CC33FF, "Argent retire du coffre.");
				House_Open(playerid);
				return 1;
			}
		}
		SendClientMessage(playerid, 0xFF4444FF, "Vous ne possedez aucune maison.");
		return 1;
	}

	return 0;
}


// ------------------------------------------------------------
//  Connexion joueur
// ------------------------------------------------------------
stock House_OnConnect(playerid)
{
	// Initialisation des compteurs par joueur
	HouseCount[playerid] = 0;
	return 1;
}


// ------------------------------------------------------------
//  Initialisation serveur
// ------------------------------------------------------------
stock House_OnInit()
{
	House_InitDB();
	// 20 maisons a vendre a Los Santos (emplacements types)
	House_Add(0,  1200.0, -1700.0, 14.0, 1200.0, -1700.0, 14.0, 50000, HOUSE_TYPE_STUDIO);
	House_Add(1,  1300.0, -1700.0, 14.0, 1300.0, -1700.0, 14.0, 75000, HOUSE_TYPE_APPART);
	House_Add(2,  1400.0, -1700.0, 14.0, 1400.0, -1700.0, 14.0, 100000, HOUSE_TYPE_MAISON);
	House_Add(3,  1500.0, -1700.0, 14.0, 1500.0, -1700.0, 14.0, 150000, HOUSE_TYPE_GRANDE);
	House_Add(4,  1600.0, -1700.0, 14.0, 1600.0, -1700.0, 14.0, 250000, HOUSE_TYPE_VILLA);
	House_Add(5,  1200.0, -1600.0, 14.0, 1200.0, -1600.0, 14.0, 60000, HOUSE_TYPE_STUDIO);
	House_Add(6,  1300.0, -1600.0, 14.0, 1300.0, -1600.0, 14.0, 85000, HOUSE_TYPE_APPART);
	House_Add(7,  1400.0, -1600.0, 14.0, 1400.0, -1600.0, 14.0, 110000, HOUSE_TYPE_MAISON);
	House_Add(8,  1500.0, -1600.0, 14.0, 1500.0, -1600.0, 14.0, 160000, HOUSE_TYPE_GRANDE);
	House_Add(9,  1600.0, -1600.0, 14.0, 1600.0, -1600.0, 14.0, 270000, HOUSE_TYPE_VILLA);
	House_Add(10, 1200.0, -1500.0, 14.0, 1200.0, -1500.0, 14.0, 55000, HOUSE_TYPE_STUDIO);
	House_Add(11, 1300.0, -1500.0, 14.0, 1300.0, -1500.0, 14.0, 80000, HOUSE_TYPE_APPART);
	House_Add(12, 1400.0, -1500.0, 14.0, 1400.0, -1500.0, 14.0, 105000, HOUSE_TYPE_MAISON);
	House_Add(13, 1500.0, -1500.0, 14.0, 1500.0, -1500.0, 14.0, 155000, HOUSE_TYPE_GRANDE);
	House_Add(14, 1600.0, -1500.0, 14.0, 1600.0, -1500.0, 14.0, 260000, HOUSE_TYPE_VILLA);
	House_Add(15, 1200.0, -1400.0, 14.0, 1200.0, -1400.0, 14.0, 65000, HOUSE_TYPE_STUDIO);
	House_Add(16, 1300.0, -1400.0, 14.0, 1300.0, -1400.0, 14.0, 90000, HOUSE_TYPE_APPART);
	House_Add(17, 1400.0, -1400.0, 14.0, 1400.0, -1400.0, 14.0, 115000, HOUSE_TYPE_MAISON);
	House_Add(18, 1500.0, -1400.0, 14.0, 1500.0, -1400.0, 14.0, 165000, HOUSE_TYPE_GRANDE);
	House_Add(19, 1600.0, -1400.0, 14.0, 1600.0, -1400.0, 14.0, 280000, HOUSE_TYPE_VILLA);
	printf("[MAISON] %d maisons chargees.", HOUSE_MAX);
	return 1;
}
