// ============================================================
//  [v30c] SYSTEMES DU GAMEMODE MTA (ECLIPSE) AVEC LEURS PAGES
// ------------------------------------------------------------
//  Pages : assets/ticket, assets/repair, assets/bodycam (SAMP.java, "pages generiques")
//    serveur -> client : ShowPlayerDialog(9970 ouvrir / 9971 mise a jour / 9972 fermer, "@page <nom>", JSON)
//    client -> serveur : /pageact <nom> <action> [arguments]   (commande cachee, envoyee par l'appli)
//
//  1) AMENDES POLICE (EC-PDTicket)
//     /ticket   (police, FBI, armee) : joueurs a moins de 7 m + 17 motifs avec leur prix (max 10 000 $).
//               Paiement tout de suite en liquide (l'agent touche TK_OFFICER_PCT %) ; si le joueur n'a pas
//               assez sur lui, l'amende est enregistree (a payer plus tard). 3 minutes entre deux amendes.
//     /amendes  (tout le monde) : liste des amendes a payer + bouton payer / tout payer.
//  2) REPARATION AUTO (autorepair) : garage d'Idlewood 2063.9, -1831.5. On y entre en vehicule -> page.
//     2 500 $, 24 secondes, vehicule bloque. Refuse si un mecanicien (faction) est en ville (REP_BLOCK_IF_MECH).
//     /reparation pres du garage pour rouvrir la page.
//  3) BODYCAM (EC-Bodycam) : /bodycam (forces de l'ordre) ; bureau au commissariat (2059.8, -1887.2).
//     Chaque agent allume / eteint sa camera. Les deux plus hauts grades (et le staff niv. 4+) voient la liste
//     et regardent la camera d'un agent ; /bodycam stop pour revenir.
//  4) ANIMATIONS DU MTA (EC-Anim) : 144 animations dans la mini page du HUD (/animq m1 ... m144).
//     [v32] toutes jouees avec les animations du jeu (les fichiers .ifp perso du MTA faisaient planter).
// ============================================================

#define PAGE_DLG_OPEN       9970
#define PAGE_DLG_UPDATE     9971
#define PAGE_DLG_CLOSE      9972

#define TK_MAX_DIST         7.0
#define TK_COOLDOWN_S       180
#define TK_OFFICER_PCT      50          // part de l'amende pour l'agent (MTA : 100)
#define TK_REASONS          17

#define REP_X               2063.9092
#define REP_Y               -1831.5459
#define REP_Z               13.5469
#define REP_RANGE           6.0
#define REP_PRICE           2500
#define REP_TIME_S          24
#define REP_BLOCK_IF_MECH   1

#define BC_X                2059.76
#define BC_Y                -1887.19
#define BC_Z                13.55
#define BC_ADMIN_LVL        4

new const g_TkReason[TK_REASONS][32] =
{
	"Stationnement interdit", "Pas de ceinture", "Pas de clignotant", "Feu rouge grille", "Phares eteints",
	"Exces de vitesse", "Conduite dangereuse", "Conduite sans permis", "Refus d'obtemperer", "Delit de fuite",
	"Activite de gang", "Munitions illegales", "Arme exhibee", "Possession de drogue", "Arme illegale",
	"Tir sur la voie publique", "Menace armee"
};
new const g_TkPrice[TK_REASONS] = { 500, 750, 1000, 1500, 1750, 2500, 4000, 5000, 7500, 10000, 4500, 5000, 6500, 7000, 8000, 9000, 10000 };

new g_PgName[MAX_PLAYERS][10];         // page ouverte ("" = aucune)
new g_PgLast[MAX_PLAYERS];             // anti-spam des actions
new g_TkLast[MAX_PLAYERS];             // derniere amende donnee (gettime)
new g_RprBusy = INVALID_PLAYER_ID;     // joueur dont le vehicule est en reparation
new g_RprVeh;
new g_RprLeft;
new g_RprArmed[MAX_PLAYERS] = {1, ...};
new Text3D:g_RprLabel = Text3D:INVALID_3DTEXT_ID;
new Text3D:g_BcLabel = Text3D:INVALID_3DTEXT_ID;
new g_BcOn[MAX_PLAYERS];
new g_BcWatch[MAX_PLAYERS] = {INVALID_PLAYER_ID, ...};
new g_BcWatchVeh[MAX_PLAYERS];
new g_MtaAnimLast[MAX_PLAYERS];

forward Pg_Tick();

// ------------------------------------------------------------
//  Outils
// ------------------------------------------------------------
stock Pg_Send(playerid, dlg, const name[], const json[])
{
	new cap[24];
	format(cap, sizeof cap, "@page %s", name);
	if (dlg == PAGE_DLG_OPEN) format(g_PgName[playerid], 10, "%s", name);
	if (dlg == PAGE_DLG_CLOSE) g_PgName[playerid][0] = 0;
	return ShowPlayerDialog(playerid, dlg, DIALOG_STYLE_MSGBOX, cap, json, "OK", "");
}

stock Pg_IsOpen(playerid, const name[])
{
	// (strcmp renvoie 0 si une chaine est vide : on teste d'abord qu'aucune ne l'est)
	return g_PgName[playerid][0] != 0 && name[0] != 0 && !strcmp(g_PgName[playerid], name, true);
}

