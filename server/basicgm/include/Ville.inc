/* ============================================================
   VILLE RP (v19b, 07/11/2026) - Crestwood RP
   ------------------------------------------------------------
   1) COURSES DE RUE (une a la fois sur le serveur)
      /course creer <mise>   au volant : le depart est la ou tu es (mise 500 a 500 000 $)
      /course arrivee        l'organisateur roule jusqu'a l'arrivee et la fixe (400 m minimum)
      /course rejoindre      au volant, pres du depart : la mise est payee en liquide
      /course depart         l'organisateur lance le 3-2-1 (2 pilotes minimum)
      /course quitter | /course annuler (organisateur, rembourse tout le monde) | /course = etat
      Le premier a l'arrivee gagne toute la cagnotte. Sans gagnant en 8 min : chacun est rembourse.
   2) MARIAGE A LA MAIRIE
      /mariage <id> devant la mairie (cote a cote) -> l'autre repond /oui ou /non (60 s).
      10 000 $ payes par celui qui demande. /divorce a la mairie (5 000 $). /examiner affiche l'epoux.
   Base : table "mariages" dans crestwood_vie.db (module Vie). Inclus apres <Vie>.
   ============================================================ */
#define RC_MIN_BET            500
#define RC_MAX_BET            500000
#define RC_MAX_PILOTS         8
#define RC_JOIN_DIST          40.0
#define RC_START_DIST         60.0
#define RC_MIN_LEN            400.0
#define RC_OPEN_TIMEOUT       600         // secondes avant annulation d'une course qui ne part pas
#define RC_RUN_TIMEOUT        480         // secondes de course maximum
#define RC_COL                0xFF9D3CFF

#define WED_COST              10000
#define WED_DIVORCE_COST      5000
#define WED_HALL_X            1481.0
#define WED_HALL_Y            -1745.0
#define WED_HALL_Z            13.5
#define WED_HALL_RANGE        45.0
#define WED_COL               0xF48FB1FF

new g_RcState;                     // 0 aucune, 1 inscriptions, 2 compte a rebours, 3 en course
new g_RcOwner = INVALID_PLAYER_ID;
new g_RcBet;
new g_RcPot;
new Float:g_RcStart[3];
new Float:g_RcFinish[3];
new g_RcHasFinish;
new g_RcIn[MAX_PLAYERS];
new g_RcSince;                     // gettime() du debut de l'etat courant
new g_RcCount;                     // compte a rebours
new g_RcOwnerName[MAX_PLAYER_NAME + 1];

new g_WedSpouse[MAX_PLAYERS][MAX_PLAYER_NAME + 1];
new g_WedAsk[MAX_PLAYERS] = {INVALID_PLAYER_ID, ...};   // [cible] = joueur qui demande
new g_WedAskAt[MAX_PLAYERS];
new g_WedLoaded[MAX_PLAYERS];
forward Ville_Tick();

stock Ville_OnInit()
{
	if (g_VieDB != DB:0)
		db_free_result(db_query(g_VieDB, "CREATE TABLE IF NOT EXISTS mariages (a TEXT PRIMARY KEY COLLATE NOCASE, b TEXT COLLATE NOCASE, ts INTEGER)"));
	SetTimer("Ville_Tick", 1000, true);
	print("[VILLE] Courses de rue et mariages prets.");
	return 1;
}

stock Ville_OnConnect(playerid)
{
	g_RcIn[playerid] = 0;
	g_WedSpouse[playerid][0] = 0;
	g_WedAsk[playerid] = INVALID_PLAYER_ID;
	g_WedAskAt[playerid] = 0;
	g_WedLoaded[playerid] = 0;
	return 1;
}

stock Ville_OnDisconnect(playerid)
{
	if (g_RcIn[playerid]) Rc_Leave(playerid, g_RcState == 1);
	if (g_RcOwner == playerid)
	{
		if (g_RcState == 1) Rc_Cancel("L'organisateur a quitte la ville : course annulee, mises remboursees.");
		else g_RcOwner = INVALID_PLAYER_ID;     // la course deja lancee continue sans lui
	}
	for (new p = 0, m = GetMaxPlayers(); p < m; p++) if (g_WedAsk[p] == playerid) g_WedAsk[p] = INVALID_PLAYER_ID;
	g_WedAsk[playerid] = INVALID_PLAYER_ID;
	return 1;
}

