// ============================================================
//  [v32] UBER : CHAUFFEURS AVEC LEUR VOITURE PERSONNELLE + TAXIMETRE
// ------------------------------------------------------------
//  Chauffeur : au volant de SA voiture (vehicule de garage) ou d'un vehicule quelconque s'il est dans la
//  faction taxi -> appli Uber du telephone, onglet "Chauffeur" -> "Passer en ligne" (ou /uber en ligne).
//  Passager  : appli Uber -> "Demander" : la course est proposee au chauffeur en ligne LE PLUS PROCHE
//              (20 s pour accepter, sinon le suivant). Le passager suit la voiture sur la carte de l'appli,
//              le chauffeur a le GPS vers le client puis vers la destination choisie sur la carte.
//  Taximetre : demarre quand le client monte, s'affiche dans le HUD des deux joueurs (prix + distance).
//              Fin : le chauffeur touche "Terminer" (ou /uber terminer) ou le client descend.
//              Le client paie en liquide ; le chauffeur recoit 80 % sur son solde Uber
//              (bouton "Retirer" de l'appli ou /uber retirer -> liquide).
//  Telephone -> serveur : /phoneact uber <on|off|req|cancel|accept|decline|end|withdraw|sync> [arg]
//  Serveur -> telephone : {"ev":"uber", ...} (Ph_Send)    HUD : {"m":"meter",...} (C23_Send)
//  Donnees : table uber_drivers de crestwood_v23.db (solde, nombre de courses)
// ============================================================

#define UB_OFFER_S          20
#define UB_SHARE_PCT        80
#define UB_ARRIVE_DIST      12.0

enum { UB_NONE, UB_SEARCH, UB_PICKUP, UB_TRIP }

// type de course : nom, prise en charge, prix au km
new const g_UbTypeKey[4][8] = { "x", "comfort", "black", "moto" };
new const g_UbTypeName[4][10] = { "UberX", "Comfort", "Black", "Moto" };
new const g_UbBase[4] = { 150, 300, 600, 100 };
new const g_UbPerKm[4] = { 500, 750, 1200, 380 };

new g_UbOn[MAX_PLAYERS];                 // chauffeur en ligne
new g_UbVeh[MAX_PLAYERS];                // vehicule du chauffeur en ligne
// course (indexee par le PASSAGER)
new g_UbSt[MAX_PLAYERS];
new g_UbType[MAX_PLAYERS];
new g_UbDrv[MAX_PLAYERS] = {INVALID_PLAYER_ID, ...};
new g_UbOffer[MAX_PLAYERS] = {INVALID_PLAYER_ID, ...};   // chauffeur a qui la course est proposee
new g_UbOfferEnd[MAX_PLAYERS];
new g_UbTried[MAX_PLAYERS][MAX_PLAYERS char];             // chauffeurs deja sollicites
new Float:g_UbDest[MAX_PLAYERS][2];
new g_UbHasDest[MAX_PLAYERS];
new Float:g_UbDist[MAX_PLAYERS];          // metres parcourus pendant la course
new Float:g_UbLast[MAX_PLAYERS][3];
new g_UbArrivedSent[MAX_PLAYERS];
new g_UbClient[MAX_PLAYERS] = {INVALID_PLAYER_ID, ...};   // chauffeur -> son client

forward Ub_Tick();

stock Ub_Init()
{
	C23_Exec("CREATE TABLE IF NOT EXISTS uber_drivers (name TEXT PRIMARY KEY COLLATE NOCASE, bal INTEGER DEFAULT 0, rides INTEGER DEFAULT 0)");
	SetTimer("Ub_Tick", 1000, true);
	return 1;
}

stock Ub_Reset(playerid)
{
	g_UbSt[playerid] = UB_NONE;
	g_UbDrv[playerid] = INVALID_PLAYER_ID;
	g_UbOffer[playerid] = INVALID_PLAYER_ID;
	g_UbHasDest[playerid] = 0;
	g_UbDist[playerid] = 0.0;
	g_UbArrivedSent[playerid] = 0;
	for (new i = 0; i < MAX_PLAYERS; i++) g_UbTried[playerid]{i} = 0;
	return 1;
}

stock Ub_OnConnect(playerid)
{
	g_UbOn[playerid] = 0;
	g_UbVeh[playerid] = 0;
	g_UbClient[playerid] = INVALID_PLAYER_ID;
	return Ub_Reset(playerid);
}

