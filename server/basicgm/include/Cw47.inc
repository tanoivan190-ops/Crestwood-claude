/* ============================================================
   CRESTWOOD RP v47 (23/11/2026) - INTERACTIONS DE FACTION EN PLUS (menu quand on touche un joueur /
   un vehicule ; tout est au toucher, pas de commande a taper)
   ------------------------------------------------------------
   Tout le monde : pousser un vehicule (moteur coupe).
   Police / FBI / armee : alcootest, consulter le casier, saisir les armes (menotte),
                          mettre dans le vehicule (menotte), verbaliser (page amende).
   EMS : bilan de sante, antidouleur (200 $, il accepte), charger dans l'ambulance (inconscient).
   Gang : ligoter / detacher (mains en l'air ou inconscient), racket (300 $ au coffre du gang s'il
          accepte), recruter dans le gang (lieutenant / chef).
   Presse : interview. Justice / gouvernement : convocation officielle.
   Mecaniciens (vehicule) : diagnostic, faire le plein (300 $).
   Branche : Int_PlayerItems / Int_VehItems / Int_UiAction / Int_Accept / Int_ReqLabel (Cw41.inc).
   ============================================================ */
#define REQ_HAND             20
#define REQ_HUG              21
#define REQ_TREAT            22
#define REQ_RACKET           23
#define REQ_GANGINV          24
#define REQ_INTERVIEW        25
#define C47_TREAT_PRICE      200
#define C47_RACKET_SUM       300
#define C47_FUEL_PRICE       300

new g_C47RacketCd[MAX_PLAYERS];

stock Cw47_ReqLabel(type, out[], size)
{
	switch (type)
	{
		case REQ_HAND:      format(out, size, "Serrer la main");
		case REQ_HUG:       format(out, size, "Calin");
		case REQ_TREAT:     format(out, size, "Antidouleur (200 $)");
		case REQ_RACKET:    format(out, size, "Payer la protection (300 $)");
		case REQ_GANGINV:   format(out, size, "Rejoindre le gang");
		case REQ_INTERVIEW: format(out, size, "Interview pour la presse");
		default: return 0;
	}
	return 1;
}

stock Cw47_PlayerItems(playerid, target, txt[], size)
{
	new t = Fac_Type(playerid);
	if (Fac_IsLaw(playerid))
	{
		strcat(txt, ";p_breath:Alcootest;p_record:Consulter le casier;p_fine:Verbaliser", size);
		if (g_PolCuffed[target]) strcat(txt, ";p_seize:Saisir les armes;p_car:Mettre dans le v\xe9hicule", size);
	}
	if (t == FACTION_MEDIC)
	{
		strcat(txt, ";m_check:Bilan de sant\xe9;m_pain:Antidouleur (200 $)", size);
		if (g_CwInjured[target] == 2) strcat(txt, ";m_load:Charger dans l'ambulance", size);
	}
	if (t == FACTION_NEWS) strcat(txt, ";n_itw:Interviewer", size);
	if (t == FACTION_JUSTICE || t == FACTION_GOVERNMENT) strcat(txt, ";j_summon:Convoquer officiellement", size);
	new g = g_GangOf[playerid];
	if (g >= 1 && g <= GANG_COUNT && !Fac_IsLaw(playerid))
	{
		strcat(txt, g_PolCuffed[target] ? (";g_tie:D\xe9tacher") : (";g_tie:Ligoter"), size);
		if (g_GangOf[target] != g) strcat(txt, ";g_racket:Racketter (300 $)", size);
		if (g_GangRank[playerid] >= 3 && g_GangOf[target] == 0) strcat(txt, ";g_invite:Recruter dans le gang", size);
	}
	return 1;
}

stock Cw47_VehItems(playerid, v, txt[], size)
{
	new engine, lights, alarm, doors, bonnet, boot, objective;
	GetVehicleParamsEx(v, engine, lights, alarm, doors, bonnet, boot, objective);
	if (engine != VEHICLE_PARAMS_ON && GetPlayerState(playerid) == PLAYER_STATE_ONFOOT) strcat(txt, "push:Pousser le v\xe9hicule;", size);
	if (Fac_Type(playerid) == FACTION_MECHANIC) strcat(txt, "m_diag:Diagnostic;m_fuel:Faire le plein (300 $);", size);
	return 1;
}