stock Ville_OnSpawn(playerid)
{
	if (!g_WedLoaded[playerid]) Wed_Load(playerid);
	return 1;
}

// ------------------------------------------------------------
//  Courses de rue
// ------------------------------------------------------------
stock Rc_Msg(const text[])
{
	for (new p = 0, m = GetMaxPlayers(); p < m; p++) if (IsPlayerConnected(p) && g_RcIn[p]) SendClientMessage(p, RC_COL, text);
	return 1;
}

stock Rc_Pilots()
{
	new n = 0;
	for (new p = 0, m = GetMaxPlayers(); p < m; p++) if (IsPlayerConnected(p) && g_RcIn[p]) n++;
	return n;
}

stock Rc_Reset()
{
	for (new p = 0, m = GetMaxPlayers(); p < m; p++)
	{
		if (!g_RcIn[p]) continue;
		g_RcIn[p] = 0;
		if (IsPlayerConnected(p))
		{
			DisablePlayerRaceCheckpoint(p);
			TogglePlayerControllable(p, 1);
		}
	}
	g_RcState = 0; g_RcOwner = INVALID_PLAYER_ID; g_RcBet = 0; g_RcPot = 0; g_RcHasFinish = 0; g_RcCount = 0;
	g_RcOwnerName[0] = 0;
	return 1;
}

// rembourse tous les pilotes encore inscrits puis remet a zero
stock Rc_Cancel(const why[])
{
	new share = 0, n = Rc_Pilots();
	if (n > 0) share = g_RcPot / n;
	for (new p = 0, m = GetMaxPlayers(); p < m; p++)
	{
		if (!IsPlayerConnected(p) || !g_RcIn[p]) continue;
		if (share > 0) GivePlayerMoney(p, share);
		SendClientMessage(p, RC_COL, why);
		Cw_Notify(p, "warning", "Course de rue", why);
	}
	return Rc_Reset();
}

stock Rc_Leave(playerid, refund)
{
	if (!g_RcIn[playerid]) return 0;
	g_RcIn[playerid] = 0;
	if (refund && g_RcBet > 0)
	{
		g_RcPot -= g_RcBet;
		if (g_RcPot < 0) g_RcPot = 0;
		if (IsPlayerConnected(playerid)) GivePlayerMoney(playerid, g_RcBet);
	}
	if (IsPlayerConnected(playerid))
	{
		DisablePlayerRaceCheckpoint(playerid);
		TogglePlayerControllable(playerid, 1);
	}
	return 1;
}

stock Rc_IsBusy(playerid)
{
	if (g_CjRun[playerid]) return 1;          // trajet d'emploi en cours (JobPro)
	if (g_Q[playerid] != Q_NONE) return 1;    // quete en cours (Npc)
	return 0;
}

stock Rc_Status(playerid)
{
	new m[170], a[16], fin[20];
	if (g_RcState == 0)
	{
		SendClientMessage(playerid, RC_COL, "Aucune course de rue. Au volant : /course creer <mise> (500 a 500 000 $).");
		return 1;
	}
	Cw_Fmt(g_RcPot, a, sizeof a);
	if (g_RcHasFinish) fin = "fixee"; else fin = "pas encore fixee";
	format(m, sizeof m, "Course de %s : %d pilote(s), cagnotte %s $, mise %d $, arrivee %s.",
		g_RcOwnerName, Rc_Pilots(), a, g_RcBet, fin);
	SendClientMessage(playerid, RC_COL, m);
	if (g_RcState == 1) SendClientMessage(playerid, RC_COL, "Pour participer : /course rejoindre au volant, pres du depart (point orange sur ta carte).");
	return 1;
}

