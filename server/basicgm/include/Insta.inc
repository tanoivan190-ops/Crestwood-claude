// ============================================================
//  [v31] TELEPHONE : INSTAGRAM DE LA VILLE + CARTE DE L'APPLI UBER
// ------------------------------------------------------------
//  INSTAGRAM : les photos (prises avec l'appli Camera du telephone) sont publiees sur un petit
//  service Node.js (server/instagram/ig-server.js) lance sur la MEME machine que le serveur SA-MP.
//    1. le telephone envoie /phoneact ig
//    2. le gamemode cree un jeton pour le joueur et le declare au service (HTTP local, IG_SECRET)
//    3. le telephone recoit {"ev":"ig","ok":1,"token":"...","name":"Prenom_Nom","port":7790}
//       et parle ensuite directement au service (http://<ip du serveur>:7790) pour le fil,
//       les publications, les j'aime et les commentaires.
//  Staff : /igdel <numero> supprime une publication (niveau IG_ADMIN_LVL).
//  UBER : /phoneact pos -> position du joueur (+ celle du chauffeur de taxi qui a pris la course)
//         /phoneact gps <x> <y> -> destination choisie sur la carte de l'appli (GPS du jeu)
// ============================================================

#define IG_SECRET           "change-moi-crestwood"   // meme valeur que IG_SECRET du service Node
#define IG_LOCAL_URL        "127.0.0.1:7790"         // adresse du service vue depuis le serveur SA-MP
#define IG_PORT             7790                     // port ouvert aux telephones
#define IG_ADMIN_LVL        3

new g_IgToken[MAX_PLAYERS][24];
new g_IgLast[MAX_PLAYERS];

forward Ig_OnAuth(index, response_code, const data[]);
forward Ig_OnDelete(index, response_code, const data[]);

stock Ig_OnConnect(playerid)
{
	g_IgToken[playerid][0] = 0;
	g_IgLast[playerid] = 0;
	return 1;
}

stock Ig_MakeToken(out[], size)
{
	static const chars[] = "abcdefghijkmnpqrstuvwxyz23456789";
	for (new i = 0; i < size - 1 && i < 20; i++) out[i] = chars[random(sizeof(chars) - 1)];
	out[min(size - 1, 20)] = 0;
	return 1;
}

stock Ig_Open(playerid)
{
	new now = GetTickCount();
	if (g_IgLast[playerid] && now - g_IgLast[playerid] < 3000 && g_IgToken[playerid][0])
	{
		// jeton deja valide : on le renvoie sans redemander au service
		return Ig_SendToken(playerid, 1);
	}
	g_IgLast[playerid] = now;
	Ig_MakeToken(g_IgToken[playerid], 24);
	new name[MAX_PLAYER_NAME + 1], data[160];
	GetPlayerName(playerid, name, sizeof name);
	format(data, sizeof data, "secret=%s&name=%s&token=%s", IG_SECRET, name, g_IgToken[playerid]);
	HTTP(playerid, HTTP_POST, IG_LOCAL_URL "/auth", data, "Ig_OnAuth");
	return 1;
}

stock Ig_SendToken(playerid, ok)
{
	new name[MAX_PLAYER_NAME + 1], json[160];
	GetPlayerName(playerid, name, sizeof name);
	if (ok) format(json, sizeof json, "{\"ev\":\"ig\",\"ok\":1,\"token\":\"%s\",\"name\":\"%s\",\"port\":%d}", g_IgToken[playerid], name, IG_PORT);
	else format(json, sizeof json, "{\"ev\":\"ig\",\"ok\":0,\"port\":%d}", IG_PORT);
	return Ph_Send(playerid, json);
}

public Ig_OnAuth(index, response_code, const data[])
{
	if (index < 0 || index >= MAX_PLAYERS || !IsPlayerConnected(index)) return 0;
	if (response_code != 200)
	{
		printf("[instagram] service injoignable (%d) : lance server/instagram/ig-server.js", response_code);
		g_IgToken[index][0] = 0;
		return Ig_SendToken(index, 0);
	}
	return Ig_SendToken(index, 1);
}

public Ig_OnDelete(index, response_code, const data[])
{
	if (index < 0 || index >= MAX_PLAYERS || !IsPlayerConnected(index)) return 0;
	if (response_code == 200) return Cw_Notify(index, "success", "Instagram", "Publication supprimee.", 3000);
	return Cw_Notify(index, "denied", "Instagram", "Publication introuvable (ou service arrete).", 3500);
}

