// ============================================================
//  [v50] CONNEXION GOOGLE (Firebase Authentication)
//  [v51] GOOGLE UNIQUEMENT + SALON + CHOIX DU POINT D'APPARITION + CINEMATIQUE D'ARRIVEE
// ------------------------------------------------------------
//  Launcher : le joueur se connecte a Google sur l'ecran JOUER (une seule fois par telephone).
//  En jeu, la page se connecte toute seule :
//    appli -> POST http://<ip>:7790/gauth/verify (service Node) -> ticket a usage unique
//    appli -> /authact google <ticket>
//    gamemode -> POST local /gauth/redeem -> "OK|ticket|uid|email|pseudo"
//  Le compte Google EST le compte : la table google_links relie l'uid Google au personnage.
//    - uid connu      -> le joueur prend le nom de SON personnage (SetPlayerName) -> SALON
//    - uid inconnu    -> CINEMATIQUE d'arrivee -> creation du personnage (prenom, nom, genre,
//                        naissance, origine, code promo) -> SALON
//                        (ou "J'ai deja un personnage" : ancien pseudo + ancien mot de passe, une fois)
//  SALON : le personnage debout dans la scene de connexion (Cw45). "Entrer en ville" ->
//  CHOIX DU POINT D'APPARITION (camera en direct sur chaque lieu) -> apparition.
//  Commandes de la page (/authact ...) :
//    create <Prenom_Nom> <jj/mm/aaaa> <0|1> <Origine> [PROMO]   link <pseudo> <mot_de_passe>
//    gpreview <0|1>   introskip   enter   back   spawnsel <n>   spawn <n>
//  Donnees : crestwood_accounts.db -> google_links, spawn_last (derniere position)
//  Staff : /gunlink <pseudo> (niveau 4) retire la liaison Google d'un compte.
// ============================================================

#define GG_TICKET_LEN    20

new g_GgTicket[MAX_PLAYERS][24];
new g_GgPromo[MAX_PLAYERS][20];
new g_GgPendUid[MAX_PLAYERS][68];      // compte Google verifie, pas encore de personnage
new g_GgPendEmail[MAX_PLAYERS][68];
new g_GgEmail[MAX_PLAYERS][68];        // e-mail du compte Google (affiche masque dans le salon)
new g_GgLast[MAX_PLAYERS];
new g_GgStage[MAX_PLAYERS];            // 0 rien, 3 cinematique, 4 creation
new g_GgIntroStep[MAX_PLAYERS];
new g_GgIntroTimer[MAX_PLAYERS];
new g_GgCreateTick[MAX_PLAYERS];

forward Gg_OnRedeem(index, response_code, const data[]);
forward Gg_IntroShot(playerid, step);
forward Gg_LinkedMsg(playerid);

stock Gg_Reset(playerid)
{
	g_GgTicket[playerid][0] = 0;
	g_GgPromo[playerid][0] = 0;
	g_GgPendUid[playerid][0] = 0;
	g_GgPendEmail[playerid][0] = 0;
	g_GgEmail[playerid][0] = 0;
	g_GgLast[playerid] = 0;
	g_GgStage[playerid] = 0;
	g_GgIntroStep[playerid] = 0;
	if (g_GgIntroTimer[playerid]) KillTimer(g_GgIntroTimer[playerid]);
	g_GgIntroTimer[playerid] = 0;
	g_GgCreateTick[playerid] = 0;
	return 1;
}

stock Gg_InitTable()
{
	if (g_AuthDB == DB:0) return 0;
	db_free_result(db_query(g_AuthDB, "CREATE TABLE IF NOT EXISTS google_links (uid TEXT PRIMARY KEY, name TEXT NOT NULL COLLATE NOCASE, email TEXT NOT NULL DEFAULT '', linked INTEGER NOT NULL DEFAULT 0)"));
	db_free_result(db_query(g_AuthDB, "CREATE INDEX IF NOT EXISTS google_links_name ON google_links (name)"));
	db_free_result(db_query(g_AuthDB, "CREATE TABLE IF NOT EXISTS spawn_last (name TEXT PRIMARY KEY COLLATE NOCASE, x REAL, y REAL, z REAL, a REAL, ts INTEGER)"));
	return 1;
}

// n-ieme champ (0..) d'une chaine separee par |
stock Gg_Field(const s[], n, out[], size)
{
	new f = 0, j = 0;
	out[0] = 0;
	for (new i = 0; s[i] != 0; i++)
	{
		if (s[i] == '|') { if (f == n) break; f++; continue; }
		if (s[i] == '\r' || s[i] == '\n') break;
		if (f == n && j < size - 1) out[j++] = s[i];
	}
	out[j] = 0;
	return j;
}