stock Rc_Command(playerid, const arg[])
{
	new m[170], a[16];
	if (!arg[0]) return Rc_Status(playerid);
	if (!strcmp(arg, "creer", true, 5))
	{
		if (g_RcState != 0) return SendClientMessage(playerid, VIE_COL_ERR, "Une course est deja organisee : /course pour voir."), 1;
		if (GetPlayerState(playerid) != PLAYER_STATE_DRIVER) return SendClientMessage(playerid, VIE_COL_ERR, "Tu dois etre au volant pour organiser une course."), 1;
		if (Rc_IsBusy(playerid)) return SendClientMessage(playerid, VIE_COL_ERR, "Termine d'abord ta quete ou ton trajet de travail."), 1;
		new bet = strval(arg[5]);
		if (bet < RC_MIN_BET || bet > RC_MAX_BET) return SendClientMessage(playerid, VIE_COL_ERR, "/course creer <mise> : entre 500 et 500 000 $."), 1;
		if (GetPlayerMoney(playerid) < bet) return SendClientMessage(playerid, VIE_COL_ERR, "Tu n'as pas la mise en liquide."), 1;
		GetVehiclePos(GetPlayerVehicleID(playerid), g_RcStart[0], g_RcStart[1], g_RcStart[2]);
		GivePlayerMoney(playerid, -bet);
		g_RcState = 1; g_RcOwner = playerid; g_RcBet = bet; g_RcPot = bet; g_RcHasFinish = 0; g_RcSince = gettime();
		g_RcIn[playerid] = 1;
		RpX_Name(playerid, g_RcOwnerName, sizeof g_RcOwnerName);
		Cw_Notify(playerid, "success", "Course de rue", "Course creee. Roule jusqu'a l'~y~arrivee~w~ (400 m minimum) et tape ~b~/course arrivee~w~.", 9000);
		Cw_Fmt(bet, a, sizeof a);
		format(m, sizeof m, "[COURSE] %s organise une course de rue (mise %s $). Viens au volant et tape /course rejoindre.", g_RcOwnerName, a);
		for (new p = 0, mx = GetMaxPlayers(); p < mx; p++)
		{
			if (p == playerid || !IsPlayerConnected(p) || !g_AuthLogged[p]) continue;
			if (IsPlayerInRangeOfPoint(p, 250.0, g_RcStart[0], g_RcStart[1], g_RcStart[2])) SendClientMessage(p, RC_COL, m);
		}
		return 1;
	}
	if (!strcmp(arg, "arrivee", true))
	{
		if (g_RcState != 1 || g_RcOwner != playerid) return SendClientMessage(playerid, VIE_COL_ERR, "Seul l'organisateur fixe l'arrivee, avant le depart."), 1;
		new Float:x, Float:y, Float:z;
		GetPlayerPos(playerid, x, y, z);
		new Float:dx = x - g_RcStart[0], Float:dy = y - g_RcStart[1];
		if (floatsqroot(dx * dx + dy * dy) < RC_MIN_LEN) return SendClientMessage(playerid, VIE_COL_ERR, "L'arrivee doit etre a 400 m au moins du depart."), 1;
		g_RcFinish[0] = x; g_RcFinish[1] = y; g_RcFinish[2] = z; g_RcHasFinish = 1;
		format(m, sizeof m, "Arrivee fixee a ~y~%d m~w~ du depart. Retourne au depart, attends les pilotes puis ~b~/course depart~w~.", floatround(floatsqroot(dx * dx + dy * dy)));
		Cw_Notify(playerid, "success", "Course de rue", m, 9000);
		return 1;
	}
	if (!strcmp(arg, "rejoindre", true))
	{
		if (g_RcState != 1) return SendClientMessage(playerid, VIE_COL_ERR, "Aucune course n'attend de pilotes."), 1;
		if (g_RcIn[playerid]) return SendClientMessage(playerid, VIE_COL_ERR, "Tu es deja inscrit."), 1;
		if (GetPlayerState(playerid) != PLAYER_STATE_DRIVER) return SendClientMessage(playerid, VIE_COL_ERR, "Il faut etre au volant."), 1;
		if (!IsPlayerInRangeOfPoint(playerid, RC_JOIN_DIST, g_RcStart[0], g_RcStart[1], g_RcStart[2])) return SendClientMessage(playerid, VIE_COL_ERR, "Approche-toi du depart de la course."), 1;
		if (Rc_IsBusy(playerid)) return SendClientMessage(playerid, VIE_COL_ERR, "Termine d'abord ta quete ou ton trajet de travail."), 1;
		if (Rc_Pilots() >= RC_MAX_PILOTS) return SendClientMessage(playerid, VIE_COL_ERR, "La course est complete (8 pilotes)."), 1;
		if (GetPlayerMoney(playerid) < g_RcBet) return SendClientMessage(playerid, VIE_COL_ERR, "Tu n'as pas la mise en liquide."), 1;
		GivePlayerMoney(playerid, -g_RcBet);
		g_RcPot += g_RcBet;
		g_RcIn[playerid] = 1;
		new me[MAX_PLAYER_NAME + 1];
		RpX_Name(playerid, me, sizeof me);
		Cw_Fmt(g_RcPot, a, sizeof a);
		format(m, sizeof m, "[COURSE] %s rejoint la course. %d pilotes, cagnotte %s $.", me, Rc_Pilots(), a);
		Rc_Msg(m);
		return 1;
	}
	if (!strcmp(arg, "depart", true))
	{
		if (g_RcState != 1 || g_RcOwner != playerid) return SendClientMessage(playerid, VIE_COL_ERR, "Seul l'organisateur lance la course."), 1;
		if (!g_RcHasFinish) return SendClientMessage(playerid, VIE_COL_ERR, "Fixe d'abord l'arrivee : /course arrivee."), 1;
		// les pilotes trop loin ou a pied sont retires (et rembourses)
		for (new p = 0, mx = GetMaxPlayers(); p < mx; p++)
		{
			if (!IsPlayerConnected(p) || !g_RcIn[p]) continue;
			if (GetPlayerState(p) != PLAYER_STATE_DRIVER || !IsPlayerInRangeOfPoint(p, RC_START_DIST, g_RcStart[0], g_RcStart[1], g_RcStart[2]))
			{
				Rc_Leave(p, 1);
				SendClientMessage(p, RC_COL, "[COURSE] Tu n'etais pas au depart (au volant) : retire de la course, mise remboursee.");
			}
		}
		if (Rc_Pilots() < 2) return SendClientMessage(playerid, VIE_COL_ERR, "Il faut au moins 2 pilotes au depart."), 1;
		g_RcState = 2; g_RcCount = 3; g_RcSince = gettime();
		for (new p = 0, mx = GetMaxPlayers(); p < mx; p++) if (IsPlayerConnected(p) && g_RcIn[p]) TogglePlayerControllable(p, 0);
		Rc_Msg("[COURSE] Tout le monde en place... depart dans 3 secondes !");
		return 1;
	}
	if (!strcmp(arg, "quitter", true))
	{
		if (!g_RcIn[playerid]) return SendClientMessage(playerid, VIE_COL_ERR, "Tu n'es inscrit a aucune course."), 1;
		if (g_RcOwner == playerid && g_RcState == 1) return Rc_Cancel("L'organisateur a annule la course : mises remboursees."), 1;
		Rc_Leave(playerid, g_RcState == 1);
		if (g_RcState == 1) SendClientMessage(playerid, RC_COL, "[COURSE] Tu quittes la course, mise remboursee.");
		else SendClientMessage(playerid, RC_COL, "[COURSE] Tu abandonnes : ta mise reste dans la cagnotte.");
		if (g_RcState >= 2 && Rc_Pilots() == 0) Rc_Reset();
		return 1;
	}
	if (!strcmp(arg, "annuler", true))
	{
		if (g_RcState == 0) return SendClientMessage(playerid, VIE_COL_ERR, "Aucune course en cours."), 1;
		if (g_RcOwner != playerid && Admin_Level(playerid) < 2) return SendClientMessage(playerid, VIE_COL_ERR, "Seul l'organisateur (ou le staff) peut annuler."), 1;
		Rc_Cancel("Course annulee : chaque pilote recupere sa part de la cagnotte.");
		return 1;
	}
	SendClientMessage(playerid, RC_COL, "/course creer <mise> | arrivee | rejoindre | depart | quitter | annuler");
	return 1;
}

