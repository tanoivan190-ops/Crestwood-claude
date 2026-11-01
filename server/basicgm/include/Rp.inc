/* ============================================================
   MENU RP HTML (@rp) - CrestwoodRP
   Page : app/src/main/assets/rp/index.html
   ------------------------------------------------------------
   serveur -> client : ShowPlayerDialog(9690 ouvrir / 9691 mise a jour,
                       caption "@rp", info = JSON)  ; 9692 = fermer
   client -> serveur : /rpact <action> [arguments]
     open | close | refresh | view <hub|anim|bill|ads|sos|me>
     go <inv|papers|lic|veh|outfit|garage|fac|gov|house|fish>
     anim <id> | animstop | fight <style> | desc <texte> | descoff
     bill <joueur> <montant> <motif> | pay <id> <cash|bank> | refuse <id> | cancel <id>
     ad <categorie> <texte> | addel <id>
     sos <service> <texte> | soscancel | sostake <id> | sosdone <id>
   IMPORTANT : "close" ne renvoie RIEN (sinon la page se rouvre en boucle
   et le joueur reste fige derriere un ecran plein).
   Ce fichier est inclus APRES les autres systemes de basicgm1.pwn (il utilise
   g_BankBalance, Fac_*, Admin_Level, Gov_HasLicense, Gps_Set...).
   ============================================================ */
#define RP_DIALOG_OPEN      9690
#define RP_DIALOG_UPDATE    9691
#define RP_DIALOG_CLOSE     9692

#define RP_COL_OK           0x33CC33FF
#define RP_COL_ERR          0xFF4444FF
#define RP_COL_INFO         0x33CCFFFF

#define RP_BILL_MAX         1000000
#define RP_BILL_RANGE       6.0
#define RP_AD_MAX           8
#define RP_AD_PRICE         150
#define RP_AD_COOLDOWN      60
#define RP_SOS_MAX          12
#define RP_ANIM_COUNT       33

new RP_Pickup;
new RP_Blip;
new DB:RpDB;

new g_RpJson[3950];
new g_RpView[MAX_PLAYERS][8];
new g_RpCur[MAX_PLAYERS][12];
new g_RpFight[MAX_PLAYERS];
new g_RpDesc[MAX_PLAYERS][72];
new g_RpDescOn[MAX_PLAYERS];
new Text3D:g_RpDescLbl[MAX_PLAYERS];
new g_RpAdLast[MAX_PLAYERS];

new g_RpAdId[RP_AD_MAX];
new g_RpAdCat[RP_AD_MAX][10];
new g_RpAdAuthor[RP_AD_MAX][MAX_PLAYER_NAME + 1];
new g_RpAdText[RP_AD_MAX][96];
new g_RpAdTs[RP_AD_MAX];
new g_RpAdCount;
new g_RpAdNext = 1;

new g_RpSosId[RP_SOS_MAX];            // 0 = emplacement libre
new g_RpSosType[RP_SOS_MAX];          // FACTION_POLICE / MEDIC / MECHANIC / TAXIDRIVER
new g_RpSosCaller[RP_SOS_MAX];
new g_RpSosTaker[RP_SOS_MAX];
new g_RpSosText[RP_SOS_MAX][84];
new g_RpSosTs[RP_SOS_MAX];
new Float:g_RpSosX[RP_SOS_MAX];
new Float:g_RpSosY[RP_SOS_MAX];
new Float:g_RpSosZ[RP_SOS_MAX];
new g_RpSosNext = 1;

// id | bibliotheque | animation   ; mode : 0 une fois, 1 boucle, 100+N = action speciale N
new const g_RpAnim[RP_ANIM_COUNT][3][22] =
{
	{"salut", "ON_LOOKERS", "wave_loop"},
	{"pointer", "ON_LOOKERS", "point_loop"},
	{"crier", "ON_LOOKERS", "shout_01"},
	{"non", "GANGS", "Invite_No"},
	{"rire", "RAPPING", "Laugh_01"},
	{"bras", "COP_AMBIENT", "Coplook_loop"},
	{"reflechir", "COP_AMBIENT", "Coplook_think"},
	{"regarder", "ON_LOOKERS", "lkup_loop"},
	{"gang1", "GANGS", "hndshkaa"},
	{"gang2", "GANGS", "DEALER_IDLE"},
	{"mainsair", "PED", "handsup"},
	{"assis", "PED", "SEAT_IDLE"},
	{"assisf", "BEACH", "ParkSit_W_loop"},
	{"allonge", "BEACH", "Lay_Bac_Loop"},
	{"bronzer", "BEACH", "bather"},
	{"dormir", "CRACK", "crckdeth2"},
	{"adosse", "GANGS", "leanIDLE"},
	{"fumer", "SMOKING", "M_smklean_loop"},
	{"boire", "PED", "handsup"},
	{"telephone", "PED", "handsup"},
	{"taichi", "PARK", "Tai_Chi_Loop"},
	{"danse1", "PED", "handsup"},
	{"danse2", "PED", "handsup"},
	{"danse3", "PED", "handsup"},
	{"danse4", "PED", "handsup"},
	{"danse5", "DANCING", "DAN_Loop_A"},
	{"rap", "RAPPING", "RAP_A_Loop"},
	{"reparer", "CAR", "Fixn_Car_Loop"},
	{"fouiller", "COP_AMBIENT", "Copbrowse_loop"},
	{"ordi", "INT_OFFICE", "OFF_Sit_Type_Loop"},
	{"cpr", "MEDIC", "CPR"},
	{"garde", "COP_AMBIENT", "Coplook_loop"},
	{"blesse", "SWEET", "Sweet_injuredloop"}
};
new const g_RpAnimMode[RP_ANIM_COUNT] =
{
	1, 1, 0, 0, 0, 1, 1, 1, 0, 1,
	110, 1, 1, 1, 1, 1, 1, 1, 120, 111,
	1, 105, 106, 107, 108, 1, 1, 1, 1, 1,
	0, 1, 1
};