// position du joueur (et du chauffeur qui a pris sa course de taxi) pour la carte de l'appli Uber
stock Ig_SendPos(playerid)
{
	new Float:x, Float:y, Float:z, json[220], drv = INVALID_PLAYER_ID;
	GetPlayerPos(playerid, x, y, z);
	for (new i = 0; i < RP_SOS_MAX; i++)
	{
		if (!g_RpSosId[i] || g_RpSosCaller[i] != playerid || g_RpSosType[i] != FACTION_TAXIDRIVER) continue;
		if (g_RpSosTaker[i] != INVALID_PLAYER_ID && IsPlayerConnected(g_RpSosTaker[i])) drv = g_RpSosTaker[i];
	}
	// [v32] course Uber : chauffeur (pour le client) ou client (pour le chauffeur) + destination
	new Float:ox, Float:oy, Float:tx, Float:ty, hasDest, ub = Ub_MapInfo(playerid, ox, oy, tx, ty, hasDest);
	if (ub)
	{
		format(json, sizeof json, "{\"ev\":\"pos\",\"x\":%.1f,\"y\":%.1f,\"dx\":%.1f,\"dy\":%.1f,\"who\":\"%s\",\"int\":%d", x, y, ox, oy, ub == 1 ? ("car") : ("client"), GetPlayerInterior(playerid));
		new part[48];
		if (hasDest) { format(part, sizeof part, ",\"tx\":%.1f,\"ty\":%.1f", tx, ty); strcat(json, part, sizeof json); }
		strcat(json, "}", sizeof json);
		return Ph_Send(playerid, json);
	}
	if (drv != INVALID_PLAYER_ID)
	{
		new Float:dx, Float:dy, Float:dz;
		GetPlayerPos(drv, dx, dy, dz);
		format(json, sizeof json, "{\"ev\":\"pos\",\"x\":%.1f,\"y\":%.1f,\"dx\":%.1f,\"dy\":%.1f,\"int\":%d}", x, y, dx, dy, GetPlayerInterior(playerid));
	}
	else format(json, sizeof json, "{\"ev\":\"pos\",\"x\":%.1f,\"y\":%.1f,\"int\":%d}", x, y, GetPlayerInterior(playerid));
	return Ph_Send(playerid, json);
}

stock Ig_OnCommand(playerid, const cmdtext[])
{
	if (!g_AuthLogged[playerid]) return 0;
	if (!strcmp(cmdtext, "/phoneact ig", true)) return Ig_Open(playerid), 1;
	if (!strcmp(cmdtext, "/phoneact pos", true)) return Ig_SendPos(playerid), 1;
	if (!strcmp(cmdtext, "/phoneact gps ", true, 14))
	{
		new pos = 14, a[16], b[16];
		Pg_Tok(cmdtext, pos, a, sizeof a);
		Pg_Tok(cmdtext, pos, b, sizeof b);
		new Float:x = floatstr(a), Float:y = floatstr(b);
		if (!a[0] || !b[0] || x < -3000.0 || x > 3000.0 || y < -3000.0 || y > 3000.0) return 1;
		Gps_Set(playerid, x, y);
		Ub_SetDest(playerid, x, y);   // [v32] destination de la course Uber
		return Cw_Notify(playerid, "info", "Uber", "Destination marquee sur ton GPS.", 2500), 1;
	}
	if (!strcmp(cmdtext, "/instagram", true))
		return Cw_Notify(playerid, "info", "Instagram", "Ouvre ton telephone : appli Instagram. Prends tes photos avec l'appli Camera.", 5000), 1;
	new a;
	if ((a = Cw_Arg(cmdtext, "/igdel")) != -1)
	{
		if (Admin_Level(playerid) < IG_ADMIN_LVL) return 0;
		new id = strval(cmdtext[a]), data[96];
		if (id <= 0) return Cw_Notify(playerid, "info", "Instagram", "/igdel <numero de la publication>", 3000), 1;
		format(data, sizeof data, "secret=%s&id=%d", IG_SECRET, id);
		HTTP(playerid, HTTP_POST, IG_LOCAL_URL "/delete", data, "Ig_OnDelete");
		return 1;
	}
	return 0;
}
