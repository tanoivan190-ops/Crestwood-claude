/* ============================================================
   CRESTWOOD RP v41b (21/11/2026)
   ------------------------------------------------------------
   1) STUDIO DE CREATION DU PERSONNAGE : a la 1re connexion (apres l'inscription), le joueur est
      place seul (monde prive) devant un MUR choisi au hasard (brique, metal, beton, tole, bois,
      marbre, studio blanc...), sol + murs lateraux, en plein jour. Il ne regle que son CORPS
      (teint, visage, cheveux, sourcils, barbe, maquillage) : ni tatouages ni vetements.
      Il porte deja 2 vetements de depart (un haut + un bas, selon homme / femme) qu'il garde ;
      il pourra se changer plus tard dans un magasin de vetements. ENREGISTRER -> retour en ville.
      /personnage : rouvre l'editeur si besoin pendant la creation.
   2) ROUE / MENU D'INTERACTION : beaucoup plus d'actions DIRECTES sur un joueur
      - a deux (la personne accepte en touchant celui qui propose) : bisou, danser ensemble,
        bras de fer, partie de des (100 $), demander ses papiers
      - observer, faire les poches (crime), braquer (arme en main, crime)
      - EMS : reanimer, soigner (500 $) ; police : menotter / demenotter, escorter, fouiller
      vehicule : remettre sur ses roues, crocheter (crime), reparer (mecanicien, le proprietaire accepte)
   Texte ASCII.
   ============================================================ */
new g_FpOpen[MAX_PLAYERS];       // panneau de faction ouvert
new g_FpLast[MAX_PLAYERS];
new g_SlashOpen;                 // 1 = commandes tapees ouvertes a tous
#define CREA_X               (-2900.0)
#define CREA_Y               (2900.0)
#define CREA_Z               (320.0)
#define CREA_WALLS           10

new const g_CreaWallModel[CREA_WALLS] = {17508, 16640, 4828, 16093, 1259, 19853, 10765, 12954, 3922, 7978};
new const g_CreaWallTxd[CREA_WALLS][20] = {"barrio1_lae2", "a51", "airport3_las", "a51_ext", "billbrd", "mihouse1", "airportgnd_sfse", "sw_furniture", "bistro", "vgssairport"};
new const g_CreaWallTex[CREA_WALLS][24] = {"brickred", "ws_metalpanel1", "gnhotelwall02_128", "corugwall_sandy", "ws_oldpainted2", "bluewall1", "white", "CJ_WOOD5", "marblekb_256128", "newall_harling_sless"};
new const g_CreaWallName[CREA_WALLS][20] = {"brique rouge", "metal", "beton", "tole", "vieille peinture", "bleu", "studio blanc", "bois", "marbre", "crepi"};

new g_CreaOn[MAX_PLAYERS];
new g_CreaObj[MAX_PLAYERS][4];
new Float:g_CreaBack[MAX_PLAYERS][4];
new g_CreaBackInt[MAX_PLAYERS], g_CreaBackVW[MAX_PLAYERS];

// vetements de depart (valeur = rang dans le catalogue South + 1) : homme corps 7, femme corps 12
stock Crea_Starter(skin, &top, &bottom)
{
	if (skin == CLOTH_SKIN_FEMALE || skin == 9) { top = 2; bottom = 3; }    // tshirts_15 + jeans_1
	else { top = 5; bottom = 4; }                                            // tshirts_20 + sweatpants_17
	return 1;
}

stock Crea_Begin(playerid)
{
	if (!IsPlayerConnected(playerid)) return 0;
	if (!g_CreaOn[playerid])
	{
		GetPlayerPos(playerid, g_CreaBack[playerid][0], g_CreaBack[playerid][1], g_CreaBack[playerid][2]);
		GetPlayerFacingAngle(playerid, g_CreaBack[playerid][3]);
		g_CreaBackInt[playerid] = GetPlayerInterior(playerid);
		g_CreaBackVW[playerid] = GetPlayerVirtualWorld(playerid);
		g_CreaOn[playerid] = 1;
		new w = random(CREA_WALLS), fl = random(2);
		// 18981 = bloc de 1 x 25 x 25 m (fin sur X) : debout = mur, couche (ry 90) = sol
		g_CreaObj[playerid][0] = CreatePlayerObject(playerid, 18981, CREA_X, CREA_Y, CREA_Z - 0.5, 0.0, 90.0, 0.0, 300.0);
		g_CreaObj[playerid][1] = CreatePlayerObject(playerid, 18981, CREA_X - 5.0, CREA_Y, CREA_Z + 11.5, 0.0, 0.0, 0.0, 300.0);
		g_CreaObj[playerid][2] = CreatePlayerObject(playerid, 18981, CREA_X, CREA_Y - 7.0, CREA_Z + 11.5, 0.0, 0.0, 90.0, 300.0);
		g_CreaObj[playerid][3] = CreatePlayerObject(playerid, 18981, CREA_X, CREA_Y + 7.0, CREA_Z + 11.5, 0.0, 0.0, 90.0, 300.0);
		if (fl) SetPlayerObjectMaterial(playerid, g_CreaObj[playerid][0], 0, 14789, "ab_sfgymmain", "gym_floor6", 0);
		else SetPlayerObjectMaterial(playerid, g_CreaObj[playerid][0], 0, 2755, "ab_dojowall", "mp_apt1_roomfloor", 0);
		for (new k = 1; k < 4; k++) SetPlayerObjectMaterial(playerid, g_CreaObj[playerid][k], 0, g_CreaWallModel[w], g_CreaWallTxd[w], g_CreaWallTex[w], 0);
		SetPlayerVirtualWorld(playerid, 9000 + playerid);
		SetPlayerInterior(playerid, 0);
		TogglePlayerControllable(playerid, 0);
		SetPlayerPos(playerid, CREA_X, CREA_Y, CREA_Z + 1.0);
		SetPlayerFacingAngle(playerid, 270.0);   // dos au mur, face a la camera
		SetCameraBehindPlayer(playerid);
		SetPlayerTime(playerid, 12, 0);
		SetPlayerWeather(playerid, 1);
		new m[128];
		format(m, sizeof m, "Studio de creation (mur : %s). Regle ton corps puis ENREGISTRER. /personnage si l'editeur se ferme.", g_CreaWallName[w]);
		SendClientMessage(playerid, CLOTH_COL_INFO, m);
	}
	// 2 vetements de depart deja portes
	new top, bottom;
	Crea_Starter(g_ClothSkin[playerid], top, bottom);
	g_ClothV[playerid][39] = top;
	g_ClothV[playerid][40] = bottom;
	return 1;
}

stock Crea_End(playerid)
{
	if (!g_CreaOn[playerid]) return 0;
	g_CreaOn[playerid] = 0;
	for (new k = 0; k < 4; k++) if (g_CreaObj[playerid][k] && g_CreaObj[playerid][k] != INVALID_OBJECT_ID) DestroyPlayerObject(playerid, g_CreaObj[playerid][k]);
	for (new k = 0; k < 4; k++) g_CreaObj[playerid][k] = 0;
	SetPlayerVirtualWorld(playerid, g_CreaBackVW[playerid]);
	SetPlayerInterior(playerid, g_CreaBackInt[playerid]);
	SetPlayerPos(playerid, g_CreaBack[playerid][0], g_CreaBack[playerid][1], g_CreaBack[playerid][2]);
	SetPlayerFacingAngle(playerid, g_CreaBack[playerid][3]);
	SetCameraBehindPlayer(playerid);
	TogglePlayerControllable(playerid, 1);
	SetPlayerWeather(playerid, g_WxNow);
	if (g_CwRealTime) { new h, mi, s; gettime(h, mi, s); SetPlayerTime(playerid, h, mi); }
	Cw_Notify(playerid, "success", "Bienvenue a Crestwood", "Ton personnage est pret. Tu pourras changer de vetements dans un ~b~magasin de vetements~w~ (icone t-shirt).", 9000);
	return 1;
}

// ============================================================
//  2) INTERACTIONS DIRECTES
// ============================================================
#define INT_REQ_S            30
#define INT_RANGE            3.0
#define INT_PICK_CD          600
#define INT_ROB_CD           900
#define INT_LOCK_CD          180
#define INT_DICE_BET         100
#define INT_HEAL_PRICE       500
#define INT_REPAIR_PRICE     800
#define INT_COL              0xC2A2DAFF
#define INT_COL_OK           0x33CC33FF
#define INT_COL_ERR          0xFF6347FF