stock Ub_Name(playerid, out[], size)
{
	GetPlayerName(playerid, out, size);
	for (new i = 0; out[i]; i++) if (out[i] == '_') out[i] = ' ';
	return 1;
}

stock Ub_Fare(rider)
{
	new t = g_UbType[rider];
	return g_UbBase[t] + floatround(g_UbDist[rider] / 1000.0 * float(g_UbPerKm[t]));
}

// solde du chauffeur (et nombre de courses)
stock Ub_Get(playerid, &bal, &rides)
{
	bal = 0; rides = 0;
	if (g_C23DB == DB:0) return 0;
	new name[MAX_PLAYER_NAME + 1], q[160], f[16];
	GetPlayerName(playerid, name, sizeof name);
	format(q, sizeof q, "SELECT bal, rides FROM uber_drivers WHERE name = '%s'", name);
	new DBResult:r = db_query(g_C23DB, q);
	if (r == DBResult:0) return 0;
	if (db_num_rows(r)) { db_get_field(r, 0, f, sizeof f); bal = strval(f); db_get_field(r, 1, f, sizeof f); rides = strval(f); }
	db_free_result(r);
	return 1;
}

stock Ub_AddBal(playerid, amount, ride)
{
	new name[MAX_PLAYER_NAME + 1], q[220];
	GetPlayerName(playerid, name, sizeof name);
	format(q, sizeof q, "INSERT OR IGNORE INTO uber_drivers (name, bal, rides) VALUES ('%s', 0, 0)", name);
	C23_Exec(q);
	format(q, sizeof q, "UPDATE uber_drivers SET bal = bal + %d, rides = rides + %d WHERE name = '%s'", amount, ride, name);
	C23_Exec(q);
	return 1;
}

// le chauffeur peut-il rouler pour Uber avec ce vehicule ?
stock Ub_CanDrive(playerid)
{
	new v = GetPlayerVehicleID(playerid);
	if (!v || GetPlayerState(playerid) != PLAYER_STATE_DRIVER) return 0;
	if (g_GarVehDb[v] && g_GarVehOwner[v] == playerid) return v;      // voiture personnelle
	if (Fac_Type(playerid) == FACTION_TAXIDRIVER) return v;           // faction taxi : n'importe quel vehicule
	return 0;
}

// ---------------- telephone ----------------
stock Ub_SendDriver(playerid, ok = -1, const msg[] = "")
{
	new json[700], bal, rides, vn[32], part[300];
	Ub_Get(playerid, bal, rides);
	format(vn, sizeof vn, "-");
	new v = g_UbOn[playerid] ? g_UbVeh[playerid] : Ub_CanDrive(playerid);
	if (v) Gar_VehName(GetVehicleModel(v), vn, sizeof vn);
	format(json, sizeof json, "{\"ev\":\"uber\",\"role\":\"driver\",\"on\":%d,\"bal\":%d,\"rides\":%d,\"veh\":\"%s\",\"can\":%d", g_UbOn[playerid], bal, rides, vn, Ub_CanDrive(playerid) ? 1 : 0);
	// course proposee
	new req = INVALID_PLAYER_ID;
	for (new i = 0, mx = GetMaxPlayers(); i < mx; i++) if (IsPlayerConnected(i) && g_UbSt[i] == UB_SEARCH && g_UbOffer[i] == playerid) { req = i; break; }
	if (req != INVALID_PLAYER_ID)
	{
		new rn[MAX_PLAYER_NAME + 1], Float:x, Float:y, Float:z;
		Ub_Name(req, rn, sizeof rn);
		GetPlayerPos(req, x, y, z);
		format(part, sizeof part, ",\"req\":{\"id\":%d,\"name\":\"%s\",\"type\":\"%s\",\"dist\":%d,\"left\":%d}", req, rn, g_UbTypeName[g_UbType[req]],
			floatround(GetPlayerDistanceFromPoint(playerid, x, y, z)), g_UbOfferEnd[req] - gettime());
		strcat(json, part, sizeof json);
	}
	new c = g_UbClient[playerid];
	if (c != INVALID_PLAYER_ID && IsPlayerConnected(c))
	{
		new cn[MAX_PLAYER_NAME + 1];
		Ub_Name(c, cn, sizeof cn);
		format(part, sizeof part, ",\"ride\":{\"id\":%d,\"name\":\"%s\",\"type\":\"%s\",\"st\":\"%s\",\"km\":%.2f,\"fare\":%d,\"dest\":%d}", c, cn, g_UbTypeName[g_UbType[c]],
			g_UbSt[c] == UB_TRIP ? ("trip") : ("pickup"), g_UbDist[c] / 1000.0, Ub_Fare(c), g_UbHasDest[c]);
		strcat(json, part, sizeof json);
	}
	if (msg[0]) { format(part, sizeof part, ",\"ok\":%d,\"msg\":\"%s\"", ok, msg); strcat(json, part, sizeof json); }
	strcat(json, "}", sizeof json);
	if (msg[0] && ok == 0) Cw_Notify(playerid, "denied", "Uber", msg, 3500);   // aussi sans le telephone
	return Ph_Send(playerid, json);
}