// nom lisible (Prenom_Nom -> Prenom Nom), sans caracteres genants pour le JSON
stock Pg_Name(playerid, out[], size)
{
	GetPlayerName(playerid, out, size);
	for (new i = 0; out[i]; i++)
	{
		if (out[i] == '_') out[i] = ' ';
		else if (out[i] == '"' || out[i] == '\\') out[i] = ' ';
	}
	return 1;
}

stock Pg_Esc(const in[], out[], size)
{
	new j = 0;
	for (new i = 0; in[i] != 0 && j < size - 1; i++)
	{
		new c = in[i];
		if (c == '"' || c == '\\' || c < 32) c = ' ';
		out[j++] = c;
	}
	out[j] = 0;
	return 1;
}

// ajoute ,"ok":x,"msg":"..." juste avant la derniere accolade
stock Pg_AddMsg(json[], size, ok, const msg[])
{
	if (!msg[0] && ok < 0) return 1;
	new l = strlen(json), m[150], part[180];
	if (l < 2 || json[l - 1] != '}') return 1;
	Pg_Esc(msg, m, sizeof m);
	json[l - 1] = 0;
	format(part, sizeof part, ",\"ok\":%d,\"msg\":\"%s\"}", ok, m);
	strcat(json, part, size);
	return 1;
}

// prochain mot de s a partir de *pos (separateurs : espaces)
stock Pg_Tok(const s[], &pos, out[], size)
{
	while (s[pos] == ' ') pos++;
	new j = 0;
	while (s[pos] != 0 && s[pos] != ' ' && j < size - 1) out[j++] = s[pos++];
	out[j] = 0;
	return j;
}

stock Pg_IsLeader(playerid)
{
	if (Admin_Level(playerid) >= BC_ADMIN_LVL) return 1;
	if (!Fac_IsLaw(playerid)) return 0;
	new fac = Fac_Get(playerid), top = Fac_RankCount(fac);
	return (g_FacRank[playerid] >= top - 1 && top > 1) || top <= 1;
}

// ------------------------------------------------------------
//  1) AMENDES
// ------------------------------------------------------------
stock Tk_BuildOfficer(playerid, json[], size)
{
	new me[MAX_PLAYER_NAME + 1], part[96], n = 0, left = 0;
	Pg_Name(playerid, me, sizeof me);
	if (g_TkLast[playerid] && gettime() - g_TkLast[playerid] < TK_COOLDOWN_S) left = TK_COOLDOWN_S - (gettime() - g_TkLast[playerid]);
	format(json, size, "{\"v\":\"officer\",\"me\":\"%s\",\"cd\":%d,\"max\":%.0f,\"near\":[", me, left, TK_MAX_DIST);
	new Float:x, Float:y, Float:z;
	GetPlayerPos(playerid, x, y, z);
	for (new i = 0, mx = GetMaxPlayers(); i < mx && n < 8; i++)
	{
		if (i == playerid || !IsPlayerConnected(i) || IsPlayerNPC(i) || !g_AuthLogged[i]) continue;
		if (GetPlayerVirtualWorld(i) != GetPlayerVirtualWorld(playerid)) continue;
		new Float:d = GetPlayerDistanceFromPoint(i, x, y, z);
		if (d > TK_MAX_DIST + 8.0) continue;
		new nm[MAX_PLAYER_NAME + 1];
		Pg_Name(i, nm, sizeof nm);
		format(part, sizeof part, "%s{\"id\":%d,\"n\":\"%s\",\"d\":%.1f,\"car\":%d}", n ? (",") : (""), i, nm, d, IsPlayerInAnyVehicle(i) ? 1 : 0);
		strcat(json, part, size);
		n++;
	}
	strcat(json, "],\"r\":[", size);
	for (new r = 0; r < TK_REASONS; r++)
	{
		format(part, sizeof part, "%s[\"%s\",%d,\"%s\"]", r ? (",") : (""), g_TkReason[r], g_TkPrice[r], r < 10 ? ("t") : ("g"));
		strcat(json, part, size);
	}
	strcat(json, "]}", size);
	return 1;
}

stock Tk_BuildCitizen(playerid, json[], size)
{
	new me[MAX_PLAYER_NAME + 1], name[MAX_PLAYER_NAME + 1], part[160], total = 0, n = 0;
	Pg_Name(playerid, me, sizeof me);
	C23_Name(playerid, name, sizeof name);
	format(json, size, "{\"v\":\"citizen\",\"me\":\"%s\",\"cash\":%d,\"fines\":[", me, GetPlayerMoney(playerid));
	if (g_C23DB != DB:0)
	{
		new q[200], f[16], rs[64], by[MAX_PLAYER_NAME + 1], rs2[64];
		format(q, sizeof q, "SELECT id, amount, reason, author, ts FROM c23_fines WHERE name = '%s' AND paid = 0 ORDER BY id DESC LIMIT 12", name);
		new DBResult:r = db_query(g_C23DB, q);
		if (r != DBResult:0)
		{
			new rows = db_num_rows(r);
			for (new i = 0; i < rows; i++)
			{
				db_get_field(r, 0, f, sizeof f); new id = strval(f);
				db_get_field(r, 1, f, sizeof f); new am = strval(f);
				db_get_field(r, 2, rs, sizeof rs);
				db_get_field(r, 3, by, sizeof by);
				db_get_field(r, 4, f, sizeof f); new ts = strval(f);
				Pg_Esc(rs, rs2, sizeof rs2);
				for (new k = 0; by[k]; k++) if (by[k] == '_' || by[k] == '"' || by[k] == '\\') by[k] = ' ';
				format(part, sizeof part, "%s{\"id\":%d,\"a\":%d,\"r\":\"%s\",\"by\":\"%s\",\"ago\":%d}", n ? (",") : (""), id, am, rs2, by, (gettime() - ts) / 60);
				strcat(json, part, size);
				total += am;
				n++;
				db_next_row(r);
			}
			db_free_result(r);
		}
	}
	format(part, sizeof part, "],\"total\":%d}", total);
	strcat(json, part, size);
	return 1;
}