stock Ville_OnRaceCheckpoint(playerid)
{
	if (g_RcState != 3 || !g_RcIn[playerid]) return 0;
	if (!IsPlayerInRangeOfPoint(playerid, 16.0, g_RcFinish[0], g_RcFinish[1], g_RcFinish[2])) return 0;
	new me[MAX_PLAYER_NAME + 1], m[170], a[16], secs = gettime() - g_RcSince;
	RpX_Name(playerid, me, sizeof me);
	new pot = g_RcPot;
	Cw_Fmt(pot, a, sizeof a);
	GivePlayerMoney(playerid, pot);
	format(m, sizeof m, "[COURSE] %s gagne la course en %d:%02d et empoche %s $ !", me, secs / 60, secs % 60, a);
	for (new p = 0, mx = GetMaxPlayers(); p < mx; p++)
	{
		if (!IsPlayerConnected(p)) continue;
		if (g_RcIn[p] || IsPlayerInRangeOfPoint(p, 200.0, g_RcFinish[0], g_RcFinish[1], g_RcFinish[2])) SendClientMessage(p, RC_COL, m);
	}
	format(m, sizeof m, "~g~Victoire !~w~ Course en %d:%02d : ~y~%s $~w~ pour toi.", secs / 60, secs % 60, a);
	Cw_Notify(playerid, "success", "Course de rue", m, 8000);
	GameTextForPlayer(playerid, "~g~VICTOIRE", 3000, 3);
	PlayerPlaySound(playerid, 1149, 0.0, 0.0, 0.0);
	g_RcPot = 0;
	Rc_Reset();
	return 1;
}

