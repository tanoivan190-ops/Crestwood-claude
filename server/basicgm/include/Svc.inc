/* ============================================================
   SERVICES (page assets/svc) - Crestwood RP
   ------------------------------------------------------------
   1) STATION-SERVICE : les vehicules consomment du carburant (plus on roule vite, plus ca consomme ;
      reservoir vide = moteur coupe). A la pompe (icone 3D + main), on choisit le plein ou un niveau,
      on paie en liquide ou par carte. Velos : pas de carburant.
   2) LOCATION DE VEHICULES : points de location (icone 3D cle), scooter / velo / voiture / moto pour
      30 minutes, paye en liquide ou par carte. Le vehicule disparait a la fin ou a la deconnexion.
   serveur -> page : ShowPlayerDialog(9810 ouvrir / 9811 mise a jour / 9812 fermer, "@svc", JSON)
   page -> serveur : /svcact <fill PCT cash|bank> | <rent MODELE cash|bank> | <rentend> | <close>
   Interaction : cles 150 (pompe) et 160 (location) -> Interact_Do groupe 15
   ============================================================ */
#define SVC_DIALOG_OPEN       9810
#define SVC_DIALOG_UPDATE     9811
#define SVC_DIALOG_CLOSE      9812
#define SVC_RANGE             2.2
#define SVC_FUEL_PRICE        6           // FCFA par % de reservoir (plein complet = 600 FCFA)
#define SVC_FUEL_LOW          15.0
#define SVC_RENT_MIN          30
#define SVC_COL_OK            0x33CC33FF
#define SVC_COL_ERR           0xFF6347FF
#define SVC_COL_INFO          0xFFB454FF

// [v13] pompes et points de location poses en jeu par les admins (panneau /points), plus aucun point fixe
#define SVC_MAX_PUMPS         60
#define SVC_MAX_RENT          20
new Float:g_SvcPumps[SVC_MAX_PUMPS][3];
new g_SvcPumpCount;
new Float:g_SvcRentPts[SVC_MAX_RENT][3];
new g_SvcRentCount;
// modele, prix (30 min)
new const g_SvcRent[][2] = { {462, 150}, {481, 50}, {510, 80}, {496, 600}, {461, 900} };
new const g_SvcRentName[][] = { "Faggio (scooter)", "BMX", "VTT", "Blista Compact", "PCJ-600" };

new g_SvcOpen[MAX_PLAYERS];
new g_SvcView[MAX_PLAYERS];          // 1 pompe, 2 location
new g_SvcVeh[MAX_PLAYERS];           // vehicule a remplir
new g_SvcRentVeh[MAX_PLAYERS];
new g_SvcRentUntil[MAX_PLAYERS];
forward Svc_Tick();

stock Svc_IsBike(model) { return (model == 481 || model == 509 || model == 510); }

stock Svc_OnInit()
{
	SetTimer("Svc_Tick", 1000, true);
	return 1;
}

stock Svc_OnConnect(playerid)
{
	g_SvcOpen[playerid] = 0; g_SvcView[playerid] = 0; g_SvcVeh[playerid] = 0;
	g_SvcRentVeh[playerid] = 0; g_SvcRentUntil[playerid] = 0;
	return 1;
}

stock Svc_EndRent(playerid, const why[])
{
	new v = g_SvcRentVeh[playerid];
	g_SvcRentVeh[playerid] = 0;
	g_SvcRentUntil[playerid] = 0;
	if (v)
	{
		for (new p = 0, m = GetMaxPlayers(); p < m; p++) if (IsPlayerConnected(p) && GetPlayerVehicleID(p) == v) RemovePlayerFromVehicle(p);
		DestroyVehicle(v);
	}
	if (why[0] && IsPlayerConnected(playerid)) SendClientMessage(playerid, SVC_COL_INFO, why);
	return 1;
}

stock Svc_OnDisconnect(playerid)
{
	if (g_SvcRentVeh[playerid]) Svc_EndRent(playerid, "");
	return 1;
}