stock Tk_OpenOfficer(playerid, dlg = PAGE_DLG_OPEN, ok = -1, const msg[] = "")
{
	new json[2600];
	Tk_BuildOfficer(playerid, json, sizeof json);
	Pg_AddMsg(json, sizeof json, ok, msg);
	return Pg_Send(playerid, dlg, "ticket", json);
}

stock Tk_OpenCitizen(playerid, dlg = PAGE_DLG_OPEN, ok = -1, const msg[] = "")
{
	new json[2600];
	Tk_BuildCitizen(playerid, json, sizeof json);
	Pg_AddMsg(json, sizeof json, ok, msg);
	return Pg_Send(playerid, dlg, "ticket", json);
}

stock Tk_Give(playerid, target, reason)
{
	if (!Fac_IsLaw(playerid)) return Tk_OpenCitizen(playerid, PAGE_DLG_UPDATE, 0, "Reserve aux forces de l'ordre.");
	if (reason < 0 || reason >= TK_REASONS) return Tk_OpenOfficer(playerid, PAGE_DLG_UPDATE, 0, "Choisis un motif.");
	if (target < 0 || target >= MAX_PLAYERS || target == playerid || !IsPlayerConnected(target) || !g_AuthLogged[target])
		return Tk_OpenOfficer(playerid, PAGE_DLG_UPDATE, 0, "Joueur introuvable.");
	new Float:x, Float:y, Float:z;
	GetPlayerPos(playerid, x, y, z);
	if (GetPlayerVirtualWorld(target) != GetPlayerVirtualWorld(playerid) || GetPlayerDistanceFromPoint(target, x, y, z) > TK_MAX_DIST)
		return Tk_OpenOfficer(playerid, PAGE_DLG_UPDATE, 0, "Trop loin : approche-toi a moins de 7 m.");
	new now = gettime();
	if (g_TkLast[playerid] && now - g_TkLast[playerid] < TK_COOLDOWN_S)
	{
		new m[80], left = TK_COOLDOWN_S - (now - g_TkLast[playerid]);
		format(m, sizeof m, "Attends encore %d min %02d s avant la prochaine amende.", left / 60, left % 60);
		return Tk_OpenOfficer(playerid, PAGE_DLG_UPDATE, 0, m);
	}
	new cost = g_TkPrice[reason], tn[MAX_PLAYER_NAME + 1], on[MAX_PLAYER_NAME + 1], a[16], m[160];
	Pg_Name(target, tn, sizeof tn);
	Pg_Name(playerid, on, sizeof on);
	Cw_Fmt(cost, a, sizeof a);
	g_TkLast[playerid] = now;
	if (GetPlayerMoney(target) >= cost)
	{
		GivePlayerMoney(target, -cost);
		Cw46_OnFine(playerid, cost);   // [v46] stats
		new share = cost * TK_OFFICER_PCT / 100;
		if (share > 0) GivePlayerMoney(playerid, share);
		new name[MAX_PLAYER_NAME + 1], by[MAX_PLAYER_NAME + 1], q[300];
		C23_Name(target, name, sizeof name);
		C23_Name(playerid, by, sizeof by);
		format(q, sizeof q, "INSERT INTO c23_fines (name, amount, reason, author, ts, paid) VALUES ('%s', %d, '%s', '%s', %d, 1)", name, cost, g_TkReason[reason], by, now);
		C23_Exec(q);
		format(m, sizeof m, "Amende de ~r~%s $~w~ payee : %s (agent %s).", a, g_TkReason[reason], on);
		Cw_Notify(target, "denied", "Police", m, 8000);
		format(m, sizeof m, "Amende de %s $ payee par %s (%s).", a, tn, g_TkReason[reason]);
		return Tk_OpenOfficer(playerid, PAGE_DLG_UPDATE, 1, m);
	}
	// pas assez de liquide : l'amende est enregistree (a payer avec /amendes)
	Fine_Give(playerid, target, cost, g_TkReason[reason]);
	Cw46_OnFine(playerid, cost);   // [v46] stats
	format(m, sizeof m, "%s n'a pas %s $ sur lui : amende enregistree.", tn, a);
	return Tk_OpenOfficer(playerid, PAGE_DLG_UPDATE, 1, m);
}