forward Rp_OnInit();
forward Rp_OnConnect(playerid);
forward Rp_OnDisconnect(playerid);
forward Rp_OnCommand(playerid, const cmdtext[]);

// ------------------------------------------------------------
//  Base de donnees RP (factures)
// ------------------------------------------------------------
stock Rp_InitDB()
{
	RpDB = db_open("crestwood_rp.db");
	if (RpDB == DB:0)
	{
		print("[RP] ERREUR : impossible d'ouvrir crestwood_rp.db");
		return 0;
	}
	db_free_result(db_query(RpDB, "CREATE TABLE IF NOT EXISTS rp_bills (id INTEGER PRIMARY KEY AUTOINCREMENT, from_name TEXT COLLATE NOCASE, to_name TEXT COLLATE NOCASE, amount INTEGER, reason TEXT, ts INTEGER, status INTEGER DEFAULT 0)"));
	print("[RP] Base de donnees prete.");
	return 1;
}

stock Rp_OnInit()
{
	Rp_InitDB();
	#pragma unused RP_Pickup   // [Crestwood] plus de pickup du menu RP (icones 3D)
	#pragma unused RP_Blip
	printf("[RP] Menu RP charge.");
	return 1;
}

stock Rp_OnConnect(playerid)
{
	g_RpView[playerid][0] = 0;
	g_RpCur[playerid][0] = 0;
	g_RpFight[playerid] = 4;
	g_RpDesc[playerid][0] = 0;
	g_RpDescOn[playerid] = 0;
	g_RpDescLbl[playerid] = Text3D:INVALID_3DTEXT_ID;
	g_RpAdLast[playerid] = 0;
	return 1;
}

stock Rp_OnDisconnect(playerid)
{
	if (g_RpDescLbl[playerid] != Text3D:INVALID_3DTEXT_ID)
	{
		Delete3DTextLabel(g_RpDescLbl[playerid]);
		g_RpDescLbl[playerid] = Text3D:INVALID_3DTEXT_ID;
	}
	for (new i = 0; i < RP_SOS_MAX; i++)
	{
		if (g_RpSosId[i] == 0) continue;
		if (g_RpSosCaller[i] == playerid) g_RpSosId[i] = 0;
		else if (g_RpSosTaker[i] == playerid) g_RpSosTaker[i] = -1;
	}
	return 1;
}

// ------------------------------------------------------------
//  Utilitaires
// ------------------------------------------------------------
stock Rp_Near(a, b, Float:range)
{
	new Float:x, Float:y, Float:z;
	GetPlayerPos(b, x, y, z);
	if (GetPlayerInterior(a) != GetPlayerInterior(b)) return 0;
	if (GetPlayerVirtualWorld(a) != GetPlayerVirtualWorld(b)) return 0;
	return IsPlayerInRangeOfPoint(a, range, x, y, z) ? 1 : 0;
}

stock Rp_FindByName(const name[])
{
	new n[MAX_PLAYER_NAME + 1];
	for (new i = 0; i < MAX_PLAYERS; i++)
	{
		if (!IsPlayerConnected(i)) continue;
		GetPlayerName(i, n, sizeof n);
		if (!strcmp(n, name, true)) return i;
	}
	return INVALID_PLAYER_ID;
}

stock Rp_Say(playerid, color, const text[])
{
	return SendClientMessage(playerid, color, text);
}

// service (cle JSON) <-> type de faction
stock Rp_SvcType(const key[])
{
	if (!strcmp(key, "police", true)) return FACTION_POLICE;
	if (!strcmp(key, "medic", true)) return FACTION_MEDIC;
	if (!strcmp(key, "mech", true)) return FACTION_MECHANIC;
	if (!strcmp(key, "taxi", true)) return FACTION_TAXIDRIVER;
	return FACTION_NONE;
}

stock Rp_SvcKey(type, out[], outsize)
{
	if (type == FACTION_POLICE) format(out, outsize, "police");
	else if (type == FACTION_MEDIC) format(out, outsize, "medic");
	else if (type == FACTION_MECHANIC) format(out, outsize, "mech");
	else if (type == FACTION_TAXIDRIVER) format(out, outsize, "taxi");
	else out[0] = 0;
	return 1;
}

stock Rp_SvcLabel(type, out[], outsize)
{
	if (type == FACTION_POLICE) format(out, outsize, "POLICE");
	else if (type == FACTION_MEDIC) format(out, outsize, "MEDIC");
	else if (type == FACTION_MECHANIC) format(out, outsize, "MECANO");
	else if (type == FACTION_TAXIDRIVER) format(out, outsize, "TAXI");
	else format(out, outsize, "SERVICE");
	return 1;
}

// ------------------------------------------------------------
//  Blocs JSON
// ------------------------------------------------------------
stock Rp_BuildMe(playerid, out[], outsize)
{
	new name[MAX_PLAYER_NAME + 1], dn[MAX_PLAYER_NAME + 1], svc[8], fname[48], rname[32], fe[100], re[64], fac;
	GetPlayerName(playerid, name, sizeof name);
	format(dn, sizeof dn, "%s", name);
	for (new i = 0; dn[i] != 0; i++)
	{
		if (dn[i] == '_') dn[i] = ' ';
	}
	Rp_SvcKey(Fac_Type(playerid), svc, sizeof svc);
	fac = Fac_Get(playerid);
	fname[0] = 0;
	rname[0] = 0;
	if (fac != FACTION_NONE)
	{
		Fac_Name(fac, fname, sizeof fname);
		Fac_RankName(fac, g_FacRank[playerid], rname, sizeof rname);
	}
	Admin_Clean(fname, fe, sizeof fe);
	Admin_Clean(rname, re, sizeof re);
	format(out, outsize, "\"me\":{\"n\":\"%s\",\"u\":\"%s\",\"cash\":%d,\"bank\":%d,\"svc\":\"%s\",\"fac\":\"%s\",\"rk\":\"%s\",\"inFac\":%d,\"adm\":%d,\"law\":%d,\"lawT\":%d}",
		dn, name, GetPlayerMoney(playerid), g_BankBalance[playerid], svc, fe, re, (fac != FACTION_NONE) ? 1 : 0, (Admin_Level(playerid) > 0) ? 1 : 0, Fac_IsLaw(playerid) ? 1 : 0, Fac_Type(playerid));
	return 1;
}

