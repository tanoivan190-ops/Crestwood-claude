/* ============================================================
   CRESTWOOD RP v25 (17/11/2026)
   ------------------------------------------------------------
   1) LOCATION DE VELO (BMX) : points places par le staff (panneau admin > Factions > Locations de velo,
      ou /adminact bikeadd 0). Marqueur 3D "velo" au sol ; en marchant dessus la page s'ouvre
      (HUD : hud/bike.js, module "bike" de CwBus). 500 $ pour 1 heure (liquide, sinon banque).
      Rendre : rouler jusqu'a n'importe quel point de location (rendu automatique) ou /rendrevelo pres d'un point.
      Pas rendu a temps : le velo disparait et une amende de 750 $ est ajoutee (MDT / /amendes).
   2) TELEPHONE EN MAIN : quand le telephone de l'appli s'ouvre, l'appli envoie /telmain 1 -> le perso
      tient son telephone (action speciale SA-MP, on peut marcher et courir) ; /telmain 0 a la fermeture.
   HUD -> serveur : /c25 bike <rent cash|rent bank|return|close>
   Texte ASCII. Inclus apres <Cw23> (utilise la base crestwood_v23.db et les amendes c23_fines).
   ============================================================ */
#define BIKE_MAX              40
#define BIKE_MODEL            481         // BMX
#define BIKE_PRICE            500
#define BIKE_TIME             3600        // secondes (1 heure)
#define BIKE_FINE             750
#define BIKE_RANGE            1.8
#define BIKE_RETURN_RANGE     4.0
#define BIKE_MARK             19          // WorldMarkers : pictogramme velo

new Float:g_BikePos[BIKE_MAX][3], g_BikeId[BIKE_MAX], g_BikeCount;
new Text3D:g_BikeLabel[BIKE_MAX] = {Text3D:INVALID_3DTEXT_ID, ...};
new g_BikeVeh[MAX_PLAYERS], g_BikeEnd[MAX_PLAYERS], g_BikeStart[MAX_PLAYERS], g_BikeWarn[MAX_PLAYERS];
new g_PhoneHand[MAX_PLAYERS];
forward Bike_Tick();

// ------------------------------------------------------------
//  base / points
// ------------------------------------------------------------
stock Bike_Build(i)
{
	if (g_BikeLabel[i] != Text3D:INVALID_3DTEXT_ID) Delete3DTextLabel(g_BikeLabel[i]);
	g_BikeLabel[i] = Create3DTextLabel("Location de velo\n{9CC1FF}Approche-toi : BMX 500 $ / heure", 0xFFFFFFFF, g_BikePos[i][0], g_BikePos[i][1], g_BikePos[i][2] + 0.5, 4.0, 0, 1);   // [v30] visible seulement tout pres
	return 1;
}

stock Bike_BuildAll()
{
	for (new i = 0; i < BIKE_MAX; i++)
	{
		if (g_BikeLabel[i] != Text3D:INVALID_3DTEXT_ID) { Delete3DTextLabel(g_BikeLabel[i]); g_BikeLabel[i] = Text3D:INVALID_3DTEXT_ID; }
	}
	for (new i = 0; i < g_BikeCount; i++) Bike_Build(i);
	return 1;
}

stock Bike_Load()
{
	g_BikeCount = 0;
	if (g_C23DB == DB:0) return 0;
	C23_Exec("CREATE TABLE IF NOT EXISTS c25_bikes (id INTEGER PRIMARY KEY AUTOINCREMENT, x REAL, y REAL, z REAL)");
	new DBResult:r = db_query(g_C23DB, "SELECT id, x, y, z FROM c25_bikes ORDER BY id");
	if (r == DBResult:0) return 0;
	new f[24], n = db_num_rows(r);
	for (new k = 0; k < n && g_BikeCount < BIKE_MAX; k++)
	{
		new i = g_BikeCount++;
		db_get_field(r, 0, f, sizeof f); g_BikeId[i] = strval(f);
		db_get_field(r, 1, f, sizeof f); g_BikePos[i][0] = floatstr(f);
		db_get_field(r, 2, f, sizeof f); g_BikePos[i][1] = floatstr(f);
		db_get_field(r, 3, f, sizeof f); g_BikePos[i][2] = floatstr(f);
		db_next_row(r);
	}
	db_free_result(r);
	Bike_BuildAll();
	printf("[VELOS] %d point(s) de location.", g_BikeCount);
	return 1;
}