stock Tk_Action(playerid, const act[], const arg[])
{
	if (!strcmp(act, "refresh", true))
	{
		if (Fac_IsLaw(playerid) && (arg[0] == 0 || strcmp(arg, "citizen", true) != 0)) return Tk_OpenOfficer(playerid, PAGE_DLG_UPDATE);
		return Tk_OpenCitizen(playerid, PAGE_DLG_UPDATE);
	}
	if (!strcmp(act, "mine", true)) return Tk_OpenCitizen(playerid, PAGE_DLG_UPDATE);
	if (!strcmp(act, "give", true))
	{
		new pos = 0, t[8], r[8];
		Pg_Tok(arg, pos, t, sizeof t);
		Pg_Tok(arg, pos, r, sizeof r);
		if (!t[0] || !r[0]) return Tk_OpenOfficer(playerid, PAGE_DLG_UPDATE, 0, "Choisis un joueur et un motif.");
		return Tk_Give(playerid, strval(t), strval(r));
	}
	if (!strcmp(act, "pay", true))
	{
		if (!arg[0]) return 1;
		Fine_Pay(playerid, arg);
		return Tk_OpenCitizen(playerid, PAGE_DLG_UPDATE, -1, "");
	}
	if (!strcmp(act, "payall", true))
	{
		Fine_Pay(playerid, "tout");
		return Tk_OpenCitizen(playerid, PAGE_DLG_UPDATE, -1, "");
	}
	return 1;
}

// ------------------------------------------------------------
//  2) REPARATION AUTO
// ------------------------------------------------------------
stock Rpr_Mechanics()
{
	new n = 0;
	for (new i = 0, mx = GetMaxPlayers(); i < mx; i++)
	{
		if (IsPlayerConnected(i) && g_AuthLogged[i] && Fac_Type(i) == FACTION_MECHANIC) n++;
	}
	return n;
}

stock Rpr_Build(playerid, json[], size)
{
	new veh = GetPlayerVehicleID(playerid), vn[32], st[8], hp = 0, pct = 0, left = 0;
	format(vn, sizeof vn, "A pied");
	if (veh)
	{
		new Float:h;
		GetVehicleHealth(veh, h);
		hp = floatround(h);
		Gar_VehName(GetVehicleModel(veh), vn, sizeof vn);
	}
	format(st, sizeof st, "idle");
	if (g_RprBusy == playerid)
	{
		format(st, sizeof st, "work");
		left = g_RprLeft;
		pct = (REP_TIME_S - g_RprLeft) * 100 / REP_TIME_S;
	}
	else if (g_RprBusy != INVALID_PLAYER_ID) format(st, sizeof st, "busy");
	format(json, size, "{\"veh\":\"%s\",\"hp\":%d,\"price\":%d,\"time\":%d,\"mech\":%d,\"block\":%d,\"cash\":%d,\"state\":\"%s\",\"left\":%d,\"pct\":%d,\"near\":%d}",
		vn, hp, REP_PRICE, REP_TIME_S, Rpr_Mechanics(), REP_BLOCK_IF_MECH, GetPlayerMoney(playerid), st, left, pct,
		IsPlayerInRangeOfPoint(playerid, REP_RANGE + 4.0, REP_X, REP_Y, REP_Z) ? 1 : 0);
	return 1;
}

stock Rpr_Open(playerid, dlg = PAGE_DLG_OPEN, ok = -1, const msg[] = "")
{
	new json[600];
	Rpr_Build(playerid, json, sizeof json);
	Pg_AddMsg(json, sizeof json, ok, msg);
	return Pg_Send(playerid, dlg, "repair", json);
}

stock Rpr_Start(playerid)
{
	if (g_RprBusy == playerid) return Rpr_Open(playerid, PAGE_DLG_UPDATE, 0, "Reparation deja en cours.");
	if (g_RprBusy != INVALID_PLAYER_ID) return Rpr_Open(playerid, PAGE_DLG_UPDATE, 0, "Le garage est occupe, attends quelques secondes.");
	new veh = GetPlayerVehicleID(playerid);
	if (!veh || GetPlayerState(playerid) != PLAYER_STATE_DRIVER) return Rpr_Open(playerid, PAGE_DLG_UPDATE, 0, "Tu dois etre au volant du vehicule.");
	if (!IsPlayerInRangeOfPoint(playerid, REP_RANGE, REP_X, REP_Y, REP_Z)) return Rpr_Open(playerid, PAGE_DLG_UPDATE, 0, "Gare-toi dans le garage.");
	if (REP_BLOCK_IF_MECH && Rpr_Mechanics() > 0) return Rpr_Open(playerid, PAGE_DLG_UPDATE, 0, "Un mecanicien est en ville : appelle-le (/service).");
	new Float:h, panels, doors, lights, tires;
	GetVehicleHealth(veh, h);
	GetVehicleDamageStatus(veh, panels, doors, lights, tires);
	if (h >= 999.0 && !panels && !doors && !lights && !tires) return Rpr_Open(playerid, PAGE_DLG_UPDATE, 0, "Ton vehicule est deja en parfait etat.");
	if (GetPlayerMoney(playerid) < REP_PRICE) return Rpr_Open(playerid, PAGE_DLG_UPDATE, 0, "Pas assez de liquide sur toi (2 500 $).");
	GivePlayerMoney(playerid, -REP_PRICE);
	g_RprBusy = playerid;
	g_RprVeh = veh;
	g_RprLeft = REP_TIME_S;
	TogglePlayerControllable(playerid, 0);
	return Rpr_Open(playerid, PAGE_DLG_UPDATE, 1, "Reparation lancee.");
}