#define REQ_KISS             1
#define REQ_DANCE            2
#define REQ_ARM              3
#define REQ_DICE             4
#define REQ_ID               5
#define REQ_REPAIR           6

new g_ReqFrom[MAX_PLAYERS] = {INVALID_PLAYER_ID, ...};
new g_ReqType[MAX_PLAYERS], g_ReqUntil[MAX_PLAYERS], g_ReqVeh[MAX_PLAYERS];
new g_IntPickCd[MAX_PLAYERS], g_IntRobCd[MAX_PLAYERS], g_IntLockCd[MAX_PLAYERS], g_IntBusy[MAX_PLAYERS];

stock Int_ReqLabel(type, out[], size)
{
	switch (type)
	{
		case REQ_KISS:   format(out, size, "Bisou");
		case REQ_DANCE:  format(out, size, "Danser ensemble");
		case REQ_ARM:    format(out, size, "Bras de fer");
		case REQ_DICE:   format(out, size, "Partie de d\xe9s (100 $)");
		case REQ_ID:     format(out, size, "Montrer tes papiers");
		case REQ_REPAIR: format(out, size, "R\xe9paration (800 $)");
		default:         if (!Cw47_ReqLabel(type, out, size)) format(out, size, "Demande");   // [v47]
	}
	return 1;
}

stock Int_Near(a, b, Float:r)
{
	if (b < 0 || b >= MAX_PLAYERS || !IsPlayerConnected(b) || a == b) return 0;
	if (GetPlayerVirtualWorld(a) != GetPlayerVirtualWorld(b) || GetPlayerInterior(a) != GetPlayerInterior(b)) return 0;
	new Float:x, Float:y, Float:z;
	GetPlayerPos(b, x, y, z);
	return IsPlayerInRangeOfPoint(a, r, x, y, z);
}

stock Int_Face(a, b)
{
	new Float:ax, Float:ay, Float:az, Float:bx, Float:by, Float:bz;
	GetPlayerPos(a, ax, ay, az);
	GetPlayerPos(b, bx, by, bz);
	SetPlayerFacingAngle(a, atan2(bx - ax, by - ay) * -1.0);
	SetPlayerFacingAngle(b, atan2(ax - bx, ay - by) * -1.0);
	return 1;
}

stock Int_Ask(playerid, t, type, veh = 0)
{
	if (!Int_Near(playerid, t, INT_RANGE + 1.5)) return Ui_Toast(playerid, 0, "Rapproche-toi du joueur."), 1;
	g_ReqFrom[t] = playerid;
	g_ReqType[t] = type;
	g_ReqVeh[t] = veh;
	g_ReqUntil[t] = gettime() + INT_REQ_S;
	new n[MAX_PLAYER_NAME + 1], lb[40], m[160];
	Fx_RPName(playerid, n, sizeof n);
	Int_ReqLabel(type, lb, sizeof lb);
	format(m, sizeof m, "%s te propose : ~b~%s~w~. Touche-le pour accepter (30 s) ou /accepterdemande.", n, lb);
	Cw_Notify(t, "info", "Interaction", m, 9000);
	return Ui_Toast(playerid, 1, "Proposition envoyee."), 1;
}

// lignes "Accepter / Refuser" en tete du menu quand le joueur touche celui qui lui propose quelque chose
stock Int_ReqItems(playerid, target, out[], size)
{
	out[0] = 0;
	if (g_ReqFrom[playerid] != target || g_ReqUntil[playerid] < gettime()) return 0;
	new lb[40];
	Int_ReqLabel(g_ReqType[playerid], lb, sizeof lb);
	format(out, size, "req_yes:Accepter : %s;req_no:Refuser;", lb);
	return 1;
}

stock Int_PlayerItems(playerid, target, txt[], size)
{
	strcat(txt, ";examine:Observer;id_req:Demander ses papiers;kiss:Bisou;dance:Danser ensemble;arm:Bras de fer;dice:D\xe9s (100 $)", size);
	new t = Fac_Type(playerid);
	if (t == FACTION_MEDIC)
	{
		if (g_CwInjured[target] == 2) strcat(txt, ";revive:R\xe9animer", size);
		strcat(txt, ";heal:Soigner (500 $)", size);
	}
	if (Fac_IsLaw(playerid))
	{
		strcat(txt, g_PolCuffed[target] ? (";p_cuff:D\xe9menotter") : (";p_cuff:Menotter"), size);
		if (g_PolCuffed[target]) strcat(txt, (g_PolDraggedBy[target] == playerid) ? (";p_drag:L\xe2cher") : (";p_drag:Escorter"), size);
		strcat(txt, ";p_frisk:Fouiller", size);
	}
	else
	{
		strcat(txt, ";pick:Faire les poches", size);
		new w = GetPlayerWeapon(playerid);
		if (w >= 22 && w <= 34) strcat(txt, ";rob:Braquer", size);
	}
	Cw47_PlayerItems(playerid, target, txt, size);   // [v47] interactions de faction en plus
	return 1;
}

stock Int_VehItems(playerid, v, txt[], size)
{
	new Float:q1, Float:q2, Float:q3, Float:q4;
	GetVehicleRotationQuat(v, q1, q2, q3, q4);
	// vehicule a l'envers / sur le cote : composante "haut" du repere
	new Float:upz = 1.0 - 2.0 * (q2 * q2 + q3 * q3);
	if (upz < 0.5) strcat(txt, "flip:Remettre sur roues;", size);
	if (Fac_Type(playerid) == FACTION_MECHANIC) strcat(txt, "m_repair:R\xe9parer (800 $);", size);
	new engine, lights, alarm, doors, bonnet, boot, objective;
	GetVehicleParamsEx(v, engine, lights, alarm, doors, bonnet, boot, objective);
	if (doors == VEHICLE_PARAMS_ON && !Ui_OwnsVehicle(playerid, v) && !Fac_IsLaw(playerid)) strcat(txt, "lockpick:Crocheter;", size);
	Cw47_VehItems(playerid, v, txt, size);   // [v47]
	return 1;
}

forward Int_ArmDone(a, b);
public Int_ArmDone(a, b)
{
	g_IntBusy[a] = 0; g_IntBusy[b] = 0;
	if (!IsPlayerConnected(a) || !IsPlayerConnected(b)) return 0;
	ClearAnimations(a); ClearAnimations(b);
	new w = random(2) ? a : b, l = (w == a) ? b : a, wn[MAX_PLAYER_NAME + 1], ln[MAX_PLAYER_NAME + 1], m[144];
	Fx_RPName(w, wn, sizeof wn); Fx_RPName(l, ln, sizeof ln);
	format(m, sizeof m, "* %s remporte le bras de fer contre %s.", wn, ln);
	RpX_Near(w, 15.0, INT_COL, m);
	return 1;
}