stock Bike_Near(playerid, Float:range = BIKE_RANGE)
{
	if (GetPlayerVirtualWorld(playerid) != 0 || GetPlayerInterior(playerid) != 0) return -1;
	for (new i = 0; i < g_BikeCount; i++) if (IsPlayerInRangeOfPoint(playerid, range, g_BikePos[i][0], g_BikePos[i][1], g_BikePos[i][2])) return i;
	return -1;
}

stock Bike_PushMarks()
{
	for (new i = 0; i < g_BikeCount; i++) Mk_Add(BIKE_MARK, g_BikePos[i][0], g_BikePos[i][1], g_BikePos[i][2] - 0.95);
	return 1;
}

stock Bike_InteractKey(playerid)
{
	if (!g_AuthLogged[playerid]) return 0;
	return (Bike_Near(playerid) >= 0) ? 200 : 0;
}

// ------------------------------------------------------------
//  page du HUD (module "bike")
// ------------------------------------------------------------
stock Bike_Send(playerid, ok, const msg[])
{
	new json[400], left = 0;
	if (g_BikeVeh[playerid]) left = g_BikeEnd[playerid] - gettime();
	if (left < 0) left = 0;
	format(json, sizeof json, "{\"m\":\"bike\",\"price\":%d,\"min\":%d,\"fine\":%d,\"renting\":%d,\"left\":%d,\"cash\":%d,\"bank\":%d,\"ok\":%d,\"msg\":\"%s\"}",
		BIKE_PRICE, BIKE_TIME / 60, BIKE_FINE, g_BikeVeh[playerid] ? 1 : 0, left, GetPlayerMoney(playerid), g_BankLoaded[playerid] ? g_BankBalance[playerid] : 0, ok, msg);
	return C23_Send(playerid, json);
}

stock Bike_Use(playerid)
{
	if (Bike_Near(playerid) < 0) return 0;
	return Bike_Send(playerid, 1, "");
}

stock Bike_Destroy(playerid)
{
	new v = g_BikeVeh[playerid];
	g_BikeVeh[playerid] = 0; g_BikeEnd[playerid] = 0; g_BikeWarn[playerid] = 0;
	if (v > 0 && GetVehicleModel(v) == BIKE_MODEL) DestroyVehicle(v);
	return 1;
}

stock Bike_Rent(playerid, bank)
{
	new i = Bike_Near(playerid, BIKE_RANGE + 1.0);
	if (i < 0) return Bike_Send(playerid, 0, "Approche-toi du point de location.");
	if (GetPlayerState(playerid) != PLAYER_STATE_ONFOOT) return Bike_Send(playerid, 0, "Descends de ton vehicule.");
	if (g_BikeVeh[playerid]) return Bike_Send(playerid, 0, "Tu loues deja un velo : rends-le d'abord.");
	if (!Svc_Pay(playerid, BIKE_PRICE, bank, "Location de velo"))
	{
		if (bank) return Bike_Send(playerid, 0, "Solde bancaire insuffisant.");
		return Bike_Send(playerid, 0, "Pas assez d'argent liquide (essaie par carte).");
	}
	new Float:a;
	GetPlayerFacingAngle(playerid, a);
	new v = CreateVehicle(BIKE_MODEL, g_BikePos[i][0] + 1.2 * floatsin(-a, degrees), g_BikePos[i][1] + 1.2 * floatcos(-a, degrees), g_BikePos[i][2], a, 3, 3, -1);
	if (v == INVALID_VEHICLE_ID || v <= 0)
	{
		if (bank) { g_BankBalance[playerid] += BIKE_PRICE; Bank_SavePlayer(playerid); } else GivePlayerMoney(playerid, BIKE_PRICE);
		return Bike_Send(playerid, 0, "Plus de velo disponible, reessaie plus tard.");
	}
	SetVehicleNumberPlate(v, "VELO");
	PutPlayerInVehicle(playerid, v, 0);
	g_BikeVeh[playerid] = v;
	g_BikeStart[playerid] = gettime();
	g_BikeEnd[playerid] = g_BikeStart[playerid] + BIKE_TIME;
	g_BikeWarn[playerid] = 0;
	C23_Send(playerid, "{\"m\":\"bike\",\"close\":1}");
	Cw_Notify(playerid, "success", "Location de velo", "BMX loue pour 1 heure. Rends-le a un point de location (marqueur velo) avant la fin pour eviter l'amende.", 7000);
	return 1;
}

