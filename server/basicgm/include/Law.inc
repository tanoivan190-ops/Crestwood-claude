/* ============================================================
   CENTRALE DES FORCES DE L'ORDRE (police / FBI / armee) - Crestwood RP
   Page : tablette RP (assets/rp) > vue "law" ("Centrale") ; /mdt ou /centrale l'ouvre directement.
   ------------------------------------------------------------
   Onglets : agents en service (GPS), avis de recherche (ajout / retrait), renforts (GPS),
             dossiers (notes d'enquete FBI / police), alerte militaire (armee).
   Commandes :
     /mdt /centrale            ouvrir la centrale
     /renfort [message]        demande de renfort (GPS a toutes les forces de l'ordre)
     /m texte (/megaphone)     megaphone (60 m)
     /recherche id niveau raison   avis de recherche (1 a 6 etoiles)  /effacer id
     /badge                    montrer son insigne
     /barrage /retirerbarrage  barrage routier (4 max par agent)
     /dossier Prenom_Nom texte (FBI + police)   /alerte texte (armee : alerte a tout le serveur)
   Page -> serveur (via /rpact) : lawwanted id lvl raison | lawclear id | lawbackup | lawbackupend
                                  lawgps id | lawnote nom texte | lawnotedel id | lawalert texte | lawbarrier | lawbarrierdel
   ============================================================ */
#define LAW_BARRIERS          4
#define LAW_BACKUP_TTL        600         // une demande de renfort reste 10 min
#define LAW_COL               0x5B8CFFFF
#define LAW_COL_ERR           0xFF6347FF
#define LAW_COL_OK            0x33CC33FF

new DB:g_LawDB;
new g_LawReason[MAX_PLAYERS][64];
new g_LawBy[MAX_PLAYERS][MAX_PLAYER_NAME + 1];
new g_LawBackupTs[MAX_PLAYERS];
new Float:g_LawBackupPos[MAX_PLAYERS][3];
new g_LawBarrier[MAX_PLAYERS][LAW_BARRIERS];
new g_LawAlert[96];
new g_LawAlertTs;

stock Law_OnInit()
{
	g_LawDB = db_open("crestwood_law.db");
	if (g_LawDB != DB:0)
		db_free_result(db_query(g_LawDB, "CREATE TABLE IF NOT EXISTS law_notes (id INTEGER PRIMARY KEY AUTOINCREMENT, target TEXT COLLATE NOCASE, author TEXT, text TEXT, ts INTEGER, type INTEGER)"));
	g_LawAlert[0] = 0;
	return 1;
}

stock Law_OnConnect(playerid)
{
	g_LawReason[playerid][0] = 0;
	g_LawBy[playerid][0] = 0;
	g_LawBackupTs[playerid] = 0;
	for (new k = 0; k < LAW_BARRIERS; k++) g_LawBarrier[playerid][k] = INVALID_OBJECT_ID;
	return 1;
}

stock Law_OnDisconnect(playerid)
{
	for (new k = 0; k < LAW_BARRIERS; k++) if (g_LawBarrier[playerid][k] != INVALID_OBJECT_ID) { DestroyObject(g_LawBarrier[playerid][k]); g_LawBarrier[playerid][k] = INVALID_OBJECT_ID; }
	g_LawBackupTs[playerid] = 0;
	return 1;
}

stock Law_TypeName(t, out[], size)
{
	switch (t)
	{
		case FACTION_FEDERAL: format(out, size, "FBI");
		case FACTION_ARMY: format(out, size, "Armee");
		default: format(out, size, "Police");
	}
	return 1;
}

stock Law_SendAll(color, const text[])
{
	for (new i = 0, m = GetMaxPlayers(); i < m; i++) if (IsPlayerConnected(i) && g_AuthLogged[i] && Fac_IsLaw(i)) SendClientMessage(i, color, text);
	return 1;
}