stock Int_Accept(playerid)
{
	new a = g_ReqFrom[playerid], type = g_ReqType[playerid];
	if (a == INVALID_PLAYER_ID || g_ReqUntil[playerid] < gettime() || !IsPlayerConnected(a)) return Ui_Toast(playerid, 0, "Aucune proposition en cours."), 1;
	g_ReqFrom[playerid] = INVALID_PLAYER_ID;
	if (type != REQ_REPAIR && !Int_Near(playerid, a, INT_RANGE + 2.0)) return Ui_Toast(playerid, 0, "Vous etes trop loin l'un de l'autre."), 1;
	if (type >= 20) return Cw47_Accept(a, playerid, type);   // [v47] types 20+ : include/Cw47.inc
	new an[MAX_PLAYER_NAME + 1], bn[MAX_PLAYER_NAME + 1], m[160];
	Fx_RPName(a, an, sizeof an); Fx_RPName(playerid, bn, sizeof bn);
	switch (type)
	{
		case REQ_KISS:
		{
			Int_Face(a, playerid);
			ApplyAnimation(a, "KISSING", "Playa_Kiss_02", 4.1, 0, 0, 0, 0, 0, 1);
			ApplyAnimation(playerid, "KISSING", "Grlfrd_Kiss_02", 4.1, 0, 0, 0, 0, 0, 1);
			format(m, sizeof m, "* %s et %s s'embrassent.", an, bn);
			RpX_Near(a, 15.0, INT_COL, m);
		}
		case REQ_DANCE:
		{
			Int_Face(a, playerid);
			ApplyAnimation(a, "DANCING", "dnce_M_b", 4.1, 1, 0, 0, 0, 0, 1);
			ApplyAnimation(playerid, "DANCING", "DAN_Loop_A", 4.1, 1, 0, 0, 0, 0, 1);
			format(m, sizeof m, "* %s et %s dansent ensemble. (/stopanim pour arreter)", an, bn);
			RpX_Near(a, 15.0, INT_COL, m);
		}
		case REQ_ARM:
		{
			if (g_IntBusy[a] || g_IntBusy[playerid]) return 1;
			g_IntBusy[a] = 1; g_IntBusy[playerid] = 1;
			Int_Face(a, playerid);
			ApplyAnimation(a, "GANGS", "hndshkfa_swt", 4.1, 1, 0, 0, 0, 0, 1);
			ApplyAnimation(playerid, "GANGS", "hndshkfa_swt", 4.1, 1, 0, 0, 0, 0, 1);
			format(m, sizeof m, "* %s et %s se lancent dans un bras de fer...", an, bn);
			RpX_Near(a, 15.0, INT_COL, m);
			SetTimerEx("Int_ArmDone", 4000, false, "ii", a, playerid);
		}
		case REQ_DICE:
		{
			if (GetPlayerMoney(a) < INT_DICE_BET || GetPlayerMoney(playerid) < INT_DICE_BET) return Ui_Toast(playerid, 0, "Il faut 100 $ en liquide chacun."), 1;
			new da = 1 + random(6) + 1 + random(6), db = 1 + random(6) + 1 + random(6);
			format(m, sizeof m, "* %s lance les des : %d  |  %s lance les des : %d", an, da, bn, db);
			RpX_Near(a, 15.0, INT_COL, m);
			if (da == db) RpX_Near(a, 15.0, INT_COL, "* Egalite : chacun garde sa mise.");
			else
			{
				new w = (da > db) ? a : playerid, l = (w == a) ? playerid : a;
				GivePlayerMoney(l, -INT_DICE_BET);
				GivePlayerMoney(w, INT_DICE_BET);
				Fx_RPName(w, an, sizeof an);
				format(m, sizeof m, "* %s remporte la mise de 100 $.", an);
				RpX_Near(a, 15.0, INT_COL, m);
			}
		}
		case REQ_ID:
		{
			new age, origin[32];
			if (!Ui_LoadId(playerid, age, origin, sizeof origin)) return Ui_Toast(playerid, 0, "Tu n'as pas de papiers : fais /papiers d'abord."), 1;
			Ui_ShowId(a, playerid);
			Ui_Toast(playerid, 1, "Tu montres tes papiers.");
		}
		case REQ_REPAIR:
		{
			new v = g_ReqVeh[playerid];
			if (!Int_Near(playerid, a, 15.0) || v <= 0 || !GetVehicleModel(v)) return Ui_Toast(playerid, 0, "Le mecanicien n'est plus la."), 1;
			if (GetPlayerMoney(playerid) >= INT_REPAIR_PRICE) GivePlayerMoney(playerid, -INT_REPAIR_PRICE);
			else if (g_BankLoaded[playerid] && g_BankBalance[playerid] >= INT_REPAIR_PRICE) { g_BankBalance[playerid] -= INT_REPAIR_PRICE; Bank_AddHistory(playerid, "Reparation (mecanicien)", -INT_REPAIR_PRICE); Bank_SavePlayer(playerid); }
			else return Ui_Toast(playerid, 0, "Pas assez d'argent (800 $)."), 1;
			Frp_PayBank(a, INT_REPAIR_PRICE, "Reparation d'un vehicule");
			ApplyAnimation(a, "CAR", "Fixn_Car_Loop", 4.1, 0, 0, 0, 0, 6000, 1);
			RepairVehicle(v);
			SetVehicleHealth(v, 1000.0);
			format(m, sizeof m, "* %s repare le vehicule de %s.", an, bn);
			RpX_Near(a, 15.0, INT_COL, m);
		}
	}
	return 1;
}

forward Int_RobDone(a, b);
public Int_RobDone(a, b)
{
	g_IntBusy[a] = 0;
	if (!IsPlayerConnected(a) || !IsPlayerConnected(b)) return 0;
	ClearAnimations(b);
	if (!Int_Near(a, b, 6.0)) return Cw_Notify(a, "denied", "Braquage", "La victime s'est enfuie.", 4000), 1;
	new cash = GetPlayerMoney(b);
	new take = cash * (10 + random(16)) / 100;
	if (take > 5000) take = 5000;
	if (take > 0) { GivePlayerMoney(b, -take); GivePlayerMoney(a, take); }
	new m[128], a2[16];
	Cw_Fmt(take, a2, sizeof a2);
	format(m, sizeof m, "Tu repars avec ~g~%s $~w~. La police est prevenue !", a2);
	Cw_Notify(a, "success", "Braquage", m, 6000);
	format(m, sizeof m, "Tu t'es fait braquer : ~r~-%s $~w~. Appelle la police (17).", a2);
	Cw_Notify(b, "denied", "Braquage", m, 7000);
	return 1;
}

forward Int_LockDone(playerid, v);
public Int_LockDone(playerid, v)
{
	g_IntBusy[playerid] = 0;
	if (!IsPlayerConnected(playerid) || !GetVehicleModel(v)) return 0;
	ClearAnimations(playerid);
	new engine, lights, alarm, doors, bonnet, boot, objective;
	GetVehicleParamsEx(v, engine, lights, alarm, doors, bonnet, boot, objective);
	if (random(100) < 35)
	{
		SetVehicleParamsEx(v, engine, lights, alarm, VEHICLE_PARAMS_OFF, bonnet, boot, objective);
		if (GetPlayerWantedLevel(playerid) < 1) SetPlayerWantedLevel(playerid, 1);
		return Cw_Notify(playerid, "success", "Crochetage", "Clic ! La portiere est ouverte.", 4000), 1;
	}
	SetVehicleParamsEx(v, engine, lights, VEHICLE_PARAMS_ON, doors, bonnet, boot, objective);
	SetTimerEx("Int_AlarmOff", 12000, false, "i", v);
	new Float:x, Float:y, Float:z, m[128];
	GetVehiclePos(v, x, y, z);
	new owner = g_VehOwner[v];
	if (Gar_DbOf(v)) owner = Gar_OwnerOf(v);
	if (owner != INVALID_PLAYER_ID && IsPlayerConnected(owner) && owner != playerid) Cw_Notify(owner, "warning", "Alarme", "L'alarme de ton vehicule se declenche ! Quelqu'un essaie de le voler.", 7000);
	format(m, sizeof m, "[CENTRALE] Alarme de vehicule : tentative de vol (%s).", g_VehName[GetVehicleModel(v) - 400]);
	Law_SendAll(0x5B8CFFFF, m);
	if (GetPlayerWantedLevel(playerid) < 2) SetPlayerWantedLevel(playerid, 2);
	return Cw_Notify(playerid, "denied", "Crochetage", "Rate : l'alarme se declenche !", 4000), 1;
}

forward Int_AlarmOff(v);
public Int_AlarmOff(v)
{
	if (!GetVehicleModel(v)) return 0;
	new engine, lights, alarm, doors, bonnet, boot, objective;
	GetVehicleParamsEx(v, engine, lights, alarm, doors, bonnet, boot, objective);
	SetVehicleParamsEx(v, engine, lights, VEHICLE_PARAMS_OFF, doors, bonnet, boot, objective);
	return 1;
}

forward Int_FlipDone(playerid, v);
public Int_FlipDone(playerid, v)
{
	g_IntBusy[playerid] = 0;
	if (!IsPlayerConnected(playerid) || !GetVehicleModel(v)) return 0;
	ClearAnimations(playerid);
	new Float:a;
	GetVehicleZAngle(v, a);
	SetVehicleZAngle(v, a);   // remet le vehicule droit (meme cap)
	return Cw_Notify(playerid, "success", "Vehicule", "Vehicule remis sur ses roues.", 3000), 1;
}