// consommation + fin de location
public Svc_Tick()
{
	new now = gettime();
	for (new p = 0, m = GetMaxPlayers(); p < m; p++)
	{
		if (!IsPlayerConnected(p)) continue;
		if (g_SvcRentVeh[p] && now >= g_SvcRentUntil[p]) Svc_EndRent(p, "Location terminee : le vehicule a ete rendu.");
		if (GetPlayerState(p) != PLAYER_STATE_DRIVER) continue;
		new v = GetPlayerVehicleID(p);
		if (!v || Svc_IsBike(GetVehicleModel(v))) continue;
		new engine, lights, alarm, doors, bonnet, boot, objective;
		GetVehicleParamsEx(v, engine, lights, alarm, doors, bonnet, boot, objective);
		if (engine != VEHICLE_PARAMS_ON) continue;
		if (g_CwFuel[v] <= 0.0)
		{
			SetVehicleParamsEx(v, VEHICLE_PARAMS_OFF, lights, alarm, doors, bonnet, boot, objective);
			GameTextForPlayer(p, "~r~Reservoir vide", 2500, 4);
			SendClientMessage(p, SVC_COL_ERR, "Plus de carburant : pousse le vehicule jusqu'a une station-service.");
			continue;
		}
		new Float:vx, Float:vy, Float:vz;
		GetVehicleVelocity(v, vx, vy, vz);
		new Float:kmh = floatsqroot(vx * vx + vy * vy + vz * vz) * 180.0;
		new Float:before = g_CwFuel[v];
		g_CwFuel[v] -= 0.004 + kmh * 0.00055;
		if (g_CwFuel[v] < 0.0) g_CwFuel[v] = 0.0;
		if (before >= SVC_FUEL_LOW && g_CwFuel[v] < SVC_FUEL_LOW) SendClientMessage(p, SVC_COL_INFO, "Reserve de carburant : passe a une station-service (icone pompe).");
	}
	return 1;
}

stock Svc_NearPump(playerid)
{
	for (new i = 0; i < g_SvcPumpCount; i++) if (IsPlayerInRangeOfPoint(playerid, SVC_RANGE + 1.0, g_SvcPumps[i][0], g_SvcPumps[i][1], g_SvcPumps[i][2])) return i;
	return -1;
}

stock Svc_NearRent(playerid)
{
	for (new i = 0; i < g_SvcRentCount; i++) if (IsPlayerInRangeOfPoint(playerid, SVC_RANGE, g_SvcRentPts[i][0], g_SvcRentPts[i][1], g_SvcRentPts[i][2])) return i;
	return -1;
}

stock Svc_InteractKey(playerid)
{
	if (g_SvcOpen[playerid] || GetPlayerVirtualWorld(playerid) != 0) return 0;
	if (Svc_NearPump(playerid) != -1) return 150;
	if (Svc_NearRent(playerid) != -1) return 160;
	return 0;
}

// vehicule a remplir : celui que le joueur conduisait en dernier, a moins de 10 m
stock Svc_PumpVehicle(playerid)
{
	new v = GetPlayerVehicleID(playerid);
	if (!v) v = GetPlayerLastVehicle(playerid);   // (voir plus bas)
	if (!v) return 0;
	new Float:x, Float:y, Float:z;
	GetVehiclePos(v, x, y, z);
	if (!IsPlayerInRangeOfPoint(playerid, 10.0, x, y, z)) return 0;
	return v;
}

stock Svc_Pay(playerid, amount, bank, const why[])
{
	if (amount <= 0) return 1;
	if (bank)
	{
		if (!g_BankLoaded[playerid] || g_BankBalance[playerid] < amount) return 0;
		g_BankBalance[playerid] -= amount;
		Bank_AddHistory(playerid, why, -amount);
		Bank_SavePlayer(playerid);
		return 1;
	}
	if (GetPlayerMoney(playerid) < amount) return 0;
	GivePlayerMoney(playerid, -amount);
	return 1;
}