stock Law_Clean(const in[], out[], size)
{
	new j = 0;
	for (new i = 0; in[i] != 0 && j < size - 1; i++)
	{
		new c = in[i];
		if (c == '"' || c == '\\' || c == '{' || c == '}' || c == 39 || c < 32) c = ' ';
		out[j++] = c;
	}
	out[j] = 0;
	return 1;
}

stock Law_SetWanted(playerid, t, lvl, const reason[])
{
	if (!Fac_IsLaw(playerid)) return 0;
	if (t < 0 || t >= MAX_PLAYERS || !IsPlayerConnected(t) || t == playerid) return SendClientMessage(playerid, LAW_COL_ERR, "Joueur introuvable."), 0;
	if (lvl < 0) lvl = 0;
	if (lvl > 6) lvl = 6;
	SetPlayerWantedLevel(t, lvl);
	new by[MAX_PLAYER_NAME + 1], tn[MAX_PLAYER_NAME + 1], msg[160];
	GetPlayerName(playerid, by, sizeof by);
	GetPlayerName(t, tn, sizeof tn);
	if (lvl > 0)
	{
		Law_Clean(reason, g_LawReason[t], 64);
		format(g_LawBy[t], MAX_PLAYER_NAME + 1, "%s", by);
		format(msg, sizeof msg, "[CENTRALE] Avis de recherche : %s (%d) - %d etoile(s) - %s (par %s)", tn, t, lvl, g_LawReason[t], by);
		Law_SendAll(LAW_COL, msg);
		SendClientMessage(t, LAW_COL_ERR, "Tu es recherche par les forces de l'ordre.");
	}
	else
	{
		g_LawReason[t][0] = 0;
		format(msg, sizeof msg, "[CENTRALE] Avis de recherche retire : %s (%d) par %s.", tn, t, by);
		Law_SendAll(LAW_COL, msg);
	}
	return 1;
}

stock Law_Backup(playerid, const text[])
{
	if (!Fac_IsLaw(playerid)) return 0;
	new Float:x, Float:y, Float:z, name[MAX_PLAYER_NAME + 1], msg[170], tn[24], t = Fac_Type(playerid);
	GetPlayerPos(playerid, x, y, z);
	GetPlayerName(playerid, name, sizeof name);
	Law_TypeName(t, tn, sizeof tn);
	g_LawBackupTs[playerid] = gettime();
	g_LawBackupPos[playerid][0] = x; g_LawBackupPos[playerid][1] = y; g_LawBackupPos[playerid][2] = z;
	format(msg, sizeof msg, "[RENFORT] %s %s (%d) demande des renforts%s%s. Position au GPS.", tn, name, playerid, text[0] ? (" : ") : (""), text);
	for (new i = 0, m = GetMaxPlayers(); i < m; i++)
	{
		if (i == playerid || !IsPlayerConnected(i) || !g_AuthLogged[i] || !Fac_IsLaw(i)) continue;
		SendClientMessage(i, 0xFF5A6AFF, msg);
		Gps_Set(i, x, y);
	}
	SendClientMessage(playerid, LAW_COL_OK, "Demande de renfort envoyee.");
	return 1;
}