stock Ub_SendRider(rider, const st[], const msg[] = "")
{
	new json[400], part[200];
	format(json, sizeof json, "{\"ev\":\"uber\",\"role\":\"rider\",\"st\":\"%s\",\"type\":\"%s\"", st, g_UbTypeName[g_UbType[rider]]);
	new d = g_UbDrv[rider];
	if (d != INVALID_PLAYER_ID && IsPlayerConnected(d))
	{
		new dn[MAX_PLAYER_NAME + 1], vn[32], Float:x, Float:y, Float:z;
		Ub_Name(d, dn, sizeof dn);
		format(vn, sizeof vn, "Vehicule");
		if (g_UbVeh[d]) Gar_VehName(GetVehicleModel(g_UbVeh[d]), vn, sizeof vn);
		GetPlayerPos(d, x, y, z);
		new dist = floatround(GetPlayerDistanceFromPoint(rider, x, y, z));
		format(part, sizeof part, ",\"drv\":\"%s\",\"car\":\"%s\",\"plate\":\"#%d\",\"eta\":%d", dn, vn, g_UbVeh[d], dist / 250 + 1);
		strcat(json, part, sizeof json);
	}
	format(part, sizeof part, ",\"km\":%.2f,\"fare\":%d", g_UbDist[rider] / 1000.0, Ub_Fare(rider));
	strcat(json, part, sizeof json);
	if (msg[0]) { format(part, sizeof part, ",\"msg\":\"%s\"", msg); strcat(json, part, sizeof json); }
	strcat(json, "}", sizeof json);
	return Ph_Send(rider, json);
}

stock Ub_Meter(playerid, on, rider)
{
	new json[160];
	if (!on) format(json, sizeof json, "{\"m\":\"meter\",\"on\":0}");
	else format(json, sizeof json, "{\"m\":\"meter\",\"on\":1,\"km\":%.2f,\"fare\":%d,\"type\":\"%s\"}", g_UbDist[rider] / 1000.0, Ub_Fare(rider), g_UbTypeName[g_UbType[rider]]);
	return C23_Send(playerid, json);
}

// ---------------- dispatch ----------------
stock Ub_Nearest(rider)
{
	new best = INVALID_PLAYER_ID, Float:bd = 999999.0, Float:x, Float:y, Float:z;
	GetPlayerPos(rider, x, y, z);
	for (new i = 0, mx = GetMaxPlayers(); i < mx; i++)
	{
		if (i == rider || !IsPlayerConnected(i) || !g_UbOn[i] || g_UbClient[i] != INVALID_PLAYER_ID || g_UbTried[rider]{i}) continue;
		new busy = 0;
		for (new j = 0; j < mx; j++) if (IsPlayerConnected(j) && g_UbSt[j] == UB_SEARCH && g_UbOffer[j] == i) { busy = 1; break; }
		if (busy) continue;
		new Float:d = GetPlayerDistanceFromPoint(i, x, y, z);
		if (d < bd) { bd = d; best = i; }
	}
	return best;
}

