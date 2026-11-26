// ============================================================
//  [v31] COMMANDES ENVOYEES PAR L'APPLI QUI N'EXISTAIENT PAS COTE SERVEUR
//  ("SERVER: Unknown command" / "commande inconnue" quand on touchait le bouton)
// ------------------------------------------------------------
//  /motor            bouton moteur du HUD (ancien nom) -> /moteur
//  /radioact <a> [v] talkie du HUD : freq <1-999> | start | stop
//  /sim              roue du HUD : ton numero, ou GPS vers le 24/7 pour acheter une puce
//  /hudaction <a>    anciens ecrans du HUD (Faggio, documents) -> commandes actuelles
//  /setentree <id>   panneau admin : entree d'un portail = ta position (meme numerotation que /tplieu)
//  /facact /gangact /govact /npcact /farmbuy /farmclose : anciennes pages, ignorees sans message
//  Derniere ligne de OnPlayerCommandText : une commande inconnue est notee dans
//  scriptfiles/commandes_inconnues.log (date, joueur, commande) et le joueur voit un message en francais.
// ============================================================

new g_RadioFreq[MAX_PLAYERS];

stock Cmp_OnConnect(playerid)
{
	g_RadioFreq[playerid] = 0;
	return 1;
}

stock Cmp_OnCommand(playerid, const cmdtext[])
{
	new a;
	if (!strcmp(cmdtext, "/motor", true)) return Veh_OnCommand(playerid, "/moteur"), 1;
	if ((a = Cw_Arg(cmdtext, "/radioact")) != -1)
	{
		new pos = a, act[10], val[8];
		Pg_Tok(cmdtext, pos, act, sizeof act);
		Pg_Tok(cmdtext, pos, val, sizeof val);
		if (act[0] && !strcmp(act, "freq", true))
		{
			new f = strval(val);
			if (f < 1 || f > 999) return Cw_Notify(playerid, "denied", "Talkie", "Frequence de 1 a 999.", 3000), 1;
			g_RadioFreq[playerid] = f;
			new m[64];
			format(m, sizeof m, "Talkie regle sur la frequence ~b~%d~w~.", f);
			return Cw_Notify(playerid, "success", "Talkie", m, 3000), 1;
		}
		return 1;   // start / stop : la voix passe par le vocal de l'appli
	}
	if (!strcmp(cmdtext, "/sim", true))
	{
		if (g_PhNum[playerid][0])
		{
			new m[96];
			format(m, sizeof m, "Ton numero : ~b~%s~w~. Ouvre ton telephone pour appeler.", g_PhNum[playerid]);
			return Cw_Notify(playerid, "info", "Carte SIM", m, 5000), 1;
		}
		new n = Shop_Nearest(playerid);
		if (n != -1) Gps_Set(playerid, g_Shops[n][shX], g_Shops[n][shY]);
		return Cw_Notify(playerid, "info", "Carte SIM", "Pas encore de puce : GPS vers le ~b~24/7~w~ le plus proche (caisse).", 5000), 1;
	}
	if ((a = Cw_Arg(cmdtext, "/hudaction")) != -1)
	{
		new pos = a, act[24];
		Pg_Tok(cmdtext, pos, act, sizeof act);
		new c2[16];
		if (act[0] && !strcmp(act, "bike_return", true)) { c2 = "/rendrevelo"; return OnPlayerCommandText(playerid, c2), 1; }
		if (act[0] && !strcmp(act, "documents_issue", true)) { c2 = "/papiers"; return OnPlayerCommandText(playerid, c2), 1; }
		if (act[0] && !strcmp(act, "bike_rent", true)) return Cw_Notify(playerid, "info", "Location", "Va sur un point de location (marqueur velo ou scooter) et touche la main.", 4000), 1;
		return 1;
	}
	if ((a = Cw_Arg(cmdtext, "/setentree")) != -1)
	{
		if (Admin_Level(playerid) < 4) return Cw_Notify(playerid, "denied", "Admin", "Reserve au staff (niveau 4).", 3000), 1;
		new id = strval(cmdtext[a]) - 1 - sizeof(g_Places);
		if (!cmdtext[a] || id < 0 || id >= g_EntryCount || g_Entries[id][enDb] <= 0)
			return Cw_Notify(playerid, "denied", "Admin", "Portail introuvable : /lieux pour les numeros (ou panneau /mappings).", 4000), 1;
		new Float:x, Float:y, Float:z, q[160];
		GetPlayerPos(playerid, x, y, z);
		format(q, sizeof q, "UPDATE map_portals SET ox = %f, oy = %f, oz = %f WHERE id = %d", x, y, z - 0.6, g_Entries[id][enDb]);
		db_free_result(db_query(g_JobDB, q));
		Pts_ReloadPortals();
		return Cw_Notify(playerid, "success", "Admin", "Entree du portail posee ici.", 3500), 1;
	}
	if (Cw_Arg(cmdtext, "/facact") != -1 || Cw_Arg(cmdtext, "/gangact") != -1 || Cw_Arg(cmdtext, "/govact") != -1
		|| Cw_Arg(cmdtext, "/npcact") != -1 || Cw_Arg(cmdtext, "/farmbuy") != -1 || Cw_Arg(cmdtext, "/farmclose") != -1) return 1;
	return 0;
}

// Appelee quand aucun module n'a reconnu la commande
stock Cmp_Unknown(playerid, const cmdtext[])
{
	new name[MAX_PLAYER_NAME + 1], line[200], y, mo, d, h, mi, s, c[64];
	GetPlayerName(playerid, name, sizeof name);
	getdate(y, mo, d);
	gettime(h, mi, s);
	format(c, sizeof c, "%s", cmdtext);
	format(line, sizeof line, "%04d-%02d-%02d %02d:%02d:%02d  %s  %s\r\n", y, mo, d, h, mi, s, name, c);
	new File:f = fopen("commandes_inconnues.log", io_append);
	if (f) { fwrite(f, line); fclose(f); }
	printf("[cmd] inconnue : %s -> %s", name, c);
	// le nom de la commande seulement (sans les arguments)
	new cmd[32], i = 0;
	while (cmdtext[i] && cmdtext[i] != ' ' && i < sizeof(cmd) - 1) { cmd[i] = cmdtext[i]; i++; }
	cmd[i] = 0;
	if (!g_AuthLogged[playerid]) return SendClientMessage(playerid, 0xFF6B72FF, "Connecte-toi d'abord pour utiliser les commandes."), 1;
	format(line, sizeof line, "Commande inconnue (ou reservee au staff) : %s. Tape /aide pour la liste.", cmd);
	return SendClientMessage(playerid, 0xFF6B72FF, line), 1;
}