stock Rpr_Finish(ok)
{
	new p = g_RprBusy;
	g_RprBusy = INVALID_PLAYER_ID;
	if (p == INVALID_PLAYER_ID || !IsPlayerConnected(p)) return 1;
	TogglePlayerControllable(p, 1);
	if (ok && g_RprVeh)
	{
		RepairVehicle(g_RprVeh);
		SetVehicleHealth(g_RprVeh, 1000.0);
		Cw_Notify(p, "success", "Garage", "Vehicule repare. Bonne route !", 4500);
		if (Pg_IsOpen(p, "repair")) Rpr_Open(p, PAGE_DLG_UPDATE, 1, "Vehicule repare !");
	}
	g_RprVeh = 0;
	return 1;
}

stock Rpr_TickPlayer(playerid)
{
	// arrivee au garage en vehicule (conducteur) -> la page s'ouvre une fois ; re-armee quand on s'eloigne
	if (GetPlayerState(playerid) == PLAYER_STATE_DRIVER && IsPlayerInRangeOfPoint(playerid, REP_RANGE - 1.0, REP_X, REP_Y, REP_Z))
	{
		if (g_RprArmed[playerid] && !g_PgName[playerid][0])
		{
			g_RprArmed[playerid] = 0;
			Rpr_Open(playerid);
		}
	}
	else if (!IsPlayerInRangeOfPoint(playerid, REP_RANGE + 6.0, REP_X, REP_Y, REP_Z)) g_RprArmed[playerid] = 1;
	return 1;
}

stock Rpr_Action(playerid, const act[], const arg[])
{
	#pragma unused arg
	if (!strcmp(act, "start", true)) return Rpr_Start(playerid);
	if (!strcmp(act, "refresh", true)) return Rpr_Open(playerid, PAGE_DLG_UPDATE);
	return 1;
}

// ------------------------------------------------------------
//  3) BODYCAM
// ------------------------------------------------------------
stock Float:GetPlayerHealthF(playerid)
{
	new Float:h;
	GetPlayerHealth(playerid, h);
	return h;
}

stock Bc_Build(playerid, json[], size)
{
	new me[MAX_PLAYER_NAME + 1], part[160], lead = Pg_IsLeader(playerid), rk[32], fn[40];
	Pg_Name(playerid, me, sizeof me);
	new fac = Fac_Get(playerid);
	Fac_RankName(fac, g_FacRank[playerid], rk, sizeof rk);
	Fac_Name(fac, fn, sizeof fn);
	Pg_Esc(rk, rk, sizeof rk);
	Pg_Esc(fn, fn, sizeof fn);
	format(json, size, "{\"me\":\"%s\",\"rank\":\"%s\",\"fac\":\"%s\",\"on\":%d,\"lead\":%d,\"watch\":%d,\"list\":[", me, rk, fn, g_BcOn[playerid], lead,
		g_BcWatch[playerid] == INVALID_PLAYER_ID ? -1 : g_BcWatch[playerid]);
	if (lead)
	{
		new n = 0, admin = Admin_Level(playerid) >= BC_ADMIN_LVL;
		for (new i = 0, mx = GetMaxPlayers(); i < mx && n < 24; i++)
		{
			if (i == playerid || !IsPlayerConnected(i) || !g_AuthLogged[i] || !Fac_IsLaw(i)) continue;
			if (!admin && Fac_Get(i) != fac) continue;
			new nm[MAX_PLAYER_NAME + 1], r2[32], Float:x, Float:y, Float:z, zone[4];
			Pg_Name(i, nm, sizeof nm);
			Fac_RankName(Fac_Get(i), g_FacRank[i], r2, sizeof r2);
			Pg_Esc(r2, r2, sizeof r2);
			GetPlayerPos(i, x, y, z);
			format(zone, sizeof zone, "%s", IsPlayerInAnyVehicle(i) ? ("car") : ("ped"));
			format(part, sizeof part, "%s{\"id\":%d,\"n\":\"%s\",\"rk\":\"%s\",\"cam\":%d,\"st\":\"%s\",\"hp\":%d}", n ? (",") : (""), i, nm, r2, g_BcOn[i], zone, floatround(GetPlayerHealthF(i)));
			strcat(json, part, size);
			n++;
		}
	}
	strcat(json, "]}", size);
	return 1;
}

stock Bc_Open(playerid, dlg = PAGE_DLG_OPEN, ok = -1, const msg[] = "")
{
	new json[3000];
	Bc_Build(playerid, json, sizeof json);
	Pg_AddMsg(json, sizeof json, ok, msg);
	return Pg_Send(playerid, dlg, "bodycam", json);
}

stock Bc_Spectate(playerid, target)
{
	SetPlayerInterior(playerid, GetPlayerInterior(target));
	SetPlayerVirtualWorld(playerid, GetPlayerVirtualWorld(target));
	// [v33] position de la camera (et du streaming) pres de l'agent, sinon rien ne s'affiche s'il est loin
	new Float:tx, Float:ty, Float:tz;
	GetPlayerPos(target, tx, ty, tz);
	SetPlayerCameraPos(playerid, tx, ty, tz + 3.0);
	SetPlayerCameraLookAt(playerid, tx, ty, tz);
	if (IsPlayerInAnyVehicle(target))
	{
		g_BcWatchVeh[playerid] = GetPlayerVehicleID(target);
		PlayerSpectateVehicle(playerid, g_BcWatchVeh[playerid]);
	}
	else
	{
		g_BcWatchVeh[playerid] = 0;
		PlayerSpectatePlayer(playerid, target);
	}
	return 1;
}