stock Law_Barrier(playerid, remove)
{
	if (!Fac_IsLaw(playerid)) return 0;
	if (remove)
	{
		new best = -1, Float:bd = 6.0, Float:ox, Float:oy, Float:oz;
		for (new k = 0; k < LAW_BARRIERS; k++)
		{
			if (g_LawBarrier[playerid][k] == INVALID_OBJECT_ID) continue;
			GetObjectPos(g_LawBarrier[playerid][k], ox, oy, oz);
			new Float:d = GetPlayerDistanceFromPoint(playerid, ox, oy, oz);
			if (d < bd) { bd = d; best = k; }
		}
		if (best == -1) return SendClientMessage(playerid, LAW_COL_ERR, "Aucun de tes barrages a cote de toi."), 1;
		DestroyObject(g_LawBarrier[playerid][best]);
		g_LawBarrier[playerid][best] = INVALID_OBJECT_ID;
		return SendClientMessage(playerid, LAW_COL_OK, "Barrage retire."), 1;
	}
	if (IsPlayerInAnyVehicle(playerid)) return SendClientMessage(playerid, LAW_COL_ERR, "Descends du vehicule."), 1;
	new slot = -1;
	for (new k = 0; k < LAW_BARRIERS; k++) if (g_LawBarrier[playerid][k] == INVALID_OBJECT_ID) { slot = k; break; }
	if (slot == -1) return SendClientMessage(playerid, LAW_COL_ERR, "Tu as deja 4 barrages (/retirerbarrage)."), 1;
	new Float:x, Float:y, Float:z, Float:a;
	GetPlayerPos(playerid, x, y, z);
	GetPlayerFacingAngle(playerid, a);
	x += 2.0 * floatsin(-a, degrees); y += 2.0 * floatcos(-a, degrees);
	g_LawBarrier[playerid][slot] = CreateObject(981, x, y, z - 0.3, 0.0, 0.0, a, 150.0);
	ApplyAnimation(playerid, "BOMBER", "BOM_Plant", 4.1, 0, 0, 0, 0, 0, 1);
	return SendClientMessage(playerid, LAW_COL_OK, "Barrage pose (/retirerbarrage pour l'enlever)."), 1;
}

stock Law_AddNote(playerid, const target[], const text[])
{
	new t = Fac_Type(playerid);
	if (t != FACTION_FEDERAL && t != FACTION_POLICE) return SendClientMessage(playerid, LAW_COL_ERR, "Dossiers : FBI et police seulement."), 0;
	if (g_LawDB == DB:0) return 0;
	new tg[MAX_PLAYER_NAME + 1], tx[110], au[MAX_PLAYER_NAME + 1], q[320];
	Law_Clean(target, tg, sizeof tg);
	Law_Clean(text, tx, sizeof tx);
	if (strlen(tg) < 3 || strlen(tx) < 3) return SendClientMessage(playerid, LAW_COL_ERR, "Dossier : nom et texte obligatoires."), 0;
	GetPlayerName(playerid, au, sizeof au);
	format(q, sizeof q, "INSERT INTO law_notes (target, author, text, ts, type) VALUES ('%s', '%s', '%s', %d, %d)", tg, au, tx, gettime(), t);
	db_free_result(db_query(g_LawDB, q));
	return SendClientMessage(playerid, LAW_COL_OK, "Note ajoutee au dossier."), 1;
}

stock Law_Alert(playerid, const text[])
{
	if (Fac_Type(playerid) != FACTION_ARMY) return SendClientMessage(playerid, LAW_COL_ERR, "Reserve a l'armee."), 0;
	if (gettime() - g_LawAlertTs < 60) return SendClientMessage(playerid, LAW_COL_ERR, "Une alerte a deja ete donnee il y a moins d'une minute."), 0;
	Law_Clean(text, g_LawAlert, sizeof g_LawAlert);
	g_LawAlertTs = gettime();
	new msg[150];
	format(msg, sizeof msg, "[ALERTE MILITAIRE] %s", g_LawAlert);
	SendClientMessageToAll(0xFFB454FF, msg);
	GameTextForAll("~r~ALERTE MILITAIRE", 4000, 3);
	return 1;
}