// action d'un menu de vehicule (cible = vehicule le plus proche)
stock Cw47_NearVeh(playerid, Float:r)
{
	new Float:x, Float:y, Float:z, best = INVALID_VEHICLE_ID, Float:bd = r;
	GetPlayerPos(playerid, x, y, z);
	for (new v = 1; v < MAX_VEHICLES; v++)
	{
		if (!IsValidVehicle(v)) continue;
		new Float:d = GetVehicleDistanceFromPoint(v, x, y, z);
		if (d < bd) { bd = d; best = v; }
	}
	return best;
}

stock Cw47_FreeBackSeat(v)
{
	new used[4];
	for (new p = 0, m = GetMaxPlayers(); p < m; p++)
		if (IsPlayerConnected(p) && GetPlayerVehicleID(p) == v) { new s = GetPlayerVehicleSeat(p); if (s >= 0 && s < 4) used[s] = 1; }
	if (!used[2]) return 2;
	if (!used[3]) return 3;
	if (!used[1]) return 1;
	return -1;
}

stock Cw47_UiAction(playerid, const action[])
{
	new t = g_UiTarget[playerid], m[180], an[MAX_PLAYER_NAME + 1], tn[MAX_PLAYER_NAME + 1];
	// ---------- vehicule ----------
	if (!strcmp(action, "push", true) || !strcmp(action, "m_diag", true) || !strcmp(action, "m_fuel", true))
	{
		new v = Cw47_NearVeh(playerid, 6.0);
		if (v == INVALID_VEHICLE_ID) return Ui_Toast(playerid, 0, "Aucun vehicule a cote."), 1;
		Fx_RPName(playerid, an, sizeof an);
		if (!strcmp(action, "push", true))
		{
			new Float:a, Float:vx, Float:vy, Float:vz;
			GetPlayerFacingAngle(playerid, a);
			GetVehicleVelocity(v, vx, vy, vz);
			SetVehicleVelocity(v, vx + 0.09 * floatsin(-a, degrees), vy + 0.09 * floatcos(-a, degrees), vz);
			ApplyAnimation(playerid, "GANGS", "shake_cara", 4.1, 0, 0, 0, 0, 0, 1);
			format(m, sizeof m, "* %s pousse le vehicule.", an);
			return RpX_Near(playerid, 15.0, INT_COL, m), 1;
		}
		if (Fac_Type(playerid) != FACTION_MECHANIC) return 1;
		if (!strcmp(action, "m_diag", true))
		{
			new Float:hp;
			GetVehicleHealth(v, hp);
			new pct = floatround((hp - 250.0) / 7.5);
			if (pct < 0) pct = 0;
			if (pct > 100) pct = 100;
			format(m, sizeof m, "Diagnostic : moteur %d %% - carburant %d %%. %s", pct, floatround(g_CwFuel[v]),
				pct < 40 ? ("Reparation conseillee.") : ("Rien d'urgent."));
			Cw_Notify(playerid, "info", "Mecanique", m, 6000);
			ApplyAnimation(playerid, "CAR", "Fixn_Car_Loop", 4.1, 0, 0, 0, 0, 2500, 1);
			format(m, sizeof m, "* %s inspecte le moteur.", an);
			return RpX_Near(playerid, 15.0, INT_COL, m), 1;
		}
		if (GetPlayerMoney(playerid) < C47_FUEL_PRICE) return Ui_Toast(playerid, 0, "Le jerrican coute 300 $."), 1;
		GivePlayerMoney(playerid, -C47_FUEL_PRICE);
		g_CwFuel[v] = 100.0;
		ApplyAnimation(playerid, "BD_FIRE", "wash_up", 4.1, 0, 0, 0, 0, 2500, 1);
		format(m, sizeof m, "* %s fait le plein du vehicule avec un jerrican.", an);
		return RpX_Near(playerid, 15.0, INT_COL, m), 1;
	}

	// ---------- joueur ----------
	if (!strcmp(action, "hand", true)) return Int_Ask(playerid, t, REQ_HAND), 1;
	if (!strcmp(action, "hug", true)) return Int_Ask(playerid, t, REQ_HUG), 1;
	if (!strcmp(action, "n_itw", true)) return (Fac_Type(playerid) == FACTION_NEWS) ? Int_Ask(playerid, t, REQ_INTERVIEW) : 1, 1;
	if (!strcmp(action, "m_pain", true)) return (Fac_Type(playerid) == FACTION_MEDIC) ? Int_Ask(playerid, t, REQ_TREAT) : 1, 1;
	if (!strcmp(action, "g_racket", true))
	{
		if (g_GangOf[playerid] < 1) return 1;
		if (g_C47RacketCd[playerid] > gettime()) return Ui_Toast(playerid, 0, "Attends un peu avant de racketter a nouveau."), 1;
		if (Gz_At(playerid) != -1) return Ui_Toast(playerid, 0, "Impossible en zone verte."), 1;
		g_C47RacketCd[playerid] = gettime() + 300;
		return Int_Ask(playerid, t, REQ_RACKET), 1;
	}
	if (!strcmp(action, "g_invite", true))
	{
		if (g_GangOf[playerid] < 1 || g_GangRank[playerid] < 3) return Ui_Toast(playerid, 0, "Reserve aux lieutenants et au chef."), 1;
		if (g_GangOf[t] != 0) return Ui_Toast(playerid, 0, "Cette personne est deja dans un gang."), 1;
		return Int_Ask(playerid, t, REQ_GANGINV), 1;
	}
	if (t == INVALID_PLAYER_ID || !IsPlayerConnected(t)) return 0;
	Fx_RPName(playerid, an, sizeof an);
	Fx_RPName(t, tn, sizeof tn);

	if (!strcmp(action, "p_breath", true) || !strcmp(action, "p_record", true) || !strcmp(action, "p_fine", true)
		|| !strcmp(action, "p_seize", true) || !strcmp(action, "p_car", true))
	{
		if (!Fac_IsLaw(playerid)) return 1;
		if (!strcmp(action, "p_record", true))
		{
			new nm[MAX_PLAYER_NAME + 1];
			GetPlayerName(t, nm, sizeof nm);
			return Jus_ShowRecord(playerid, nm), 1;
		}
		if (!strcmp(action, "p_fine", true)) return CallLocalFunction("OnPlayerCommandText", "is", playerid, "/pv"), 1;
		if (!Int_Near(playerid, t, INT_RANGE)) return Ui_Toast(playerid, 0, "Rapproche-toi."), 1;
		if (!strcmp(action, "p_breath", true))
		{
			new lvl = GetPlayerDrunkLevel(t);
			new Float:g = float(lvl) / 5000.0;
			if (g > 2.5) g = 2.5;
			ApplyAnimation(playerid, "COP_AMBIENT", "Coplook_think", 4.1, 0, 0, 0, 0, 2000, 1);
			format(m, sizeof m, "* %s fait souffler %s dans l'ethylotest : %.2f g/L%s", an, tn, g, g >= 0.5 ? (" - POSITIF.") : (" - negatif."));
			return RpX_Near(playerid, 15.0, INT_COL, m), 1;
		}
		if (!g_PolCuffed[t]) return Ui_Toast(playerid, 0, "Le suspect doit etre menotte."), 1;
		if (!strcmp(action, "p_seize", true))
		{
			ResetPlayerWeapons(t);
			Jus_RecordId(t, "saisie", "Armes saisies lors d'un controle", playerid);
			format(m, sizeof m, "* %s saisit les armes de %s.", an, tn);
			return RpX_Near(playerid, 15.0, INT_COL, m), 1;
		}
		new v = Cw47_NearVeh(playerid, 7.0);
		if (v == INVALID_VEHICLE_ID) return Ui_Toast(playerid, 0, "Aucun vehicule a cote."), 1;
		new seat = Cw47_FreeBackSeat(v);
		if (seat < 0) return Ui_Toast(playerid, 0, "Le vehicule est plein."), 1;
		Pol_StopDragOf(t);
		PutPlayerInVehicle(t, v, seat);
		format(m, sizeof m, "* %s fait monter %s a l'arriere du vehicule.", an, tn);
		return RpX_Near(playerid, 15.0, INT_COL, m), 1;
	}
	if (!strcmp(action, "m_check", true) || !strcmp(action, "m_load", true))
	{
		if (Fac_Type(playerid) != FACTION_MEDIC) return 1;
		if (!Int_Near(playerid, t, INT_RANGE + 1.0)) return Ui_Toast(playerid, 0, "Rapproche-toi du patient."), 1;
		if (!strcmp(action, "m_check", true))
		{
			new Float:hp, Float:ar;
			GetPlayerHealth(t, hp);
			GetPlayerArmour(t, ar);
			format(m, sizeof m, "Bilan de %s : sante %d %%, gilet %d %%, %s.", tn, floatround(hp), floatround(ar),
				g_CwInjured[t] == 2 ? ("INCONSCIENT : reanimer") : (hp < 40.0 ? ("blesse : soigner") : ("stable")));
			Cw_Notify(playerid, "info", "EMS", m, 7000);
			ApplyAnimation(playerid, "MEDIC", "CPR", 4.1, 0, 0, 0, 0, 2500, 1);
			format(m, sizeof m, "* %s examine %s.", an, tn);
			return RpX_Near(playerid, 15.0, INT_COL, m), 1;
		}
		if (g_CwInjured[t] != 2) return Ui_Toast(playerid, 0, "Le patient n'est pas inconscient."), 1;
		new v = Cw47_NearVeh(playerid, 8.0);
		if (v == INVALID_VEHICLE_ID) return Ui_Toast(playerid, 0, "Approche l'ambulance."), 1;
		new seat = Cw47_FreeBackSeat(v);
		if (seat < 0) return Ui_Toast(playerid, 0, "Le vehicule est plein."), 1;
		PutPlayerInVehicle(t, v, seat);
		format(m, sizeof m, "* %s installe %s sur le brancard dans l'ambulance.", an, tn);
		return RpX_Near(playerid, 15.0, INT_COL, m), 1;
	}
	if (!strcmp(action, "j_summon", true))
	{
		new ft = Fac_Type(playerid);
		if (ft != FACTION_JUSTICE && ft != FACTION_GOVERNMENT) return 1;
		format(m, sizeof m, "%s te remet une convocation officielle. Presente-toi a la mairie / au tribunal.", an);
		Cw_Notify(t, "warning", "Convocation", m, 9000);
		format(m, sizeof m, "* %s remet une convocation officielle a %s.", an, tn);
		return RpX_Near(playerid, 15.0, INT_COL, m), 1;
	}
	if (!strcmp(action, "g_tie", true))
	{
		if (g_GangOf[playerid] < 1 || Fac_IsLaw(playerid)) return 1;
		if (!Int_Near(playerid, t, INT_RANGE)) return Ui_Toast(playerid, 0, "Rapproche-toi."), 1;
		if (g_PolCuffed[t])
		{
			Pol_Uncuff(t);
			format(m, sizeof m, "* %s detache les liens de %s.", an, tn);
			return RpX_Near(playerid, 15.0, INT_COL, m), 1;
		}
		if (GetPlayerSpecialAction(t) != SPECIAL_ACTION_HANDSUP && g_CwInjured[t] != 2)
			return Ui_Toast(playerid, 0, "Il faut qu'il ait les mains en l'air (ou qu'il soit a terre)."), 1;
		if (Gz_At(playerid) != -1) return Ui_Toast(playerid, 0, "Impossible en zone verte."), 1;
		Pol_Cuff(t);
		format(m, sizeof m, "* %s ligote les mains de %s.", an, tn);
		return RpX_Near(playerid, 15.0, INT_COL, m), 1;
	}
	return 0;
}