stock Bc_Watch(playerid, target)
{
	if (!Pg_IsLeader(playerid)) return Bc_Open(playerid, PAGE_DLG_UPDATE, 0, "Reserve aux chefs (deux plus hauts grades).");
	if (target < 0 || target >= MAX_PLAYERS || target == playerid || !IsPlayerConnected(target)) return Bc_Open(playerid, PAGE_DLG_UPDATE, 0, "Agent introuvable.");
	if (!g_BcOn[target]) return Bc_Open(playerid, PAGE_DLG_UPDATE, 0, "La camera de cet agent est eteinte.");
	if (Admin_Level(playerid) < BC_ADMIN_LVL && Fac_Get(target) != Fac_Get(playerid)) return Bc_Open(playerid, PAGE_DLG_UPDATE, 0, "Agent d'une autre faction.");
	if (IsPlayerInAnyVehicle(playerid) && g_BcWatch[playerid] == INVALID_PLAYER_ID) return Bc_Open(playerid, PAGE_DLG_UPDATE, 0, "Descends du vehicule d'abord.");
	if (g_BcWatch[playerid] == INVALID_PLAYER_ID)
	{
		if (!g_AdmSpec[playerid])
		{
			GetPlayerPos(playerid, g_AdmSpecPos[playerid][0], g_AdmSpecPos[playerid][1], g_AdmSpecPos[playerid][2]);
			g_AdmSpecInt[playerid] = GetPlayerInterior(playerid);
			g_AdmSpecWorld[playerid] = GetPlayerVirtualWorld(playerid);
		}
		g_AdmSpec[playerid] = 1;
		TogglePlayerSpectating(playerid, 1);
	}
	g_BcWatch[playerid] = target;
	Bc_Spectate(playerid, target);
	// [v33] le client a besoin d'un court instant apres TogglePlayerSpectating : on recadre 2 fois
	SetTimerEx("Bc_SpecAgain", 300, false, "dd", playerid, target);
	SetTimerEx("Bc_SpecAgain", 1200, false, "dd", playerid, target);
	new tn[MAX_PLAYER_NAME + 1], m[120];
	Pg_Name(target, tn, sizeof tn);
	format(m, sizeof m, "Camera de %s. Tape /bodycam stop (ou rouvre /bodycam) pour revenir.", tn);
	Cw_Notify(playerid, "info", "Bodycam", m, 6000);
	Pg_Send(playerid, PAGE_DLG_CLOSE, "bodycam", "{}");
	return 1;
}

// [v33] position de "vue" d'un joueur qui regarde une bodycam (le streamer charge les objets autour de l'agent)
stock Bc_ViewPos(playerid, &Float:x, &Float:y, &Float:z)
{
	if (playerid < 0 || playerid >= MAX_PLAYERS) return 0;
	new t = g_BcWatch[playerid];
	if (t == INVALID_PLAYER_ID || !IsPlayerConnected(t)) return 0;
	GetPlayerPos(t, x, y, z);
	return 1;
}

forward Bc_SpecAgain(playerid, target);
public Bc_SpecAgain(playerid, target)
{
	if (!IsPlayerConnected(playerid) || g_BcWatch[playerid] != target || !IsPlayerConnected(target)) return 0;
	return Bc_Spectate(playerid, target);
}

stock Bc_Stop(playerid, notify = 1)
{
	if (g_BcWatch[playerid] == INVALID_PLAYER_ID) return 0;
	g_BcWatch[playerid] = INVALID_PLAYER_ID;
	g_BcWatchVeh[playerid] = 0;
	if (IsPlayerConnected(playerid))
	{
		g_AdmSpec[playerid] = 2;   // position restauree au respawn (Staff_OnSpawn)
		TogglePlayerSpectating(playerid, 0);
		if (notify) Cw_Notify(playerid, "info", "Bodycam", "Retour a ta position.", 3000);
	}
	return 1;
}

// arrete tous ceux qui regardent "target"
stock Bc_StopWatchersOf(target)
{
	for (new i = 0, mx = GetMaxPlayers(); i < mx; i++)
	{
		if (g_BcWatch[i] == target) Bc_Stop(i);
	}
	return 1;
}

stock Bc_Toggle(playerid, on)
{
	if (!Fac_IsLaw(playerid)) return Cw_Notify(playerid, "denied", "Bodycam", "Reserve aux forces de l'ordre.", 3000), 1;
	g_BcOn[playerid] = on ? 1 : 0;
	if (!on) Bc_StopWatchersOf(playerid);
	if (on) Cw_Notify(playerid, "success", "Bodycam", "Camera allumee : ta hierarchie peut te suivre.", 3500);
	else Cw_Notify(playerid, "info", "Bodycam", "Camera eteinte.", 3500);
	if (Pg_IsOpen(playerid, "bodycam")) Bc_Open(playerid, PAGE_DLG_UPDATE);
	return 1;
}

stock Bc_TickPlayer(playerid)
{
	new t = g_BcWatch[playerid];
	if (t == INVALID_PLAYER_ID) return 1;
	if (!IsPlayerConnected(t) || !g_BcOn[t] || !Fac_IsLaw(t)) return Bc_Stop(playerid);
	new v = IsPlayerInAnyVehicle(t) ? GetPlayerVehicleID(t) : 0;
	if (v != g_BcWatchVeh[playerid] || GetPlayerInterior(playerid) != GetPlayerInterior(t) || GetPlayerVirtualWorld(playerid) != GetPlayerVirtualWorld(t)) Bc_Spectate(playerid, t);
	new tn[MAX_PLAYER_NAME + 1], m[80];
	Pg_Name(t, tn, sizeof tn);
	format(m, sizeof m, "~r~REC ~w~BODYCAM~n~~b~%s", tn);
	GameTextForPlayer(playerid, m, 1300, 4);
	return 1;
}

