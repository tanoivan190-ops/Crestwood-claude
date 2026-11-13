/* ============================================================
   CRESTWOOD RP v21 (12/11/2026)
   ------------------------------------------------------------
   1) CAMERA DU TELEPHONE : modes Selfie et Drone (le serveur place la camera du jeu)
      "/phoneact cam <mode>" envoye par le HUD : back, selfie, drone, left, right, in, out, up, down, land, off.
      Drone : camera volante (120 m autour du joueur, qui reste immobile). Staff : portee illimitee et
      "Me poser" = le joueur apparait la ou est le drone (SetPlayerPosFindZ).
   2) /cadeau : recompense quotidienne, serie de 7 jours (500 $ -> 10 000 $). Un jour rate = serie remise a 1.
   3) /parrain Prenom_Nom : une seule fois, niveau 2 maximum. Le filleul recoit 2 500 $, le parrain 5 000 $ (banque).
   4) /sondage question (staff) + /voter oui|non : resultat a tous apres 60 s.
   5) Meteo vivante : change toute seule toutes les 25 min (soleil, nuages, pluie, brouillard). /meteo = meteo
      actuelle et suivante. Staff : /meteo auto | /meteo 0-45 (fige, comme le panneau admin).
   6) /afk : etiquette [AFK] au-dessus du personnage (retiree en reprenant).
   7) /mapdata on|off (staff 7) : active les mappings a modeles personnalises au redemarrage, sans recompiler.
   8) Magasins de vetements : places par le staff (panneau admin > Factions), main "Vetements" -> studio de style.
   9) Garage : depanneuse automatique (vehicule abandonne 30 min -> ramene au garage, etat sauvegarde).
   Base : crestwood_vie.db (module Vie). Inclus apres <Ville>. Texte ASCII.
   ============================================================ */
#define PCAM_OFF              0
#define PCAM_SELFIE           1
#define PCAM_DRONE            2
#define PCAM_DRONE_RANGE      120.0
#define PCAM_COL              0x6AA5FFFF

#define GIFT_COL              0x5CC85FFF
#define POLL_COL              0x9CC1FFFF
#define WX_PERIOD             1500        // secondes entre deux changements de meteo

new g_PcMode[MAX_PLAYERS];
new Float:g_PcAng[MAX_PLAYERS];
new Float:g_PcDist[MAX_PLAYERS];
new Float:g_DrPos[MAX_PLAYERS][3];
new Float:g_DrYaw[MAX_PLAYERS];
new Float:g_PcLast[MAX_PLAYERS][6];      // derniere position + visee (pour un mouvement fluide)

new g_PollOn, g_PollEnd, g_PollYes, g_PollNo, g_PollQ[96];
new g_PollVoted[MAX_PLAYERS];
new g_WxAuto = 1, g_WxNow = 1, g_WxNext = 2, g_WxAt;
new Text3D:g_AfkLabel[MAX_PLAYERS] = {Text3D:INVALID_3DTEXT_ID, ...};
forward Cw21_Tick();

stock Cw21_OnInit()
{
	if (g_VieDB != DB:0)
	{
		db_free_result(db_query(g_VieDB, "CREATE TABLE IF NOT EXISTS cadeau (name TEXT PRIMARY KEY COLLATE NOCASE, day INTEGER, streak INTEGER)"));
		db_free_result(db_query(g_VieDB, "CREATE TABLE IF NOT EXISTS parrains (filleul TEXT PRIMARY KEY COLLATE NOCASE, parrain TEXT COLLATE NOCASE, ts INTEGER)"));
	}
	if (g_VetCount == 0 && g_GarDB != DB:0)
	{
		// [v21] premier magasin de vetements devant la mairie (a 7 m du spawn), supprimable depuis le panneau
		db_free_result(db_query(g_GarDB, "INSERT INTO vetpoints (x, y, z) VALUES (1474.0, -1745.0, 13.55)"));
		Gar_Load();
	}
	Cs_BuildAll();
	g_WxNow = 1; g_WxNext = Wx_Pick(); g_WxAt = gettime() + WX_PERIOD;
	SetWeather(g_WxNow);
	SetTimer("Cw21_Tick", 1000, true);
	print("[CW21] Camera selfie/drone, cadeau, parrainage, sondages, meteo, AFK prets.");
	return 1;
}

stock Cw21_OnConnect(playerid)
{
	g_PcMode[playerid] = PCAM_OFF;
	g_PollVoted[playerid] = 0;
	g_AfkLabel[playerid] = Text3D:INVALID_3DTEXT_ID;
	return 1;
}

stock Cw21_OnDisconnect(playerid)
{
	g_PcMode[playerid] = PCAM_OFF;
	Afk_Clear(playerid);
	return 1;
}

stock Cw21_OnSpawn(playerid)
{
	if (g_PcMode[playerid] != PCAM_OFF) Pcam_Stop(playerid);
	Afk_Clear(playerid);
	return 1;
}