stock Bike_Return(playerid)
{
	if (!g_BikeVeh[playerid]) return 0;
	Bike_Destroy(playerid);
	// pas de reouverture automatique de la page de location juste apres avoir rendu le velo
	g_InteractArmed[playerid] = 0;
	GetPlayerPos(playerid, g_InteractAutoPos[playerid][0], g_InteractAutoPos[playerid][1], g_InteractAutoPos[playerid][2]);
	Cw_Notify(playerid, "success", "Location de velo", "Velo rendu, merci ! A bientot.", 4000);
	return 1;
}

public Bike_Tick()
{
	new now = gettime();
	for (new p = 0, m = GetMaxPlayers(); p < m; p++)
	{
		if (!IsPlayerConnected(p) || !g_BikeVeh[p]) continue;
		new v = g_BikeVeh[p];
		if (GetVehicleModel(v) != BIKE_MODEL) { g_BikeVeh[p] = 0; continue; }   // velo detruit / respawn
		// rendu automatique : sur le velo, a un point de location (apres 15 s pour ne pas le rendre au depart)
		if (GetPlayerVehicleID(p) == v && now - g_BikeStart[p] > 15 && Bike_Near(p, BIKE_RETURN_RANGE) >= 0)
		{
			RemovePlayerFromVehicle(p);
			Bike_Return(p);
			continue;
		}
		new left = g_BikeEnd[p] - now;
		if (left <= 300 && !g_BikeWarn[p])
		{
			g_BikeWarn[p] = 1;
			Cw_Notify(p, "warning", "Location de velo", "Plus que 5 minutes : rends le velo a un point de location (marqueur velo).", 7000);
		}
		if (left <= 0)
		{
			new name[MAX_PLAYER_NAME + 1], q[220];
			GetPlayerName(p, name, sizeof name);
			format(q, sizeof q, "INSERT INTO c23_fines (name, amount, reason, author, ts) VALUES ('%s', %d, 'Velo de location pas rendu', 'Location', %d)", name, BIKE_FINE, now);
			C23_Exec(q);
			if (GetPlayerVehicleID(p) == v) RemovePlayerFromVehicle(p);
			Bike_Destroy(p);
			Cw_Notify(p, "denied", "Location de velo", "Temps ecoule : le velo est reparti et tu as une amende de 750 $ (/amendes).", 8000);
		}
	}
	return 1;
}

// ------------------------------------------------------------
//  telephone en main
// ------------------------------------------------------------
stock Phone_Hand(playerid, on)
{
	if (on)
	{
		if (GetPlayerState(playerid) != PLAYER_STATE_ONFOOT) return 1;
		new sa = GetPlayerSpecialAction(playerid);
		if (sa != SPECIAL_ACTION_NONE && sa != SPECIAL_ACTION_USECELLPHONE) return 1;   // menotte, porte un objet, accroupi...
		SetPlayerSpecialAction(playerid, SPECIAL_ACTION_USECELLPHONE);
		g_PhoneHand[playerid] = 1;
	}
	else
	{
		if (g_PhoneHand[playerid] && GetPlayerSpecialAction(playerid) == SPECIAL_ACTION_USECELLPHONE) SetPlayerSpecialAction(playerid, SPECIAL_ACTION_STOPUSECELLPHONE);
		g_PhoneHand[playerid] = 0;
	}
	return 1;
}

// ------------------------------------------------------------
//  panneau admin (onglet Factions > Locations de velo)
// ------------------------------------------------------------
stock Bike_AdminSend(playerid)
{
	new json[2600], part[96];
	format(json, sizeof json, "{\"x\":\"bike\",\"l\":[");
	for (new i = 0; i < g_BikeCount; i++)
	{
		format(part, sizeof part, "%s{\"i\":%d,\"x\":%d,\"y\":%d,\"z\":%d}", i ? (",") : (""), g_BikeId[i], floatround(g_BikePos[i][0]), floatround(g_BikePos[i][1]), floatround(g_BikePos[i][2]));
		if (strlen(json) + strlen(part) > sizeof(json) - 4) break;
		strcat(json, part, sizeof json);
	}
	strcat(json, "]}", sizeof json);
	return ShowPlayerDialog(playerid, ADMIN_DIALOG_UPDATE, DIALOG_STYLE_MSGBOX, "@admin", json, "OK", "");
}