stock Rp_CountBills(playerid)
{
	if (RpDB == DB:0) return 0;
	new name[MAX_PLAYER_NAME + 1], q[140], f[12], n = 0;
	GetPlayerName(playerid, name, sizeof name);
	format(q, sizeof q, "SELECT COUNT(*) FROM rp_bills WHERE to_name = '%s' AND status = 0", name);
	new DBResult:res = db_query(RpDB, q);
	if (res != DBResult:0)
	{
		if (db_num_rows(res) > 0)
		{
			db_get_field(res, 0, f, sizeof f);
			n = strval(f);
		}
		db_free_result(res);
	}
	return n;
}

stock Rp_CountCalls(playerid)
{
	new t = Fac_Type(playerid), n = 0;
	if (t != FACTION_POLICE && t != FACTION_MEDIC && t != FACTION_MECHANIC && t != FACTION_TAXIDRIVER) return 0;
	for (new i = 0; i < RP_SOS_MAX; i++)
	{
		if (g_RpSosId[i] != 0 && g_RpSosType[i] == t) n++;
	}
	return n;
}

stock Rp_BuildBills(playerid, out[], outsize)
{
	new name[MAX_PLAYER_NAME + 1], q[200], a[32], b[12], c[12], d[64], e[12], f[12], part[200], DBResult:res, rows;
	GetPlayerName(playerid, name, sizeof name);
	strcat(out, ",\"in\":[", outsize);
	if (RpDB != DB:0)
	{
		format(q, sizeof q, "SELECT id, from_name, amount, reason, ts FROM rp_bills WHERE to_name = '%s' AND status = 0 ORDER BY id DESC LIMIT 6", name);
		res = db_query(RpDB, q);
		if (res != DBResult:0)
		{
			rows = db_num_rows(res);
			for (new i = 0; i < rows; i++)
			{
				db_get_field(res, 0, b, sizeof b);
				db_get_field(res, 1, a, sizeof a);
				db_get_field(res, 2, c, sizeof c);
				db_get_field(res, 3, d, sizeof d);
				db_get_field(res, 4, e, sizeof e);
				format(part, sizeof part, "%s[%d,\"%s\",%d,\"%s\",%d,\"\"]", (i > 0) ? (",") : (""), strval(b), a, strval(c), d, strval(e));
				strcat(out, part, outsize);
				db_next_row(res);
			}
			db_free_result(res);
		}
	}
	strcat(out, "],\"out\":[", outsize);
	if (RpDB != DB:0)
	{
		format(q, sizeof q, "SELECT id, to_name, amount, reason, ts, status FROM rp_bills WHERE from_name = '%s' ORDER BY id DESC LIMIT 6", name);
		res = db_query(RpDB, q);
		if (res != DBResult:0)
		{
			rows = db_num_rows(res);
			for (new i = 0; i < rows; i++)
			{
				db_get_field(res, 0, b, sizeof b);
				db_get_field(res, 1, a, sizeof a);
				db_get_field(res, 2, c, sizeof c);
				db_get_field(res, 3, d, sizeof d);
				db_get_field(res, 4, e, sizeof e);
				db_get_field(res, 5, f, sizeof f);
				format(part, sizeof part, "%s[%d,\"%s\",%d,\"%s\",%d,%d]", (i > 0) ? (",") : (""), strval(b), a, strval(c), d, strval(e), strval(f));
				strcat(out, part, outsize);
				db_next_row(res);
			}
			db_free_result(res);
		}
	}
	strcat(out, "],\"near\":[", outsize);
	new n = 0, pn[MAX_PLAYER_NAME + 1];
	for (new i = 0; i < MAX_PLAYERS && n < 8; i++)
	{
		if (i == playerid || !IsPlayerConnected(i) || !g_AuthLogged[i]) continue;
		if (!Rp_Near(playerid, i, RP_BILL_RANGE)) continue;
		GetPlayerName(i, pn, sizeof pn);
		for (new k = 0; pn[k] != 0; k++)
		{
			if (pn[k] == '_') pn[k] = ' ';
		}
		format(part, sizeof part, "%s[%d,\"%s\"]", (n > 0) ? (",") : (""), i, pn);
		strcat(out, part, outsize);
		n++;
	}
	format(part, sizeof part, "],\"to\":-1,\"for\":\"\",\"max\":%d", RP_BILL_MAX);
	strcat(out, part, outsize);
	return 1;
}

stock Rp_BuildAds(playerid, out[], outsize)
{
	new part[200], cd = 0;
	if (g_RpAdLast[playerid] + RP_AD_COOLDOWN > gettime()) cd = g_RpAdLast[playerid] + RP_AD_COOLDOWN - gettime();
	strcat(out, ",\"list\":[", outsize);
	for (new i = 0; i < g_RpAdCount; i++)
	{
		format(part, sizeof part, "%s[%d,\"%s\",\"%s\",\"%s\",%d]", (i > 0) ? (",") : (""), g_RpAdId[i], g_RpAdCat[i], g_RpAdAuthor[i], g_RpAdText[i], g_RpAdTs[i]);
		strcat(out, part, outsize);
	}
	format(part, sizeof part, "],\"price\":%d,\"cd\":%d", RP_AD_PRICE, cd);
	strcat(out, part, outsize);
	return 1;
}