stock Int_UiAction(playerid, const action[])
{
	new t = g_UiTarget[playerid], now = gettime(), m[160], an[MAX_PLAYER_NAME + 1], tn[MAX_PLAYER_NAME + 1];
	if (Cw47_UiAction(playerid, action)) return 1;   // [v47] interactions de faction en plus
	if (!strcmp(action, "req_yes", true)) return Int_Accept(playerid);
	if (!strcmp(action, "req_no", true))
	{
		new a = g_ReqFrom[playerid];
		g_ReqFrom[playerid] = INVALID_PLAYER_ID;
		if (a != INVALID_PLAYER_ID && IsPlayerConnected(a)) Ui_Toast(a, 0, "Ta proposition a ete refusee.");
		return 1;
	}
	if (!strcmp(action, "kiss", true)) return Int_Ask(playerid, t, REQ_KISS);
	if (!strcmp(action, "dance", true)) return Int_Ask(playerid, t, REQ_DANCE);
	if (!strcmp(action, "arm", true)) return Int_Ask(playerid, t, REQ_ARM);
	if (!strcmp(action, "dice", true)) return Int_Ask(playerid, t, REQ_DICE);
	if (!strcmp(action, "id_req", true)) return Int_Ask(playerid, t, REQ_ID);
	if (!strcmp(action, "examine", true))
	{
		format(m, sizeof m, "/examiner %d", t);
		return Vie_OnCommand(playerid, m), 1;
	}
	if (!strcmp(action, "revive", true))
	{
		format(m, sizeof m, "/reanimer %d", t);
		return Cw_OnCommand(playerid, m), 1;
	}
	if (!strcmp(action, "heal", true))
	{
		format(m, sizeof m, "%d %d", t, INT_HEAL_PRICE);
		return RpX_Offer(playerid, m, RPX_OFF_HEAL, FACTION_MEDIC), 1;
	}
	// ---- police : directement depuis le menu ----
	if (!strcmp(action, "p_cuff", true) || !strcmp(action, "p_drag", true) || !strcmp(action, "p_frisk", true))
	{
		if (!Fac_IsLaw(playerid) || GetPlayerState(playerid) != PLAYER_STATE_ONFOOT || g_PolCuffed[playerid]) return Ui_Toast(playerid, 0, "Reserve aux forces de l'ordre, a pied."), 1;
		if (!Int_Near(playerid, t, INT_RANGE) || GetPlayerState(t) != PLAYER_STATE_ONFOOT) return Ui_Toast(playerid, 0, "Rapproche-toi du suspect (a pied)."), 1;
		Fx_RPName(playerid, an, sizeof an); Fx_RPName(t, tn, sizeof tn);
		if (!strcmp(action, "p_cuff", true))
		{
			if (g_PolCuffed[t]) { Pol_Uncuff(t); format(m, sizeof m, "* %s retire les menottes de %s.", an, tn); }
			else { Pol_Cuff(t); format(m, sizeof m, "* %s passe les menottes a %s.", an, tn); }
			return RpX_Near(playerid, 15.0, INT_COL, m), 1;
		}
		if (!strcmp(action, "p_drag", true))
		{
			if (!g_PolCuffed[t]) return Ui_Toast(playerid, 0, "Le suspect doit etre menotte."), 1;
			if (g_PolDragging[playerid] == t) { Pol_StopDrag(playerid, 1); return Ui_Toast(playerid, 1, "Tu laches le suspect."), 1; }
			if (g_PolDraggedBy[t] != POL_NONE && g_PolDraggedBy[t] != playerid) return Ui_Toast(playerid, 0, "Un collegue escorte deja ce suspect."), 1;
			if (g_PolDragging[playerid] != POL_NONE) Pol_StopDrag(playerid, 0);
			g_PolDragging[playerid] = t;
			g_PolDraggedBy[t] = playerid;
			format(m, sizeof m, "* %s escorte %s.", an, tn);
			return RpX_Near(playerid, 15.0, INT_COL, m), 1;
		}
		// fouille : liquide + armes
		new line[220], part[48], wn[32], w, ammo, n = 0;
		format(line, sizeof line, "Fouille de %s : %d $ en liquide. Armes :", tn, GetPlayerMoney(t));
		for (new sl = 0; sl < 13; sl++)
		{
			GetPlayerWeaponData(t, sl, w, ammo);
			if (w <= 0 || ammo <= 0) continue;
			GetWeaponName(w, wn, sizeof wn);
			format(part, sizeof part, " %s (%d)", wn, ammo);
			strcat(line, part, sizeof line);
			n++;
		}
		if (!n) strcat(line, " aucune", sizeof line);
		SendClientMessage(playerid, 0x8AB4FFFF, line);
		ApplyAnimation(playerid, "POLICE", "plc_drgbst_02", 4.1, 0, 0, 0, 0, 0, 1);
		format(m, sizeof m, "* %s fouille %s.", an, tn);
		return RpX_Near(playerid, 15.0, INT_COL, m), 1;
	}
	// ---- crimes ----
	if (!strcmp(action, "pick", true))
	{
		if (Fac_IsLaw(playerid)) return 1;
		if (!Int_Near(playerid, t, 1.8)) return Ui_Toast(playerid, 0, "Colle-toi a la personne (1,5 m)."), 1;
		if (g_IntPickCd[playerid] > now) return Ui_Toast(playerid, 0, "Trop risque : attends un peu avant de recommencer."), 1;
		if (Gz_At(playerid) != -1) return Ui_Toast(playerid, 0, "Impossible en zone verte."), 1;
		g_IntPickCd[playerid] = now + INT_PICK_CD;
		Fx_RPName(playerid, an, sizeof an);
		if (random(100) < 40)
		{
			new take = GetPlayerMoney(t) * (2 + random(5)) / 100;
			if (take > 800) take = 800;
			if (take > 0) { GivePlayerMoney(t, -take); GivePlayerMoney(playerid, take); }
			format(m, sizeof m, "Tu glisses la main dans ses poches : ~g~+%d $~w~.", take);
			return Cw_Notify(playerid, "success", "Pickpocket", m, 5000), 1;
		}
		if (GetPlayerWantedLevel(playerid) < 1) SetPlayerWantedLevel(playerid, 1);
		format(m, sizeof m, "Tu sens une main dans ta poche : ~r~%s~w~ essaie de te voler !", an);
		Cw_Notify(t, "warning", "Pickpocket", m, 7000);
		return Cw_Notify(playerid, "denied", "Pickpocket", "Rate ! La personne t'a vu.", 4000), 1;
	}
	if (!strcmp(action, "rob", true))
	{
		new w = GetPlayerWeapon(playerid);
		if (Fac_IsLaw(playerid) || w < 22 || w > 34) return Ui_Toast(playerid, 0, "Il te faut une arme a feu en main."), 1;
		if (!Int_Near(playerid, t, 5.0)) return Ui_Toast(playerid, 0, "Rapproche-toi de la victime."), 1;
		if (g_IntRobCd[playerid] > now) return Ui_Toast(playerid, 0, "Tu as deja braque quelqu'un recemment."), 1;
		if (Gz_At(playerid) != -1 || Gz_At(t) != -1) return Ui_Toast(playerid, 0, "Impossible en zone verte."), 1;
		if (g_IntBusy[playerid]) return 1;
		g_IntBusy[playerid] = 1;
		g_IntRobCd[playerid] = now + INT_ROB_CD;
		ApplyAnimation(t, "ROB_BANK", "SHP_HandsUp_Scr", 4.1, 0, 0, 0, 1, 0, 1);
		Fx_RPName(playerid, an, sizeof an); Fx_RPName(t, tn, sizeof tn);
		format(m, sizeof m, "* %s braque %s avec une arme : \"Les mains en l'air, ton argent !\"", an, tn);
		RpX_Near(playerid, 20.0, INT_COL, m);
		if (GetPlayerWantedLevel(playerid) < 3) SetPlayerWantedLevel(playerid, 3);
		new Float:x, Float:y, Float:z;
		GetPlayerPos(playerid, x, y, z);
		format(m, sizeof m, "[CENTRALE] Braquage a main armee en cours : %s (%d). Position au GPS.", an, playerid);
		for (new i = 0, mx = GetMaxPlayers(); i < mx; i++) if (IsPlayerConnected(i) && Fac_IsLaw(i)) { SendClientMessage(i, 0x5B8CFFFF, m); Gps_Set(i, x, y); }
		SetTimerEx("Int_RobDone", 5000, false, "ii", playerid, t);
		return 1;
	}
	// ---- vehicule ----
	new v = g_UiVeh[playerid];
	if (!strcmp(action, "flip", true) || !strcmp(action, "lockpick", true) || !strcmp(action, "m_repair", true))
	{
		if (v <= 0 || !GetVehicleModel(v) || GetPlayerState(playerid) != PLAYER_STATE_ONFOOT) return 1;
		new Float:x, Float:y, Float:z;
		GetVehiclePos(v, x, y, z);
		if (!IsPlayerInRangeOfPoint(playerid, 5.0, x, y, z)) return Ui_Toast(playerid, 0, "Rapproche-toi du vehicule."), 1;
		if (g_IntBusy[playerid]) return 1;
		if (!strcmp(action, "flip", true))
		{
			g_IntBusy[playerid] = 1;
			ApplyAnimation(playerid, "CARRY", "liftup", 4.1, 0, 0, 0, 0, 0, 1);
			SetTimerEx("Int_FlipDone", 3000, false, "ii", playerid, v);
			return Cw_Notify(playerid, "info", "Vehicule", "Tu pousses le vehicule pour le remettre droit...", 3000), 1;
		}
		if (!strcmp(action, "lockpick", true))
		{
			if (Fac_IsLaw(playerid) || Ui_OwnsVehicle(playerid, v)) return 1;
			if (g_IntLockCd[playerid] > now) return Ui_Toast(playerid, 0, "Attends un peu avant de recommencer."), 1;
			g_IntLockCd[playerid] = now + INT_LOCK_CD;
			g_IntBusy[playerid] = 1;
			ApplyAnimation(playerid, "BOMBER", "BOM_Plant_Loop", 4.1, 1, 0, 0, 0, 0, 1);
			SetTimerEx("Int_LockDone", 7000, false, "ii", playerid, v);
			return Cw_Notify(playerid, "info", "Crochetage", "Tu crochetes la serrure (7 s)...", 7000), 1;
		}
		// mecanicien : le proprietaire (ou le conducteur) accepte et paie
		if (Fac_Type(playerid) != FACTION_MECHANIC) return 1;
		new owner = g_VehOwner[v];
		if (Gar_DbOf(v)) owner = Gar_OwnerOf(v);
		if (owner == INVALID_PLAYER_ID || !IsPlayerConnected(owner))
		{
			for (new p = 0, mx = GetMaxPlayers(); p < mx; p++) if (IsPlayerConnected(p) && GetPlayerVehicleID(p) == v && GetPlayerState(p) == PLAYER_STATE_DRIVER) { owner = p; break; }
		}
		if (owner == INVALID_PLAYER_ID || !IsPlayerConnected(owner) || owner == playerid) return Ui_Toast(playerid, 0, "Le proprietaire doit etre la pour accepter."), 1;
		if (!Int_Near(playerid, owner, 15.0)) return Ui_Toast(playerid, 0, "Le proprietaire doit etre a moins de 15 m."), 1;
		g_ReqFrom[owner] = playerid;
		g_ReqType[owner] = REQ_REPAIR;
		g_ReqVeh[owner] = v;
		g_ReqUntil[owner] = now + INT_REQ_S;
		Fx_RPName(playerid, an, sizeof an);
		format(m, sizeof m, "Le mecanicien %s propose de reparer ton vehicule (800 $). Touche-le ou /accepterdemande.", an);
		Cw_Notify(owner, "info", "Mecanicien", m, 9000);
		return Ui_Toast(playerid, 1, "Proposition envoyee au proprietaire."), 1;
	}
	return 0;
}