stock Ub_Offer(rider)
{
	new d = Ub_Nearest(rider);
	if (d == INVALID_PLAYER_ID)
	{
		g_UbOffer[rider] = INVALID_PLAYER_ID;
		Ub_Reset(rider);
		Ub_SendRider(rider, "none", "Aucun chauffeur Uber disponible pour le moment.");
		return Cw_Notify(rider, "info", "Uber", "Aucun chauffeur disponible. Reessaie dans un instant.", 4500), 0;
	}
	g_UbOffer[rider] = d;
	g_UbOfferEnd[rider] = gettime() + UB_OFFER_S;
	g_UbTried[rider]{d} = 1;
	new rn[MAX_PLAYER_NAME + 1], m[140];
	Ub_Name(rider, rn, sizeof rn);
	format(m, sizeof m, "Nouvelle course %s : %s. Ouvre l'appli Uber (ou /uber accepter).", g_UbTypeName[g_UbType[rider]], rn);
	Cw_Notify(d, "info", "Uber", m, 8000);
	PlayerPlaySound(d, 1150, 0.0, 0.0, 0.0);
	Ub_SendDriver(d);
	return 1;
}

stock Ub_Request(rider, const type[])
{
	if (g_UbSt[rider] != UB_NONE) return Ub_SendRider(rider, g_UbSt[rider] == UB_SEARCH ? ("search") : ("enroute"));
	if (g_UbOn[rider]) return Cw_Notify(rider, "denied", "Uber", "Passe hors ligne avant de commander une course.", 3500), 1;
	new t = 0;
	for (new i = 0; i < 4; i++) if (type[0] && !strcmp(type, g_UbTypeKey[i], true)) t = i;
	Ub_Reset(rider);
	g_UbType[rider] = t;
	g_UbSt[rider] = UB_SEARCH;
	Ub_SendRider(rider, "search");
	return Ub_Offer(rider);
}

stock Ub_Cancel(rider, const why[])
{
	new d = g_UbDrv[rider], o = g_UbOffer[rider];
	if (d != INVALID_PLAYER_ID && IsPlayerConnected(d))
	{
		g_UbClient[d] = INVALID_PLAYER_ID;
		Ub_Meter(d, 0, rider);
		DisablePlayerCheckpoint(d);
		Cw_Notify(d, "info", "Uber", why, 4000);
		Ub_SendDriver(d);
	}
	else if (o != INVALID_PLAYER_ID && IsPlayerConnected(o) && g_UbSt[rider] == UB_SEARCH) { g_UbOffer[rider] = INVALID_PLAYER_ID; Ub_SendDriver(o); }
	if (IsPlayerConnected(rider)) { Ub_Meter(rider, 0, rider); Ub_SendRider(rider, "none", why); }
	Ub_Reset(rider);
	return 1;
}

stock Ub_Accept(d)
{
	new r = INVALID_PLAYER_ID;
	for (new i = 0, mx = GetMaxPlayers(); i < mx; i++) if (IsPlayerConnected(i) && g_UbSt[i] == UB_SEARCH && g_UbOffer[i] == d) { r = i; break; }
	if (r == INVALID_PLAYER_ID) return Ub_SendDriver(d, 0, "Plus de course en attente.");
	if (!Ub_CanDrive(d)) return Ub_SendDriver(d, 0, "Monte au volant de ta voiture d'abord.");
	g_UbVeh[d] = GetPlayerVehicleID(d);
	g_UbDrv[r] = d;
	g_UbOffer[r] = INVALID_PLAYER_ID;
	g_UbSt[r] = UB_PICKUP;
	g_UbClient[d] = r;
	new Float:x, Float:y, Float:z;
	GetPlayerPos(r, x, y, z);
	SetPlayerCheckpoint(d, x, y, z, 6.0);
	new dn[MAX_PLAYER_NAME + 1], m[120];
	Ub_Name(d, dn, sizeof dn);
	format(m, sizeof m, "%s arrive ! Suis sa voiture sur la carte de l'appli Uber.", dn);
	Cw_Notify(r, "success", "Uber", m, 6000);
	Ub_SendRider(r, "enroute");
	return Ub_SendDriver(d, 1, "Course acceptee : va chercher le client (GPS).");
}

stock Ub_Decline(d)
{
	for (new i = 0, mx = GetMaxPlayers(); i < mx; i++)
	{
		if (IsPlayerConnected(i) && g_UbSt[i] == UB_SEARCH && g_UbOffer[i] == d) { Ub_Offer(i); break; }
	}
	return Ub_SendDriver(d);
}