stock Rp_BuildSos(playerid, out[], outsize)
{
	new part[240], t = Fac_Type(playerid), n = 0, cn[MAX_PLAYER_NAME + 1], tn[MAX_PLAYER_NAME + 1], key[8];
	strcat(out, ",\"calls\":[", outsize);
	if (t == FACTION_POLICE || t == FACTION_MEDIC || t == FACTION_MECHANIC || t == FACTION_TAXIDRIVER)
	{
		for (new i = 0; i < RP_SOS_MAX; i++)
		{
			if (g_RpSosId[i] == 0 || g_RpSosType[i] != t) continue;
			cn[0] = 0;
			tn[0] = 0;
			if (IsPlayerConnected(g_RpSosCaller[i])) GetPlayerName(g_RpSosCaller[i], cn, sizeof cn);
			if (g_RpSosTaker[i] >= 0 && IsPlayerConnected(g_RpSosTaker[i])) GetPlayerName(g_RpSosTaker[i], tn, sizeof tn);
			format(part, sizeof part, "%s[%d,%d,\"%s\",\"%s\",%d,%d,\"%s\",%d]", (n > 0) ? (",") : (""), g_RpSosId[i], t, cn, g_RpSosText[i], g_RpSosTs[i],
				floatround(GetPlayerDistanceFromPoint(playerid, g_RpSosX[i], g_RpSosY[i], g_RpSosZ[i])), tn, (g_RpSosTaker[i] == playerid) ? 1 : 0);
			strcat(out, part, outsize);
			n++;
		}
	}
	strcat(out, "],\"my\":0", outsize);
	for (new i = 0; i < RP_SOS_MAX; i++)
	{
		if (g_RpSosId[i] == 0 || g_RpSosCaller[i] != playerid) continue;
		tn[0] = 0;
		if (g_RpSosTaker[i] >= 0 && IsPlayerConnected(g_RpSosTaker[i])) GetPlayerName(g_RpSosTaker[i], tn, sizeof tn);
		Rp_SvcKey(g_RpSosType[i], key, sizeof key);
		out[strlen(out) - 1] = 0;   // retire le "0" de "my":0
		format(part, sizeof part, "[%d,\"%s\",\"%s\",%d,\"%s\"]", g_RpSosId[i], key, g_RpSosText[i], g_RpSosTs[i], tn);
		strcat(out, part, outsize);
		break;
	}
	return 1;
}

stock Rp_BuildMeView(playerid, out[], outsize)
{
	new part[300], Float:hp, de[100];
	GetPlayerHealth(playerid, hp);
	Admin_Clean(g_RpDesc[playerid], de, sizeof de);
	format(part, sizeof part, ",\"p\":{\"age\":0,\"org\":\"\",\"lvl\":%d,\"fs\":%d,\"desc\":\"%s\",\"don\":%d,\"lic\":[%d,%d,%d,%d],\"wl\":%d,\"hp\":%d,\"skin\":%d}",
		GetPlayerScore(playerid), g_RpFight[playerid], de, g_RpDescOn[playerid],
		(Gov_HasLicense(playerid, LIC_DRIVE) == 1) ? 1 : 0, (Gov_HasLicense(playerid, LIC_WEAPON) == 1) ? 1 : 0,
		(Gov_HasLicense(playerid, LIC_TRUCK) == 1) ? 1 : 0, (Gov_HasLicense(playerid, LIC_BOAT) == 1) ? 1 : 0,
		GetPlayerWantedLevel(playerid), floatround(hp), GetPlayerSkin(playerid));
	strcat(out, part, outsize);
	return 1;
}

// Envoie une vue a la page (dialogid : 9690 ouvrir, 9691 mise a jour)
stock Rp_Send(playerid, const view[], dialogid, ok, const msg[])
{
	if (!IsPlayerConnected(playerid)) return 0;
	new me[480], extra[3000], safe[120];
	extra[0] = 0;
	Rp_BuildMe(playerid, me, sizeof me);
	if (!strcmp(view, "anim", true)) format(extra, sizeof extra, ",\"cur\":\"%s\"", g_RpCur[playerid]);
	else if (!strcmp(view, "bill", true)) Rp_BuildBills(playerid, extra, sizeof extra);
	else if (!strcmp(view, "ads", true)) Rp_BuildAds(playerid, extra, sizeof extra);
	else if (!strcmp(view, "sos", true)) Rp_BuildSos(playerid, extra, sizeof extra);
	else if (!strcmp(view, "me", true)) Rp_BuildMeView(playerid, extra, sizeof extra);
	else if (!strcmp(view, "promo", true)) Promo_Build(playerid, extra, sizeof extra);
	else if (!strcmp(view, "law", true)) Law_Build(playerid, extra, sizeof extra);          // centrale police / FBI / armee (include/Law.inc)   // codes promo (include/Promo.inc)
	else format(extra, sizeof extra, ",\"hub\":{\"gov\":%d,\"veh\":%d,\"anim\":\"%s\"}", Gov_Is(playerid) ? 1 : 0, IsPlayerInAnyVehicle(playerid) ? 1 : 0, g_RpCur[playerid]);
	Admin_Clean(msg, safe, sizeof safe);
	format(g_RpJson, sizeof g_RpJson, "{\"v\":\"%s\",%s,\"cnt\":{\"bills\":%d,\"calls\":%d}%s,\"ok\":%d,\"msg\":\"%s\"}",
		view, me, Rp_CountBills(playerid), Rp_CountCalls(playerid), extra, ok, safe);
	return ShowPlayerDialog(playerid, dialogid, DIALOG_STYLE_MSGBOX, "@rp", g_RpJson, "OK", "");
}