stock Gg_Alnum(const s[])
{
	if (!s[0]) return 0;
	for (new i = 0; s[i] != 0; i++)
	{
		new c = s[i];
		if (!((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9'))) return 0;
	}
	return 1;
}

// joueur connecte (autre que except) qui porte ce nom, sinon INVALID_PLAYER_ID
stock Gg_OnlineNamed(const name[], except)
{
	new nm[MAX_PLAYER_NAME + 1];
	for (new i = 0, m = GetMaxPlayers(); i < m; i++)
	{
		if (i == except || !IsPlayerConnected(i)) continue;
		GetPlayerName(i, nm, sizeof nm);
		if (!strcmp(nm, name, true)) return i;
	}
	return INVALID_PLAYER_ID;
}

// Le joueur devient son personnage : nom de jeu + donnees chargees au nom du personnage
stock Gg_BecomeCharacter(playerid, const name[])
{
	new cur[MAX_PLAYER_NAME + 1];
	GetPlayerName(playerid, cur, sizeof cur);
	if (!strcmp(cur, name, false)) return 1;
	new other = Gg_OnlineNamed(name, playerid);
	if (other != INVALID_PLAYER_ID)
	{
		if (g_AuthLogged[other]) return -2;          // deja en jeu (autre telephone)
		Kick(other);                                  // ancienne connexion fantome au meme nom
		return -3;                                    // reessayer une fois la place liberee
	}
	if (SetPlayerName(playerid, name) == -1) return -1;
	// les modules charges au nom de connexion sont recharges au nom du personnage
	Bank_LoadPlayer(playerid);
	Gang_Load(playerid);
	g_ClothLoaded[playerid] = 0;
	g_AdminLoaded[playerid] = 0;
	g_FacLoaded[playerid] = 0;
	g_JobLoaded[playerid] = 0;
	g_FishLoaded[playerid] = 0;
	g_PhLoaded[playerid] = 0;
	g_WedLoaded[playerid] = 0;
	g_VieLoaded[playerid] = 0;
	g_C23Loaded[playerid] = 0;
	return 1;
}

// /authact google <ticket> [CODE_PROMO]
stock Gg_Do(playerid, const params[])
{
	if (g_AuthLogged[playerid]) return Sp_Resend(playerid);
	new now = GetTickCount();
	if (g_GgLast[playerid] && now - g_GgLast[playerid] < 2500) return 1;
	g_GgLast[playerid] = now;
	new idx = 0, ticket[32], promo[24];
	Bank_NextToken(params, idx, ticket, sizeof ticket);
	Bank_NextToken(params, idx, promo, sizeof promo);
	if (strlen(ticket) != GG_TICKET_LEN || !Gg_Alnum(ticket)) return Auth_Reply(playerid, "error", "Google", "Ticket de connexion invalide. Reessaie.");
	format(g_GgTicket[playerid], 24, "%s", ticket);
	g_GgPromo[playerid][0] = 0;
	if (promo[0] && strlen(promo) <= 15 && Gg_Alnum(promo)) format(g_GgPromo[playerid], 20, "%s", promo);
	new data[96];
	format(data, sizeof data, "secret=%s&ticket=%s", IG_SECRET, ticket);
	HTTP(playerid, HTTP_POST, IG_LOCAL_URL "/gauth/redeem", data, "Gg_OnRedeem");
	return 1;
}

public Gg_OnRedeem(index, response_code, const data[])
{
	if (index < 0 || index >= MAX_PLAYERS || !IsPlayerConnected(index) || g_AuthLogged[index]) return 0;
	new playerid = index;
	if (response_code != 200)
	{
		printf("[gauth] service injoignable (%d) : lance server/instagram/ig-server.js", response_code);
		return Gg_Fail(playerid, "Serveur de connexion indisponible. Reessaie dans un instant.");
	}
	new st[8], ticket[32], uid[68], email[68], who[MAX_PLAYER_NAME + 1];
	Gg_Field(data, 0, st, sizeof st);
	if (strcmp(st, "OK", false) != 0) return Gg_Fail(playerid, "Connexion Google expiree. Reessaie.");
	Gg_Field(data, 1, ticket, sizeof ticket);
	Gg_Field(data, 2, uid, sizeof uid);
	Gg_Field(data, 3, email, sizeof email);
	Gg_Field(data, 4, who, sizeof who);
	if (!g_GgTicket[playerid][0] || strcmp(ticket, g_GgTicket[playerid], false) != 0) return 0;
	g_GgTicket[playerid][0] = 0;
	if (!Gg_Alnum(uid)) return Gg_Fail(playerid, "Compte Google illisible.");
	if (!strcmp(email, "-", false) || !Auth_IsPlainText(email, true) || strlen(email) > 63) email[0] = 0;
	format(g_GgEmail[playerid], 68, "%s", email);
	Gg_InitTable();

	new q[220], linked[MAX_PLAYER_NAME + 1];
	linked[0] = 0;
	format(q, sizeof q, "SELECT name FROM google_links WHERE uid = '%s' LIMIT 1", uid);
	new DBResult:r = db_query(g_AuthDB, q);
	if (db_num_rows(r) > 0) db_get_field(r, 0, linked, sizeof linked);
	db_free_result(r);
	if (linked[0] && !Auth_Exists(linked))
	{
		format(q, sizeof q, "DELETE FROM google_links WHERE uid = '%s'", uid);   // personnage supprime
		db_free_result(db_query(g_AuthDB, q));
		linked[0] = 0;
	}
	if (linked[0])
	{
		new res = Gg_BecomeCharacter(playerid, linked);
		if (res == -2) return Gg_Fail(playerid, "Ton personnage est deja en jeu sur un autre telephone.");
		if (res == -3) return Gg_Fail(playerid, "Ancienne connexion fermee. Reessaie dans 3 secondes.");
		if (res < 0) return Gg_Fail(playerid, "Impossible de prendre le nom de ton personnage. Previens un admin.");
		g_AuthFails[playerid] = 0;
		printf("[gauth] connexion de %s (%s)", linked, email[0] ? email : uid);
		return Auth_LoginOk(playerid, linked, "");
	}
	// nouveau compte Google : cinematique d'arrivee puis creation du personnage
	format(g_GgPendUid[playerid], 68, "%s", uid);
	format(g_GgPendEmail[playerid], 68, "%s", email);
	return Gg_IntroStart(playerid);
}

stock Gg_Fail(playerid, const msg[])
{
	new json[260];
	format(json, sizeof json, "{\"type\":\"error\",\"title\":\"Connexion\",\"message\":\"%s\",\"retry\":1}", msg);
	return Auth_Send(playerid, AUTH_DIALOG_REPLY, json);
}

// La page redemande son ecran (rechargement) : 1 si on a repondu
stock Gg_Resume(playerid)
{
	if (g_GgStage[playerid] == 4) return Gg_SendCreate(playerid), 1;
	if (g_GgStage[playerid] == 3) return Gg_SendIntro(playerid), 1;
	return 0;
}

// ------------------------------------------------------------ cinematique d'arrivee (nouveau joueur)
#define GG_SHOTS 5
new const Float:g_GgShotCam[GG_SHOTS][6] =
{
	{  40.0, -2100.0,  60.0,   260.0, -1960.0,  40.0},    // la mer, Santa Maria
	{1200.0, -1150.0, 170.0,  1450.0, -1050.0, 150.0},    // les tours du centre-ville
	{2440.0, -1720.0,  18.0,  2540.0, -1715.0,  16.0},    // Grove Street, au ras du sol
	{1300.0, -1000.0, 140.0,  1520.0,  -980.0, 130.0},    // les collines de Vinewood
	{1481.0, -1920.0,  55.0,  1481.0, -1830.0,  22.0}     // la mairie : on s'approche
};
new const Float:g_GgShotLook[GG_SHOTS][6] =
{
	{ 370.0, -1810.0,  10.0,   840.0, -1900.0,  15.0},
	{1550.0, -1350.0,  60.0,  1700.0, -1350.0,  60.0},
	{2495.0, -1665.0,  14.0,  2495.0, -1660.0,  14.0},
	{1400.0,  -800.0,  90.0,  1420.0,  -790.0,  90.0},
	{1481.0, -1744.0,  30.0,  1481.0, -1744.0,  18.0}
};
new const Float:g_GgShotPed[GG_SHOTS][3] =
{
	{ 370.0, -1810.0,  7.9},
	{1554.0, -1675.0, 16.2},
	{2495.0, -1687.0, 13.6},
	{1554.0, -1675.0, 16.2},
	{1481.0, -1744.0, 13.6}
};
new const g_GgShotMs[GG_SHOTS] = {5600, 5600, 5200, 5200, 6400};

stock Gg_SendIntro(playerid)
{
	new json[80];
	format(json, sizeof json, "{\"screen\":\"intro\",\"step\":%d,\"n\":%d}", g_GgIntroStep[playerid], GG_SHOTS);
	return Auth_Send(playerid, AUTH_DIALOG_OPEN, json);
}

stock Gg_IntroStart(playerid)
{
	g_GgStage[playerid] = 3;
	g_GgIntroStep[playerid] = 0;
	Lg_End(playerid);                       // le decor de la terrasse est retire pendant le film
	SetPlayerVirtualWorld(playerid, 7000 + playerid);
	SetPlayerInterior(playerid, 0);
	SetPlayerTime(playerid, 20, 40);        // la ville s'allume
	SetPlayerWeather(playerid, 1);
	if (g_GgIntroTimer[playerid]) KillTimer(g_GgIntroTimer[playerid]);
	g_GgIntroTimer[playerid] = SetTimerEx("Gg_IntroShot", 400, false, "ii", playerid, 0);
	return Gg_SendIntro(playerid);
}

public Gg_IntroShot(playerid, step)
{
	g_GgIntroTimer[playerid] = 0;
	if (!IsPlayerConnected(playerid) || g_AuthLogged[playerid] || g_GgStage[playerid] != 3) return 0;
	if (step >= GG_SHOTS) return Gg_IntroEnd(playerid);
	g_GgIntroStep[playerid] = step;
	SetPlayerVirtualWorld(playerid, 7000 + playerid);
	SetPlayerPos(playerid, g_GgShotPed[step][0], g_GgShotPed[step][1], g_GgShotPed[step][2]);
	// [v59] camera fixe d'abord : un client sans InterpolateCamera reste sur la ville (pas sur la mer noire)
	SetPlayerCameraPos(playerid, g_GgShotCam[step][0], g_GgShotCam[step][1], g_GgShotCam[step][2]);
	SetPlayerCameraLookAt(playerid, g_GgShotLook[step][0], g_GgShotLook[step][1], g_GgShotLook[step][2]);
	InterpolateCameraPos(playerid, g_GgShotCam[step][0], g_GgShotCam[step][1], g_GgShotCam[step][2], g_GgShotCam[step][3], g_GgShotCam[step][4], g_GgShotCam[step][5], g_GgShotMs[step], CAMERA_MOVE);
	InterpolateCameraLookAt(playerid, g_GgShotLook[step][0], g_GgShotLook[step][1], g_GgShotLook[step][2], g_GgShotLook[step][3], g_GgShotLook[step][4], g_GgShotLook[step][5], g_GgShotMs[step], CAMERA_MOVE);
	Gg_SendIntro(playerid);
	g_GgIntroTimer[playerid] = SetTimerEx("Gg_IntroShot", g_GgShotMs[step], false, "ii", playerid, step + 1);
	return 1;
}

stock Gg_IntroEnd(playerid)
{
	if (g_GgIntroTimer[playerid]) KillTimer(g_GgIntroTimer[playerid]);
	g_GgIntroTimer[playerid] = 0;
	g_GgStage[playerid] = 4;
	// genre par defaut pour l'apercu (le joueur choisit ensuite)
	if (g_IdGender[playerid] != 0 && g_IdGender[playerid] != 1) g_IdGender[playerid] = 0;
	Gg_PreviewBody(playerid);
	Lg_Begin(playerid);                     // retour sur la terrasse : le personnage attend
	return Gg_SendCreate(playerid);
}

// "suggest" : le pseudo de connexion possede deja un compte (ancien joueur) -> on propose de le recuperer
stock Gg_SendCreate(playerid)
{
	new json[200], name[MAX_PLAYER_NAME + 1], sug[MAX_PLAYER_NAME + 1];
	GetPlayerName(playerid, name, sizeof name);
	sug[0] = 0;
	if (Auth_Exists(name))
	{
		new q[160];
		format(q, sizeof q, "SELECT uid FROM google_links WHERE name = '%s' LIMIT 1", name);
		new DBResult:r = db_query(g_AuthDB, q), n = db_num_rows(r);
		db_free_result(r);
		if (n < 1) format(sug, sizeof sug, "%s", name);
	}
	format(json, sizeof json, "{\"screen\":\"create\",\"suggest\":\"%s\",\"gender\":%d}", sug, g_IdGender[playerid] == 1 ? 1 : 0);
	return Auth_Send(playerid, AUTH_DIALOG_OPEN, json);
}

// corps du genre choisi, tenue par defaut (apercu dans la scene)
stock Gg_PreviewBody(playerid)
{
	if (!g_ClothLoaded[playerid]) Cloth_Load(playerid);
	new body = Id_BodyOf(playerid);
	if (g_ClothSkin[playerid] != body || g_GgStage[playerid] == 4)
	{
		for (new i = 0; i < CLOTH_COUNT; i++) g_ClothV[playerid][i] = g_ClothDefault[i];
		g_ClothSkin[playerid] = body;
	}
	g_ClothSaved[playerid] = 0;
	Cloth_Send(playerid, CLOTH_DIALOG_APPLY);
	return 1;
}

// ------------------------------------------------------------ creation du personnage
// Prenom ou Nom : 2 a 12 lettres, majuscule puis minuscules (la page met en forme)
stock Gg_NamePart(const s[], from, to)
{
	if (to - from < 2 || to - from > 12) return 0;
	for (new i = from; i < to; i++)
	{
		new c = s[i];
		if (i == from) { if (c < 'A' || c > 'Z') return 0; }
		else if (c < 'a' || c > 'z') return 0;
	}
	return 1;
}

stock Gg_ValidName(const name[])
{
	new len = strlen(name), us = -1;
	if (len < 5 || len > 24) return 0;
	for (new i = 0; i < len; i++) if (name[i] == '_') { if (us != -1) return 0; us = i; }
	if (us < 1) return 0;
	return Gg_NamePart(name, 0, us) && Gg_NamePart(name, us + 1, len);
}

// date jj/mm/aaaa -> 1 si valide
stock Gg_ParseBirth(const birth[], &d, &mo, &y)
{
	new part = 0, val = 0, len = strlen(birth);
	d = 0; mo = 0; y = 0;
	if (len < 8 || len > 10) return 0;
	for (new i = 0; i <= len; i++)
	{
		if (i == len || birth[i] == '/')
		{
			if (part == 0) d = val; else if (part == 1) mo = val; else y = val;
			part++; val = 0;
		}
		else if (birth[i] >= '0' && birth[i] <= '9') val = val * 10 + (birth[i] - '0');
		else return 0;
	}
	return (part == 3 && d >= 1 && d <= 31 && mo >= 1 && mo <= 12 && y >= 1930 && y <= 2010);
}

// /authact create <Prenom_Nom> <jj/mm/aaaa> <0|1> <Origine> [PROMO]
stock Gg_Create(playerid, const params[])
{
	if (g_AuthLogged[playerid] || !g_GgPendUid[playerid][0]) return Auth_Reply(playerid, "error", "Personnage", "Connecte-toi d'abord avec Google.");
	new now = GetTickCount();
	if (g_GgCreateTick[playerid] && now - g_GgCreateTick[playerid] < 1500) return 1;
	g_GgCreateTick[playerid] = now;
	new idx = 0, name[32], birth[16], gtok[4], origin[32], promo[24];
	Bank_NextToken(params, idx, name, sizeof name);
	Bank_NextToken(params, idx, birth, sizeof birth);
	Bank_NextToken(params, idx, gtok, sizeof gtok);
	Bank_NextToken(params, idx, origin, sizeof origin);
	Bank_NextToken(params, idx, promo, sizeof promo);
	if (!Gg_ValidName(name)) return Auth_Reply(playerid, "error", "Nom du personnage", "Prenom et nom : 2 a 12 lettres chacun, sans chiffres ni accents.");
	if ((gtok[0] != '0' && gtok[0] != '1') || gtok[1] != 0) return Auth_Reply(playerid, "error", "Genre", "Choisis Homme ou Femme.");
	new d, mo, y;
	if (!Gg_ParseBirth(birth, d, mo, y)) return Auth_Reply(playerid, "error", "Date de naissance", "Date invalide (annee entre 1930 et 2010).");
	new clean[26], j = 0;
	for (new i = 0; origin[i] != 0 && j < sizeof(clean) - 1; i++)
	{
		new c = origin[i];
		if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '-') clean[j++] = c;
		else if (c == '_') clean[j++] = ' ';
	}
	clean[j] = 0;
	if (j < 2) return Auth_Reply(playerid, "error", "Origine", "Indique ton origine.");
	if (Auth_Exists(name)) return Auth_Reply(playerid, "error", "Nom du personnage", "Ce nom existe deja a Crestwood. Choisis-en un autre.");
	new res = Gg_BecomeCharacter(playerid, name);
	if (res == -2 || res == -3 || res == -1) return Auth_Reply(playerid, "error", "Nom du personnage", "Ce nom est deja pris en ville. Choisis-en un autre.");
	// code promo (verifie apres le changement de nom : on ne peut pas utiliser son propre code)
	new promoCode[24];
	promoCode[0] = 0;
	if (promo[0])
	{
		new perr[96];
		if (!Promo_SignupValidate(playerid, promo, promoCode, sizeof promoCode, perr, sizeof perr)) return Auth_Reply(playerid, "error", "Code promo", perr);
	}
	new email[68], q[400];
	format(email, sizeof email, "%s", g_GgPendEmail[playerid]);
	if (email[0])
	{
		format(q, sizeof q, "SELECT name FROM accounts WHERE email = '%s' LIMIT 1", email);
		new DBResult:r = db_query(g_AuthDB, q);
		if (db_num_rows(r) > 0) email[0] = 0;
		db_free_result(r);
	}
	new salt[24], hash[24], rnd[24];
	Auth_MakeSalt(salt, sizeof salt);
	format(rnd, sizeof rnd, "G%08x%08x", random(2147483647), random(2147483647));
	Auth_Hash(rnd, salt, hash, sizeof hash);
	g_IdGender[playerid] = strval(gtok);
	format(g_IdBirth[playerid], 12, "%02d/%02d/%04d", d, mo, y);
	format(g_IdOrigin[playerid], 26, "%s", clean);
	format(q, sizeof q, "INSERT INTO accounts (name, email, salt, hash, created, birth, origin, gender) VALUES ('%s', '%s', '%s', '%s', %d, '%s', '%s', %d)",
		name, email, salt, hash, gettime(), g_IdBirth[playerid], g_IdOrigin[playerid], g_IdGender[playerid]);
	db_free_result(db_query(g_AuthDB, q));
	if (!Auth_Exists(name)) return Auth_Reply(playerid, "error", "Personnage", "Creation impossible. Previens un admin.");
	Gg_Link(g_GgPendUid[playerid], name, email);
	if (promoCode[0]) Promo_SignupStore(name, promoCode);
	printf("[gauth] personnage cree : %s (%s)", name, email[0] ? email : g_GgPendUid[playerid]);
	g_GgPendUid[playerid][0] = 0;
	g_GgPendEmail[playerid][0] = 0;
	g_GgStage[playerid] = 0;
	// apparence : corps du genre, tenue par defaut, editeur de vetements a la 1re apparition
	g_ClothLoaded[playerid] = 1;
	g_ClothSkin[playerid] = Id_BodyOf(playerid);
	for (new i = 0; i < CLOTH_COUNT; i++) g_ClothV[playerid][i] = g_ClothDefault[i];
	g_ClothSaved[playerid] = 0;
	return Auth_LoginOk(playerid, name, "");
}

// /authact link <pseudo> <mot_de_passe> : recupere un ancien personnage (une seule fois)
stock Gg_LinkOld(playerid, const params[])
{
	if (g_AuthLogged[playerid] || !g_GgPendUid[playerid][0]) return Auth_Reply(playerid, "error", "Ancien personnage", "Connecte-toi d'abord avec Google.");
	if (GetTickCount() < g_AuthLockUntil[playerid]) return Auth_Reply(playerid, "warning", "Patiente", "Trop d'essais. Reessaie dans une minute.");
	new idx = 0, user[32], pass[128];
	Bank_NextToken(params, idx, user, sizeof user);
	Bank_NextToken(params, idx, pass, sizeof pass);
	if (!user[0] || !pass[0] || strlen(user) > MAX_PLAYER_NAME || !Auth_IsPlainText(user, false)) return Auth_Reply(playerid, "error", "Ancien personnage", "Entre le pseudo et l'ancien mot de passe.");
	new q[200];
	format(q, sizeof q, "SELECT name, salt, hash FROM accounts WHERE name = '%s' LIMIT 1", user);
	new DBResult:r = db_query(g_AuthDB, q);
	if (db_num_rows(r) < 1) { db_free_result(r); return Auth_Reply(playerid, "error", "Ancien personnage", "Ce personnage n'existe pas."); }
	new real[MAX_PLAYER_NAME + 1], salt[24], hash[24], check[24];
	db_get_field(r, 0, real, sizeof real);
	db_get_field(r, 1, salt, sizeof salt);
	db_get_field(r, 2, hash, sizeof hash);
	db_free_result(r);
	format(q, sizeof q, "SELECT uid FROM google_links WHERE name = '%s' LIMIT 1", real);
	r = db_query(g_AuthDB, q);
	new taken = db_num_rows(r);
	db_free_result(r);
	if (taken > 0) return Auth_Reply(playerid, "error", "Ancien personnage", "Ce personnage est deja relie a un compte Google.");
	Auth_Hash(pass, salt, check, sizeof check);
	if (strcmp(check, hash, false) != 0)
	{
		g_AuthFails[playerid]++;
		if (g_AuthFails[playerid] >= AUTH_MAX_FAILS)
		{
			g_AuthFails[playerid] = 0;
			g_AuthLockUntil[playerid] = GetTickCount() + AUTH_LOCK_MS;
			return Auth_Reply(playerid, "warning", "Bloque", "Trop d'essais. Reessaie dans une minute.");
		}
		new m[64];
		format(m, sizeof m, "Mot de passe incorrect (%d/%d).", g_AuthFails[playerid], AUTH_MAX_FAILS);
		return Auth_Reply(playerid, "error", "Ancien personnage", m);
	}
	new res = Gg_BecomeCharacter(playerid, real);
	if (res == -2) return Auth_Reply(playerid, "error", "Ancien personnage", "Ce personnage est deja en jeu.");
	if (res < 0) return Auth_Reply(playerid, "error", "Ancien personnage", "Reessaie dans quelques secondes.");
	Gg_Link(g_GgPendUid[playerid], real, g_GgPendEmail[playerid]);
	printf("[gauth] ancien personnage %s relie a %s", real, g_GgPendEmail[playerid][0] ? g_GgPendEmail[playerid] : g_GgPendUid[playerid]);
	g_GgPendUid[playerid][0] = 0;
	g_GgPendEmail[playerid][0] = 0;
	g_GgStage[playerid] = 0;
	g_AuthFails[playerid] = 0;
	SetTimerEx("Gg_LinkedMsg", 2500, false, "i", playerid);
	return Auth_LoginOk(playerid, real, "");
}

stock Gg_Link(const uid[], const name[], const email[])
{
	new q[240];
	format(q, sizeof q, "INSERT OR REPLACE INTO google_links (uid, name, email, linked) VALUES ('%s', '%s', '%s', %d)", uid, name, email, gettime());
	db_free_result(db_query(g_AuthDB, q));
	return 1;
}

// (v50) liaison apres un mot de passe : plus utilise depuis v51 (gardee pour compatibilite)
stock Gg_OnPasswordLogin(playerid, const user[])
{
	if (!g_GgPendUid[playerid][0]) return 0;
	Gg_InitTable();
	Gg_Link(g_GgPendUid[playerid], user, g_GgPendEmail[playerid]);
	g_GgPendUid[playerid][0] = 0;
	g_GgPendEmail[playerid][0] = 0;
	SetTimerEx("Gg_LinkedMsg", 2500, false, "i", playerid);
	return 1;
}

public Gg_LinkedMsg(playerid)
{
	if (!IsPlayerConnected(playerid)) return 0;
	return Cw_Notify(playerid, "success", "Compte Google", "Ton personnage est maintenant relie a ton compte Google.", 6000);
}

// ------------------------------------------------------------ salon + points d'apparition
#define SP_COUNT 6
new const Float:g_SpPos[SP_COUNT][4] =
{
	{1481.0, -1743.0, 13.55,   0.0},     // 0 Mairie de Los Santos (City Hall)
	{1742.9, -1860.5, 13.58,   0.0},     // 1 Gare Unity Station
	{1685.7, -2240.0, 13.55, 180.0},     // 2 Aeroport de Los Santos
	{ 370.0, -1810.0,  7.84,  90.0},     // 3 Plage de Santa Maria
	{2495.0, -1687.0, 13.52,   0.0},     // 4 Grove Street
	{1172.6, -1323.3, 15.40, 270.0}      // 5 Hopital All Saints
};
new g_SpSel[MAX_PLAYERS];
new g_SpReset[MAX_PLAYERS];
new g_SpSpawned[MAX_PLAYERS];
new g_SpHasLast[MAX_PLAYERS];
new Float:g_SpLast[MAX_PLAYERS][4];
new g_SpStage[MAX_PLAYERS];            // 1 salon, 2 choix du lieu

stock Sp_LoadLast(playerid)
{
	g_SpHasLast[playerid] = 0;
	Gg_InitTable();
	new name[MAX_PLAYER_NAME + 1], q[160], f[24];
	GetPlayerName(playerid, name, sizeof name);
	format(q, sizeof q, "SELECT x, y, z, a FROM spawn_last WHERE name = '%s' LIMIT 1", name);
	new DBResult:r = db_query(g_AuthDB, q);
	if (r == DBResult:0) return 0;
	if (db_num_rows(r) > 0)
	{
		for (new k = 0; k < 4; k++) { db_get_field(r, k, f, sizeof f); g_SpLast[playerid][k] = floatstr(f); }
		g_SpHasLast[playerid] = (floatabs(g_SpLast[playerid][0]) > 1.0 || floatabs(g_SpLast[playerid][1]) > 1.0);
	}
	db_free_result(r);
	return g_SpHasLast[playerid];
}

// e-mail masque pour l'affichage : an****@gmail.com
stock Sp_MaskEmail(const email[], out[], size)
{
	new at = strfind(email, "@");
	if (at < 1) { format(out, size, ""); return 0; }
	new keep = (at >= 3) ? 2 : 1, j = 0;
	for (new i = 0; email[i] != 0 && j < size - 1; i++)
	{
		if (i < keep || i >= at) out[j++] = email[i];
		else if (i < keep + 4) out[j++] = '*';
	}
	out[j] = 0;
	return 1;
}

stock Sp_Lobby(playerid)
{
	g_LgLobby[playerid] = 1;
	g_SpStage[playerid] = 1;
	Sp_LoadLast(playerid);
	Lg_Begin(playerid);   // (applique aussi l'apparence du personnage)
	return Sp_SendLobby(playerid);
}

stock Sp_SendLobby(playerid)
{
	new json[420], name[MAX_PLAYER_NAME + 1], mail[72], cash[16], bank[16];
	GetPlayerName(playerid, name, sizeof name);
	Sp_MaskEmail(g_GgEmail[playerid], mail, sizeof mail);
	Cw_Fmt(g_BankCashLoad[playerid], cash, sizeof cash);
	Cw_Fmt(g_BankBalance[playerid], bank, sizeof bank);
	format(json, sizeof json, "{\"screen\":\"lobby\",\"name\":\"%s\",\"email\":\"%s\",\"birth\":\"%s\",\"origin\":\"%s\",\"gender\":%d,\"cash\":\"%s\",\"bank\":\"%s\",\"last\":%d}",
		name, mail, g_IdBirth[playerid], g_IdOrigin[playerid], g_IdGender[playerid], cash, bank, g_SpHasLast[playerid]);
	return Auth_Send(playerid, AUTH_DIALOG_OPEN, json);
}

stock Sp_SendSpawn(playerid)
{
	new json[120];
	format(json, sizeof json, "{\"screen\":\"spawn\",\"sel\":%d,\"last\":%d,\"count\":%d}", g_SpSel[playerid], g_SpHasLast[playerid], SP_COUNT);
	return Auth_Send(playerid, AUTH_DIALOG_OPEN, json);
}

stock Sp_Resend(playerid)
{
	if (!g_AuthLogged[playerid] || !g_LgLobby[playerid]) return 1;
	if (g_SpStage[playerid] == 2) return Sp_SendSpawn(playerid);
	return Sp_SendLobby(playerid);
}

// position d'un choix : 0..SP_COUNT-1 lieux fixes, SP_COUNT = derniere position
stock Sp_Get(playerid, idx, &Float:x, &Float:y, &Float:z, &Float:a)
{
	if (idx == SP_COUNT && g_SpHasLast[playerid]) { x = g_SpLast[playerid][0]; y = g_SpLast[playerid][1]; z = g_SpLast[playerid][2]; a = g_SpLast[playerid][3]; return 1; }
	if (idx < 0 || idx >= SP_COUNT) idx = 0;
	x = g_SpPos[idx][0]; y = g_SpPos[idx][1]; z = g_SpPos[idx][2]; a = g_SpPos[idx][3];
	return 1;
}

// camera en direct sur le lieu : lent travelling autour du personnage
stock Sp_Preview(playerid, idx)
{
	new Float:x, Float:y, Float:z, Float:a;
	Sp_Get(playerid, idx, x, y, z, a);
	g_SpSel[playerid] = idx;
	Lg_End(playerid);
	SetPlayerVirtualWorld(playerid, 7000 + playerid);
	SetPlayerInterior(playerid, 0);
	if (g_CwRealTime) { new h, mi, s; gettime(h, mi, s); SetPlayerTime(playerid, h, mi); }
	else SetPlayerTime(playerid, 12, 0);
	SetPlayerWeather(playerid, g_WxNow);
	SetPlayerPos(playerid, x, y, z + 0.4);
	SetPlayerFacingAngle(playerid, a);
	new Float:r = 11.0, Float:h = 5.5, Float:a0 = a + 35.0, Float:a1 = a - 35.0;
	// [v59] camera fixe d'abord (secours si le client ne gere pas InterpolateCamera)
	SetPlayerCameraPos(playerid, x + r * floatsin(-a0, degrees), y + r * floatcos(-a0, degrees), z + h);
	SetPlayerCameraLookAt(playerid, x, y, z + 1.0);
	InterpolateCameraPos(playerid, x + r * floatsin(-a0, degrees), y + r * floatcos(-a0, degrees), z + h,
		x + r * floatsin(-a1, degrees), y + r * floatcos(-a1, degrees), z + h - 1.5, 12000, CAMERA_MOVE);
	InterpolateCameraLookAt(playerid, x, y, z + 1.0, x, y, z + 0.8, 12000, CAMERA_MOVE);
	return 1;
}

stock Sp_Go(playerid, idx)
{
	new Float:x, Float:y, Float:z, Float:a;
	Sp_Get(playerid, idx, x, y, z, a);
	g_LgLobby[playerid] = 0;
	g_SpStage[playerid] = 0;
	SetSpawnInfo(playerid, NO_TEAM, Id_BodyOf(playerid), x, y, z, a, 0, 0, 0, 0, 0, 0);
	g_SpReset[playerid] = 1;
	printf("[spawn] %d apparait au choix %d (%.0f, %.0f)", playerid, idx, x, y);
	SetTimerEx("Sp_SafeCam", 1500, false, "i", playerid);   // [v59] secours anti ecran noir
	return Auth_Finish(playerid);
}

// [v59] 1,5 s apres l'apparition : camera derriere le joueur, monde normal, page fermee
forward Sp_SafeCam(playerid);
public Sp_SafeCam(playerid)
{
	if (!IsPlayerConnected(playerid) || !g_AuthLogged[playerid] || g_LgLobby[playerid]) return 0;
	new st = GetPlayerState(playerid);
	if (st == PLAYER_STATE_NONE || st == PLAYER_STATE_WASTED || st == PLAYER_STATE_SPECTATING) return 0;
	if (GetPlayerVirtualWorld(playerid) == 7000 + playerid) SetPlayerVirtualWorld(playerid, 0);
	new e[3] = "{}";
	Auth_Send(playerid, AUTH_DIALOG_CLOSE, e);
	SetCameraBehindPlayer(playerid);
	return 1;
}

stock Sp_Action(playerid, const action[], const params[])
{
	if (!g_AuthLogged[playerid] || !g_LgLobby[playerid] || g_IdPending[playerid]) return 0;
	if (!strcmp(action, "enter", true))
	{
		g_SpStage[playerid] = 2;
		if (g_SpSel[playerid] < 0 || g_SpSel[playerid] > SP_COUNT || (g_SpSel[playerid] == SP_COUNT && !g_SpHasLast[playerid])) g_SpSel[playerid] = 0;
		if (g_SpHasLast[playerid] && g_SpSel[playerid] == 0) g_SpSel[playerid] = SP_COUNT;
		Sp_Preview(playerid, g_SpSel[playerid]);
		return Sp_SendSpawn(playerid), 1;
	}
	if (!strcmp(action, "back", true))
	{
		g_SpStage[playerid] = 1;
		Lg_Begin(playerid);
		return Sp_SendLobby(playerid), 1;
	}
	new tk = 0, tok[8];
	Bank_NextToken(params, tk, tok, sizeof tok);
	new n = strval(tok);
	if (n < 0 || n > SP_COUNT || (n == SP_COUNT && !g_SpHasLast[playerid])) n = 0;
	if (!strcmp(action, "spawnsel", true)) { if (g_SpStage[playerid] == 2) Sp_Preview(playerid, n); return 1; }
	if (!strcmp(action, "spawn", true)) { if (g_SpStage[playerid] == 2) Sp_Go(playerid, n); return 1; }
	return 0;
}

// routeur des actions v51 de la page (appele par Auth_OnCommand)
stock Gg_Action(playerid, const action[], const params[])
{
	if (!strcmp(action, "create", true)) return Gg_Create(playerid, params);
	if (!strcmp(action, "link", true)) return Gg_LinkOld(playerid, params);
	if (!strcmp(action, "introskip", true))
	{
		if (!g_AuthLogged[playerid] && g_GgStage[playerid] == 3) Gg_IntroEnd(playerid);
		return 1;
	}
	if (!strcmp(action, "gpreview", true))
	{
		if (g_AuthLogged[playerid] || g_GgStage[playerid] != 4) return 1;
		new tk = 0, tok[8];
		Bank_NextToken(params, tk, tok, sizeof tok);
		g_IdGender[playerid] = (strval(tok) == 1) ? 1 : 0;
		return Gg_PreviewBody(playerid);
	}
	return Sp_Action(playerid, action, params);
}

stock Cw50_OnConnect(playerid)
{
	g_SpSel[playerid] = 0;
	g_SpReset[playerid] = 0;
	g_SpSpawned[playerid] = 0;
	g_SpHasLast[playerid] = 0;
	g_SpStage[playerid] = 0;
	return 1;
}

stock Cw50_OnSpawn(playerid)
{
	g_SpSpawned[playerid] = 1;
	if (g_SpReset[playerid])
	{
		// reapparitions suivantes (mort, hopital...) : point d'apparition habituel du serveur
		g_SpReset[playerid] = 0;
		SetSpawnInfo(playerid, NO_TEAM, Id_BodyOf(playerid), 1958.3783, 1343.1572, 15.3746, 269.1425, 0, 0, 0, 0, 0, 0);
		SetPlayerVirtualWorld(playerid, 0);
		SetPlayerInterior(playerid, 0);
		SetCameraBehindPlayer(playerid);
	}
	return 1;
}

// derniere position (dehors seulement : pas d'interieur, pas de monde prive)
stock Cw50_OnDisconnect(playerid)
{
	if (g_GgIntroTimer[playerid]) { KillTimer(g_GgIntroTimer[playerid]); g_GgIntroTimer[playerid] = 0; }
	if (!g_AuthLogged[playerid] || !g_SpSpawned[playerid] || g_LgLobby[playerid]) return 0;
	if (GetPlayerInterior(playerid) != 0 || GetPlayerVirtualWorld(playerid) != 0) return 0;
	new st = GetPlayerState(playerid);
	if (st == PLAYER_STATE_WASTED || st == PLAYER_STATE_SPECTATING || st == PLAYER_STATE_NONE) return 0;
	new Float:x, Float:y, Float:z, Float:a;
	GetPlayerPos(playerid, x, y, z);
	if (IsPlayerInAnyVehicle(playerid)) GetVehicleZAngle(GetPlayerVehicleID(playerid), a); else GetPlayerFacingAngle(playerid, a);
	if (floatabs(x) < 1.0 && floatabs(y) < 1.0) return 0;
	Gg_InitTable();
	new name[MAX_PLAYER_NAME + 1], q[220];
	GetPlayerName(playerid, name, sizeof name);
	format(q, sizeof q, "INSERT OR REPLACE INTO spawn_last (name, x, y, z, a, ts) VALUES ('%s', %.2f, %.2f, %.2f, %.1f, %d)", name, x, y, z + 0.3, a, gettime());
	db_free_result(db_query(g_AuthDB, q));
	return 1;
}

stock Cw50_OnCommand(playerid, const cmdtext[])
{
	new idx;
	if (Fx_IsCmd(cmdtext, "/gunlink", idx))
	{
		if (Admin_Level(playerid) < 4) return 0;
		new who[MAX_PLAYER_NAME + 1], q[160];
		Frp_Tok(cmdtext, idx, who, sizeof who);
		if (!who[0] || !Auth_IsPlainText(who, false)) return Cw_Notify(playerid, "denied", "Staff", "/gunlink <pseudo>", 3500), 1;
		Gg_InitTable();
		format(q, sizeof q, "SELECT uid FROM google_links WHERE name = '%s'", who);
		new DBResult:r = db_query(g_AuthDB, q), n = db_num_rows(r);
		db_free_result(r);
		if (n < 1) return Cw_Notify(playerid, "denied", "Staff", "Ce compte n'est pas lie a Google.", 3500), 1;
		format(q, sizeof q, "DELETE FROM google_links WHERE name = '%s'", who);
		db_free_result(db_query(g_AuthDB, q));
		new m[96];
		format(m, sizeof m, "Liaison Google retiree pour %s.", who);
		return Cw_Notify(playerid, "success", "Staff", m, 4000), 1;
	}
	return 0;
}