// ------------------------------------------------------------
//  1) Camera du telephone
// ------------------------------------------------------------
stock Pcam_Move(playerid, Float:cx, Float:cy, Float:cz, Float:lx, Float:ly, Float:lz, time = 260)
{
	if (g_PcLast[playerid][0] == 0.0 && g_PcLast[playerid][1] == 0.0)
	{
		SetPlayerCameraPos(playerid, cx, cy, cz);
		SetPlayerCameraLookAt(playerid, lx, ly, lz);
	}
	else
	{
		InterpolateCameraPos(playerid, g_PcLast[playerid][0], g_PcLast[playerid][1], g_PcLast[playerid][2], cx, cy, cz, time, CAMERA_MOVE);
		InterpolateCameraLookAt(playerid, g_PcLast[playerid][3], g_PcLast[playerid][4], g_PcLast[playerid][5], lx, ly, lz, time, CAMERA_MOVE);
	}
	g_PcLast[playerid][0] = cx; g_PcLast[playerid][1] = cy; g_PcLast[playerid][2] = cz;
	g_PcLast[playerid][3] = lx; g_PcLast[playerid][4] = ly; g_PcLast[playerid][5] = lz;
	return 1;
}

stock Pcam_Selfie(playerid)
{
	new Float:x, Float:y, Float:z, Float:a;
	GetPlayerPos(playerid, x, y, z);
	GetPlayerFacingAngle(playerid, a);
	a += g_PcAng[playerid];
	new Float:cx = x + g_PcDist[playerid] * floatsin(-a, degrees);
	new Float:cy = y + g_PcDist[playerid] * floatcos(-a, degrees);
	return Pcam_Move(playerid, cx, cy, z + 0.65, x, y, z + 0.55);
}

stock Pcam_Drone(playerid, time = 260)
{
	new Float:lx = g_DrPos[playerid][0] + 9.0 * floatsin(-g_DrYaw[playerid], degrees);
	new Float:ly = g_DrPos[playerid][1] + 9.0 * floatcos(-g_DrYaw[playerid], degrees);
	return Pcam_Move(playerid, g_DrPos[playerid][0], g_DrPos[playerid][1], g_DrPos[playerid][2], lx, ly, g_DrPos[playerid][2] - 6.0, time);
}

stock Pcam_Stop(playerid)
{
	if (g_PcMode[playerid] == PCAM_DRONE) TogglePlayerControllable(playerid, 1);
	g_PcMode[playerid] = PCAM_OFF;
	g_PcLast[playerid][0] = 0.0; g_PcLast[playerid][1] = 0.0;
	SetCameraBehindPlayer(playerid);
	return 1;
}