stock Bc_Action(playerid, const act[], const arg[])
{
	if (!strcmp(act, "toggle", true)) return Bc_Toggle(playerid, !g_BcOn[playerid]);
	if (!strcmp(act, "watch", true)) return Bc_Watch(playerid, strval(arg));
	if (!strcmp(act, "stop", true)) { Bc_Stop(playerid); return Bc_Open(playerid, PAGE_DLG_UPDATE); }
	if (!strcmp(act, "refresh", true)) return Bc_Open(playerid, PAGE_DLG_UPDATE);
	return 1;
}

stock Bc_Command(playerid, const arg[])
{
	if (arg[0] && !strcmp(arg, "stop", true)) { if (!Bc_Stop(playerid)) Cw_Notify(playerid, "info", "Bodycam", "Tu ne regardes aucune camera.", 3000); return 1; }
	if (!Fac_IsLaw(playerid) && Admin_Level(playerid) < BC_ADMIN_LVL) return Cw_Notify(playerid, "denied", "Bodycam", "Reserve aux forces de l'ordre.", 3000), 1;
	if (arg[0] && !strcmp(arg, "on", true)) return Bc_Toggle(playerid, 1);
	if (arg[0] && !strcmp(arg, "off", true)) return Bc_Toggle(playerid, 0);
	return Bc_Open(playerid);
}

// ------------------------------------------------------------
//  4) ANIMATIONS DU MTA (/animq m<n>)
// ------------------------------------------------------------
#include <MtaAnims>

stock MtaAnim_Play(playerid, const key[])
{
	if ((key[0] != 'm' && key[0] != 'M') || key[1] < '0' || key[1] > '9') return 0;
	new i = strval(key[1]) - 1;
	if (i < 0 || i >= MTA_ANIM_COUNT) return 0;
	if (GetPlayerState(playerid) != PLAYER_STATE_ONFOOT) return Cw_Notify(playerid, "denied", "Animations", "Descends du vehicule d'abord.", 3000), 1;
	if (GetPlayerSpecialAction(playerid) == SPECIAL_ACTION_CUFFED) return Cw_Notify(playerid, "denied", "Animations", "Impossible : tu es menotte.", 3000), 1;
	Rp_AnimStop(playerid);
	new fl = g_MtaAnimFlag[i], loop = fl & 1, upd = (fl & 2) ? 1 : 0;
	// non bouclee : la derniere image reste (comme setPedAnimation du MTA)
	ApplyAnimation(playerid, g_MtaAnim[i][0], g_MtaAnim[i][1], 4.1, loop, upd, upd, loop ? 0 : 1, 0, 1);
	format(g_RpCur[playerid], 12, "%s", key);
	// animation perso : le client charge le bloc "SNM" au 1er appel puis la joue au 2e
	if (fl & 4) SetTimerEx("MtaAnim_Again", 700, false, "dd", playerid, i);
	return 1;
}

forward MtaAnim_Again(playerid, i);
public MtaAnim_Again(playerid, i)
{
	if (!IsPlayerConnected(playerid) || i < 0 || i >= MTA_ANIM_COUNT) return 0;
	if (GetPlayerState(playerid) != PLAYER_STATE_ONFOOT) return 0;
	new key[8];
	format(key, sizeof key, "m%d", i + 1);
	if (!g_RpCur[playerid][0] || strcmp(g_RpCur[playerid], key, true) != 0) return 0;   // autre animation entre-temps
	new fl = g_MtaAnimFlag[i], loop = fl & 1, upd = (fl & 2) ? 1 : 0;
	ApplyAnimation(playerid, g_MtaAnim[i][0], g_MtaAnim[i][1], 4.1, loop, upd, upd, loop ? 0 : 1, 0, 1);
	return 1;
}

// ------------------------------------------------------------
//  Branchements
// ------------------------------------------------------------
stock Pg_OnInit()
{
	g_RprLabel = Create3DTextLabel("{FFB347}Garage auto\n{FFFFFF}Reparation 2 500 $", 0xFFFFFFFF, REP_X, REP_Y, REP_Z + 0.6, 7.0, 0, 1);
	g_BcLabel = Create3DTextLabel("{5DA9FF}Bodycam\n{FFFFFF}Forces de l'ordre", 0xFFFFFFFF, BC_X, BC_Y, BC_Z + 0.4, 5.0, 0, 1);
	SetTimer("Pg_Tick", 1000, true);
	return 1;
}

stock Pg_OnExit()
{
	if (g_RprLabel != Text3D:INVALID_3DTEXT_ID) Delete3DTextLabel(g_RprLabel);
	if (g_BcLabel != Text3D:INVALID_3DTEXT_ID) Delete3DTextLabel(g_BcLabel);
	return 1;
}