// Re-envoie la vue courante avec un message (retour d'une action)
stock Rp_Reply(playerid, ok, const msg[])
{
	if (g_RpView[playerid][0] == 0) format(g_RpView[playerid], 8, "hub");
	return Rp_Send(playerid, g_RpView[playerid], RP_DIALOG_UPDATE, ok, msg);
}

// ------------------------------------------------------------
//  Animations / combat / description
// ------------------------------------------------------------
stock Rp_AnimStop(playerid)
{
	ClearAnimations(playerid);
	SetPlayerSpecialAction(playerid, SPECIAL_ACTION_NONE);
	g_RpCur[playerid][0] = 0;
	return 1;
}

stock Rp_AnimStart(playerid, const id[])
{
	if (GetPlayerState(playerid) != PLAYER_STATE_ONFOOT) return Rp_Reply(playerid, 0, "Descends de ton vehicule d'abord.");
	for (new i = 0; i < RP_ANIM_COUNT; i++)
	{
		if (strcmp(id, g_RpAnim[i][0], true) != 0) continue;
		Rp_AnimStop(playerid);
		new mode = g_RpAnimMode[i];
		if (mode >= 100) SetPlayerSpecialAction(playerid, mode - 100);
		else ApplyAnimation(playerid, g_RpAnim[i][1], g_RpAnim[i][2], 4.1, mode, 0, 0, 0, 0);
		format(g_RpCur[playerid], 12, "%s", g_RpAnim[i][0]);
		return Rp_Reply(playerid, 1, "");
	}
	return Rp_Reply(playerid, 0, "Animation inconnue.");
}

stock Rp_SetDesc(playerid, const text[])
{
	if (g_RpDescLbl[playerid] != Text3D:INVALID_3DTEXT_ID)
	{
		Delete3DTextLabel(g_RpDescLbl[playerid]);
		g_RpDescLbl[playerid] = Text3D:INVALID_3DTEXT_ID;
	}
	g_RpDescOn[playerid] = 0;
	if (text[0] == 0) return 1;
	format(g_RpDesc[playerid], 72, "%s", text);
	g_RpDescLbl[playerid] = Create3DTextLabel(g_RpDesc[playerid], 0xDDDDDDFF, 0.0, 0.0, 0.0, 12.0, -1, 1);
	if (g_RpDescLbl[playerid] != Text3D:INVALID_3DTEXT_ID)
	{
		Attach3DTextLabelToPlayer(g_RpDescLbl[playerid], playerid, 0.0, 0.0, 0.35);
		g_RpDescOn[playerid] = 1;
	}
	return 1;
}

// ------------------------------------------------------------
//  Factures
// ------------------------------------------------------------
stock Rp_BillSend(playerid, const arg[], const rest[])
{
	new target = strval(arg), ridx = 0, amt[12], pn[MAX_PLAYER_NAME + 1], tn[MAX_PLAYER_NAME + 1], q[300], reason[52];
	Bank_NextToken(rest, ridx, amt, sizeof amt);
	while (rest[ridx] == ' ') ridx++;
	format(reason, sizeof reason, "%s", rest[ridx]);
	new amount = strval(amt);
	if (arg[0] == 0 || target < 0 || target >= MAX_PLAYERS || !IsPlayerConnected(target) || target == playerid || !g_AuthLogged[target]) return Rp_Reply(playerid, 0, "Choisis un client.");
	if (!Rp_Near(playerid, target, RP_BILL_RANGE)) return Rp_Reply(playerid, 0, "Ce joueur est trop loin.");
	if (amount < 1 || amount > RP_BILL_MAX) return Rp_Reply(playerid, 0, "Montant invalide.");
	if (strlen(reason) < 2) format(reason, sizeof reason, "Facture");
	if (RpDB == DB:0) return Rp_Reply(playerid, 0, "Base RP indisponible.");
	GetPlayerName(playerid, pn, sizeof pn);
	GetPlayerName(target, tn, sizeof tn);
	format(q, sizeof q, "INSERT INTO rp_bills (from_name, to_name, amount, reason, ts, status) VALUES ('%s', '%s', %d, '%s', %d, 0)", pn, tn, amount, reason, gettime());
	db_free_result(db_query(RpDB, q));
	new line[120];
	format(line, sizeof line, "Nouvelle facture de %s : %d FCFA (%s). Ouvre /rp > Factures.", pn, amount, reason);
	Rp_Say(target, RP_COL_INFO, line);
	return Rp_Reply(playerid, 1, "Facture envoyee.");
}

stock Rp_BillCredit(const name[], amount, const descr[])
{
	new who = Rp_FindByName(name);
	if (who != INVALID_PLAYER_ID && g_BankLoaded[who])
	{
		g_BankBalance[who] += amount;
		Bank_AddHistory(who, descr, amount);
		Bank_SavePlayer(who);
		new line[100];
		format(line, sizeof line, "Facture reglee : +%d FCFA sur ton compte.", amount);
		Rp_Say(who, RP_COL_OK, line);
	}
	else if (g_BankDB != DB:0)
	{
		new q[160];
		format(q, sizeof q, "UPDATE bank_accounts SET balance = balance + %d WHERE name = '%s'", amount, name);
		db_free_result(db_query(g_BankDB, q));
	}
	return 1;
}

