/* ============================================================
   CRESTWOOD RP v45 (21/11/2026) - ECRAN DE CONNEXION AVEC LE PERSONNAGE
   ------------------------------------------------------------
   Pendant la connexion (selection de classe), le joueur voit SON personnage debout (corps South,
   respiration / petits mouvements du jeu) sur une terrasse au-dessus de la mer, au coucher du soleil,
   avec une voiture de sport et deux lampadaires en decor. La page de connexion est a droite,
   transparente a gauche pour laisser voir la scene. Monde prive par joueur.
   Le client (localplayer.cpp 32/64) met le corps South (modele 7) pendant la selection de classe et
   cache les boutons "<< Spawn >>" (la page de connexion fait apparaitre le joueur).
   Texte ASCII.
   ============================================================ */
#define LG_X                 (-2850.0)
#define LG_Y                 (2850.0)
#define LG_Z                 (6.0)
#define LG_OBJS              5

new g_LgOn[MAX_PLAYERS];
new g_LgObj[MAX_PLAYERS][LG_OBJS];
new g_LgVeh[MAX_PLAYERS];

stock Lg_Begin(playerid)
{
	if (!IsPlayerConnected(playerid) || IsPlayerNPC(playerid) || (g_AuthLogged[playerid] && !g_LgLobby[playerid])) return 0;
	SetPlayerVirtualWorld(playerid, 7000 + playerid);
	SetPlayerInterior(playerid, 0);
	if (!g_LgOn[playerid])
	{
		g_LgOn[playerid] = 1;
		// terrasse (bloc 1 x 25 x 25 couche) en parquet + deux lampadaires
		g_LgObj[playerid][0] = CreatePlayerObject(playerid, 18981, LG_X, LG_Y, LG_Z - 0.5, 0.0, 90.0, 0.0, 300.0);
		SetPlayerObjectMaterial(playerid, g_LgObj[playerid][0], 0, 12954, "sw_furniture", "CJ_WOOD5", 0);
		g_LgObj[playerid][1] = CreatePlayerObject(playerid, 1226, LG_X - 3.0, LG_Y - 3.5, LG_Z + 3.3, 0.0, 0.0, 90.0, 300.0);
		g_LgObj[playerid][2] = CreatePlayerObject(playerid, 1226, LG_X - 3.0, LG_Y + 5.5, LG_Z + 3.3, 0.0, 0.0, 90.0, 300.0);
		g_LgObj[playerid][3] = 0;
		g_LgObj[playerid][4] = 0;
		// voiture de sport garee derriere, de travers (decor)
		static const cars[] = {411, 541, 451, 560, 562, 559};
		new v = CreateVehicle(cars[random(sizeof cars)], LG_X - 5.5, LG_Y + 3.0, LG_Z + 0.6, 205.0, 1, 1, -1);
		if (v != INVALID_VEHICLE_ID)
		{
			SetVehicleVirtualWorld(v, 7000 + playerid);
			new engine, lights, alarm, doors, bonnet, boot, objective;
			GetVehicleParamsEx(v, engine, lights, alarm, doors, bonnet, boot, objective);
			SetVehicleParamsEx(v, VEHICLE_PARAMS_OFF, VEHICLE_PARAMS_ON, VEHICLE_PARAMS_OFF, VEHICLE_PARAMS_ON, VEHICLE_PARAMS_OFF, VEHICLE_PARAMS_OFF, VEHICLE_PARAMS_OFF);
			g_LgVeh[playerid] = v;
		}
	}
	SetPlayerTime(playerid, 19, 10);          // coucher du soleil
	SetPlayerWeather(playerid, 1);
	SetPlayerPos(playerid, LG_X, LG_Y, LG_Z + 1.0);
	SetPlayerFacingAngle(playerid, 270.0);    // face a la camera (est)
	// camera a l'est qui regarde l'ouest (le soleil couchant derriere le personnage) ; le point vise est
	// decale vers le nord pour mettre le personnage dans le tiers gauche de l'ecran (la page est a droite)
	SetPlayerCameraPos(playerid, LG_X + 4.2, LG_Y - 0.9, LG_Z + 1.75);
	SetPlayerCameraLookAt(playerid, LG_X, LG_Y + 1.55, LG_Z + 1.05);
	// son apparence enregistree (si le compte existe deja)
	SetTimerEx("Lg_ApplyLook", 1500, false, "i", playerid);
	return 1;
}

forward Lg_ApplyLook(playerid);
public Lg_ApplyLook(playerid)
{
	if (!IsPlayerConnected(playerid) || !g_LgOn[playerid] || (g_AuthLogged[playerid] && !g_LgLobby[playerid])) return 0;
	if (!g_ClothLoaded[playerid]) Cloth_Load(playerid);
	Cloth_Send(playerid, CLOTH_DIALOG_APPLY);
	return 1;
}

stock Lg_End(playerid)
{
	if (!g_LgOn[playerid]) return 0;
	g_LgOn[playerid] = 0;
	for (new k = 0; k < LG_OBJS; k++)
	{
		if (g_LgObj[playerid][k] && g_LgObj[playerid][k] != INVALID_OBJECT_ID && IsPlayerConnected(playerid)) DestroyPlayerObject(playerid, g_LgObj[playerid][k]);
		g_LgObj[playerid][k] = 0;
	}
	if (g_LgVeh[playerid]) { DestroyVehicle(g_LgVeh[playerid]); g_LgVeh[playerid] = 0; }
	if (IsPlayerConnected(playerid))
	{
		if (GetPlayerVirtualWorld(playerid) == 7000 + playerid) SetPlayerVirtualWorld(playerid, 0);
		SetPlayerWeather(playerid, g_WxNow);
		if (g_CwRealTime) { new h, mi, s; gettime(h, mi, s); SetPlayerTime(playerid, h, mi); }
		else SetPlayerTime(playerid, 12, 0);
	}
	return 1;
}

stock Cw45_OnConnect(playerid)
{
	g_LgOn[playerid] = 0;
	g_LgLobby[playerid] = 0;
	g_LgVeh[playerid] = 0;
	for (new k = 0; k < LG_OBJS; k++) g_LgObj[playerid][k] = 0;
	return 1;
}

stock Cw45_OnDisconnect(playerid)
{
	if (g_LgVeh[playerid]) { DestroyVehicle(g_LgVeh[playerid]); g_LgVeh[playerid] = 0; }
	g_LgOn[playerid] = 0;
	return 1;
}