public Ville_Tick()
{
	if (g_RcState == 0) return 1;
	new now = gettime();
	if (g_RcState == 1)
	{
		if (now - g_RcSince > RC_OPEN_TIMEOUT) Rc_Cancel("La course n'est jamais partie (10 min) : mises remboursees.");
		return 1;
	}
	if (g_RcState == 2)
	{
		new t[24];
		for (new p = 0, mx = GetMaxPlayers(); p < mx; p++)
		{
			if (!IsPlayerConnected(p) || !g_RcIn[p]) continue;
			if (g_RcCount > 0)
			{
				format(t, sizeof t, "~y~%d", g_RcCount);
				GameTextForPlayer(p, t, 1000, 3);
				PlayerPlaySound(p, 1056, 0.0, 0.0, 0.0);
			}
			else
			{
				GameTextForPlayer(p, "~g~GO !", 1500, 3);
				PlayerPlaySound(p, 1057, 0.0, 0.0, 0.0);
				TogglePlayerControllable(p, 1);
				SetPlayerRaceCheckpoint(p, 1, g_RcFinish[0], g_RcFinish[1], g_RcFinish[2], g_RcFinish[0], g_RcFinish[1], g_RcFinish[2], 12.0);
			}
		}
		if (g_RcCount <= 0) { g_RcState = 3; g_RcSince = now; }
		g_RcCount--;
		return 1;
	}
	// en course : les pilotes descendus de voiture ou morts abandonnent
	for (new p = 0, mx = GetMaxPlayers(); p < mx; p++)
	{
		if (!IsPlayerConnected(p) || !g_RcIn[p]) continue;
		if (GetPlayerState(p) != PLAYER_STATE_DRIVER)
		{
			Rc_Leave(p, 0);
			SendClientMessage(p, RC_COL, "[COURSE] Tu as quitte ton volant : abandon (ta mise reste dans la cagnotte).");
		}
	}
	if (Rc_Pilots() == 0) { g_RcPot = 0; Rc_Reset(); return 1; }
	if (now - g_RcSince > RC_RUN_TIMEOUT) Rc_Cancel("Personne n'a fini en 8 minutes : la cagnotte est partagee entre les pilotes.");
	return 1;
}

// ------------------------------------------------------------
//  Mariage
// ------------------------------------------------------------
stock Wed_Load(playerid)
{
	g_WedSpouse[playerid][0] = 0;
	if (g_VieDB == DB:0) return 0;
	g_WedLoaded[playerid] = 1;
	new name[MAX_PLAYER_NAME + 1], q[140];
	GetPlayerName(playerid, name, sizeof name);
	format(q, sizeof q, "SELECT b FROM mariages WHERE a = '%s'", name);
	new DBResult:r = db_query(g_VieDB, q);
	if (r == DBResult:0) return 0;
	if (db_num_rows(r) > 0) db_get_field(r, 0, g_WedSpouse[playerid], MAX_PLAYER_NAME + 1);
	db_free_result(r);
	return 1;
}