stock Rp_BillPay(playerid, const arg[], const rest[])
{
	new id = strval(arg), me[MAX_PLAYER_NAME + 1], q[200], from[MAX_PLAYER_NAME + 1], f[16], amount = 0;
	GetPlayerName(playerid, me, sizeof me);
	if (id <= 0 || RpDB == DB:0) return Rp_Reply(playerid, 0, "Facture introuvable.");
	format(q, sizeof q, "SELECT amount, from_name FROM rp_bills WHERE id = %d AND to_name = '%s' AND status = 0 LIMIT 1", id, me);
	new DBResult:res = db_query(RpDB, q);
	if (res == DBResult:0) return Rp_Reply(playerid, 0, "Facture introuvable.");
	if (db_num_rows(res) > 0)
	{
		db_get_field(res, 0, f, sizeof f);
		amount = strval(f);
		db_get_field(res, 1, from, sizeof from);
	}
	db_free_result(res);
	if (amount <= 0) return Rp_Reply(playerid, 0, "Facture introuvable.");
	if (!strcmp(rest, "bank", true))
	{
		if (g_BankBalance[playerid] < amount) return Rp_Reply(playerid, 0, "Solde bancaire insuffisant.");
		g_BankBalance[playerid] -= amount;
		Bank_AddHistory(playerid, "Facture payee", -amount);
	}
	else
	{
		if (GetPlayerMoney(playerid) < amount) return Rp_Reply(playerid, 0, "Pas assez de liquide.");
		GivePlayerMoney(playerid, -amount);
	}
	Bank_SavePlayer(playerid);
	format(q, sizeof q, "UPDATE rp_bills SET status = 1 WHERE id = %d", id);
	db_free_result(db_query(RpDB, q));
	Rp_BillCredit(from, amount, "Facture encaissee");
	return Rp_Reply(playerid, 1, "Facture payee.");
}

stock Rp_BillClose(playerid, const arg[], status)
{
	new id = strval(arg), me[MAX_PLAYER_NAME + 1], q[200];
	GetPlayerName(playerid, me, sizeof me);
	if (id <= 0 || RpDB == DB:0) return Rp_Reply(playerid, 0, "Facture introuvable.");
	if (status == 2) format(q, sizeof q, "UPDATE rp_bills SET status = 2 WHERE id = %d AND to_name = '%s' AND status = 0", id, me);
	else format(q, sizeof q, "UPDATE rp_bills SET status = 3 WHERE id = %d AND from_name = '%s' AND status = 0", id, me);
	db_free_result(db_query(RpDB, q));
	if (status == 2) return Rp_Reply(playerid, 1, "Facture refusee.");
	return Rp_Reply(playerid, 1, "Facture annulee.");
}

// ------------------------------------------------------------
//  Annonces
// ------------------------------------------------------------
stock Rp_AdPublish(playerid, const cat[], const text[])
{
	new ok = 0, line[144], name[MAX_PLAYER_NAME + 1];
	if (!strcmp(cat, "vente", true) || !strcmp(cat, "achat", true) || !strcmp(cat, "service", true) || !strcmp(cat, "emploi", true) || !strcmp(cat, "event", true)) ok = 1;
	if (!ok) return Rp_Reply(playerid, 0, "Categorie invalide.");
	if (strlen(text) < 6) return Rp_Reply(playerid, 0, "Ton annonce est trop courte.");
	if (g_RpAdLast[playerid] + RP_AD_COOLDOWN > gettime()) return Rp_Reply(playerid, 0, "Patiente avant la prochaine annonce.");
	if (GetPlayerMoney(playerid) < RP_AD_PRICE) return Rp_Reply(playerid, 0, "Pas assez de liquide.");
	GivePlayerMoney(playerid, -RP_AD_PRICE);
	Bank_SavePlayer(playerid);
	g_RpAdLast[playerid] = gettime();
	GetPlayerName(playerid, name, sizeof name);
	if (g_RpAdCount >= RP_AD_MAX)
	{
		for (new i = RP_AD_MAX - 1; i > 0; i--)
		{
			g_RpAdId[i] = g_RpAdId[i - 1];
			format(g_RpAdCat[i], 10, "%s", g_RpAdCat[i - 1]);
			format(g_RpAdAuthor[i], MAX_PLAYER_NAME + 1, "%s", g_RpAdAuthor[i - 1]);
			format(g_RpAdText[i], 96, "%s", g_RpAdText[i - 1]);
			g_RpAdTs[i] = g_RpAdTs[i - 1];
		}
	}
	else
	{
		for (new i = g_RpAdCount; i > 0; i--)
		{
			g_RpAdId[i] = g_RpAdId[i - 1];
			format(g_RpAdCat[i], 10, "%s", g_RpAdCat[i - 1]);
			format(g_RpAdAuthor[i], MAX_PLAYER_NAME + 1, "%s", g_RpAdAuthor[i - 1]);
			format(g_RpAdText[i], 96, "%s", g_RpAdText[i - 1]);
			g_RpAdTs[i] = g_RpAdTs[i - 1];
		}
		g_RpAdCount++;
	}
	g_RpAdId[0] = g_RpAdNext++;
	format(g_RpAdCat[0], 10, "%s", cat);
	format(g_RpAdAuthor[0], MAX_PLAYER_NAME + 1, "%s", name);
	strmid(g_RpAdText[0], text, 0, 90, 96);
	g_RpAdTs[0] = gettime();
	format(line, sizeof line, "[ANNONCE %s] %s : %s", cat, name, g_RpAdText[0]);
	SendClientMessageToAll(0xFFD700FF, line);
	return Rp_Reply(playerid, 1, "Annonce publiee.");
}

stock Rp_AdDelete(playerid, const arg[])
{
	new id = strval(arg), me[MAX_PLAYER_NAME + 1];
	GetPlayerName(playerid, me, sizeof me);
	for (new i = 0; i < g_RpAdCount; i++)
	{
		if (g_RpAdId[i] != id) continue;
		if (strcmp(g_RpAdAuthor[i], me, true) != 0 && Admin_Level(playerid) < 1) return Rp_Reply(playerid, 0, "Ce n'est pas ton annonce.");
		for (new j = i; j < g_RpAdCount - 1; j++)
		{
			g_RpAdId[j] = g_RpAdId[j + 1];
			format(g_RpAdCat[j], 10, "%s", g_RpAdCat[j + 1]);
			format(g_RpAdAuthor[j], MAX_PLAYER_NAME + 1, "%s", g_RpAdAuthor[j + 1]);
			format(g_RpAdText[j], 96, "%s", g_RpAdText[j + 1]);
			g_RpAdTs[j] = g_RpAdTs[j + 1];
		}
		g_RpAdCount--;
		return Rp_Reply(playerid, 1, "Annonce retiree.");
	}
	return Rp_Reply(playerid, 0, "Annonce introuvable.");
}