stock Ub_Finish(d, const why[])
{
	new r = g_UbClient[d];
	if (r == INVALID_PLAYER_ID || !IsPlayerConnected(r)) { g_UbClient[d] = INVALID_PLAYER_ID; return Ub_SendDriver(d); }
	if (g_UbSt[r] != UB_TRIP) return Ub_Cancel(r, why);
	new fare = Ub_Fare(r), paid = fare, cash = GetPlayerMoney(r);
	if (paid > cash) paid = cash < 0 ? 0 : cash;
	if (paid > 0) GivePlayerMoney(r, -paid);
	new gain = paid * UB_SHARE_PCT / 100;
	Ub_AddBal(d, gain, 1);
	new m[160], a[16], b[16];
	Cw_Fmt(paid, a, sizeof a);
	Cw_Fmt(gain, b, sizeof b);
	format(m, sizeof m, "Course terminee : %s $ payes (%.1f km).%s", a, g_UbDist[r] / 1000.0, paid < fare ? (" Pas assez de liquide pour tout payer.") : (""));
	Cw_Notify(r, paid < fare ? ("warning") : ("success"), "Uber", m, 6000);
	format(m, sizeof m, "Course terminee : +%s $ sur ton solde Uber.", b);
	Cw_Notify(d, "success", "Uber", m, 5000);
	Ub_Meter(r, 0, r);
	Ub_Meter(d, 0, r);
	DisablePlayerCheckpoint(d);
	Ub_SendRider(r, "done");
	g_UbClient[d] = INVALID_PLAYER_ID;
	Ub_Reset(r);
	return Ub_SendDriver(d);
}

stock Ub_SetOnline(playerid, on)
{
	if (on)
	{
		new v = Ub_CanDrive(playerid);
		if (!v) return Ub_SendDriver(playerid, 0, "Monte au volant de TA voiture (garage) pour passer en ligne.");
		g_UbOn[playerid] = 1;
		g_UbVeh[playerid] = v;
		Cw_Notify(playerid, "success", "Uber", "Tu es en ligne : les clients proches te seront proposes.", 4500);
		return Ub_SendDriver(playerid, 1, "En ligne.");
	}
	if (g_UbClient[playerid] != INVALID_PLAYER_ID) return Ub_SendDriver(playerid, 0, "Termine d'abord ta course.");
	g_UbOn[playerid] = 0;
	Ub_Decline(playerid);
	return Ub_SendDriver(playerid, 1, "Hors ligne.");
}

stock Ub_Withdraw(playerid)
{
	new bal, rides;
	Ub_Get(playerid, bal, rides);
	if (bal <= 0) return Ub_SendDriver(playerid, 0, "Solde vide.");
	Ub_AddBal(playerid, -bal, 0);
	GivePlayerMoney(playerid, bal);
	new m[80], a[16];
	Cw_Fmt(bal, a, sizeof a);
	format(m, sizeof m, "%s $ retires en liquide.", a);
	return Ub_SendDriver(playerid, 1, m);
}

// ---------------- evenements ----------------
stock Ub_OnStateChange(playerid, newstate, oldstate)
{
	// le client monte dans la voiture du chauffeur -> taximetre
	if (newstate == PLAYER_STATE_PASSENGER && g_UbSt[playerid] == UB_PICKUP)
	{
		new d = g_UbDrv[playerid];
		if (d != INVALID_PLAYER_ID && GetPlayerVehicleID(playerid) == g_UbVeh[d])
		{
			g_UbSt[playerid] = UB_TRIP;
			g_UbDist[playerid] = 0.0;
			GetVehiclePos(g_UbVeh[d], g_UbLast[playerid][0], g_UbLast[playerid][1], g_UbLast[playerid][2]);
			DisablePlayerCheckpoint(d);
			if (g_UbHasDest[playerid]) SetPlayerCheckpoint(d, g_UbDest[playerid][0], g_UbDest[playerid][1], 0.0, 8.0);
			Cw_Notify(d, "info", "Uber", "Client a bord : le taximetre tourne.", 3500);
			Ub_SendRider(playerid, "trip");
			Ub_SendDriver(d);
		}
	}
	// le client descend pendant la course -> fin
	if (oldstate == PLAYER_STATE_PASSENGER && g_UbSt[playerid] == UB_TRIP)
	{
		new d = g_UbDrv[playerid];
		if (d != INVALID_PLAYER_ID && IsPlayerConnected(d)) Ub_Finish(d, "Le client est descendu.");
	}
	// le chauffeur quitte sa voiture : plus en ligne (la course en cours est annulee si le client n'est pas monte)
	if (oldstate == PLAYER_STATE_DRIVER && g_UbOn[playerid])
	{
		new r = g_UbClient[playerid];
		if (r != INVALID_PLAYER_ID && g_UbSt[r] == UB_TRIP) Ub_Finish(playerid, "Course terminee.");
		else if (r != INVALID_PLAYER_ID) Ub_Cancel(r, "Le chauffeur a annule la course.");
		g_UbOn[playerid] = 0;
		Ub_Decline(playerid);
		Cw_Notify(playerid, "info", "Uber", "Tu es hors ligne (tu as quitte ta voiture).", 3500);
	}
	return 1;
}