// proposition acceptee : a = celui qui propose, b = celui qui accepte
stock Cw47_Accept(a, b, type)
{
	new an[MAX_PLAYER_NAME + 1], bn[MAX_PLAYER_NAME + 1], m[180];
	Fx_RPName(a, an, sizeof an);
	Fx_RPName(b, bn, sizeof bn);
	switch (type)
	{
		case REQ_HAND:
		{
			Int_Face(a, b);
			ApplyAnimation(a, "GANGS", "hndshkfa_swt", 4.1, 0, 0, 0, 0, 0, 1);
			ApplyAnimation(b, "GANGS", "hndshkfa_swt", 4.1, 0, 0, 0, 0, 0, 1);
			format(m, sizeof m, "* %s et %s se serrent la main.", an, bn);
		}
		case REQ_HUG:
		{
			Int_Face(a, b);
			ApplyAnimation(a, "GANGS", "hndshkba", 4.1, 0, 0, 0, 0, 0, 1);
			ApplyAnimation(b, "GANGS", "hndshkba", 4.1, 0, 0, 0, 0, 0, 1);
			format(m, sizeof m, "* %s et %s se prennent dans les bras.", an, bn);
		}
		case REQ_TREAT:
		{
			if (GetPlayerMoney(b) < C47_TREAT_PRICE) return Ui_Toast(b, 0, "Tu n'as pas 200 $ sur toi."), 1;
			GivePlayerMoney(b, -C47_TREAT_PRICE);
			GivePlayerMoney(a, C47_TREAT_PRICE);
			new Float:hp;
			GetPlayerHealth(b, hp);
			hp += 25.0;
			if (hp > 100.0) hp = 100.0;
			SetPlayerHealth(b, hp);
			ApplyAnimation(a, "MEDIC", "CPR", 4.1, 0, 0, 0, 0, 2000, 1);
			format(m, sizeof m, "* %s fait une injection d'antidouleur a %s.", an, bn);
		}
		case REQ_RACKET:
		{
			new g = g_GangOf[a];
			if (g < 1 || g > GANG_COUNT) return 1;
			if (GetPlayerMoney(b) < C47_RACKET_SUM) return Ui_Toast(b, 0, "Tu n'as pas 300 $ sur toi."), 1;
			GivePlayerMoney(b, -C47_RACKET_SUM);
			Gb_Add(g, C47_RACKET_SUM);
			Gs_Add(a, "gain", C47_RACKET_SUM);
			format(m, sizeof m, "%s a paye la protection : +%d $", bn, C47_RACKET_SUM);
			Gb_Log(g, m);
			ApplyAnimation(b, "DEALER", "shop_pay", 4.1, 0, 0, 0, 0, 0, 1);
			format(m, sizeof m, "* %s tend une liasse de billets a %s.", bn, an);
		}
		case REQ_GANGINV:
		{
			new g = g_GangOf[a];
			if (g < 1 || g > GANG_COUNT || g_GangOf[b] != 0) return 1;
			Gang_Set(b, g, 1);
			format(m, sizeof m, "Tu fais maintenant partie des %s. Tablette RP > Mon groupe.", g_GangName[g]);
			Cw_Notify(b, "success", g_GangName[g], m, 7000);
			format(m, sizeof m, "%s a recrute %s", an, bn);
			Gb_Log(g, m);
			format(m, sizeof m, "* %s et %s se checkent : %s rejoint le gang.", an, bn, bn);
		}
		case REQ_INTERVIEW:
		{
			Int_Face(a, b);
			ApplyAnimation(a, "GHANDS", "gsign2", 4.1, 0, 0, 0, 0, 0, 1);
			ApplyAnimation(b, "MISC", "Idle_Chat_02", 4.1, 0, 0, 0, 0, 0, 1);
			format(m, sizeof m, "* %s tend son micro a %s pour une interview.", an, bn);
		}
		default: return 0;
	}
	return RpX_Near(a, 15.0, INT_COL, m), 1;
}