// nom de l'epoux (vide si celibataire) : utilise par /examiner (Vie.inc)
stock Ville_Spouse(playerid, out[], size)
{
	format(out, size, "%s", g_WedSpouse[playerid]);
	for (new i = 0; out[i]; i++) if (out[i] == '_') out[i] = ' ';
	return out[0] != 0;
}

stock Wed_AtHall(playerid)
{
	return IsPlayerInRangeOfPoint(playerid, WED_HALL_RANGE, WED_HALL_X, WED_HALL_Y, WED_HALL_Z);
}

stock Wed_Command(playerid, const cmdtext[])
{
	new a, m[170];
	if ((a = Cw_Arg(cmdtext, "/mariage")) != -1)
	{
		if (!cmdtext[a]) return SendClientMessage(playerid, WED_COL, "/mariage <id> : demande en mariage, devant la mairie (/mairie pour le GPS). 10 000 $."), 1;
		new t = strval(cmdtext[a]);
		if (t == playerid || !IsPlayerConnected(t) || !g_AuthLogged[t]) return SendClientMessage(playerid, VIE_COL_ERR, "Joueur introuvable."), 1;
		if (!Wed_AtHall(playerid) || !Wed_AtHall(t)) return Cw_Notify(playerid, "denied", "Mariage", "Les mariages se celebrent devant la ~b~mairie~w~ de Los Santos (/mairie pour le GPS)."), 1;
		new Float:x, Float:y, Float:z;
		GetPlayerPos(t, x, y, z);
		if (!IsPlayerInRangeOfPoint(playerid, 4.0, x, y, z)) return SendClientMessage(playerid, VIE_COL_ERR, "Mettez-vous cote a cote."), 1;
		if (!g_WedSpouse[playerid][0]) Wed_Load(playerid);
		if (!g_WedSpouse[t][0]) Wed_Load(t);
		if (g_WedSpouse[playerid][0]) return SendClientMessage(playerid, VIE_COL_ERR, "Tu es deja marie(e). (/divorce a la mairie)"), 1;
		if (g_WedSpouse[t][0]) return SendClientMessage(playerid, VIE_COL_ERR, "Cette personne est deja mariee."), 1;
		if (GetPlayerMoney(playerid) < WED_COST) return Cw_Notify(playerid, "denied", "Mariage", "La ceremonie coute ~y~10 000 $~w~ en liquide."), 1;
		g_WedAsk[t] = playerid;
		g_WedAskAt[t] = gettime();
		new me[MAX_PLAYER_NAME + 1], tn[MAX_PLAYER_NAME + 1];
		RpX_Name(playerid, me, sizeof me);
		RpX_Name(t, tn, sizeof tn);
		format(m, sizeof m, "~p~%s~w~ te demande en mariage ! Reponds ~g~/oui~w~ ou ~r~/non~w~ (60 secondes).", me);
		Cw_Notify(t, "admin", "Demande en mariage", m, 12000);
		format(m, sizeof m, "* %s pose un genou a terre devant %s et lui tend une bague.", me, tn);
		RpX_Near(playerid, 20.0, RPX_COL_ME, m);
		return 1;
	}
	if (!strcmp(cmdtext, "/oui", true) || !strcmp(cmdtext, "/non", true))
	{
		new from = g_WedAsk[playerid];
		if (from == INVALID_PLAYER_ID || !IsPlayerConnected(from) || gettime() - g_WedAskAt[playerid] > 60)
		{
			g_WedAsk[playerid] = INVALID_PLAYER_ID;
			return SendClientMessage(playerid, VIE_COL_ERR, "Personne ne t'a demande en mariage (ou la demande a expire)."), 1;
		}
		g_WedAsk[playerid] = INVALID_PLAYER_ID;
		new me[MAX_PLAYER_NAME + 1], fn[MAX_PLAYER_NAME + 1];
		RpX_Name(playerid, me, sizeof me);
		RpX_Name(from, fn, sizeof fn);
		if (!strcmp(cmdtext, "/non", true))
		{
			format(m, sizeof m, "* %s refuse la demande en mariage de %s.", me, fn);
			RpX_Near(playerid, 20.0, RPX_COL_ME, m);
			return Cw_Notify(from, "denied", "Mariage", "Ta demande en mariage a ete refusee."), 1;
		}
		if (!Wed_AtHall(playerid) || !Wed_AtHall(from)) return SendClientMessage(playerid, VIE_COL_ERR, "Vous devez etre tous les deux devant la mairie."), 1;
		if (GetPlayerMoney(from) < WED_COST) return Cw_Notify(from, "denied", "Mariage", "Il te manque les 10 000 $ de la ceremonie."), 1;
		if (g_VieDB == DB:0) return SendClientMessage(playerid, VIE_COL_ERR, "La mairie est fermee pour le moment."), 1;
		GivePlayerMoney(from, -WED_COST);
		new na[MAX_PLAYER_NAME + 1], nb[MAX_PLAYER_NAME + 1], q[200];
		GetPlayerName(from, na, sizeof na);
		GetPlayerName(playerid, nb, sizeof nb);
		format(q, sizeof q, "INSERT OR REPLACE INTO mariages (a, b, ts) VALUES ('%s', '%s', %d)", na, nb, gettime());
		db_free_result(db_query(g_VieDB, q));
		format(q, sizeof q, "INSERT OR REPLACE INTO mariages (a, b, ts) VALUES ('%s', '%s', %d)", nb, na, gettime());
		db_free_result(db_query(g_VieDB, q));
		format(g_WedSpouse[from], MAX_PLAYER_NAME + 1, "%s", nb);
		format(g_WedSpouse[playerid], MAX_PLAYER_NAME + 1, "%s", na);
		ApplyAnimation(from, "KISSING", "Playa_Kiss_02", 4.1, 0, 0, 0, 0, 0, 1);
		ApplyAnimation(playerid, "KISSING", "Playa_Kiss_01", 4.1, 0, 0, 0, 0, 0, 1);
		format(m, sizeof m, "[MAIRIE] %s et %s se sont maries ! Tous nos voeux de bonheur.", fn, me);
		SendClientMessageToAll(WED_COL, m);
		format(m, sizeof m, "Vous etes maintenant ~p~maries~w~. Felicitations a ~y~%s~w~ et ~y~%s~w~ !", fn, me);
		Cw_Notify(from, "success", "Mairie de Crestwood", m, 9000);
		Cw_Notify(playerid, "success", "Mairie de Crestwood", m, 9000);
		return 1;
	}
	if (!strcmp(cmdtext, "/divorce", true))
	{
		if (!g_WedSpouse[playerid][0]) Wed_Load(playerid);
		if (!g_WedSpouse[playerid][0]) return SendClientMessage(playerid, VIE_COL_ERR, "Tu n'es pas marie(e)."), 1;
		if (!Wed_AtHall(playerid)) return Cw_Notify(playerid, "denied", "Divorce", "Le divorce se signe a la ~b~mairie~w~ (/mairie pour le GPS)."), 1;
		if (GetPlayerMoney(playerid) < WED_DIVORCE_COST) return Cw_Notify(playerid, "denied", "Divorce", "Les frais de divorce sont de ~y~5 000 $~w~ en liquide."), 1;
		GivePlayerMoney(playerid, -WED_DIVORCE_COST);
		new na[MAX_PLAYER_NAME + 1], nb[MAX_PLAYER_NAME + 1], q[200];
		GetPlayerName(playerid, na, sizeof na);
		format(nb, sizeof nb, "%s", g_WedSpouse[playerid]);
		format(q, sizeof q, "DELETE FROM mariages WHERE a = '%s' OR a = '%s'", na, nb);
		if (g_VieDB != DB:0) db_free_result(db_query(g_VieDB, q));
		g_WedSpouse[playerid][0] = 0;
		new other = Cw_PlayerByName(nb);
		if (other != INVALID_PLAYER_ID)
		{
			g_WedSpouse[other][0] = 0;
			Cw_Notify(other, "warning", "Mairie de Crestwood", "Ton epoux / ton epouse a signe le divorce.", 8000);
		}
		Cw_Notify(playerid, "system", "Mairie de Crestwood", "Divorce prononce. Tu es de nouveau celibataire.");
		return 1;
	}
	return 0;
}

stock Ville_OnCommand(playerid, const cmdtext[])
{
	if (!g_AuthLogged[playerid]) return 0;
	new a;
	if ((a = Cw_Arg(cmdtext, "/course")) != -1) return Rc_Command(playerid, cmdtext[a]), 1;
	return Wed_Command(playerid, cmdtext);
}