stock Cw41_OnConnect(playerid)
{
	g_ReqFrom[playerid] = INVALID_PLAYER_ID;
	g_ReqUntil[playerid] = 0;
	g_IntPickCd[playerid] = 0; g_IntRobCd[playerid] = 0; g_IntLockCd[playerid] = 0; g_IntBusy[playerid] = 0;
	g_CreaOn[playerid] = 0;
	for (new k = 0; k < 4; k++) g_CreaObj[playerid][k] = 0;
	g_FpOpen[playerid] = 0;
	g_FpLast[playerid] = 0;
	return 1;
}

stock Cw41_OnInit()
{
	Fp_InitDB();
	g_SlashOpen = Frp_GetSetting("slash_open", 1);   // [v53] ouvert par defaut : les pages (connexion, HUD) passent par ce filtre
	return 1;
}

stock Cw41_OnDisconnect(playerid)
{
	for (new p = 0, m = GetMaxPlayers(); p < m; p++) if (g_ReqFrom[p] == playerid) g_ReqFrom[p] = INVALID_PLAYER_ID;
	g_CreaOn[playerid] = 0;   // les objets du joueur disparaissent avec lui
	return 1;
}

stock Cw41_OnCommand(playerid, const cmdtext[])
{
	new idx;
	if (Fx_IsCmd(cmdtext, "/accepterdemande", idx)) return Int_Accept(playerid);
	if (Fx_IsCmd(cmdtext, "/refuserdemande", idx)) { g_ReqFrom[playerid] = INVALID_PLAYER_ID; return Ui_Toast(playerid, 1, "Proposition refusee."), 1; }
	if (Tc_Handle(playerid, cmdtext)) return 1;   // commandes tapees dans le chat
	if (!strcmp(cmdtext, "/facact ", true, 8) || !strcmp(cmdtext, "/facact", true)) return Fp_Action(playerid, cmdtext[(cmdtext[7] == ' ') ? 8 : 7]), 1;
	if (Fx_IsCmd(cmdtext, "/factionpanel", idx)) return Fp_Open(playerid), 1;
	if (Fx_IsCmd(cmdtext, "/slashjoueurs", idx))
	{
		if (!Frp_Staff(playerid)) return 0;
		new tok[8];
		Frp_Tok(cmdtext, idx, tok, sizeof tok);
		g_SlashOpen = !strcmp(tok, "on", true) ? 1 : 0;
		Frp_SetSetting("slash_open", g_SlashOpen);
		return Cw_Notify(playerid, "success", "Commandes", g_SlashOpen ? ("Commandes tapees ouvertes a tous les joueurs.") : ("Joueurs : seulement /aide (staff : tout)."), 4000), 1;
	}
	if (Fx_IsCmd(cmdtext, "/personnage", idx))
	{
		if (!g_ClothFirst[playerid]) return Cw_Notify(playerid, "info", "Personnage", "Ton personnage est deja cree : change de tenue dans un magasin de vetements.", 4000), 1;
		Cloth_OpenFirst(playerid);
		return 1;
	}
	return 0;
}

// ============================================================
//  3) PANNEAU DE FACTION (page assets/faction/index.html, ouverte par la tablette RP > "Ma faction")
//     serveur -> page : "@fac" 9680 ouvrir / 9681 mise a jour / 9682 fermer
//     page -> serveur : /facact refresh | view dash|members|events | deposit n | withdraw n |
//                       promote nom | demote nom | kick nom | evadd ts titre | evdel id | invite id | tool cle | close
// ============================================================
#define FP_DIALOG_OPEN       9680
#define FP_DIALOG_UPDATE     9681
#define FP_DIALOG_CLOSE      9682


stock Fp_Exec(const q[])
{
	if (g_FacDB != DB:0) db_free_result(db_query(g_FacDB, q));
	return 1;
}

stock Fp_InitDB()
{
	Fp_Exec("CREATE TABLE IF NOT EXISTS faction_bank (fac INTEGER PRIMARY KEY, bal INTEGER NOT NULL DEFAULT 0)");
	Fp_Exec("CREATE TABLE IF NOT EXISTS faction_log (id INTEGER PRIMARY KEY AUTOINCREMENT, fac INTEGER, ts INTEGER, text TEXT)");
	Fp_Exec("CREATE TABLE IF NOT EXISTS faction_events (id INTEGER PRIMARY KEY AUTOINCREMENT, fac INTEGER, ts INTEGER, title TEXT, author TEXT)");
	return 1;
}