stock Pcam_Action(playerid, const mode[])
{
	if (!mode[0]) return 1;
	if (!strcmp(mode, "back", true) || !strcmp(mode, "off", true)) return Pcam_Stop(playerid);
	if (!strcmp(mode, "selfie", true))
	{
		if (g_PcMode[playerid] == PCAM_DRONE) TogglePlayerControllable(playerid, 1);
		g_PcMode[playerid] = PCAM_SELFIE;
		g_PcAng[playerid] = 0.0; g_PcDist[playerid] = 1.3;
		g_PcLast[playerid][0] = 0.0; g_PcLast[playerid][1] = 0.0;
		return Pcam_Selfie(playerid);
	}
	if (!strcmp(mode, "drone", true))
	{
		if (IsPlayerInAnyVehicle(playerid) && GetPlayerState(playerid) == PLAYER_STATE_DRIVER) return Cw_Notify(playerid, "denied", "Drone", "Arrete-toi avant de faire voler le drone."), 1;
		new Float:x, Float:y, Float:z, Float:a;
		GetPlayerPos(playerid, x, y, z);
		GetPlayerFacingAngle(playerid, a);
		g_PcMode[playerid] = PCAM_DRONE;
		g_DrYaw[playerid] = a;
		g_DrPos[playerid][0] = x - 5.0 * floatsin(-a, degrees);
		g_DrPos[playerid][1] = y - 5.0 * floatcos(-a, degrees);
		g_DrPos[playerid][2] = z + 9.0;
		g_PcLast[playerid][0] = x; g_PcLast[playerid][1] = y; g_PcLast[playerid][2] = z + 1.0;
		g_PcLast[playerid][3] = x; g_PcLast[playerid][4] = y; g_PcLast[playerid][5] = z;
		TogglePlayerControllable(playerid, 0);
		Pcam_Drone(playerid, 900);
		if (Admin_Level(playerid) >= 1) Cw_Notify(playerid, "admin", "Drone (staff)", "Portee illimitee. ~b~Me poser~w~ : tu apparais la ou est le drone.", 7000);
		else Cw_Notify(playerid, "info", "Drone", "Pilote avec les fleches, ~b~+~w~ / ~b~-~w~ pour avancer, haut / bas pour l'altitude. Portee : 120 m.", 7000);
		return 1;
	}
	if (g_PcMode[playerid] == PCAM_SELFIE)
	{
		if (!strcmp(mode, "left", true)) g_PcAng[playerid] -= 15.0;
		else if (!strcmp(mode, "right", true)) g_PcAng[playerid] += 15.0;
		else if (!strcmp(mode, "in", true)) { g_PcDist[playerid] -= 0.2; if (g_PcDist[playerid] < 0.7) g_PcDist[playerid] = 0.7; }
		else if (!strcmp(mode, "out", true)) { g_PcDist[playerid] += 0.25; if (g_PcDist[playerid] > 3.2) g_PcDist[playerid] = 3.2; }
		else return 1;
		return Pcam_Selfie(playerid);
	}
	if (g_PcMode[playerid] != PCAM_DRONE) return 1;
	if (!strcmp(mode, "land", true))
	{
		if (Admin_Level(playerid) < 1) return Cw_Notify(playerid, "denied", "Drone", "Se poser avec le drone est reserve au staff."), 1;
		new Float:dx = g_DrPos[playerid][0], Float:dy = g_DrPos[playerid][1], Float:dz = g_DrPos[playerid][2];
		Pcam_Stop(playerid);
		if (IsPlayerInAnyVehicle(playerid)) RemovePlayerFromVehicle(playerid);
		SetPlayerPosFindZ(playerid, dx, dy, dz);
		SetPlayerInterior(playerid, 0);
		Cw_Notify(playerid, "admin", "Drone (staff)", "Tu es pose a la position du drone.");
		return 1;
	}
	new Float:step = 4.0, Float:nx = g_DrPos[playerid][0], Float:ny = g_DrPos[playerid][1], Float:nz = g_DrPos[playerid][2];
	if (!strcmp(mode, "left", true)) g_DrYaw[playerid] += 12.0;
	else if (!strcmp(mode, "right", true)) g_DrYaw[playerid] -= 12.0;
	else if (!strcmp(mode, "in", true)) { nx += step * floatsin(-g_DrYaw[playerid], degrees); ny += step * floatcos(-g_DrYaw[playerid], degrees); }
	else if (!strcmp(mode, "out", true)) { nx -= step * floatsin(-g_DrYaw[playerid], degrees); ny -= step * floatcos(-g_DrYaw[playerid], degrees); }
	else if (!strcmp(mode, "up", true)) nz += 2.5;
	else if (!strcmp(mode, "down", true)) nz -= 2.5;
	else return 1;
	new Float:px, Float:py, Float:pz;
	GetPlayerPos(playerid, px, py, pz);
	if (nz < pz + 1.5) nz = pz + 1.5;
	if (Admin_Level(playerid) < 1)
	{
		new Float:ddx = nx - px, Float:ddy = ny - py;
		if (floatsqroot(ddx * ddx + ddy * ddy) > PCAM_DRONE_RANGE || nz > pz + 80.0)
		{
			GameTextForPlayer(playerid, "~r~Signal faible", 1200, 4);
			return Pcam_Drone(playerid);
		}
	}
	g_DrPos[playerid][0] = nx; g_DrPos[playerid][1] = ny; g_DrPos[playerid][2] = nz;
	return Pcam_Drone(playerid);
}

// ------------------------------------------------------------
//  2) Cadeau quotidien
// ------------------------------------------------------------
stock Gift_Amount(streak)
{
	switch (streak)
	{
		case 1: return 500;
		case 2: return 1000;
		case 3: return 1500;
		case 4: return 2500;
		case 5: return 3500;
		case 6: return 5000;
	}
	return 10000;
}

stock Gift_Command(playerid)
{
	if (g_VieDB == DB:0) return SendClientMessage(playerid, VIE_COL_ERR, "Cadeaux indisponibles pour le moment."), 1;
	new name[MAX_PLAYER_NAME + 1], q[180], today = gettime() / 86400, last = -1, streak = 0;
	GetPlayerName(playerid, name, sizeof name);
	format(q, sizeof q, "SELECT day, streak FROM cadeau WHERE name = '%s'", name);
	new DBResult:r = db_query(g_VieDB, q);
	if (r != DBResult:0)
	{
		if (db_num_rows(r) > 0)
		{
			new f[16];
			db_get_field(r, 0, f, sizeof f); last = strval(f);
			db_get_field(r, 1, f, sizeof f); streak = strval(f);
		}
		db_free_result(r);
	}
	if (last == today)
	{
		new h = 24 - ((gettime() % 86400) / 3600), m[140];
		format(m, sizeof m, "Cadeau deja recupere aujourd'hui. Reviens dans ~y~%d h~w~ (serie : ~b~%d jour(s)~w~).", h, streak);
		return Cw_Notify(playerid, "warning", "Cadeau quotidien", m), 1;
	}
	if (last == today - 1 && streak < 7) streak++;
	else streak = 1;
	new amount = Gift_Amount(streak), a[16], m[170];
	GivePlayerMoney(playerid, amount);
	format(q, sizeof q, "INSERT OR REPLACE INTO cadeau (name, day, streak) VALUES ('%s', %d, %d)", name, today, streak);
	db_free_result(db_query(g_VieDB, q));
	Cw_Fmt(amount, a, sizeof a);
	if (streak >= 7) format(m, sizeof m, "Jour ~b~7/7~w~ : ~g~%s $~w~ ! La serie recommence demain.", a);
	else format(m, sizeof m, "Jour ~b~%d/7~w~ : ~g~%s $~w~. Reviens demain pour plus (jour 7 : 10 000 $).", streak, a);
	Cw_Notify(playerid, "success", "Cadeau quotidien", m, 8000);
	if (streak >= 7) { new q2[120]; format(q2, sizeof q2, "UPDATE cadeau SET streak = 0 WHERE name = '%s'", name); db_free_result(db_query(g_VieDB, q2)); }
	return 1;
}