// ------------------------------------------------------------
//  Urgences
// ------------------------------------------------------------
stock Rp_SosFind(id)
{
	for (new i = 0; i < RP_SOS_MAX; i++)
	{
		if (g_RpSosId[i] == id && id != 0) return i;
	}
	return -1;
}

stock Rp_SosCreate(playerid, const svc[], const text[])
{
	new type = Rp_SvcType(svc), slot = -1;
	if (type == FACTION_NONE) return Rp_Reply(playerid, 0, "Service inconnu.");
	if (strlen(text) < 4) return Rp_Reply(playerid, 0, "Decris la situation en quelques mots.");
	for (new i = 0; i < RP_SOS_MAX; i++)
	{
		if (g_RpSosId[i] != 0 && g_RpSosCaller[i] == playerid) return Rp_Reply(playerid, 0, "Tu as deja un appel en cours.");
		if (g_RpSosId[i] == 0 && slot < 0) slot = i;
	}
	if (slot < 0) return Rp_Reply(playerid, 0, "Les lignes sont saturees, reessaie.");
	g_RpSosId[slot] = g_RpSosNext++;
	g_RpSosType[slot] = type;
	g_RpSosCaller[slot] = playerid;
	g_RpSosTaker[slot] = -1;
	g_RpSosTs[slot] = gettime();
	strmid(g_RpSosText[slot], text, 0, 80, 84);
	GetPlayerPos(playerid, g_RpSosX[slot], g_RpSosY[slot], g_RpSosZ[slot]);
	new label[12], name[MAX_PLAYER_NAME + 1], line[160];
	Rp_SvcLabel(type, label, sizeof label);
	GetPlayerName(playerid, name, sizeof name);
	format(line, sizeof line, "[URGENCE %s] %s : %s (appel #%d)", label, name, g_RpSosText[slot], g_RpSosId[slot]);
	for (new i = 0; i < MAX_PLAYERS; i++)
	{
		if (IsPlayerConnected(i) && g_AuthLogged[i] && Fac_Type(i) == type) Rp_Say(i, RP_COL_ERR, line);
	}
	return Rp_Reply(playerid, 1, "Appel envoye aux services.");
}

stock Rp_SosCancel(playerid)
{
	for (new i = 0; i < RP_SOS_MAX; i++)
	{
		if (g_RpSosId[i] != 0 && g_RpSosCaller[i] == playerid)
		{
			g_RpSosId[i] = 0;
			return Rp_Reply(playerid, 1, "Appel annule.");
		}
	}
	return Rp_Reply(playerid, 0, "Aucun appel en cours.");
}

stock Rp_SosTake(playerid, const arg[])
{
	new slot = Rp_SosFind(strval(arg));
	if (slot < 0) return Rp_Reply(playerid, 0, "Appel introuvable.");
	if (Fac_Type(playerid) != g_RpSosType[slot]) return Rp_Reply(playerid, 0, "Ce n'est pas ton service.");
	if (g_RpSosTaker[slot] >= 0 && g_RpSosTaker[slot] != playerid && IsPlayerConnected(g_RpSosTaker[slot])) return Rp_Reply(playerid, 0, "Appel deja pris.");
	if (g_RpSosTaker[slot] != playerid)
	{
		g_RpSosTaker[slot] = playerid;
		new name[MAX_PLAYER_NAME + 1], line[100];
		GetPlayerName(playerid, name, sizeof name);
		format(line, sizeof line, "%s a pris ton appel et arrive.", name);
		if (IsPlayerConnected(g_RpSosCaller[slot])) Rp_Say(g_RpSosCaller[slot], RP_COL_OK, line);
	}
	Gps_Set(playerid, g_RpSosX[slot], g_RpSosY[slot]);
	return Rp_Reply(playerid, 1, "GPS pose vers l'appelant.");
}

stock Rp_SosDone(playerid, const arg[])
{
	new slot = Rp_SosFind(strval(arg));
	if (slot < 0) return Rp_Reply(playerid, 0, "Appel introuvable.");
	if (Fac_Type(playerid) != g_RpSosType[slot]) return Rp_Reply(playerid, 0, "Ce n'est pas ton service.");
	if (IsPlayerConnected(g_RpSosCaller[slot])) Rp_Say(g_RpSosCaller[slot], RP_COL_INFO, "Ton appel d'urgence a ete cloture.");
	g_RpSosId[slot] = 0;
	return Rp_Reply(playerid, 1, "Appel cloture.");
}

// ------------------------------------------------------------
//  Raccourcis du hub (go) : on relance la commande du systeme cible
// ------------------------------------------------------------
stock Rp_Go(playerid, const target[])
{
	new cmd[16];
	cmd[0] = 0;
	if (!strcmp(target, "inv", true)) format(cmd, sizeof cmd, "/inventaire");
	else if (!strcmp(target, "papers", true)) format(cmd, sizeof cmd, "/papiers");
	else if (!strcmp(target, "lic", true)) format(cmd, sizeof cmd, "/permis");
	else if (!strcmp(target, "veh", true)) format(cmd, sizeof cmd, "/vmenu");
	else if (!strcmp(target, "outfit", true)) format(cmd, sizeof cmd, "/tenues");
	else if (!strcmp(target, "garage", true)) format(cmd, sizeof cmd, "/garage");   // [v15b]
	else if (!strcmp(target, "fac", true)) format(cmd, sizeof cmd, "/faction");
	else if (!strcmp(target, "gov", true)) format(cmd, sizeof cmd, "/gouv");
	else if (!strcmp(target, "house", true)) format(cmd, sizeof cmd, "/maison");
	if (cmd[0] == 0)
	{
		// pas de page serveur pour ce raccourci : on garde le menu RP ouvert
		Rp_Say(playerid, RP_COL_INFO, "Ce raccourci n'est pas disponible pour le moment.");
		return Rp_Send(playerid, "hub", RP_DIALOG_OPEN, 0, "Raccourci indisponible.");
	}
	CallLocalFunction("OnPlayerCommandText", "is", playerid, cmd);
	return 1;
}