stock Fp_Int(const q[])
{
	new v = 0, f[16];
	if (g_FacDB == DB:0) return 0;
	new DBResult:r = db_query(g_FacDB, q);
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

stock Fp_Bal(fac)
{
	new q[80];
	format(q, sizeof q, "SELECT bal FROM faction_bank WHERE fac = %d", fac);
	return Fp_Int(q);
}

stock Fp_SetBal(fac, bal)
{
	new q[100];
	format(q, sizeof q, "INSERT OR REPLACE INTO faction_bank (fac, bal) VALUES (%d, %d)", fac, bal);
	return Fp_Exec(q);
}

stock Fp_Log(fac, const text[])
{
	new t[100], q[220];
	Law_Clean(text, t, sizeof t);
	format(q, sizeof q, "INSERT INTO faction_log (fac, ts, text) VALUES (%d, %d, '%s')", fac, gettime(), t);
	return Fp_Exec(q);
}

stock Fp_IsMg(playerid) return Fx_CanManage(playerid) || Frp_Staff(playerid);
stock Fp_IsLd(playerid) return Fx_IsLeader(playerid);

// boutons "actions de service" selon le type de faction : [["cle","Libelle"],...]
stock Fp_Tools(playerid, out[], size)
{
	new t = Fac_Type(playerid);
	out[0] = 0;
	if (Fac_IsLaw(playerid)) strcat(out, "[\"mdt\",\"Ouvrir le MDT\"],[\"renfort\",\"Demander du renfort\"],[\"mandats\",\"Mandats actifs\"],", size);
	if (t == FACTION_FEDERAL) strcat(out, "[\"traceurs\",\"Traceurs GPS\"],[\"stopsuivre\",\"Arreter le suivi\"],[\"ecoutes\",\"Ecoutes en cours\"],", size);
	if (t == FACTION_ARMY && Army_Can(playerid)) strcat(out, g_ArmyZoneOn ? ("[\"zoneoff\",\"Couper la zone militaire\"],") : ("[\"zoneon\",\"Activer la zone militaire\"],"), size);
	if (t == FACTION_MEDIC) strcat(out, "[\"blesses\",\"Blesses en attente\"],[\"incendies\",\"Incendies en cours\"],[\"secourir\",\"Secourir une victime\"],", size);
	if (t == FACTION_FIRE) strcat(out, "[\"incendies\",\"Incendies en cours (GPS)\"],[\"secourir\",\"Secourir une victime\"],", size);
	if (t == FACTION_JUSTICE) strcat(out, "[\"mandats\",\"Mandats actifs\"],", size);
	if (t == FACTION_GOVERNMENT) strcat(out, "[\"tresor\",\"Caisse de l'Etat\"],", size);
	if ((t == FACTION_GOVERNMENT || t == FACTION_ARMY) && Frp_IsTop(playerid, 1)) strcat(out, g_Emergency ? ("[\"urgoff\",\"Lever l'etat d'urgence\"],") : ("[\"urgon\",\"Declarer l'etat d'urgence\"],"), size);
	strcat(out, "[\"vestiaire\",\"Vestiaire (si proche)\"],[\"quitter\",\"Quitter la faction\"]", size);
	return 1;
}

// joueurs proches sans faction (pour inviter) : [[id,"Nom"],...]
stock Fp_Near(playerid, out[], size)
{
	out[0] = 0;
	new n = 0, part[64], nm[MAX_PLAYER_NAME + 1];
	for (new i = 0, m = GetMaxPlayers(); i < m && n < 8; i++)
	{
		if (i == playerid || !IsPlayerConnected(i) || !g_AuthLogged[i] || IsPlayerNPC(i)) continue;
		if (Fac_Get(i) != FACTION_NONE || !Int_Near(playerid, i, 10.0)) continue;
		GetPlayerName(i, nm, sizeof nm);
		format(part, sizeof part, "%s[%d,\"%s\"]", n ? (",") : (""), i, nm);
		strcat(out, part, size);
		n++;
	}
	return n;
}

stock Fp_Head(playerid, out[], size)
{
	new fac = Fac_Get(playerid), fn[100], sh[50], me[MAX_PLAYER_NAME + 1], rn[32], tn[16];
	Law_Clean(g_FxName[fac], fn, sizeof fn);
	Law_Clean(g_FxShort[fac], sh, sizeof sh);
	GetPlayerName(playerid, me, sizeof me);
	Fac_RankName(fac, g_FacRank[playerid], rn, sizeof rn);
	new t = g_FxType[fac];
	if (t >= 0 && t <= FACTION_TYPE_MAX) format(tn, sizeof tn, "%s", g_FxTypeNames[t]); else format(tn, sizeof tn, "none");
	format(out, size, "\"fac\":{\"id\":%d,\"n\":\"%s\",\"s\":\"%s\",\"t\":\"%s\",\"c\":\"%06x\",\"rc\":%d},\"me\":{\"n\":\"%s\",\"r\":%d,\"rn\":\"%s\",\"mg\":%d,\"ld\":%d,\"cash\":%d}",
		fac, fn, sh, tn, (g_FxColor[fac] >>> 8) & 0xFFFFFF, Fac_RankCount(fac), me, g_FacRank[playerid], rn, Fp_IsMg(playerid), Fp_IsLd(playerid), GetPlayerMoney(playerid));
	return 1;
}

stock Fp_Send(playerid, const view[], ok = 1, const msg[] = "", dialogid = FP_DIALOG_UPDATE)
{
	new fac = Fac_Get(playerid);
	if (fac == FACTION_NONE) { if (g_FpOpen[playerid]) Fp_Close(playerid, 1); return 0; }
	static json[4096];
	new head[420], q[200], m2[100], part[200], f[100], f2[40], f3[40];
	Fp_Head(playerid, head, sizeof head);
	Law_Clean(msg, m2, sizeof m2);
	format(json, sizeof json, "{\"v\":\"%s\",\"ok\":%d,\"msg\":\"%s\",%s", view, ok, m2, head);
	if (!strcmp(view, "members"))
	{
		strcat(json, ",\"list\":[", sizeof json);
		format(q, sizeof q, "SELECT name, rank FROM faction_members WHERE faction = %d ORDER BY rank DESC, name LIMIT 60", fac);
		new DBResult:r = db_query(g_FacDB, q), n = 0;
		if (r != DBResult:0)
		{
			for (new rows = db_num_rows(r); n < rows; n++)
			{
				new nm[MAX_PLAYER_NAME + 1], rn[32];
				db_get_field(r, 0, nm, sizeof nm);
				db_get_field(r, 1, f2, sizeof f2);
				Fac_RankName(fac, strval(f2), rn, sizeof rn);
				format(part, sizeof part, "%s[\"%s\",%d,\"%s\",%d]", n ? (",") : (""), nm, strval(f2), rn, (Cw_PlayerByName(nm) != INVALID_PLAYER_ID) ? 1 : 0);
				if (strlen(json) + strlen(part) > sizeof(json) - 700) break;
				strcat(json, part, sizeof json);
				db_next_row(r);
			}
			db_free_result(r);
		}
		strcat(json, "],\"near\":[", sizeof json);
		new near[600];
		if (Fp_IsMg(playerid)) Fp_Near(playerid, near, sizeof near); else near[0] = 0;
		strcat(json, near, sizeof json);
		strcat(json, "]", sizeof json);
	}
	else if (!strcmp(view, "events"))
	{
		strcat(json, ",\"list\":[", sizeof json);
		format(q, sizeof q, "SELECT id, ts, title, author FROM faction_events WHERE fac = %d AND ts > %d ORDER BY ts LIMIT 30", fac, gettime() - 86400 * 31);
		new DBResult:r = db_query(g_FacDB, q), n = 0;
		if (r != DBResult:0)
		{
			for (new rows = db_num_rows(r); n < rows; n++)
			{
				db_get_field(r, 0, f2, sizeof f2);
				db_get_field(r, 1, f3, sizeof f3);
				db_get_field(r, 2, f, sizeof f);
				new au[MAX_PLAYER_NAME + 1];
				db_get_field(r, 3, au, sizeof au);
				format(part, sizeof part, "%s[%d,%d,\"%s\",\"%s\"]", n ? (",") : (""), strval(f2), strval(f3), f, au);
				strcat(json, part, sizeof json);
				db_next_row(r);
			}
			db_free_result(r);
		}
		strcat(json, "]", sizeof json);
	}
	else
	{
		new members, online = 0, evs;
		format(q, sizeof q, "SELECT COUNT(*) FROM faction_members WHERE faction = %d", fac);
		members = Fp_Int(q);
		for (new i = 0, m = GetMaxPlayers(); i < m; i++) if (IsPlayerConnected(i) && g_AuthLogged[i] && Fac_Get(i) == fac) online++;
		format(q, sizeof q, "SELECT COUNT(*) FROM faction_events WHERE fac = %d AND ts > %d", fac, gettime() - 3600);
		evs = Fp_Int(q);
		format(part, sizeof part, ",\"st\":{\"m\":%d,\"on\":%d,\"bal\":%d,\"ev\":%d},\"log\":[", members, online, Fp_Bal(fac), evs);
		strcat(json, part, sizeof json);
		format(q, sizeof q, "SELECT ts, text FROM faction_log WHERE fac = %d ORDER BY id DESC LIMIT 7", fac);
		new DBResult:r = db_query(g_FacDB, q), n = 0;
		if (r != DBResult:0)
		{
			for (new rows = db_num_rows(r); n < rows; n++)
			{
				db_get_field(r, 0, f2, sizeof f2);
				db_get_field(r, 1, f, sizeof f);
				format(part, sizeof part, "%s[%d,\"%s\"]", n ? (",") : (""), strval(f2), f);
				strcat(json, part, sizeof json);
				db_next_row(r);
			}
			db_free_result(r);
		}
		strcat(json, "],\"next\":[", sizeof json);
		format(q, sizeof q, "SELECT id, ts, title, author FROM faction_events WHERE fac = %d AND ts > %d ORDER BY ts LIMIT 4", fac, gettime() - 3600);
		r = db_query(g_FacDB, q); n = 0;
		if (r != DBResult:0)
		{
			for (new rows = db_num_rows(r); n < rows; n++)
			{
				new au[MAX_PLAYER_NAME + 1];
				db_get_field(r, 0, f2, sizeof f2);
				db_get_field(r, 1, f3, sizeof f3);
				db_get_field(r, 2, f, sizeof f);
				db_get_field(r, 3, au, sizeof au);
				format(part, sizeof part, "%s[%d,%d,\"%s\",\"%s\"]", n ? (",") : (""), strval(f2), strval(f3), f, au);
				strcat(json, part, sizeof json);
				db_next_row(r);
			}
			db_free_result(r);
		}
		strcat(json, "],\"tools\":[", sizeof json);
		new tools[900];
		Fp_Tools(playerid, tools, sizeof tools);
		strcat(json, tools, sizeof json);
		strcat(json, "]", sizeof json);
	}
	strcat(json, "}", sizeof json);
	return ShowPlayerDialog(playerid, dialogid, DIALOG_STYLE_MSGBOX, "@fac", json, "OK", "");
}

stock Fp_Open(playerid)
{
	if (Fac_Get(playerid) == FACTION_NONE) return Cw_Notify(playerid, "denied", "Faction", "Tu ne fais partie d'aucune faction.", 3000), 1;
	g_FpOpen[playerid] = 1;
	return Fp_Send(playerid, "dash", 1, "", FP_DIALOG_OPEN), 1;
}

stock Fp_Close(playerid, notify)
{
	g_FpOpen[playerid] = 0;
	if (notify) ShowPlayerDialog(playerid, FP_DIALOG_CLOSE, DIALOG_STYLE_MSGBOX, "@fac", "{}", "OK", "");
	return 1;
}

// membre par nom (en ligne ou non) : rang, 0 si pas dans la faction
stock Fp_MemberRank(fac, const name[])
{
	new q[160];
	format(q, sizeof q, "SELECT rank FROM faction_members WHERE name = '%s' AND faction = %d", name, fac);
	return Fp_Int(q);
}

stock Fp_SetRankByName(fac, const name[], rank)
{
	new t = Cw_PlayerByName(name), q[160];
	if (t != INVALID_PLAYER_ID) return Fac_Set(t, (rank <= 0) ? FACTION_NONE : fac, (rank <= 0) ? 1 : rank);
	if (rank <= 0) format(q, sizeof q, "DELETE FROM faction_members WHERE name = '%s' AND faction = %d", name, fac);
	else format(q, sizeof q, "UPDATE faction_members SET rank = %d WHERE name = '%s' AND faction = %d", rank, name, fac);
	return Fp_Exec(q);
}

stock Fp_Tool(playerid, const k[])
{
	new c[48];
	if (!strcmp(k, "mdt")) format(c, sizeof c, "/mdt");
	else if (!strcmp(k, "renfort")) format(c, sizeof c, "/renfort");
	else if (!strcmp(k, "mandats")) format(c, sizeof c, "/mandats");
	else if (!strcmp(k, "traceurs")) format(c, sizeof c, "/traceurs");
	else if (!strcmp(k, "stopsuivre")) format(c, sizeof c, "/stopsuivre");
	else if (!strcmp(k, "ecoutes")) format(c, sizeof c, "/ecoute liste");
	else if (!strcmp(k, "zoneon")) format(c, sizeof c, "/zonemilitaire on");
	else if (!strcmp(k, "zoneoff")) format(c, sizeof c, "/zonemilitaire off");
	else if (!strcmp(k, "blesses")) format(c, sizeof c, "/blesses");
	else if (!strcmp(k, "incendies")) format(c, sizeof c, "/incendies");
	else if (!strcmp(k, "secourir")) format(c, sizeof c, "/secourir");
	else if (!strcmp(k, "tresor")) format(c, sizeof c, "/tresor");
	else if (!strcmp(k, "urgon")) format(c, sizeof c, "/etatdurgence on");
	else if (!strcmp(k, "urgoff")) format(c, sizeof c, "/etatdurgence off");
	else if (!strcmp(k, "vestiaire")) format(c, sizeof c, "/vestiaire");
	else if (!strcmp(k, "quitter"))
	{
		new fac = Fac_Get(playerid), n[MAX_PLAYER_NAME + 1], m[80];
		if (Fx_IsLeader(playerid)) return Fp_Send(playerid, "dash", 0, "Le chef ne peut pas quitter : passe le grade a quelqu'un d'abord."), 1;
		Fx_RPName(playerid, n, sizeof n);
		format(m, sizeof m, "%s a quitte la faction", n);
		Fp_Log(fac, m);
		Fac_Set(playerid, FACTION_NONE, 1);
		Fp_Close(playerid, 1);
		return Cw_Notify(playerid, "info", "Faction", "Tu as quitte ta faction.", 4000), 1;
	}
	else return 1;
	// la page se ferme pour laisser voir le resultat (GPS, MDT, notifications)
	Fp_Close(playerid, 1);
	return OnPlayerCommandText(playerid, c), 1;
}

stock Fp_Action(playerid, const params[])
{
	new idx = 0, act[12], arg[80], fac = Fac_Get(playerid), now = GetTickCount();
	Bank_NextToken(params, idx, act, sizeof act);
	while (params[idx] == ' ') idx++;
	format(arg, sizeof arg, "%s", params[idx]);
	if (!strcmp(act, "close")) { g_FpOpen[playerid] = 0; return 1; }
	if (fac == FACTION_NONE) return Fp_Close(playerid, 1);
	if (g_FpLast[playerid] && now - g_FpLast[playerid] < 400) return 1;
	g_FpLast[playerid] = (now == 0) ? 1 : now;
	new mg = Fp_IsMg(playerid), me[MAX_PLAYER_NAME + 1], m[140], a[16];
	Fx_RPName(playerid, me, sizeof me);
	if (!strcmp(act, "refresh") || !strcmp(act, "view"))
	{
		if (!strcmp(arg, "members") || !strcmp(arg, "events")) return Fp_Send(playerid, arg), 1;
		return Fp_Send(playerid, "dash"), 1;
	}
	if (!strcmp(act, "tool")) return Fp_Tool(playerid, arg);
	if (!strcmp(act, "deposit") || !strcmp(act, "withdraw"))
	{
		new v = strval(arg), bal = Fp_Bal(fac);
		if (v <= 0 || v > 10000000) return Fp_Send(playerid, "dash", 0, "Montant invalide."), 1;
		if (!strcmp(act, "deposit"))
		{
			if (GetPlayerMoney(playerid) < v) return Fp_Send(playerid, "dash", 0, "Tu n'as pas assez de liquide."), 1;
			GivePlayerMoney(playerid, -v);
			Fp_SetBal(fac, bal + v);
			Cw_Fmt(v, a, sizeof a);
			format(m, sizeof m, "%s a depose %s $ au coffre", me, a);
		}
		else
		{
			if (!mg) return Fp_Send(playerid, "dash", 0, "Retrait reserve aux grades de gestion."), 1;
			if (bal < v) return Fp_Send(playerid, "dash", 0, "Le coffre n'a pas assez d'argent."), 1;
			Fp_SetBal(fac, bal - v);
			GivePlayerMoney(playerid, v);
			Cw_Fmt(v, a, sizeof a);
			format(m, sizeof m, "%s a retire %s $ du coffre", me, a);
		}
		Fp_Log(fac, m);
		return Fp_Send(playerid, "dash", 1, m), 1;
	}
	if (!strcmp(act, "promote") || !strcmp(act, "demote") || !strcmp(act, "kick"))
	{
		if (!mg) return Fp_Send(playerid, "members", 0, "Reserve aux grades de gestion."), 1;
		new nm[MAX_PLAYER_NAME + 1];
		if (!Frp_SafeName(arg, nm, sizeof nm)) return 1;
		new rk = Fp_MemberRank(fac, nm), myr = g_FacRank[playerid], ld = Fp_IsLd(playerid) || Frp_Staff(playerid), rc = Fac_RankCount(fac);
		new mine[MAX_PLAYER_NAME + 1];
		GetPlayerName(playerid, mine, sizeof mine);
		if (!rk) return Fp_Send(playerid, "members", 0, "Ce joueur n'est pas dans la faction."), 1;
		if (!strcmp(nm, mine, true)) return Fp_Send(playerid, "members", 0, "Pas sur toi-meme."), 1;
		if (!ld && rk >= myr) return Fp_Send(playerid, "members", 0, "Son grade est egal ou superieur au tien."), 1;
		new sp[MAX_PLAYER_NAME + 1], rn[32];
		Frp_Spaces(nm, sp, sizeof sp);
		if (!strcmp(act, "kick"))
		{
			Fp_SetRankByName(fac, nm, 0);
			new t = Cw_PlayerByName(nm);
			if (t != INVALID_PLAYER_ID) Cw_Notify(t, "denied", "Faction", "Tu as ete renvoye de ta faction.", 6000);
			format(m, sizeof m, "%s a renvoye %s", me, sp);
		}
		else
		{
			new nr = rk + (!strcmp(act, "promote") ? 1 : -1);
			if (nr < 1 || nr > rc) return Fp_Send(playerid, "members", 0, "Grade impossible."), 1;
			if (!ld && nr >= myr) return Fp_Send(playerid, "members", 0, "Tu ne peux pas monter quelqu'un a ton grade."), 1;
			Fp_SetRankByName(fac, nm, nr);
			Fac_RankName(fac, nr, rn, sizeof rn);
			new t = Cw_PlayerByName(nm);
			if (t != INVALID_PLAYER_ID) { format(m, sizeof m, "Nouveau grade : ~b~%s~w~.", rn); Cw_Notify(t, "info", "Faction", m, 6000); }
			format(m, sizeof m, "%s : %s passe %s", me, sp, rn);
		}
		Fp_Log(fac, m);
		return Fp_Send(playerid, "members", 1, m), 1;
	}
	if (!strcmp(act, "invite"))
	{
		if (!mg) return Fp_Send(playerid, "members", 0, "Reserve aux grades de gestion."), 1;
		new t = strval(arg);
		if (t < 0 || t >= MAX_PLAYERS || !IsPlayerConnected(t) || !g_AuthLogged[t] || Fac_Get(t) != FACTION_NONE || !Int_Near(playerid, t, 12.0)) return Fp_Send(playerid, "members", 0, "Joueur introuvable, trop loin ou deja dans une faction."), 1;
		new fn[48], body[300];
		Fac_Name(fac, fn, sizeof fn);
		format(body, sizeof body, "%s vous invite a rejoindre '{%06x}%s{FFFFFF}'.\n\nVoulez-vous accepter ?", me, (g_FxColor[fac] >>> 8) & 0xFFFFFF, fn);
		ShowPlayerDialog(t, FACX_DIALOG_INVITE, DIALOG_STYLE_MSGBOX, "Invitation de faction", body, "Accepter", "Refuser");
		g_FxOffer[t] = playerid;
		g_FxOfferFac[t] = fac;
		return Fp_Send(playerid, "members", 1, "Invitation envoyee."), 1;
	}
	if (!strcmp(act, "evadd"))
	{
		if (!mg) return Fp_Send(playerid, "events", 0, "Reserve aux grades de gestion."), 1;
		new j = 0, ts = strval(arg), title[60], tc[60], q[240], au[MAX_PLAYER_NAME + 1];
		while (arg[j] && arg[j] != ' ') j++;
		while (arg[j] == ' ') j++;
		format(title, sizeof title, "%s", arg[j]);
		Law_Clean(title, tc, sizeof tc);
		if (strlen(tc) < 3 || ts < gettime() - 3600) return Fp_Send(playerid, "events", 0, "Titre ou date invalide."), 1;
		GetPlayerName(playerid, au, sizeof au);
		format(q, sizeof q, "INSERT INTO faction_events (fac, ts, title, author) VALUES (%d, %d, '%s', '%s')", fac, ts, tc, au);
		Fp_Exec(q);
		format(m, sizeof m, "[FACTION] Nouvel evenement : %s (panneau de faction > Calendrier)", tc);
		Fx_SendFaction(fac, FACX_COL_FAC, m);
		return Fp_Send(playerid, "events", 1, "Evenement ajoute."), 1;
	}
	if (!strcmp(act, "evdel"))
	{
		if (!mg) return 1;
		new q[120];
		format(q, sizeof q, "DELETE FROM faction_events WHERE id = %d AND fac = %d", strval(arg), fac);
		Fp_Exec(q);
		return Fp_Send(playerid, "events", 1, "Evenement supprime."), 1;
	}
	return 1;
}

// ============================================================
//  4) COMMANDES TAPEES : le client envoie "/tc /commande" pour ce que le joueur tape dans le chat.
//     Joueurs : seulement /aide (le reste passe par les menus tactiles, la tablette et les pages).
//     Staff : toutes les commandes. /slashjoueurs on|off (staff) rouvre les commandes a tout le monde.
// ============================================================
stock Tc_Allowed(const cmd[])
{
	static const ok[][] = {"/aide", "/help", "/commandes", "/cmds"};
	new n = 0;
	while (cmd[n] && cmd[n] != ' ') n++;
	for (new i = 0; i < sizeof ok; i++) if (strlen(ok[i]) == n && !strcmp(cmd, ok[i], true, n)) return 1;
	return 0;
}

stock Tc_Handle(playerid, const cmdtext[])
{
	if (strcmp(cmdtext, "/tc ", true, 4)) return 0;
	new cmd[144];
	format(cmd, sizeof cmd, "%s", cmdtext[4]);
	if (cmd[0] != '/') return 1;
	if (!strcmp(cmd, "/tc", true, 3) && (cmd[3] == ' ' || !cmd[3])) return 1;
	if (g_SlashOpen || Admin_Level(playerid) >= 1 || Tc_Allowed(cmd)) return OnPlayerCommandText(playerid, cmd), 1;
	Cw_Notify(playerid, "info", "Commandes", "Les commandes sont remplacees par le tactile : touche un joueur, un vehicule, la main bleue, ou ouvre la ~b~tablette RP~w~. Liste : ~b~/aide~w~.", 7000);
	return 1;
}

// /aide : toucher une ligne lance la commande (sans taper)
stock Aide_RunLine(playerid, const t[], n)
{
	new line = -1, i = 0, start = 0;
	for (; t[i]; i++)
	{
		if (t[i] != '\n') continue;
		line++;
		if (line == n) { start = i + 1; break; }
	}
	if (line != n) return 1;
	new col[96], j = 0, k = start;
	while (t[k] && t[k] != '\t' && t[k] != '\n' && j < sizeof(col) - 1)
	{
		if (t[k] == '{') { while (t[k] && t[k] != '}') k++; if (t[k]) k++; continue; }
		col[j++] = t[k++];
	}
	col[j] = 0;
	if (col[0] != '/') return Cw_Notify(playerid, "info", "Aide", "Ce n'est pas une commande : fais-le en touchant (joueur, vehicule, main bleue).", 5000), 1;
	if (strfind(col, "<") != -1) return Cw_Notify(playerid, "info", "Aide", "Cette action demande un joueur ou une valeur : touche le joueur / ouvre la tablette RP.", 5000), 1;
	new cmd[48], c = 0;
	while (col[c] && col[c] != ' ' && c < sizeof(cmd) - 1) { cmd[c] = col[c]; c++; }
	cmd[c] = 0;
	return OnPlayerCommandText(playerid, cmd), 1;
}