// ------------------------------------------------------------
//  3) Parrainage
// ------------------------------------------------------------
stock Ref_Command(playerid, const arg[])
{
	if (!arg[0]) return SendClientMessage(playerid, POLL_COL, "/parrain Prenom_Nom : la personne qui t'a fait venir (une seule fois, niveau 2 maximum). Toi 2 500 $, elle 5 000 $."), 1;
	if (g_VieDB == DB:0 || g_BankDB == DB:0) return SendClientMessage(playerid, VIE_COL_ERR, "Parrainage indisponible pour le moment."), 1;
	if (g_VieLvl[playerid] > 2) return Cw_Notify(playerid, "denied", "Parrainage", "Le parrainage est reserve aux nouveaux (niveau 2 maximum)."), 1;
	new len = strlen(arg);
	if (len < 3 || len > MAX_PLAYER_NAME) return SendClientMessage(playerid, VIE_COL_ERR, "Nom invalide (format Prenom_Nom)."), 1;
	for (new i = 0; i < len; i++)
	{
		new c = arg[i];
		if (!((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '_')) return SendClientMessage(playerid, VIE_COL_ERR, "Nom invalide (format Prenom_Nom)."), 1;
	}
	new me[MAX_PLAYER_NAME + 1], q[200];
	GetPlayerName(playerid, me, sizeof me);
	if (!strcmp(me, arg, true)) return SendClientMessage(playerid, VIE_COL_ERR, "Tu ne peux pas te parrainer toi-meme."), 1;
	format(q, sizeof q, "SELECT parrain FROM parrains WHERE filleul = '%s'", me);
	new DBResult:r = db_query(g_VieDB, q);
	if (r != DBResult:0)
	{
		new had = db_num_rows(r);
		db_free_result(r);
		if (had) return Cw_Notify(playerid, "denied", "Parrainage", "Tu as deja indique ton parrain."), 1;
	}
	new real[MAX_PLAYER_NAME + 1];
	format(q, sizeof q, "SELECT name FROM bank_accounts WHERE name = '%s'", arg);
	r = db_query(g_BankDB, q);
	if (r == DBResult:0) return 1;
	if (db_num_rows(r) == 0) { db_free_result(r); return Cw_Notify(playerid, "denied", "Parrainage", "Aucun citoyen de Crestwood ne porte ce nom."), 1; }
	db_get_field(r, 0, real, sizeof real);
	db_free_result(r);
	format(q, sizeof q, "SELECT COUNT(*) FROM parrains WHERE parrain = '%s'", real);
	r = db_query(g_VieDB, q);
	if (r != DBResult:0)
	{
		new f[12];
		db_get_field(r, 0, f, sizeof f);
		db_free_result(r);
		if (strval(f) >= 25) return Cw_Notify(playerid, "denied", "Parrainage", "Ce parrain a deja atteint le maximum de filleuls."), 1;
	}
	format(q, sizeof q, "INSERT INTO parrains (filleul, parrain, ts) VALUES ('%s', '%s', %d)", me, real, gettime());
	db_free_result(db_query(g_VieDB, q));
	GivePlayerMoney(playerid, 2500);
	Cw_CreditByName(real, 5000, "Parrainage");
	new m[160], sp[MAX_PLAYER_NAME + 1];
	format(sp, sizeof sp, "%s", real);
	for (new i = 0; sp[i]; i++) if (sp[i] == '_') sp[i] = ' ';
	format(m, sizeof m, "~b~%s~w~ est ton parrain. Tu recois ~g~2 500 $~w~, et lui ~g~5 000 $~w~ sur son compte.", sp);
	Cw_Notify(playerid, "success", "Parrainage", m, 8000);
	new p = Cw_PlayerByName(real);
	if (p != INVALID_PLAYER_ID)
	{
		new mm[MAX_PLAYER_NAME + 1];
		format(mm, sizeof mm, "%s", me);
		for (new i = 0; mm[i]; i++) if (mm[i] == '_') mm[i] = ' ';
		format(m, sizeof m, "~b~%s~w~ t'a choisi comme parrain : ~g~5 000 $~w~ verses sur ton compte.", mm);
		Cw_Notify(p, "success", "Parrainage", m, 8000);
	}
	return 1;
}

// ------------------------------------------------------------
//  4) Sondages
// ------------------------------------------------------------
stock Poll_Command(playerid, const arg[])
{
	if (Admin_Level(playerid) < 1) return SendClientMessage(playerid, VIE_COL_ERR, "Reserve au staff."), 1;
	if (g_PollOn) return SendClientMessage(playerid, VIE_COL_ERR, "Un sondage est deja en cours."), 1;
	if (strlen(arg) < 5) return SendClientMessage(playerid, POLL_COL, "/sondage <question> : les joueurs votent /voter oui ou /voter non pendant 60 secondes."), 1;
	format(g_PollQ, sizeof g_PollQ, "%s", arg);
	for (new i = 0; g_PollQ[i]; i++) if (g_PollQ[i] == '~' || g_PollQ[i] == '"' || g_PollQ[i] == '\\') g_PollQ[i] = ' ';
	g_PollOn = 1; g_PollYes = 0; g_PollNo = 0; g_PollEnd = gettime() + 60;
	for (new p = 0, mx = GetMaxPlayers(); p < mx; p++) g_PollVoted[p] = 0;
	new m[200];
	format(m, sizeof m, "[SONDAGE] %s  ->  /voter oui  ou  /voter non (60 s)", g_PollQ);
	SendClientMessageToAll(POLL_COL, m);
	format(m, sizeof m, "%s~n~Vote avec ~g~/voter oui~w~ ou ~r~/voter non~w~ (60 s).", g_PollQ);
	Cw_NotifyAll("system", "Sondage", m, 9000);
	return 1;
}

stock Poll_Vote(playerid, const arg[])
{
	if (!g_PollOn) return SendClientMessage(playerid, VIE_COL_ERR, "Aucun sondage en cours."), 1;
	if (g_PollVoted[playerid]) return SendClientMessage(playerid, VIE_COL_ERR, "Tu as deja vote."), 1;
	if (!strcmp(arg, "oui", true)) g_PollYes++;
	else if (!strcmp(arg, "non", true)) g_PollNo++;
	else return SendClientMessage(playerid, POLL_COL, "/voter oui  ou  /voter non"), 1;
	g_PollVoted[playerid] = 1;
	return SendClientMessage(playerid, POLL_COL, "[SONDAGE] Vote enregistre, merci."), 1;
}

stock Poll_End()
{
	g_PollOn = 0;
	new tot = g_PollYes + g_PollNo, py = tot ? (g_PollYes * 100 / tot) : 0, m[200];
	format(m, sizeof m, "[SONDAGE] %s  ->  OUI %d (%d pour cent)  NON %d (%d pour cent)", g_PollQ, g_PollYes, py, g_PollNo, tot ? (100 - py) : 0);
	SendClientMessageToAll(POLL_COL, m);
	format(m, sizeof m, "%s~n~~g~Oui %d~w~  -  ~r~Non %d~w~  (%d votes)", g_PollQ, g_PollYes, g_PollNo, tot);
	Cw_NotifyAll("system", "Resultat du sondage", m, 9000);
	return 1;
}

// ------------------------------------------------------------
//  5) Meteo vivante
// ------------------------------------------------------------
stock Wx_Pick()
{
	new r = random(100);
	if (r < 46) return 1;            // soleil
	if (r < 70) return 4;            // nuageux
	if (r < 84) return 7;            // couvert
	if (r < 94) return 8;            // pluie / orage
	return 9;                        // brouillard
}

stock Wx_Name(id, out[], size)
{
	switch (id)
	{
		case 0, 1, 2, 3, 10, 11, 13, 14, 17, 18: format(out, size, "ensoleille");
		case 4, 5, 6, 12, 15: format(out, size, "nuageux");
		case 7: format(out, size, "couvert");
		case 8, 16: format(out, size, "pluie");
		case 9, 20: format(out, size, "brouillard");
		case 19: format(out, size, "tempete de sable");
		default: format(out, size, "variable");
	}
	return 1;
}

stock Wx_Fix(id)
{
	g_WxAuto = 0; g_WxNow = id;
	SetWeather(id);
	return 1;
}

stock Wx_Command(playerid, const arg[])
{
	new a[24], b[24], m[160];
	if (arg[0] && Admin_Level(playerid) >= 1)
	{
		if (!strcmp(arg, "auto", true)) { g_WxAuto = 1; g_WxAt = gettime() + 5; return SendClientMessage(playerid, POLL_COL, "Meteo : cycle automatique reactive."), 1; }
		new id = strval(arg);
		if (id < 0 || id > 45 || (!id && arg[0] != '0')) return SendClientMessage(playerid, VIE_COL_ERR, "/meteo auto  ou  /meteo 0-45"), 1;
		Wx_Fix(id);
		Wx_Name(id, a, sizeof a);
		format(m, sizeof m, "Meteo figee : %s (%d). /meteo auto pour relancer le cycle.", a, id);
		return SendClientMessage(playerid, POLL_COL, m), 1;
	}
	Wx_Name(g_WxNow, a, sizeof a);
	Wx_Name(g_WxNext, b, sizeof b);
	if (g_WxAuto) format(m, sizeof m, "Meteo a Los Santos : ~b~%s~w~. Prevision dans %d min : ~b~%s~w~.", a, (g_WxAt - gettime()) / 60 + 1, b);
	else format(m, sizeof m, "Meteo a Los Santos : ~b~%s~w~ (fixee par le staff).", a);
	return Cw_Notify(playerid, "info", "Meteo", m), 1;
}

// ------------------------------------------------------------
//  6) AFK
// ------------------------------------------------------------
stock Afk_Clear(playerid)
{
	if (g_AfkLabel[playerid] != Text3D:INVALID_3DTEXT_ID)
	{
		Delete3DTextLabel(g_AfkLabel[playerid]);
		g_AfkLabel[playerid] = Text3D:INVALID_3DTEXT_ID;
		return 1;
	}
	return 0;
}

stock Afk_Command(playerid)
{
	if (Afk_Clear(playerid)) return Cw_Notify(playerid, "success", "AFK", "Bon retour en ville !"), 1;
	g_AfkLabel[playerid] = Create3DTextLabel("[AFK]", 0xB0B7C3FF, 0.0, 0.0, 0.0, 20.0, GetPlayerVirtualWorld(playerid), 1);
	Attach3DTextLabelToPlayer(g_AfkLabel[playerid], playerid, 0.0, 0.0, 0.45);
	return Cw_Notify(playerid, "info", "AFK", "Tu es absent ([AFK] au-dessus de toi). Retape ~b~/afk~w~ en revenant."), 1;
}

// ------------------------------------------------------------
//  7) Mappings a modeles personnalises sans recompiler
// ------------------------------------------------------------
stock MapData_Next()
{
#if MAP_MODELES_DATA
	return 1;
#else
	return fexist("mappings/data_ok.txt") ? 1 : 0;
#endif
}

stock MapData_Command(playerid, const arg[])
{
	if (Admin_Level(playerid) < 7) return SendClientMessage(playerid, VIE_COL_ERR, "Reserve au staff niveau 7."), 1;
	new m[200];
	if (!strcmp(arg, "on", true))
	{
		new File:f = fopen("mappings/data_ok.txt", io_write);
		if (!f) return SendClientMessage(playerid, VIE_COL_ERR, "Impossible d'ecrire scriptfiles/mappings/data_ok.txt."), 1;
		fwrite(f, "La data du jeu contient les modeles du dossier DATA_MAPPINGS.\r\n");
		fclose(f);
		return SendClientMessage(playerid, POLL_COL, "Mappings personnalises (Banque Atlas, Mecano, Concession SF) : ACTIFS au prochain redemarrage. A faire SEULEMENT si la data du jeu est a jour."), 1;
	}
	if (!strcmp(arg, "off", true))
	{
		if (fexist("mappings/data_ok.txt")) fremove("mappings/data_ok.txt");
		return SendClientMessage(playerid, POLL_COL, "Mappings personnalises : desactives au prochain redemarrage."), 1;
	}
	format(m, sizeof m, "Mappings personnalises : %s maintenant, %s au prochain redemarrage. /mapdata on | off", g_MapData ? ("actifs") : ("inactifs"), MapData_Next() ? ("actifs") : ("inactifs"));
	return SendClientMessage(playerid, POLL_COL, m), 1;
}


// ------------------------------------------------------------
//  8) MAGASINS DE VETEMENTS (points "vetpoints" de crestwood_garage.db)
//     Le staff les place depuis le panneau admin (Factions > Magasins de vetements > Creer ici).
//     Joueur : t-shirt 3D + texte, icone sur la carte, main "Vetements" tout pres -> studio de style.
// ------------------------------------------------------------
#define CS_BLIP               45          // assets/hud/radar/blips/blip_45.png (vetements)
new Text3D:g_CsLabel[VET_MAX] = {Text3D:INVALID_3DTEXT_ID, ...};

stock Cs_Build(i)
{
	if (g_CsLabel[i] != Text3D:INVALID_3DTEXT_ID) Delete3DTextLabel(g_CsLabel[i]);
	g_CsLabel[i] = Create3DTextLabel("Magasin de vetements\n{9CC1FF}Approche-toi et touche la main", 0xFFFFFFFF, g_VetPos[i][0], g_VetPos[i][1], g_VetPos[i][2] + 0.4, 12.0, -1, 1);
	return 1;
}

stock Cs_BuildAll()
{
	for (new i = 0; i < VET_MAX; i++)
	{
		if (g_CsLabel[i] != Text3D:INVALID_3DTEXT_ID) { Delete3DTextLabel(g_CsLabel[i]); g_CsLabel[i] = Text3D:INVALID_3DTEXT_ID; }
	}
	for (new i = 0; i < g_VetCount; i++) Cs_Build(i);
	return 1;
}

stock Cs_InteractKey(playerid)
{
	if (!g_AuthLogged[playerid]) return 0;
	if (Gar_NearVet(playerid) < 0) return 0;
	return 190;
}

stock Cs_Use(playerid)
{
	if (Locker_InUniform(playerid)) return Cw_Notify(playerid, "warning", "Vetements", "Remets ta tenue civile au vestiaire avant d'essayer des vetements."), 1;
	return Cloth_OnCommand(playerid, "/vetements");
}

stock Cs_PushMarks()
{
	for (new i = 0; i < g_VetCount; i++) Mk_Add(1, g_VetPos[i][0], g_VetPos[i][1], g_VetPos[i][2] - 0.95);
	return 1;
}

stock Cs_AppendBlips(json[], size, &n)
{
	new part[64];
	for (new i = 0; i < g_VetCount; i++)
	{
		n++;
		format(part, sizeof part, "%s{\"x\":%.1f,\"y\":%.1f,\"icon\":%d}", (n > 1) ? (",") : (""), g_VetPos[i][0], g_VetPos[i][1], CS_BLIP);
		strcat(json, part, size);
	}
	return 1;
}

stock Cs_AdminSend(playerid)
{
	new json[2600], part[96];
	format(json, sizeof json, "{\"x\":\"cshop\",\"shops\":[");
	for (new i = 0; i < g_VetCount; i++)
	{
		format(part, sizeof part, "%s{\"i\":%d,\"x\":%d,\"y\":%d,\"z\":%d}", i ? (",") : (""), g_VetId[i], floatround(g_VetPos[i][0]), floatround(g_VetPos[i][1]), floatround(g_VetPos[i][2]));
		if (strlen(json) + strlen(part) > sizeof(json) - 4) break;
		strcat(json, part, sizeof json);
	}
	strcat(json, "]}", sizeof json);
	return ShowPlayerDialog(playerid, ADMIN_DIALOG_UPDATE, DIALOG_STYLE_MSGBOX, "@admin", json, "OK", "");
}

stock Cs_AdminNeed(const action[])
{
	if (!strcmp(action, "cshopadd", true) || !strcmp(action, "cshopdel", true)) return 4;
	if (!strcmp(action, "cshopgoto", true) || !strcmp(action, "cshoplist", true)) return 2;
	return 0;
}

stock Cs_Slot(dbid)
{
	for (new i = 0; i < g_VetCount; i++) if (g_VetId[i] == dbid) return i;
	return -1;
}

stock Cs_AdminAction(playerid, const action[], const tid[], msg[], msgsize, &ok)
{
	if (!Cs_AdminNeed(action)) return 0;
	if (!strcmp(action, "cshoplist", true)) { ok = 1; format(msg, msgsize, "%d magasin(s) de vetements.", g_VetCount); return 1; }
	if (!strcmp(action, "cshopadd", true))
	{
		if (g_GarDB == DB:0) { format(msg, msgsize, "Base des garages indisponible."); return 1; }
		if (GetPlayerState(playerid) != PLAYER_STATE_ONFOOT) { format(msg, msgsize, "Descends de ton vehicule pour placer le magasin."); return 1; }
		if (g_VetCount >= VET_MAX) { format(msg, msgsize, "Limite de %d magasins atteinte.", VET_MAX); return 1; }
		if (Gar_NearVet(playerid) >= 0) { format(msg, msgsize, "Il y a deja un magasin de vetements ici."); return 1; }
		new Float:x, Float:y, Float:z, q[160];
		GetPlayerPos(playerid, x, y, z);
		format(q, sizeof q, "INSERT INTO vetpoints (x, y, z) VALUES (%f, %f, %f)", x, y, z);
		db_free_result(db_query(g_GarDB, q));
		new DBResult:r = db_query(g_GarDB, "SELECT last_insert_rowid()"), f[12], id = 0;
		if (r != DBResult:0) { db_get_field(r, 0, f, sizeof f); id = strval(f); db_free_result(r); }
		new i = g_VetCount++;
		g_VetId[i] = id; g_VetPos[i][0] = x; g_VetPos[i][1] = y; g_VetPos[i][2] = z;
		Cs_Build(i);
		Marks_SendAll();
		format(msg, msgsize, "Magasin de vetements #%d cree a ta position.", id);
		ok = 1;
		Cs_AdminSend(playerid);
		return 1;
	}
	new id = strval(tid), i = Cs_Slot(id);
	if (i == -1) { format(msg, msgsize, "Magasin de vetements introuvable."); return 1; }
	if (!strcmp(action, "cshopgoto", true))
	{
		SetPlayerInterior(playerid, 0);
		SetPlayerVirtualWorld(playerid, 0);
		SetPlayerPos(playerid, g_VetPos[i][0] + 1.2, g_VetPos[i][1], g_VetPos[i][2]);
		format(msg, msgsize, "Teleporte au magasin de vetements #%d.", id);
		ok = 1;
		return 1;
	}
	// cshopdel
	new q[96];
	format(q, sizeof q, "DELETE FROM vetpoints WHERE id = %d", id);
	db_free_result(db_query(g_GarDB, q));
	for (new k = i; k < g_VetCount - 1; k++)
	{
		g_VetId[k] = g_VetId[k + 1];
		g_VetPos[k][0] = g_VetPos[k + 1][0]; g_VetPos[k][1] = g_VetPos[k + 1][1]; g_VetPos[k][2] = g_VetPos[k + 1][2];
	}
	g_VetCount--;
	Cs_BuildAll();
	Marks_SendAll();
	format(msg, msgsize, "Magasin de vetements #%d supprime.", id);
	ok = 1;
	Cs_AdminSend(playerid);
	return 1;
}

// ------------------------------------------------------------
//  9) GARAGE v21 : depanneuse automatique. Un vehicule de garage laisse 30 min sans personne dedans
//     est ramene au garage (etat, essence et km sauvegardes) : plus de voitures perdues dans la ville.
// ------------------------------------------------------------
#define GAR_AUTOPARK_MIN      30
new g_GarIdle[MAX_VEHICLES + 1];

stock GarAuto_Minute()
{
	for (new p = 0, mx = GetMaxPlayers(); p < mx; p++)
	{
		if (!IsPlayerConnected(p) || !IsPlayerInAnyVehicle(p)) continue;
		new v = GetPlayerVehicleID(p);
		if (v > 0 && v <= MAX_VEHICLES) g_GarIdle[v] = -1;   // occupe pendant cette minute
	}
	for (new v = 1; v <= MAX_VEHICLES; v++)
	{
		if (!g_GarVehDb[v]) { g_GarIdle[v] = 0; continue; }
		if (g_GarIdle[v] < 0) { g_GarIdle[v] = 0; continue; }
		if (++g_GarIdle[v] < GAR_AUTOPARK_MIN) continue;
		g_GarIdle[v] = 0;
		new owner = g_GarVehOwner[v], vname[32];
		Gar_VehName(GetVehicleModel(v), vname, sizeof vname);
		if (Gar_StoreVehicle(v) && owner != INVALID_PLAYER_ID && IsPlayerConnected(owner))
		{
			new m[150];
			format(m, sizeof m, "Ton ~b~%s~w~ est reste %d min sans conducteur : la depanneuse l'a ramene au garage.", vname, GAR_AUTOPARK_MIN);
			Cw_Notify(owner, "info", "Garage", m, 8000);
		}
	}
	return 1;
}

// ------------------------------------------------------------
public Cw21_Tick()
{
	new now = gettime();
	if (g_PollOn && now >= g_PollEnd) Poll_End();
	if ((now % 60) == 0) GarAuto_Minute();
	if (g_WxAuto && now >= g_WxAt)
	{
		g_WxNow = g_WxNext; g_WxNext = Wx_Pick(); g_WxAt = now + WX_PERIOD;
		SetWeather(g_WxNow);
	}
	// la camera selfie suit le joueur s'il bouge
	for (new p = 0, mx = GetMaxPlayers(); p < mx; p++)
		if (g_PcMode[p] == PCAM_SELFIE && IsPlayerConnected(p)) Pcam_Selfie(p);
	return 1;
}

stock Cw21_OnCommand(playerid, const cmdtext[])
{
	if (!g_AuthLogged[playerid]) return 0;
	new a;
	if (!strcmp(cmdtext, "/cadeau", true) || !strcmp(cmdtext, "/daily", true)) return Gift_Command(playerid), 1;
	if ((a = Cw_Arg(cmdtext, "/parrain")) != -1) return Ref_Command(playerid, cmdtext[a]), 1;
	if ((a = Cw_Arg(cmdtext, "/sondage")) != -1) return Poll_Command(playerid, cmdtext[a]), 1;
	if ((a = Cw_Arg(cmdtext, "/voter")) != -1) return Poll_Vote(playerid, cmdtext[a]), 1;
	if ((a = Cw_Arg(cmdtext, "/meteo")) != -1) return Wx_Command(playerid, cmdtext[a]), 1;
	if (!strcmp(cmdtext, "/afk", true)) return Afk_Command(playerid), 1;
	if ((a = Cw_Arg(cmdtext, "/mapdata")) != -1) return MapData_Command(playerid, cmdtext[a]), 1;
	return 0;
}