stock Svc_Send(playerid, dialogid, ok, const msg[])
{
	new json[1200], part[120], safe[100];
	Admin_Clean(msg, safe, sizeof safe);
	if (g_SvcView[playerid] == 1)
	{
		new v = g_SvcVeh[playerid], vname[32];
		if (v) Gar_VehName(GetVehicleModel(v), vname, sizeof vname); else vname[0] = 0;
		format(json, sizeof json, "{\"view\":\"fuel\",\"vid\":%d,\"veh\":\"%s\",\"fuel\":%.1f,\"price\":%d,\"bike\":%d",
			v, vname, v ? g_CwFuel[v] : 0.0, SVC_FUEL_PRICE, v ? Svc_IsBike(GetVehicleModel(v)) : 0);
	}
	else
	{
		format(json, sizeof json, "{\"view\":\"rent\",\"min\":%d,\"list\":[", SVC_RENT_MIN);
		for (new i = 0; i < sizeof(g_SvcRent); i++)
		{
			format(part, sizeof part, "%s[%d,\"%s\",%d]", i ? (",") : (""), g_SvcRent[i][0], g_SvcRentName[i], g_SvcRent[i][1]);
			strcat(json, part, sizeof json);
		}
		format(part, sizeof part, "],\"active\":%d,\"left\":%d", g_SvcRentVeh[playerid] ? GetVehicleModel(g_SvcRentVeh[playerid]) : 0,
			g_SvcRentVeh[playerid] ? g_SvcRentUntil[playerid] - gettime() : 0);
		strcat(json, part, sizeof json);
	}
	format(part, sizeof part, ",\"cash\":%d,\"bank\":%d,\"card\":%d,\"ok\":%d,\"msg\":\"%s\"}", GetPlayerMoney(playerid), g_BankBalance[playerid], g_BankLoaded[playerid], ok, safe);
	strcat(json, part, sizeof json);
	return ShowPlayerDialog(playerid, dialogid, DIALOG_STYLE_MSGBOX, "@svc", json, "OK", "");
}

stock Svc_InteractDo(playerid)
{
	new k = Svc_InteractKey(playerid);
	if (k == 150)
	{
		g_SvcView[playerid] = 1;
		g_SvcVeh[playerid] = Svc_PumpVehicle(playerid);
		g_SvcOpen[playerid] = 1;
		return Svc_Send(playerid, SVC_DIALOG_OPEN, 1, g_SvcVeh[playerid] ? ("") : ("Gare ton vehicule a cote de la pompe."));
	}
	if (k == 160)
	{
		g_SvcView[playerid] = 2;
		g_SvcOpen[playerid] = 1;
		return Svc_Send(playerid, SVC_DIALOG_OPEN, 1, "");
	}
	return 0;
}