// JSON de la vue "law" (ajoute par Rp_Send)
stock Law_Build(playerid, out[], size)
{
	new t = Fac_Type(playerid), tn[16], part[200], n = 0, name[MAX_PLAYER_NAME + 1], fn[48];
	Law_TypeName(t, tn, sizeof tn);
	format(out, size, ",\"law\":{\"ok\":%d,\"type\":%d,\"tn\":\"%s\"", Fac_IsLaw(playerid) ? 1 : 0, t, tn);
	if (!Fac_IsLaw(playerid)) { strcat(out, "}", size); return 1; }
	new Float:x, Float:y, Float:z;
	GetPlayerPos(playerid, x, y, z);
	// agents
	strcat(out, ",\"agents\":[", size);
	for (new i = 0, m = GetMaxPlayers(); i < m; i++)
	{
		if (!IsPlayerConnected(i) || !g_AuthLogged[i] || !Fac_IsLaw(i)) continue;
		GetPlayerName(i, name, sizeof name);
		new fac = Fac_Get(i), rk[32], ti[16];
		Fac_RankName(fac, g_FacRank[i], rk, sizeof rk);
		Law_TypeName(Fac_Type(i), ti, sizeof ti);
		new Float:ax, Float:ay, Float:az;
		GetPlayerPos(i, ax, ay, az);
		Law_Clean(rk, fn, sizeof fn);
		format(part, sizeof part, "%s[%d,\"%s\",\"%s\",\"%s\",%d]", n ? (",") : (""), i, name, ti, fn, floatround(GetPlayerDistanceFromPoint(playerid, ax, ay, az)));
		if (strlen(out) + strlen(part) > size - 1700) break;
		strcat(out, part, size);
		n++;
	}
	// avis de recherche
	strcat(out, "],\"wanted\":[", size);
	n = 0;
	for (new i = 0, m = GetMaxPlayers(); i < m; i++)
	{
		if (!IsPlayerConnected(i) || GetPlayerWantedLevel(i) <= 0) continue;
		GetPlayerName(i, name, sizeof name);
		format(part, sizeof part, "%s[%d,\"%s\",%d,\"%s\",\"%s\"]", n ? (",") : (""), i, name, GetPlayerWantedLevel(i), g_LawReason[i], g_LawBy[i]);
		if (strlen(out) + strlen(part) > size - 1100) break;
		strcat(out, part, size);
		n++;
	}
	// renforts
	strcat(out, "],\"backup\":[", size);
	n = 0;
	for (new i = 0, m = GetMaxPlayers(); i < m; i++)
	{
		if (!IsPlayerConnected(i) || !g_LawBackupTs[i] || gettime() - g_LawBackupTs[i] > LAW_BACKUP_TTL) continue;
		GetPlayerName(i, name, sizeof name);
		format(part, sizeof part, "%s[%d,\"%s\",%d,%d]", n ? (",") : (""), i, name, g_LawBackupTs[i],
			floatround(GetPlayerDistanceFromPoint(playerid, g_LawBackupPos[i][0], g_LawBackupPos[i][1], g_LawBackupPos[i][2])));
		if (strlen(out) + strlen(part) > size - 900) break;
		strcat(out, part, size);
		n++;
	}
	strcat(out, "]", size);
	// dossiers (FBI + police)
	strcat(out, ",\"notes\":[", size);
	if ((t == FACTION_FEDERAL || t == FACTION_POLICE) && g_LawDB != DB:0)
	{
		new DBResult:r = db_query(g_LawDB, "SELECT id, target, author, text, ts FROM law_notes ORDER BY id DESC LIMIT 6");
		if (r != DBResult:0)
		{
			new f1[12], tg[MAX_PLAYER_NAME + 1], au[MAX_PLAYER_NAME + 1], tx[112], ts[16];
			for (new i = 0, rows = db_num_rows(r); i < rows; i++)
			{
				db_get_field(r, 0, f1, sizeof f1); db_get_field(r, 1, tg, sizeof tg); db_get_field(r, 2, au, sizeof au);
				db_get_field(r, 3, tx, sizeof tx); db_get_field(r, 4, ts, sizeof ts);
				format(part, sizeof part, "%s[%d,\"%s\",\"%s\",\"%s\",%d]", i ? (",") : (""), strval(f1), tg, au, tx, strval(ts));
				if (strlen(out) + strlen(part) > size - 200) break;
				strcat(out, part, size);
				db_next_row(r);
			}
			db_free_result(r);
		}
	}
	format(part, sizeof part, "],\"alert\":\"%s\",\"alertTs\":%d,\"bar\":%d}", g_LawAlert, g_LawAlertTs, Law_BarrierCount(playerid));
	strcat(out, part, size);
	return 1;
}