stock Bike_AdminNeed(const action[])
{
	if (!strcmp(action, "bikeadd", true) || !strcmp(action, "bikedel", true)) return 4;
	if (!strcmp(action, "bikegoto", true) || !strcmp(action, "bikelist", true)) return 2;
	return 0;
}

stock Bike_Slot(dbid)
{
	for (new i = 0; i < g_BikeCount; i++) if (g_BikeId[i] == dbid) return i;
	return -1;
}

stock Bike_AdminAction(playerid, const action[], const tid[], msg[], msgsize, &ok)
{
	if (!Bike_AdminNeed(action)) return 0;
	if (!strcmp(action, "bikelist", true)) { ok = 1; format(msg, msgsize, "%d point(s) de location de velo.", g_BikeCount); Bike_AdminSend(playerid); return 1; }
	if (!strcmp(action, "bikeadd", true))
	{
		if (g_C23DB == DB:0) { format(msg, msgsize, "Base indisponible."); return 1; }
		if (GetPlayerState(playerid) != PLAYER_STATE_ONFOOT) { format(msg, msgsize, "Descends de ton vehicule pour placer le point."); return 1; }
		if (GetPlayerVirtualWorld(playerid) != 0 || GetPlayerInterior(playerid) != 0) { format(msg, msgsize, "Seulement dehors (monde 0, interieur 0)."); return 1; }
		if (g_BikeCount >= BIKE_MAX) { format(msg, msgsize, "Limite de %d points atteinte.", BIKE_MAX); return 1; }
		if (Bike_Near(playerid, 6.0) >= 0) { format(msg, msgsize, "Il y a deja un point de location ici."); return 1; }
		new Float:x, Float:y, Float:z, q[160];
		GetPlayerPos(playerid, x, y, z);
		format(q, sizeof q, "INSERT INTO c25_bikes (x, y, z) VALUES (%f, %f, %f)", x, y, z);
		C23_Exec(q);
		new DBResult:r = db_query(g_C23DB, "SELECT last_insert_rowid()"), f[12], id = 0;
		if (r != DBResult:0) { db_get_field(r, 0, f, sizeof f); id = strval(f); db_free_result(r); }
		new i = g_BikeCount++;
		g_BikeId[i] = id; g_BikePos[i][0] = x; g_BikePos[i][1] = y; g_BikePos[i][2] = z;
		Bike_Build(i);
		Marks_SendAll();
		format(msg, msgsize, "Location de velo #%d creee a ta position.", id);
		ok = 1;
		Bike_AdminSend(playerid);
		return 1;
	}
	new id = strval(tid), i = Bike_Slot(id);
	if (i == -1) { format(msg, msgsize, "Point de location introuvable."); return 1; }
	if (!strcmp(action, "bikegoto", true))
	{
		SetPlayerInterior(playerid, 0);
		SetPlayerVirtualWorld(playerid, 0);
		SetPlayerPos(playerid, g_BikePos[i][0] + 2.5, g_BikePos[i][1], g_BikePos[i][2]);
		format(msg, msgsize, "Teleporte a la location de velo #%d.", id);
		ok = 1;
		return 1;
	}
	// bikedel
	new q[96];
	format(q, sizeof q, "DELETE FROM c25_bikes WHERE id = %d", id);
	C23_Exec(q);
	for (new k = i; k < g_BikeCount - 1; k++)
	{
		g_BikeId[k] = g_BikeId[k + 1];
		g_BikePos[k][0] = g_BikePos[k + 1][0]; g_BikePos[k][1] = g_BikePos[k + 1][1]; g_BikePos[k][2] = g_BikePos[k + 1][2];
	}
	g_BikeCount--;
	Bike_BuildAll();
	Marks_SendAll();
	format(msg, msgsize, "Location de velo #%d supprimee.", id);
	ok = 1;
	Bike_AdminSend(playerid);
	return 1;
}

// ------------------------------------------------------------
//  [v25] textes 3D des pompes et des locations de vehicules, meme style que le magasin de vetements
// ------------------------------------------------------------
new Text3D:g_SvcPumpLabel[SVC_MAX_PUMPS] = {Text3D:INVALID_3DTEXT_ID, ...};
new Text3D:g_SvcRentLabel[SVC_MAX_RENT] = {Text3D:INVALID_3DTEXT_ID, ...};