stock Svc_Action(playerid, const params[])
{
	new idx = 0, action[10], a1[8], a2[8];
	Bank_NextToken(params, idx, action, sizeof action);
	Bank_NextToken(params, idx, a1, sizeof a1);
	Bank_NextToken(params, idx, a2, sizeof a2);
	if (!strcmp(action, "close", true)) { g_SvcOpen[playerid] = 0; return 1; }
	if (!g_SvcOpen[playerid]) return 1;
	new bank = !strcmp(a2, "bank", true), msg[110];
	if (!strcmp(action, "fill", true))
	{
		if (Svc_NearPump(playerid) == -1) return Svc_Send(playerid, SVC_DIALOG_UPDATE, 0, "Tu t'es eloigne de la pompe.");
		new v = Svc_PumpVehicle(playerid);
		g_SvcVeh[playerid] = v;
		if (!v) return Svc_Send(playerid, SVC_DIALOG_UPDATE, 0, "Aucun vehicule a remplir a cote de toi.");
		if (Svc_IsBike(GetVehicleModel(v))) return Svc_Send(playerid, SVC_DIALOG_UPDATE, 0, "Un velo n'a pas besoin d'essence !");
		new target = strval(a1);
		if (target < 1 || target > 100) target = 100;
		new Float:add = float(target) - g_CwFuel[v];
		if (add <= 0.5) return Svc_Send(playerid, SVC_DIALOG_UPDATE, 0, "Le reservoir est deja a ce niveau.");
		new cost = floatround(add * SVC_FUEL_PRICE, floatround_ceil);
		if (!Svc_Pay(playerid, cost, bank, "Station-service")) return Svc_Send(playerid, SVC_DIALOG_UPDATE, 0, bank ? ("Solde bancaire insuffisant.") : ("Pas assez d'argent liquide."));
		g_CwFuel[v] = float(target);
		format(msg, sizeof msg, "Plein fait : reservoir a %d %% (%d FCFA).", target, cost);
		return Svc_Send(playerid, SVC_DIALOG_UPDATE, 1, msg);
	}
	if (!strcmp(action, "rent", true))
	{
		new pt = Svc_NearRent(playerid);
		if (pt == -1) return Svc_Send(playerid, SVC_DIALOG_UPDATE, 0, "Tu t'es eloigne du point de location.");
		if (g_SvcRentVeh[playerid]) return Svc_Send(playerid, SVC_DIALOG_UPDATE, 0, "Tu as deja une location en cours.");
		new model = strval(a1), k = -1;
		for (new i = 0; i < sizeof(g_SvcRent); i++) if (g_SvcRent[i][0] == model) k = i;
		if (k == -1) return Svc_Send(playerid, SVC_DIALOG_UPDATE, 0, "Vehicule indisponible.");
		if (!Svc_Pay(playerid, g_SvcRent[k][1], bank, "Location de vehicule")) return Svc_Send(playerid, SVC_DIALOG_UPDATE, 0, bank ? ("Solde bancaire insuffisant.") : ("Pas assez d'argent liquide."));
		new Float:a;
		GetPlayerFacingAngle(playerid, a);
		new v = CreateVehicle(model, g_SvcRentPts[pt][0] + 3.0 * floatsin(-a, degrees), g_SvcRentPts[pt][1] + 3.0 * floatcos(-a, degrees), g_SvcRentPts[pt][2] + 0.5, a, -1, -1, -1);
		if (v == INVALID_VEHICLE_ID) return Svc_Send(playerid, SVC_DIALOG_UPDATE, 0, "Location impossible pour le moment.");
		SetVehicleParamsEx(v, VEHICLE_PARAMS_ON, VEHICLE_PARAMS_OFF, VEHICLE_PARAMS_OFF, VEHICLE_PARAMS_OFF, VEHICLE_PARAMS_OFF, VEHICLE_PARAMS_OFF, VEHICLE_PARAMS_OFF);
		g_SvcRentVeh[playerid] = v;
		g_SvcRentUntil[playerid] = gettime() + SVC_RENT_MIN * 60;
		PutPlayerInVehicle(playerid, v, 0);
		format(msg, sizeof msg, "%s loue pour %d minutes.", g_SvcRentName[k], SVC_RENT_MIN);
		SendClientMessage(playerid, SVC_COL_OK, msg);
		g_SvcOpen[playerid] = 0;
		return ShowPlayerDialog(playerid, SVC_DIALOG_CLOSE, DIALOG_STYLE_MSGBOX, "@svc", "{}", "OK", "");
	}
	if (!strcmp(action, "rentend", true))
	{
		if (!g_SvcRentVeh[playerid]) return Svc_Send(playerid, SVC_DIALOG_UPDATE, 0, "Aucune location en cours.");
		Svc_EndRent(playerid, "");
		return Svc_Send(playerid, SVC_DIALOG_UPDATE, 1, "Location rendue.");
	}
	return 1;
}

stock Svc_OnCommand(playerid, const cmdtext[])
{
	if (!strcmp(cmdtext, "/svcact ", true, 8)) { Svc_Action(playerid, cmdtext[8]); return 1; }
	if (!strcmp(cmdtext, "/essence", true))
	{
		new v = GetPlayerVehicleID(playerid), m[80];
		if (!v) return SendClientMessage(playerid, SVC_COL_INFO, "Monte dans un vehicule pour voir son carburant."), 1;
		format(m, sizeof m, "Carburant : %.0f %%", g_CwFuel[v]);
		return SendClientMessage(playerid, SVC_COL_INFO, m), 1;
	}
	if (!strcmp(cmdtext, "/rendrelocation", true))
	{
		if (!g_SvcRentVeh[playerid]) return SendClientMessage(playerid, SVC_COL_ERR, "Aucune location en cours."), 1;
		return Svc_EndRent(playerid, "Location rendue."), 1;
	}
	return 0;
}

stock Svc_PushMarks()
{
	for (new i = 0; i < g_SvcPumpCount; i++) Mk_Add(16, g_SvcPumps[i][0], g_SvcPumps[i][1], g_SvcPumps[i][2] - 0.95);
	for (new i = 0; i < g_SvcRentCount; i++) Mk_Add(17, g_SvcRentPts[i][0], g_SvcRentPts[i][1], g_SvcRentPts[i][2] - 0.95);
	return 1;
}