stock Law_BarrierCount(playerid)
{
	new n = 0;
	for (new k = 0; k < LAW_BARRIERS; k++) if (g_LawBarrier[playerid][k] != INVALID_OBJECT_ID) n++;
	return n;
}

// actions de la tablette (appele par Rp_Action) ; retourne -1 si l'action n'est pas pour la centrale
stock Law_Action(playerid, const action[], const arg[], const rest[])
{
	if (strcmp(action, "law", true, 3)) return -1;
	if (!Fac_IsLaw(playerid)) return Rp_Reply(playerid, 0, "Reserve aux forces de l'ordre.");
	if (!strcmp(action, "lawwanted", true))
	{
		new idx = 0, tok[8], reason[100];
		Bank_NextToken(rest, idx, tok, sizeof tok);
		while (rest[idx] == ' ') idx++;
		format(reason, sizeof reason, "%s", rest[idx]);
		if (strlen(reason) < 3) return Rp_Reply(playerid, 0, "Indique une raison.");
		Law_SetWanted(playerid, strval(arg), strval(tok), reason);
		return Rp_Reply(playerid, 1, "Avis de recherche mis a jour.");
	}
	if (!strcmp(action, "lawclear", true)) { Law_SetWanted(playerid, strval(arg), 0, ""); return Rp_Reply(playerid, 1, "Avis retire."); }
	if (!strcmp(action, "lawbackup", true)) { Law_Backup(playerid, ""); return Rp_Reply(playerid, 1, "Renfort demande."); }
	if (!strcmp(action, "lawbackupend", true)) { g_LawBackupTs[playerid] = 0; return Rp_Reply(playerid, 1, "Demande de renfort terminee."); }
	if (!strcmp(action, "lawgps", true))
	{
		new t = strval(arg);
		if (t < 0 || t >= MAX_PLAYERS || !IsPlayerConnected(t)) return Rp_Reply(playerid, 0, "Introuvable.");
		if (g_LawBackupTs[t] && gettime() - g_LawBackupTs[t] <= LAW_BACKUP_TTL) Gps_Set(playerid, g_LawBackupPos[t][0], g_LawBackupPos[t][1]);
		else
		{
			if (!Fac_IsLaw(t)) return Rp_Reply(playerid, 0, "Position non disponible.");
			new Float:x, Float:y, Float:z;
			GetPlayerPos(t, x, y, z);
			Gps_Set(playerid, x, y);
		}
		return Rp_Reply(playerid, 1, "Position envoyee au GPS.");
	}
	if (!strcmp(action, "lawnote", true))
	{
		Law_AddNote(playerid, arg, rest);
		return Rp_Reply(playerid, 1, "");
	}
	if (!strcmp(action, "lawnotedel", true))
	{
		if (g_LawDB == DB:0) return 1;
		new q[80];
		format(q, sizeof q, "DELETE FROM law_notes WHERE id = %d", strval(arg));
		db_free_result(db_query(g_LawDB, q));
		return Rp_Reply(playerid, 1, "Note supprimee.");
	}
	if (!strcmp(action, "lawalert", true))
	{
		new full[110];
		format(full, sizeof full, "%s %s", arg, rest);
		Law_Alert(playerid, full);
		return Rp_Reply(playerid, 1, "");
	}
	if (!strcmp(action, "lawbarrier", true)) { Law_Barrier(playerid, 0); return Rp_Reply(playerid, 1, ""); }
	if (!strcmp(action, "lawbarrierdel", true)) { Law_Barrier(playerid, 1); return Rp_Reply(playerid, 1, ""); }
	return 1;
}