stock Svc_BuildLabels()
{
	for (new i = 0; i < SVC_MAX_PUMPS; i++)
	{
		if (g_SvcPumpLabel[i] != Text3D:INVALID_3DTEXT_ID) { Delete3DTextLabel(g_SvcPumpLabel[i]); g_SvcPumpLabel[i] = Text3D:INVALID_3DTEXT_ID; }
		if (i < g_SvcPumpCount) g_SvcPumpLabel[i] = Create3DTextLabel("Station-service\n{9CC1FF}Approche-toi et touche la main", 0xFFFFFFFF, g_SvcPumps[i][0], g_SvcPumps[i][1], g_SvcPumps[i][2] + 0.4, 4.0, 0, 1);   // [v30] visible seulement tout pres
	}
	for (new i = 0; i < SVC_MAX_RENT; i++)
	{
		if (g_SvcRentLabel[i] != Text3D:INVALID_3DTEXT_ID) { Delete3DTextLabel(g_SvcRentLabel[i]); g_SvcRentLabel[i] = Text3D:INVALID_3DTEXT_ID; }
		if (i < g_SvcRentCount) g_SvcRentLabel[i] = Create3DTextLabel("Location de vehicules\n{9CC1FF}Approche-toi et touche la main", 0xFFFFFFFF, g_SvcRentPts[i][0], g_SvcRentPts[i][1], g_SvcRentPts[i][2] + 0.4, 4.0, 0, 1);   // [v30] visible seulement tout pres
	}
	return 1;
}

// ------------------------------------------------------------
//  hooks
// ------------------------------------------------------------
stock Cw25_OnInit()
{
	Bike_Load();
	Svc_BuildLabels();
	SetTimer("Bike_Tick", 1000, true);
	return 1;
}

stock Cw25_OnConnect(playerid)
{
	g_BikeVeh[playerid] = 0; g_BikeEnd[playerid] = 0; g_BikeStart[playerid] = 0; g_BikeWarn[playerid] = 0;
	g_PhoneHand[playerid] = 0;
	return 1;
}

stock Cw25_OnDisconnect(playerid)
{
	Bike_Destroy(playerid);
	g_PhoneHand[playerid] = 0;
	return 1;
}

stock Cw25_OnCommand(playerid, const cmdtext[])
{
	new a;
	if ((a = Cw_Arg(cmdtext, "/cwdev")) != -1) return Ms_SetDevice(playerid, strval(cmdtext[a])), 1;   // RAM du telephone (appli), aussi avant la connexion
	if (!g_AuthLogged[playerid]) return 0;
	if ((a = Cw_Arg(cmdtext, "/telmain")) != -1) return Phone_Hand(playerid, cmdtext[a] == '0' ? 0 : 1), 1;
	if (!strcmp(cmdtext, "/rendrevelo", true))
	{
		if (!g_BikeVeh[playerid]) return Cw_Notify(playerid, "info", "Location de velo", "Tu ne loues pas de velo.", 3000), 1;
		if (Bike_Near(playerid, 8.0) < 0) return Cw_Notify(playerid, "warning", "Location de velo", "Va a un point de location (marqueur velo) pour rendre le velo.", 4000), 1;
		new Float:x, Float:y, Float:z;
		GetVehiclePos(g_BikeVeh[playerid], x, y, z);
		if (!IsPlayerInRangeOfPoint(playerid, 15.0, x, y, z)) return Cw_Notify(playerid, "warning", "Location de velo", "Ramene le velo avec toi (moins de 15 m).", 4000), 1;
		if (GetPlayerVehicleID(playerid) == g_BikeVeh[playerid]) RemovePlayerFromVehicle(playerid);
		return Bike_Return(playerid), 1;
	}
	if ((a = Cw_Arg(cmdtext, "/c25")) != -1)
	{
		new idx = a, mod[8], act[10], arg[8];
		Bank_NextToken(cmdtext, idx, mod, sizeof mod);
		Bank_NextToken(cmdtext, idx, act, sizeof act);
		Bank_NextToken(cmdtext, idx, arg, sizeof arg);
		if (!strcmp(mod, "bike", true))
		{
			if (!strcmp(act, "rent", true)) return Bike_Rent(playerid, !strcmp(arg, "bank", true) ? 1 : 0), 1;
			if (!strcmp(act, "return", true)) return Cw25_OnCommand(playerid, "/rendrevelo"), 1;
			if (!strcmp(act, "refresh", true)) return Bike_Send(playerid, 1, ""), 1;
			return 1;   // close
		}
		return 1;
	}
	return 0;
}