stock Pg_OnConnect(playerid)
{
	g_PgName[playerid][0] = 0;
	g_PgLast[playerid] = 0;
	g_TkLast[playerid] = 0;
	g_RprArmed[playerid] = 1;
	g_BcOn[playerid] = 1;   // [v33] camera allumee par defaut (un agent peut l'eteindre)
	g_BcWatch[playerid] = INVALID_PLAYER_ID;
	g_BcWatchVeh[playerid] = 0;
	g_MtaAnimLast[playerid] = 0;
	return 1;
}

stock Pg_OnDisconnect(playerid)
{
	if (g_RprBusy == playerid) { g_RprBusy = INVALID_PLAYER_ID; g_RprVeh = 0; }
	g_BcWatch[playerid] = INVALID_PLAYER_ID;
	g_BcOn[playerid] = 0;
	Bc_StopWatchersOf(playerid);
	g_PgName[playerid][0] = 0;
	return 1;
}

stock Pg_PushMarks()
{
	Mk_Add(2, REP_X, REP_Y, REP_Z - 0.95);    // icone garage
	Mk_Add(3, BC_X, BC_Y, BC_Z - 0.95);       // epingle : bureau bodycam
	return 1;
}

// touche d'interaction a pied (bureau bodycam) : 210
stock Pg_InteractKey(playerid)
{
	if (!g_AuthLogged[playerid]) return 0;
	if (IsPlayerInRangeOfPoint(playerid, 1.8, BC_X, BC_Y, BC_Z) && (Fac_IsLaw(playerid) || Admin_Level(playerid) >= BC_ADMIN_LVL)) return 210;
	return 0;
}

public Pg_Tick()
{
	// reparation en cours
	if (g_RprBusy != INVALID_PLAYER_ID)
	{
		new p = g_RprBusy;
		if (!IsPlayerConnected(p) || GetPlayerVehicleID(p) != g_RprVeh) { g_RprBusy = INVALID_PLAYER_ID; if (IsPlayerConnected(p)) TogglePlayerControllable(p, 1); g_RprVeh = 0; }
		else
		{
			g_RprLeft--;
			if (g_RprLeft <= 0) Rpr_Finish(1);
			else
			{
				new m[64];
				format(m, sizeof m, "~y~Reparation ~w~%d s", g_RprLeft);
				GameTextForPlayer(p, m, 1200, 4);
				if (Pg_IsOpen(p, "repair")) Rpr_Open(p, PAGE_DLG_UPDATE);
			}
		}
	}
	for (new i = 0, mx = GetMaxPlayers(); i < mx; i++)
	{
		if (!IsPlayerConnected(i) || !g_AuthLogged[i]) continue;
		Rpr_TickPlayer(i);
		Bc_TickPlayer(i);
	}
	return 1;
}

stock Pg_OnCommand(playerid, const cmdtext[])
{
	if (!g_AuthLogged[playerid]) return 0;
	new a;
	if ((a = Cw_Arg(cmdtext, "/pageact")) != -1)
	{
		new now = GetTickCount();
		if (g_PgLast[playerid] && now - g_PgLast[playerid] < 300) return 1;
		g_PgLast[playerid] = now;
		new pos = a, page[12], act[14];
		Pg_Tok(cmdtext, pos, page, sizeof page);
		Pg_Tok(cmdtext, pos, act, sizeof act);
		while (cmdtext[pos] == ' ') pos++;
		if (!page[0] || !act[0]) return 1;
		if (!strcmp(act, "close", true)) { if (Pg_IsOpen(playerid, page)) g_PgName[playerid][0] = 0; return 1; }
		if (!Pg_IsOpen(playerid, page)) return 1;
		if (!strcmp(page, "ticket", true)) return Tk_Action(playerid, act, cmdtext[pos]);
		if (!strcmp(page, "repair", true)) return Rpr_Action(playerid, act, cmdtext[pos]);
		if (!strcmp(page, "bodycam", true)) return Bc_Action(playerid, act, cmdtext[pos]);
		if (Cw44_PageAction(playerid, page, act, cmdtext[pos])) return 1;   // [v44] casino, bourse, evenements
		if (Cw46_PageAction(playerid, page, act, cmdtext[pos])) return 1;   // [v46] mon groupe
		return 1;
	}
	if (!strcmp(cmdtext, "/ticket", true) || !strcmp(cmdtext, "/pv", true))
	{
		if (!Fac_IsLaw(playerid)) return Cw_Notify(playerid, "denied", "Police", "Reserve aux forces de l'ordre. Tes amendes : /amendes", 3500), 1;
		return Tk_OpenOfficer(playerid);
	}
	if (!strcmp(cmdtext, "/amendes", true) || !strcmp(cmdtext, "/mesamendes", true)) return Tk_OpenCitizen(playerid);
	if (!strcmp(cmdtext, "/amendestexte", true)) return Fine_List(playerid);
	if (!strcmp(cmdtext, "/reparation", true))
	{
		if (!IsPlayerInRangeOfPoint(playerid, REP_RANGE + 6.0, REP_X, REP_Y, REP_Z))
		{
			SetPlayerCheckpoint(playerid, REP_X, REP_Y, REP_Z, 4.0);
			return Cw_Notify(playerid, "info", "Garage", "Garage auto d'Idlewood marque sur la carte.", 4000), 1;
		}
		return Rpr_Open(playerid);
	}
	if ((a = Cw_Arg(cmdtext, "/bodycam")) != -1) return Bc_Command(playerid, cmdtext[a]);
	return 0;
}