stock Law_OnCommand(playerid, const cmdtext[])
{
	if (!g_AuthLogged[playerid]) return 0;
	if (!strcmp(cmdtext, "/mdt", true) || !strcmp(cmdtext, "/centrale", true))
	{
		if (!Fac_IsLaw(playerid)) return SendClientMessage(playerid, LAW_COL_ERR, "Reserve aux forces de l'ordre (police, FBI, armee)."), 1;
		format(g_RpView[playerid], 8, "law");
		Rp_Send(playerid, "law", RP_DIALOG_OPEN, 1, "");
		return 1;
	}
	new a;
	if ((a = RpX_Arg(cmdtext, "/renfort")) != -1) { if (!Fac_IsLaw(playerid)) return 0; Law_Backup(playerid, cmdtext[a]); return 1; }
	if ((a = RpX_Arg(cmdtext, "/m")) != -1 || (a = RpX_Arg(cmdtext, "/megaphone")) != -1)
	{
		if (!Fac_IsLaw(playerid)) return 0;
		if (!cmdtext[a]) return SendClientMessage(playerid, LAW_COL, "/m <texte>"), 1;
		new name[MAX_PLAYER_NAME + 1], tn[16], msg[170];
		RpX_Name(playerid, name, sizeof name);
		Law_TypeName(Fac_Type(playerid), tn, sizeof tn);
		format(msg, sizeof msg, "[Megaphone %s] %s : %s", tn, name, cmdtext[a]);
		RpX_NearLong(playerid, 60.0, 0xFFDD33FF, msg);
		return 1;
	}
	if ((a = RpX_Arg(cmdtext, "/recherche")) != -1)
	{
		if (!Fac_IsLaw(playerid)) return 0;
		new idx = a, t1[8], t2[8];
		if (!Bank_NextToken(cmdtext, idx, t1, sizeof t1) || !Bank_NextToken(cmdtext, idx, t2, sizeof t2)) return SendClientMessage(playerid, LAW_COL, "/recherche <id> <1-6> <raison>"), 1;
		while (cmdtext[idx] == ' ') idx++;
		if (strlen(cmdtext[idx]) < 3) return SendClientMessage(playerid, LAW_COL, "/recherche <id> <1-6> <raison>"), 1;
		Law_SetWanted(playerid, strval(t1), strval(t2), cmdtext[idx]);
		return 1;
	}
	if ((a = RpX_Arg(cmdtext, "/effacer")) != -1)
	{
		if (!Fac_IsLaw(playerid)) return 0;
		if (!cmdtext[a]) return SendClientMessage(playerid, LAW_COL, "/effacer <id>"), 1;
		Law_SetWanted(playerid, strval(cmdtext[a]), 0, "");
		return 1;
	}
	if (!strcmp(cmdtext, "/badge", true))
	{
		if (!Fac_IsLaw(playerid)) return 0;
		new name[MAX_PLAYER_NAME + 1], tn[16], fname[48], msg[170];
		RpX_Name(playerid, name, sizeof name);
		Law_TypeName(Fac_Type(playerid), tn, sizeof tn);
		Fac_Name(Fac_Get(playerid), fname, sizeof fname);
		format(msg, sizeof msg, "* %s montre son insigne : %s - %s.", name, tn, fname);
		RpX_Near(playerid, 15.0, 0xC2A2DAFF, msg);
		return 1;
	}
	if (!strcmp(cmdtext, "/barrage", true)) return Law_Barrier(playerid, 0), 1;
	if (!strcmp(cmdtext, "/retirerbarrage", true)) return Law_Barrier(playerid, 1), 1;
	if ((a = RpX_Arg(cmdtext, "/alerte")) != -1)
	{
		if (Fac_Type(playerid) != FACTION_ARMY) return 0;
		if (!cmdtext[a]) return SendClientMessage(playerid, LAW_COL, "/alerte <texte>"), 1;
		Law_Alert(playerid, cmdtext[a]);
		return 1;
	}
	if ((a = RpX_Arg(cmdtext, "/dossier")) != -1)
	{
		new idx = a, tg[MAX_PLAYER_NAME + 1];
		if (!Bank_NextToken(cmdtext, idx, tg, sizeof tg)) return SendClientMessage(playerid, LAW_COL, "/dossier <Prenom_Nom> <note>"), 1;
		while (cmdtext[idx] == ' ') idx++;
		Law_AddNote(playerid, tg, cmdtext[idx]);
		return 1;
	}
	return 0;
}