stock Ub_OnDisconnect(playerid)
{
	if (g_UbSt[playerid] != UB_NONE) Ub_Cancel(playerid, "Le client s'est deconnecte.");
	new r = g_UbClient[playerid];
	if (r != INVALID_PLAYER_ID && IsPlayerConnected(r)) { g_UbDrv[r] = INVALID_PLAYER_ID; Ub_Cancel(r, "Le chauffeur s'est deconnecte."); }
	if (g_UbOn[playerid]) { g_UbOn[playerid] = 0; Ub_Decline(playerid); }
	return Ub_OnConnect(playerid);
}

public Ub_Tick()
{
	new now = gettime();
	for (new r = 0, mx = GetMaxPlayers(); r < mx; r++)
	{
		if (!IsPlayerConnected(r) || g_UbSt[r] == UB_NONE) continue;
		if (g_UbSt[r] == UB_SEARCH)
		{
			new o = g_UbOffer[r];
			if (o == INVALID_PLAYER_ID || !IsPlayerConnected(o) || !g_UbOn[o] || now >= g_UbOfferEnd[r])
			{
				if (o != INVALID_PLAYER_ID && IsPlayerConnected(o)) { g_UbOffer[r] = INVALID_PLAYER_ID; Ub_SendDriver(o); }
				Ub_Offer(r);
			}
			continue;
		}
		new d = g_UbDrv[r];
		if (d == INVALID_PLAYER_ID || !IsPlayerConnected(d)) { Ub_Cancel(r, "Course annulee."); continue; }
		if (g_UbSt[r] == UB_PICKUP)
		{
			new Float:x, Float:y, Float:z;
			GetPlayerPos(r, x, y, z);
			if (!g_UbArrivedSent[r] && GetPlayerDistanceFromPoint(d, x, y, z) < UB_ARRIVE_DIST)
			{
				g_UbArrivedSent[r] = 1;
				Ub_SendRider(r, "arrived");
				Cw_Notify(r, "success", "Uber", "Ton chauffeur est arrive : monte a l'arriere.", 5000);
			}
			continue;
		}
		// course : distance parcourue par la voiture
		new Float:x, Float:y, Float:z;
		GetVehiclePos(g_UbVeh[d], x, y, z);
		new Float:dx = x - g_UbLast[r][0], Float:dy = y - g_UbLast[r][1], Float:dz = z - g_UbLast[r][2];
		new Float:step = floatsqroot(dx * dx + dy * dy + dz * dz);
		if (step < 120.0) g_UbDist[r] += step;   // pas de saut (teleportation)
		g_UbLast[r][0] = x; g_UbLast[r][1] = y; g_UbLast[r][2] = z;
		Ub_Meter(r, 1, r);
		Ub_Meter(d, 1, r);
	}
	return 1;
}

// destination choisie par le passager sur la carte (Insta.inc : /phoneact gps)
stock Ub_SetDest(rider, Float:x, Float:y)
{
	g_UbDest[rider][0] = x; g_UbDest[rider][1] = y; g_UbHasDest[rider] = 1;
	new d = g_UbDrv[rider];
	if (d != INVALID_PLAYER_ID && IsPlayerConnected(d) && g_UbSt[rider] == UB_TRIP) SetPlayerCheckpoint(d, x, y, 0.0, 8.0);
	return 1;
}