stock Rp_Action(playerid, const params[])
{
	if (!g_AuthLogged[playerid]) return 0;
	new idx = 0, action[16], arg[26], rest[110];
	Bank_NextToken(params, idx, action, sizeof action);
	Bank_NextToken(params, idx, arg, sizeof arg);
	while (params[idx] == ' ') idx++;
	Admin_Clean(params[idx], rest, sizeof rest);
	for (new k = 0; rest[k] != 0; k++)
	{
		if (rest[k] == 39) rest[k] = ' ';   // apostrophe : casserait les requetes SQL
	}

	// "close" : AUCUNE reponse (sinon la page se rouvre et le joueur reste fige)
	if (!strcmp(action, "close", true)) return 1;

	if (!strcmp(action, "open", true))
	{
		format(g_RpView[playerid], 8, "hub");
		return Rp_Send(playerid, "hub", RP_DIALOG_OPEN, 1, "");
	}
	if (!strcmp(action, "refresh", true)) return Rp_Reply(playerid, 1, "");
	if (!strcmp(action, "view", true))
	{
		if (!strcmp(arg, "hub", true) || !strcmp(arg, "anim", true) || !strcmp(arg, "bill", true) || !strcmp(arg, "ads", true) || !strcmp(arg, "sos", true) || !strcmp(arg, "me", true) || !strcmp(arg, "promo", true) || !strcmp(arg, "law", true))
		{
			format(g_RpView[playerid], 8, "%s", arg);
			return Rp_Send(playerid, arg, RP_DIALOG_UPDATE, 1, "");
		}
		return 1;
	}
	if (!strcmp(action, "go", true)) return Rp_Go(playerid, arg);

	if (!strcmp(action, "anim", true)) return Rp_AnimStart(playerid, arg);
	if (!strcmp(action, "animstop", true))
	{
		Rp_AnimStop(playerid);
		return Rp_Reply(playerid, 1, "");
	}
	if (!strcmp(action, "fight", true))
	{
		new st = strval(arg);
		if (st != 4 && st != 5 && st != 6 && st != 7 && st != 15 && st != 16) return Rp_Reply(playerid, 0, "Style de combat invalide.");
		g_RpFight[playerid] = st;
		SetPlayerFightingStyle(playerid, st);
		return Rp_Reply(playerid, 1, "Style de combat mis a jour.");
	}
	if (!strcmp(action, "desc", true))
	{
		new full[80];
		format(full, sizeof full, "%s %s", arg, rest);
		new len = strlen(full);
		while (len > 0 && full[len - 1] == ' ') { full[len - 1] = 0; len--; }
		if (len < 3) return Rp_Reply(playerid, 0, "Description trop courte.");
		Rp_SetDesc(playerid, full);
		return Rp_Reply(playerid, 1, "Description affichee.");
	}
	if (!strcmp(action, "descoff", true))
	{
		Rp_SetDesc(playerid, "");
		return Rp_Reply(playerid, 1, "Description masquee.");
	}

	if (!strcmp(action, "bill", true)) return Rp_BillSend(playerid, arg, rest);
	if (!strcmp(action, "pay", true)) return Rp_BillPay(playerid, arg, rest);
	if (!strcmp(action, "refuse", true)) return Rp_BillClose(playerid, arg, 2);
	if (!strcmp(action, "cancel", true)) return Rp_BillClose(playerid, arg, 3);

	if (!strcmp(action, "ad", true)) return Rp_AdPublish(playerid, arg, rest);
	if (!strcmp(action, "addel", true)) return Rp_AdDelete(playerid, arg);

	if (!strcmp(action, "sos", true)) return Rp_SosCreate(playerid, arg, rest);
	if (!strcmp(action, "soscancel", true)) return Rp_SosCancel(playerid);
	if (!strcmp(action, "sostake", true)) return Rp_SosTake(playerid, arg);
	if (!strcmp(action, "sosdone", true)) return Rp_SosDone(playerid, arg);

	new lawRes = Law_Action(playerid, action, arg, rest);
	if (lawRes != -1) return lawRes;
	if (!strcmp(action, "promouse", true)) return Promo_Use(playerid, arg);
	if (!strcmp(action, "promoadd", true)) return Promo_Add(playerid, arg, rest);
	if (!strcmp(action, "promodel", true)) return Promo_Del(playerid, arg);

	// action inconnue : on ne fait RIEN (surtout pas rouvrir la page)
	return 1;
}

// ------------------------------------------------------------
//  Commandes
// ------------------------------------------------------------
stock Rp_OnCommand(playerid, const cmdtext[])
{
	if (!strcmp(cmdtext, "/rp", true) || !strcmp(cmdtext, "/menu", true))
	{
		format(g_RpView[playerid], 8, "hub");
		Rp_Send(playerid, "hub", RP_DIALOG_OPEN, 1, "");
		return 1;
	}
	if (!strcmp(cmdtext, "/rpact ", true, 7))
	{
		Rp_Action(playerid, cmdtext[7]);
		return 1;
	}
	return 0;
}