// positions pour la carte de l'appli : retourne 1 si une course Uber est en cours
stock Ub_MapInfo(playerid, &Float:ox, &Float:oy, &Float:tx, &Float:ty, &hasDest)
{
	new Float:z;
	hasDest = 0;
	if (g_UbSt[playerid] == UB_PICKUP || g_UbSt[playerid] == UB_TRIP)
	{
		new d = g_UbDrv[playerid];
		if (d == INVALID_PLAYER_ID || !IsPlayerConnected(d)) return 0;
		GetPlayerPos(d, ox, oy, z);
		if (g_UbHasDest[playerid]) { tx = g_UbDest[playerid][0]; ty = g_UbDest[playerid][1]; hasDest = 1; }
		return 1;
	}
	new r = g_UbClient[playerid];
	if (r != INVALID_PLAYER_ID && IsPlayerConnected(r))
	{
		GetPlayerPos(r, ox, oy, z);
		if (g_UbHasDest[r]) { tx = g_UbDest[r][0]; ty = g_UbDest[r][1]; hasDest = 1; }
		return 2;
	}
	return 0;
}

stock Ub_Action(playerid, const sub[], const arg[])
{
	if (!sub[0]) return 1;
	if (!strcmp(sub, "sync", true))
	{
		if (g_UbSt[playerid] != UB_NONE)
		{
			static const stn[4][8] = { "none", "search", "enroute", "trip" };
			Ub_SendRider(playerid, stn[g_UbSt[playerid]]);
		}
		return Ub_SendDriver(playerid);
	}
	if (!strcmp(sub, "req", true)) return Ub_Request(playerid, arg);
	if (!strcmp(sub, "cancel", true)) { if (g_UbSt[playerid] != UB_NONE && g_UbSt[playerid] != UB_TRIP) Ub_Cancel(playerid, "Course annulee."); return 1; }
	if (!strcmp(sub, "on", true)) return Ub_SetOnline(playerid, 1);
	if (!strcmp(sub, "off", true)) return Ub_SetOnline(playerid, 0);
	if (!strcmp(sub, "accept", true)) return Ub_Accept(playerid);
	if (!strcmp(sub, "decline", true)) return Ub_Decline(playerid);
	if (!strcmp(sub, "end", true)) return Ub_Finish(playerid, "Course terminee par le chauffeur.");
	if (!strcmp(sub, "withdraw", true)) return Ub_Withdraw(playerid);
	return 1;
}

stock Ub_OnCommand(playerid, const cmdtext[])
{
	if (!g_AuthLogged[playerid]) return 0;
	if (!strcmp(cmdtext, "/phoneact uber ", true, 15))
	{
		new pos = 15, sub[12];
		Pg_Tok(cmdtext, pos, sub, sizeof sub);
		while (cmdtext[pos] == ' ') pos++;
		return Ub_Action(playerid, sub, cmdtext[pos]), 1;
	}
	new a;
	if ((a = Cw_Arg(cmdtext, "/uber")) != -1)
	{
		new pos = a, sub[16];
		Pg_Tok(cmdtext, pos, sub, sizeof sub);
		if (!sub[0])
		{
			new bal, rides, m[160], b[16];
			Ub_Get(playerid, bal, rides);
			Cw_Fmt(bal, b, sizeof b);
			format(m, sizeof m, "%s - solde %s $ - %d course(s). /uber en ligne | hors ligne | accepter | refuser | terminer | retirer", g_UbOn[playerid] ? ("En ligne") : ("Hors ligne"), b, rides);
			return Cw_Notify(playerid, "info", "Uber", m, 7000), 1;
		}
		if (!strcmp(sub, "en", true) || !strcmp(sub, "on", true)) return Ub_SetOnline(playerid, 1), 1;
		if (!strcmp(sub, "hors", true) || !strcmp(sub, "off", true)) return Ub_SetOnline(playerid, 0), 1;
		if (!strcmp(sub, "accepter", true)) return Ub_Accept(playerid), 1;
		if (!strcmp(sub, "refuser", true)) return Ub_Decline(playerid), 1;
		if (!strcmp(sub, "terminer", true)) return Ub_Finish(playerid, "Course terminee par le chauffeur."), 1;
		if (!strcmp(sub, "retirer", true)) return Ub_Withdraw(playerid), 1;
		return Cw_Notify(playerid, "info", "Uber", "/uber en ligne | hors ligne | accepter | refuser | terminer | retirer", 5000), 1;
	}
	return 0;
}
