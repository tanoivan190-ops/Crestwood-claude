// ============================================================
//  [v18] PNJ VIVANTS + QUETES  (include/Npc.inc)
// ------------------------------------------------------------
//  PNJ (acteurs SA-MP) poses par le staff, sauvegardes dans scriptfiles/crestwood_npc.db :
//    nom, skin, role, animation, position, interieur, monde.
//  Roles :
//    0 Ambiance        repliques aleatoires, animation de fond
//    1 Vendeur         comptoir : "Acheter" ouvre la boutique 24/7 la plus proche.
//                      BRAQUABLE : un joueur arme qui le tient en joue -> mains en l'air,
//                      progression, la caisse est videe, alerte police, recherche.
//    2 Banquier        ouvre la banque (si point banque proche)
//    3 Guide           quete "Balade en scooter" (Faggio prete, 5 lieux emblematiques)
//    4 Instructeur     quete "Permis de pilote" (quiz + vol avec anneaux + atterrissage)
//    5 Coursier        quete "Colis urgent" (livraisons chronometrees, serie = bonus)
//    6 Securite        ambiance + avertit quand une arme est sortie pres de lui
//  Interaction : main du HUD "Parler" a 2,5 m (cle 170) -> dialogue du PNJ, le PNJ se tourne vers toi.
//  Etiquette 3D au-dessus de chaque PNJ (nom + role).
//  Staff (niveau 4+) : panneau admin > Monde > PNJ, ou commandes /pnj (aide).
//  Quetes : objectifs sur le HUD (panneau "@obj"), GPS, checkpoints de course, chrono.
//    /quetes = journal, /abandonner = abandonner la quete en cours.
// ============================================================

#define NPC_MAX               80
#define NPC_RANGE             2.5
#define NPC_DIALOG_TALK       9870
#define NPC_DIALOG_QUIZ       9871
#define NPC_DIALOG_JOURNAL    9872
#define NPC_DIALOG_CONFIRM    9873
#define NPC_COL               0x3B82FFFF
#define NPC_COL_OK            0x3DDC97FF
#define NPC_COL_ERR           0xFF5A63FF
#define NPC_COL_INFO          0xC9D1D9FF

#define NPC_ROLE_AMBIANCE     0
#define NPC_ROLE_VENDOR       1
#define NPC_ROLE_BANK         2
#define NPC_ROLE_GUIDE        3
#define NPC_ROLE_PILOT        4
#define NPC_ROLE_COURIER      5
#define NPC_ROLE_GUARD        6
#define NPC_ROLE_GOV          7           // etat civil : carte d'identite (formulaire + signature)
#define NPC_ROLE_COUNT        8
#define NPC_DIALOG_IDFORM     9880        // "@idform" -> panneau natif (formulaire + signature)
#define NPC_ID_FEE            500         // renouvellement (la 1re carte est gratuite)
#define NPC_ID_YEARS          10

#define NPC_ANIM_COUNT        12

// braquage des vendeurs
#define NPC_ROB_HOLD          12          // secondes a tenir le vendeur en joue
#define NPC_ROB_RANGE         7.0
#define NPC_ROB_MIN           1500
#define NPC_ROB_MAX           4500
#define NPC_ROB_COOLDOWN      1200        // 20 min par vendeur
#define NPC_ROB_WANTED        4

// quetes
#define Q_NONE                0
#define Q_SCOOTER             1
#define Q_PILOT               2
#define Q_COURIER             3
#define Q_COUNT               4
#define LIC_PLANE             4           // table gov_licenses (lic = 4) : permis de pilote
#define Q_PILOT_FEE           25000
#define Q_SCOOTER_REWARD      5000
#define Q_TICK_MS             500

new const g_NpcRoleName[NPC_ROLE_COUNT][16] = { "Ambiance", "Vendeur", "Banquier", "Guide", "Instructeur", "Coursier", "Securite", "Etat civil" };
new const g_NpcRoleKey[NPC_ROLE_COUNT][12] = { "ambiance", "vendeur", "banquier", "guide", "instructeur", "coursier", "securite", "etatcivil" };
new const g_NpcAnimName[NPC_ANIM_COUNT][20] = { "Aucune", "Bras croises", "Fume", "Appuye au mur", "Au telephone", "Discute", "Comptoir", "Assis", "Danse", "Garde", "Mecano", "Pense" };
new const g_NpcAnimLib[NPC_ANIM_COUNT][16] = { "", "COP_AMBIENT", "SMOKING", "GANGS", "PED", "GANGS", "INT_SHOP", "PED", "DANCING", "COP_AMBIENT", "CAR", "COP_AMBIENT" };
new const g_NpcAnimId[NPC_ANIM_COUNT][24] = { "", "Coplook_loop", "M_smklean_loop", "leanIDLE", "phone_talk", "prtial_gngtlkA", "shop_loop", "SEAT_idle", "dnce_M_a", "Coplook_nod", "Fixn_Car_Loop", "Coplook_think" };
new const g_QuestName[Q_COUNT][24] = { "", "Balade en scooter", "Permis de pilote", "Colis urgent" };

new DB:g_NpcDB = DB:0;
new g_NpcCount;
new g_NpcDb[NPC_MAX];
new g_NpcActor[NPC_MAX];
new Text3D:g_NpcLabel[NPC_MAX];
new g_NpcName[NPC_MAX][24];
new g_NpcSkin[NPC_MAX];
new g_NpcRole[NPC_MAX];
new g_NpcAnim[NPC_MAX];
new Float:g_NpcPos[NPC_MAX][4];
new g_NpcInt[NPC_MAX];
new g_NpcVw[NPC_MAX];
new g_NpcRobUntil[NPC_MAX];        // vendeur : caisse vide jusqu'a (unix)
new g_NpcRobBy[NPC_MAX] = {INVALID_PLAYER_ID, ...};
new g_NpcRobProgress[NPC_MAX];     // ticks de 500 ms
new g_NpcRobLost[NPC_MAX];
new g_NpcWarnAt[NPC_MAX];          // securite : dernier avertissement

new g_NpcTalk[MAX_PLAYERS] = {-1, ...};   // PNJ du dialogue ouvert
new g_NpcGreetAt[MAX_PLAYERS];            // ambiance : dernier "bonjour" recu
new g_NpcTickN;
new g_NpcTalkOpt[MAX_PLAYERS][6];         // options du dialogue (codes)

// quete en cours
new g_Q[MAX_PLAYERS];
new g_QStep[MAX_PLAYERS];
new g_QSteps[MAX_PLAYERS];
new g_QVeh[MAX_PLAYERS] = {INVALID_VEHICLE_ID, ...};
new g_QStart[MAX_PLAYERS];
new g_QLimit[MAX_PLAYERS];
new g_QNpc[MAX_PLAYERS] = {-1, ...};
new Float:g_QPt[MAX_PLAYERS][8][3];
new g_QPtName[MAX_PLAYERS][8][28];
new g_QOutSince[MAX_PLAYERS];
new g_QStreak[MAX_PLAYERS];
new g_QQuiz[MAX_PLAYERS];
new g_QQuizOk[MAX_PLAYERS];
new Float:g_QGround[MAX_PLAYERS];
new g_QObjAt[MAX_PLAYERS];

forward Npc_Tick();

// ------------------------------------------------------------
//  Lieux emblematiques (balade + colis) : x, y, z
// ------------------------------------------------------------
#define NPC_SPOTS 30
new const Float:g_NpcSpot[NPC_SPOTS][3] = {
	{1481.0, -1742.0, 13.5}, {1544.0, -1675.0, 13.5}, {370.0, -1810.0, 7.8}, {836.0, -2060.0, 12.9}, {2495.0, -1687.0, 13.5},
	{1180.0, -1323.0, 13.6}, {1127.0, -1440.0, 15.8}, {1680.0, -2260.0, 13.5}, {1750.0, -1890.0, 13.6}, {2229.0, -1722.0, 13.5},
	{2105.0, -1806.0, 13.5}, {1832.0, -2000.0, 13.5}, {2500.0, -1960.0, 13.4}, {830.0, -1800.0, 13.4}, {2400.0, -1400.0, 24.0},
	{-1975.0, 137.0, 27.7}, {-2655.0, 640.0, 14.5}, {-1420.0, -290.0, 14.1}, {-2026.0, -100.0, 35.1}, {-2240.0, 120.0, 35.3},
	{-1700.0, 1350.0, 7.2}, {-2440.0, 520.0, 30.0}, {2030.0, 1000.0, 10.8}, {2240.0, 1280.0, 10.8}, {1680.0, 1450.0, 10.8},
	{1607.0, 1820.0, 10.8}, {2290.0, 2420.0, 10.8}, {2100.0, 2190.0, 10.8}, {1960.0, 1620.0, 12.8}, {2500.0, 1480.0, 10.8}
};
new const g_NpcSpotName[NPC_SPOTS][28] = {
	"Mairie de Los Santos", "Commissariat central", "Plage de Santa Maria", "Jetee de Verona", "Grove Street",
	"Hopital All Saints", "Centre commercial Verona", "Aeroport de Los Santos", "Gare Unity", "Salle de Ganton",
	"Pizzeria d'Idlewood", "El Corona", "Willowfield", "Verona Beach", "East Los Santos",
	"Gare de San Fierro", "Hopital de San Fierro", "Aeroport de San Fierro", "Doherty", "Garcia",
	"Esplanade Nord", "Juniper Hill", "Le Strip", "Pyramide", "Aeroport de Las Venturas",
	"Hopital de Las Venturas", "Commissariat LV", "Roca Escalante", "Royal Casino", "Starfish"
};

// ------------------------------------------------------------
//  Outils
// ------------------------------------------------------------
stock Npc_Msg(playerid, color, const text[]) return SendClientMessage(playerid, color, text);

stock Npc_Clean(const in[], out[], outsize)
{
	new j = 0;
	for (new i = 0; in[i] != 0 && j < outsize - 1; i++)
	{
		new c = in[i];
		if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == ' ' || c == '-' || c == '.') out[j++] = c;
		else if (c == '_') out[j++] = ' ';
	}
	out[j] = 0;
	return j;
}

stock Npc_Exec(const q[])
{
	if (g_NpcDB == DB:0) return 0;
	new DBResult:r = db_query(g_NpcDB, q);
	if (r != DBResult:0) db_free_result(r);
	return 1;
}

stock Npc_Pay(playerid, amount, const reason[])
{
	if (amount <= 0) return 1;
	if (GetPlayerMoney(playerid) >= amount) { GivePlayerMoney(playerid, -amount); return 1; }
	if (g_BankLoaded[playerid] && g_BankBalance[playerid] >= amount)
	{
		g_BankBalance[playerid] -= amount;
		Bank_AddHistory(playerid, reason, -amount);
		Bank_SavePlayer(playerid);
		return 1;
	}
	return 0;
}

stock Npc_RoleFromName(const s[])
{
	if (s[0] >= '0' && s[0] <= '9') { new r = strval(s); return (r >= 0 && r < NPC_ROLE_COUNT) ? r : -1; }
	for (new i = 0; i < NPC_ROLE_COUNT; i++) if (!strcmp(s, g_NpcRoleKey[i], true)) return i;
	if (!strcmp(s, "pilote", true) || !strcmp(s, "avion", true)) return NPC_ROLE_PILOT;
	if (!strcmp(s, "banque", true)) return NPC_ROLE_BANK;
	if (!strcmp(s, "scooter", true)) return NPC_ROLE_GUIDE;
	if (!strcmp(s, "garde", true)) return NPC_ROLE_GUARD;
	if (!strcmp(s, "gouvernement", true) || !strcmp(s, "mairie", true) || !strcmp(s, "identite", true)) return NPC_ROLE_GOV;
	return -1;
}

stock Npc_Find(dbid)
{
	for (new i = 0; i < g_NpcCount; i++) if (g_NpcDb[i] == dbid) return i;
	return -1;
}

stock Npc_Nearest(playerid, Float:range)
{
	new best = -1, Float:bd = range, Float:x, Float:y, Float:z;
	GetPlayerPos(playerid, x, y, z);
	new vw = GetPlayerVirtualWorld(playerid), in = GetPlayerInterior(playerid);
	for (new i = 0; i < g_NpcCount; i++)
	{
		if (g_NpcVw[i] != vw || g_NpcInt[i] != in) continue;
		new Float:dx = x - g_NpcPos[i][0], Float:dy = y - g_NpcPos[i][1], Float:dz = z - g_NpcPos[i][2];
		new Float:d = floatsqroot(dx * dx + dy * dy + dz * dz);
		if (d < bd) { bd = d; best = i; }
	}
	return best;
}

stock Npc_Dir(Float:a, &Float:dx, &Float:dy)
{
	dx = -floatsin(a, degrees);
	dy = floatcos(a, degrees);
	return 1;
}

stock Npc_ApplyAnim(i)
{
	if (g_NpcActor[i] == INVALID_ACTOR_ID) return 0;
	new a = g_NpcAnim[i];
	if (a <= 0 || a >= NPC_ANIM_COUNT) { ClearActorAnimations(g_NpcActor[i]); return 1; }
	ApplyActorAnimation(g_NpcActor[i], g_NpcAnimLib[a], g_NpcAnimId[a], 4.1, 1, 0, 0, 0, 0);
	return 1;
}

stock Npc_UpdateLabel(i)
{
	new txt[96], color = NPC_COL;
	if (g_NpcRole[i] == NPC_ROLE_VENDOR && g_NpcRobUntil[i] > gettime())
		format(txt, sizeof txt, "%s\n{FF5A63}Caisse vide", g_NpcName[i]);
	else
		format(txt, sizeof txt, "%s\n{C9D1D9}%s", g_NpcName[i], g_NpcRoleName[g_NpcRole[i]]);
	if (g_NpcLabel[i] != Text3D:INVALID_3DTEXT_ID) Update3DTextLabelText(g_NpcLabel[i], color, txt);
	return 1;
}

stock Npc_Spawn(i)
{
	g_NpcActor[i] = CreateActor(g_NpcSkin[i], g_NpcPos[i][0], g_NpcPos[i][1], g_NpcPos[i][2], g_NpcPos[i][3]);
	if (g_NpcActor[i] != INVALID_ACTOR_ID)
	{
		SetActorVirtualWorld(g_NpcActor[i], g_NpcVw[i]);
		SetActorInvulnerable(g_NpcActor[i], true);
		Npc_ApplyAnim(i);
	}
	new txt[96];
	format(txt, sizeof txt, "%s\n{C9D1D9}%s", g_NpcName[i], g_NpcRoleName[g_NpcRole[i]]);
	g_NpcLabel[i] = Create3DTextLabel(txt, NPC_COL, g_NpcPos[i][0], g_NpcPos[i][1], g_NpcPos[i][2] + 1.12, 12.0, g_NpcVw[i], 1);
	Npc_UpdateLabel(i);
	return 1;
}

stock Npc_Despawn(i)
{
	if (g_NpcActor[i] != INVALID_ACTOR_ID) DestroyActor(g_NpcActor[i]);
	g_NpcActor[i] = INVALID_ACTOR_ID;
	if (g_NpcLabel[i] != Text3D:INVALID_3DTEXT_ID) Delete3DTextLabel(g_NpcLabel[i]);
	g_NpcLabel[i] = Text3D:INVALID_3DTEXT_ID;
	return 1;
}

stock Npc_Load()
{
	for (new i = 0; i < g_NpcCount; i++) Npc_Despawn(i);
	g_NpcCount = 0;
	if (g_NpcDB == DB:0) return 0;
	new DBResult:res = db_query(g_NpcDB, "SELECT id, name, skin, role, anim, x, y, z, a, interior, vw FROM npcs ORDER BY id LIMIT 80");
	if (res == DBResult:0) return 0;
	new rows = db_num_rows(res), f[32];
	for (new r = 0; r < rows && g_NpcCount < NPC_MAX; r++)
	{
		new i = g_NpcCount;
		db_get_field(res, 0, f, sizeof f); g_NpcDb[i] = strval(f);
		db_get_field(res, 1, f, sizeof f); Npc_Clean(f, g_NpcName[i], 24);
		db_get_field(res, 2, f, sizeof f); g_NpcSkin[i] = strval(f);
		db_get_field(res, 3, f, sizeof f); g_NpcRole[i] = strval(f);
		db_get_field(res, 4, f, sizeof f); g_NpcAnim[i] = strval(f);
		db_get_field(res, 5, f, sizeof f); g_NpcPos[i][0] = floatstr(f);
		db_get_field(res, 6, f, sizeof f); g_NpcPos[i][1] = floatstr(f);
		db_get_field(res, 7, f, sizeof f); g_NpcPos[i][2] = floatstr(f);
		db_get_field(res, 8, f, sizeof f); g_NpcPos[i][3] = floatstr(f);
		db_get_field(res, 9, f, sizeof f); g_NpcInt[i] = strval(f);
		db_get_field(res, 10, f, sizeof f); g_NpcVw[i] = strval(f);
		if (g_NpcRole[i] < 0 || g_NpcRole[i] >= NPC_ROLE_COUNT) g_NpcRole[i] = 0;
		if (g_NpcAnim[i] < 0 || g_NpcAnim[i] >= NPC_ANIM_COUNT) g_NpcAnim[i] = 0;
		if (!g_NpcName[i][0]) format(g_NpcName[i], 24, "PNJ %d", g_NpcDb[i]);
		g_NpcRobBy[i] = INVALID_PLAYER_ID;
		g_NpcRobProgress[i] = 0;
		g_NpcLabel[i] = Text3D:INVALID_3DTEXT_ID;
		g_NpcActor[i] = INVALID_ACTOR_ID;
		Npc_Spawn(i);
		g_NpcCount++;
		db_next_row(res);
	}
	db_free_result(res);
	Marks_SendAll();   // main d'interaction 3D au-dessus des PNJ
	return g_NpcCount;
}

// marqueurs 3D (type 18 : main bleue au-dessus de la tete, pas de pastille)
stock Npc_PushMarks()
{
	for (new i = 0; i < g_NpcCount; i++)
		if (g_NpcVw[i] == 0) Mk_Add(18, g_NpcPos[i][0], g_NpcPos[i][1], g_NpcPos[i][2] - 0.95);
	return 1;
}

stock Npc_OnInit()
{
	for (new i = 0; i < NPC_MAX; i++) { g_NpcActor[i] = INVALID_ACTOR_ID; g_NpcLabel[i] = Text3D:INVALID_3DTEXT_ID; }
	g_NpcDB = db_open("crestwood_npc.db");
	if (g_NpcDB == DB:0) { print("[PNJ] base crestwood_npc.db impossible a ouvrir"); return 0; }
	Npc_Exec("CREATE TABLE IF NOT EXISTS npcs (id INTEGER PRIMARY KEY AUTOINCREMENT, name TEXT, skin INTEGER, role INTEGER, anim INTEGER, x REAL, y REAL, z REAL, a REAL, interior INTEGER, vw INTEGER)");
	Npc_Exec("CREATE TABLE IF NOT EXISTS quests (name TEXT, quest INTEGER, done INTEGER DEFAULT 0, best INTEGER DEFAULT 0, ts INTEGER DEFAULT 0, PRIMARY KEY (name, quest))");
	Npc_Load();
	SetTimer("Npc_Tick", Q_TICK_MS, true);
	printf("[PNJ] %d PNJ charges.", g_NpcCount);
	return 1;
}

stock Npc_OnExit()
{
	for (new i = 0; i < g_NpcCount; i++) Npc_Despawn(i);
	if (g_NpcDB != DB:0) db_close(g_NpcDB);
	g_NpcDB = DB:0;
	return 1;
}

public OnActorStreamIn(actorid, forplayerid)
{
	// SA-MP : l'animation d'un acteur doit etre reappliquee quand il apparait pour un joueur
	for (new i = 0; i < g_NpcCount; i++)
	{
		if (g_NpcActor[i] != actorid) continue;
		if (g_NpcRobBy[i] != INVALID_PLAYER_ID) ApplyActorAnimation(actorid, "ROB_BANK", "SHP_HandsUp_Scr", 4.1, 0, 0, 0, 1, 0);
		else Npc_ApplyAnim(i);
		break;
	}
	return 1;
}

// ------------------------------------------------------------
//  Creation / edition (staff)
// ------------------------------------------------------------
stock Npc_Create(playerid, role, skin, const name[])
{
	if (g_NpcDB == DB:0 || g_NpcCount >= NPC_MAX) return 0;
	new Float:x, Float:y, Float:z, Float:a, clean[24], q[300];
	GetPlayerPos(playerid, x, y, z);
	GetPlayerFacingAngle(playerid, a);
	Npc_Clean(name, clean, sizeof clean);
	if (!clean[0]) format(clean, sizeof clean, "%s", g_NpcRoleName[role]);
	if (skin < 0 || skin > 311 || skin == 74) skin = 211;
	new anim = (role == NPC_ROLE_VENDOR || role == NPC_ROLE_GOV) ? 6 : ((role == NPC_ROLE_GUARD) ? 9 : ((role == NPC_ROLE_AMBIANCE) ? 5 : 1));
	format(q, sizeof q, "INSERT INTO npcs (name, skin, role, anim, x, y, z, a, interior, vw) VALUES ('%s', %d, %d, %d, %f, %f, %f, %f, %d, %d)",
		clean, skin, role, anim, x, y, z, a, GetPlayerInterior(playerid), GetPlayerVirtualWorld(playerid));
	Npc_Exec(q);
	// le staff recule d'un pas pour ne pas etre dans le PNJ
	SetPlayerPos(playerid, x - 1.2 * floatsin(-a, degrees), y - 1.2 * floatcos(-a, degrees), z);
	Npc_Load();
	return 1;
}

stock Npc_SetField(dbid, const field[], value)
{
	new q[120];
	format(q, sizeof q, "UPDATE npcs SET %s = %d WHERE id = %d", field, value, dbid);
	Npc_Exec(q);
	return Npc_Load();
}

stock Npc_MoveHere(playerid, dbid)
{
	new Float:x, Float:y, Float:z, Float:a, q[200];
	GetPlayerPos(playerid, x, y, z);
	GetPlayerFacingAngle(playerid, a);
	format(q, sizeof q, "UPDATE npcs SET x = %f, y = %f, z = %f, a = %f, interior = %d, vw = %d WHERE id = %d", x, y, z, a, GetPlayerInterior(playerid), GetPlayerVirtualWorld(playerid), dbid);
	Npc_Exec(q);
	SetPlayerPos(playerid, x - 1.2 * floatsin(-a, degrees), y - 1.2 * floatcos(-a, degrees), z);
	return Npc_Load();
}

stock Npc_Delete(dbid)
{
	new q[64];
	format(q, sizeof q, "DELETE FROM npcs WHERE id = %d", dbid);
	Npc_Exec(q);
	return Npc_Load();
}

stock Npc_Goto(playerid, i)
{
	new Float:dx, Float:dy;
	Npc_Dir(g_NpcPos[i][3], dx, dy);
	SetPlayerInterior(playerid, g_NpcInt[i]);
	SetPlayerVirtualWorld(playerid, g_NpcVw[i]);
	SetPlayerPos(playerid, g_NpcPos[i][0] + dx * 1.6, g_NpcPos[i][1] + dy * 1.6, g_NpcPos[i][2]);
	SetPlayerFacingAngle(playerid, g_NpcPos[i][3] + 180.0);
	return 1;
}

// ------------------------------------------------------------
//  Panneau admin (onglet Monde) : {"x":"npc","l":[[id,"nom",role,anim,skin,x,y,z],...]}
// ------------------------------------------------------------
stock Npc_AdminNeed(const action[])
{
	if (!strcmp(action, "npclist", true) || !strcmp(action, "npctp", true)) return 2;
	if (!strcmp(action, "npcadd", true) || !strcmp(action, "npcdel", true) || !strcmp(action, "npchere", true) ||
		!strcmp(action, "npcrole", true) || !strcmp(action, "npcanim", true) || !strcmp(action, "npcskin", true) || !strcmp(action, "npcname", true)) return 4;
	return 0;
}

stock Npc_AdminSend(playerid)
{
	new json[3800], part[110];
	format(json, sizeof json, "{\"x\":\"npc\",\"l\":[");
	for (new i = 0; i < g_NpcCount; i++)
	{
		format(part, sizeof part, "%s[%d,\"%s\",%d,%d,%d,%d,%d,%d]", i ? (",") : (""), g_NpcDb[i], g_NpcName[i], g_NpcRole[i], g_NpcAnim[i], g_NpcSkin[i],
			floatround(g_NpcPos[i][0]), floatround(g_NpcPos[i][1]), floatround(g_NpcPos[i][2]));
		if (strlen(json) + strlen(part) > sizeof(json) - 4) break;
		strcat(json, part, sizeof json);
	}
	strcat(json, "]}", sizeof json);
	return ShowPlayerDialog(playerid, ADMIN_DIALOG_UPDATE, DIALOG_STYLE_MSGBOX, "@admin", json, "OK", "");
}

// arg : npcadd "role skin nom" ; npcrole/anim/skin "valeur" ; npcname "nom"
stock Npc_AdminAction(playerid, const action[], const tid[], const arg[], msg[], msgsize, &ok)
{
	new need = Npc_AdminNeed(action);
	if (!need) return 0;
	new dbid = strval(tid), i = Npc_Find(dbid);
	if (!strcmp(action, "npclist", true)) { format(msg, msgsize, "%d PNJ.", g_NpcCount); ok = 1; }
	else if (!strcmp(action, "npcadd", true))
	{
		new idx = 0, sRole[16], sSkin[8];
		Bank_NextToken(arg, idx, sRole, sizeof sRole);
		Bank_NextToken(arg, idx, sSkin, sizeof sSkin);
		while (arg[idx] == ' ') idx++;
		new role = Npc_RoleFromName(sRole);
		if (role < 0) format(msg, msgsize, "Role inconnu (vendeur, banquier, guide, instructeur, coursier, securite, ambiance).");
		else if (g_NpcCount >= NPC_MAX) format(msg, msgsize, "Maximum %d PNJ atteint.", NPC_MAX);
		else if (Npc_Create(playerid, role, sSkin[0] ? strval(sSkin) : 211, arg[idx])) { format(msg, msgsize, "PNJ %s cree a ta place.", g_NpcRoleName[role]); ok = 1; }
		else format(msg, msgsize, "Creation impossible.");
	}
	else if (i < 0) format(msg, msgsize, "PNJ introuvable.");
	else if (!strcmp(action, "npctp", true)) { Npc_Goto(playerid, i); format(msg, msgsize, "Teleporte devant %s.", g_NpcName[i]); ok = 1; }
	else if (!strcmp(action, "npcdel", true)) { Npc_Delete(dbid); format(msg, msgsize, "PNJ supprime."); ok = 1; }
	else if (!strcmp(action, "npchere", true)) { Npc_MoveHere(playerid, dbid); format(msg, msgsize, "PNJ deplace a ta place (meme orientation que toi)."); ok = 1; }
	else if (!strcmp(action, "npcrole", true))
	{
		new role = Npc_RoleFromName(arg);
		if (role < 0) format(msg, msgsize, "Role inconnu.");
		else { Npc_SetField(dbid, "role", role); format(msg, msgsize, "Role : %s.", g_NpcRoleName[role]); ok = 1; }
	}
	else if (!strcmp(action, "npcanim", true))
	{
		new a = strval(arg);
		if (a < 0 || a >= NPC_ANIM_COUNT) format(msg, msgsize, "Animation inconnue (0 a %d).", NPC_ANIM_COUNT - 1);
		else { Npc_SetField(dbid, "anim", a); format(msg, msgsize, "Animation : %s.", g_NpcAnimName[a]); ok = 1; }
	}
	else if (!strcmp(action, "npcskin", true))
	{
		new s = strval(arg);
		if (s < 0 || s > 311 || s == 74) format(msg, msgsize, "Skin invalide (0 a 311).");
		else { Npc_SetField(dbid, "skin", s); format(msg, msgsize, "Skin %d applique.", s); ok = 1; }
	}
	else if (!strcmp(action, "npcname", true))
	{
		new clean[24], q[120];
		Npc_Clean(arg, clean, sizeof clean);
		if (!clean[0]) format(msg, msgsize, "Nom vide.");
		else
		{
			format(q, sizeof q, "UPDATE npcs SET name = '%s' WHERE id = %d", clean, dbid);
			Npc_Exec(q);
			Npc_Load();
			format(msg, msgsize, "Nom : %s.", clean);
			ok = 1;
		}
	}
	return 1;
}

// ------------------------------------------------------------
//  Interaction (main du HUD)
// ------------------------------------------------------------
// vendeur (non braque) a portee : la boutique s'ouvre devant lui, meme sans point 24/7
stock Npc_NearVendor(playerid)
{
	new i = Npc_Nearest(playerid, NPC_RANGE + 1.5);
	if (i < 0 || g_NpcRole[i] != NPC_ROLE_VENDOR) return 0;
	if (g_NpcRobBy[i] != INVALID_PLAYER_ID || g_NpcRobUntil[i] > gettime()) return 0;
	return 1;
}

stock Npc_InteractKey(playerid)
{
	if (g_NpcCount <= 0) return 0;
	return (Npc_Nearest(playerid, NPC_RANGE) >= 0) ? 170 : 0;
}

stock Npc_FacePlayer(i, playerid)
{
	if (g_NpcActor[i] == INVALID_ACTOR_ID) return 0;
	new Float:x, Float:y, Float:z;
	GetPlayerPos(playerid, x, y, z);
	new Float:a = atan2(y - g_NpcPos[i][1], x - g_NpcPos[i][0]) - 90.0;
	SetActorFacingAngle(g_NpcActor[i], a);
	return 1;
}

stock Npc_Line(i, out[], size)
{
	switch (g_NpcRole[i])
	{
		case NPC_ROLE_VENDOR:
		{
			switch (random(3))
			{
				case 0: format(out, size, "Bienvenue ! Prends ce qu'il te faut, tout est frais du jour.");
				case 1: format(out, size, "Salut l'ami. Eau, sandwichs, telephones... on a tout ici.");
				default: format(out, size, "Hey ! Pas de grabuge dans ma boutique, hein ?");
			}
		}
		case NPC_ROLE_BANK: format(out, size, "Bonjour, Banque de Crestwood. Que puis-je faire pour vous ?");
		case NPC_ROLE_GUIDE: format(out, size, "Tu debarques a Crestwood ? Je te prete un scooter et je te fais visiter.");
		case NPC_ROLE_PILOT: format(out, size, "Ecole de pilotage de Crestwood. Tu veux tes ailes ? Theorie puis vol d'examen.");
		case NPC_ROLE_COURIER: format(out, size, "J'ai des colis urgents a livrer. Rapide et propre = gros pourboire.");
		case NPC_ROLE_GUARD: format(out, size, "Circule. Ici, on garde les armes rangees.");
		case NPC_ROLE_GOV: format(out, size, "Bonjour, service de l'etat civil de la Republique de Crestwood. Je vous ecoute.");
		default:
		{
			switch (random(5))
			{
				case 0: format(out, size, "Il fait une chaleur aujourd'hui...");
				case 1: format(out, size, "T'as vu les nouvelles voitures au concessionnaire ? Une folie.");
				case 2: format(out, size, "Fais attention a toi la nuit dans le coin.");
				case 3: format(out, size, "On m'a dit que le maire prepare de grosses annonces.");
				default: format(out, size, "Bonne journee a toi !");
			}
		}
	}
	return 1;
}

stock Npc_QuestDone(playerid, quest, &best)
{
	best = 0;
	if (g_NpcDB == DB:0) return 0;
	new name[MAX_PLAYER_NAME + 1], q[160], f[16], done = 0;
	GetPlayerName(playerid, name, sizeof name);
	format(q, sizeof q, "SELECT done, best FROM quests WHERE name = '%s' AND quest = %d", name, quest);
	new DBResult:res = db_query(g_NpcDB, q);
	if (res == DBResult:0) return 0;
	if (db_num_rows(res) > 0)
	{
		db_get_field(res, 0, f, sizeof f); done = strval(f);
		db_get_field(res, 1, f, sizeof f); best = strval(f);
	}
	db_free_result(res);
	return done;
}

stock Npc_QuestSave(playerid, quest, seconds)
{
	new name[MAX_PLAYER_NAME + 1], q[260], best;
	GetPlayerName(playerid, name, sizeof name);
	new done = Npc_QuestDone(playerid, quest, best);
	if (!best || seconds < best) best = seconds;
	format(q, sizeof q, "INSERT OR REPLACE INTO quests (name, quest, done, best, ts) VALUES ('%s', %d, %d, %d, %d)", name, quest, done + 1, best, gettime());
	Npc_Exec(q);
	return done + 1;
}

stock Npc_Talk(playerid)
{
	new i = Npc_Nearest(playerid, NPC_RANGE + 0.5);
	if (i < 0) return 0;
	if (g_NpcRobBy[i] != INVALID_PLAYER_ID) return Npc_Msg(playerid, NPC_COL_ERR, "Le vendeur tremble, il ne peut pas te parler maintenant.");
	Npc_FacePlayer(i, playerid);
	if (g_NpcActor[i] != INVALID_ACTOR_ID && g_NpcAnim[i] != 6 && g_NpcAnim[i] != 7)
		ApplyActorAnimation(g_NpcActor[i], "GANGS", "prtial_gngtlkA", 4.1, 0, 0, 0, 0, 0);
	g_NpcTalk[playerid] = i;
	new line[128], body[600], list[320], caption[64], n = 0, best;
	Npc_Line(i, line, sizeof line);
	format(caption, sizeof caption, "{3B82FF}%s {9AA0A8}- %s", g_NpcName[i], g_NpcRoleName[g_NpcRole[i]]);
	for (new k = 0; k < 6; k++) g_NpcTalkOpt[playerid][k] = 0;
	list[0] = 0;
	switch (g_NpcRole[i])
	{
		case NPC_ROLE_VENDOR:
		{
			if (g_NpcRobUntil[i] > gettime()) strcat(list, "{FF5A63}Boutique fermee (braquage recent)\n", sizeof list), g_NpcTalkOpt[playerid][n++] = 0;
			else strcat(list, "Acheter\n", sizeof list), g_NpcTalkOpt[playerid][n++] = 1;
		}
		case NPC_ROLE_BANK: strcat(list, "Acceder a mon compte\n", sizeof list), g_NpcTalkOpt[playerid][n++] = 2;
		case NPC_ROLE_GUIDE:
		{
			new d = Npc_QuestDone(playerid, Q_SCOOTER, best);
			if (d) format(body, sizeof body, "Refaire la balade en scooter {9AA0A8}(record %d:%02d)\n", best / 60, best % 60);
			else format(body, sizeof body, "Balade en scooter {3DDC97}(+%d $ la 1re fois)\n", Q_SCOOTER_REWARD);
			strcat(list, body, sizeof list); g_NpcTalkOpt[playerid][n++] = 3;
		}
		case NPC_ROLE_PILOT:
		{
			if (Gov_HasLicense(playerid, LIC_PLANE)) strcat(list, "{3DDC97}Tu as deja ton permis de pilote\n", sizeof list), g_NpcTalkOpt[playerid][n++] = 0;
			else
			{
				format(body, sizeof body, "Passer le permis de pilote {3B82FF}(%d $)\n", Q_PILOT_FEE);
				strcat(list, body, sizeof list); g_NpcTalkOpt[playerid][n++] = 4;
			}
		}
		case NPC_ROLE_GOV:
		{
			new age, origin[32];
			if (Ui_LoadId(playerid, age, origin, sizeof origin)) strcat(list, "Renouveler ma carte d'identite\nVoir ma carte d'identite\n", sizeof list), g_NpcTalkOpt[playerid][n++] = 8, g_NpcTalkOpt[playerid][n++] = 9;
			else strcat(list, "Faire ma carte d'identite {3DDC97}(gratuit)\n", sizeof list), g_NpcTalkOpt[playerid][n++] = 8;
		}
		case NPC_ROLE_COURIER:
		{
			if (g_QStreak[playerid] > 0) format(body, sizeof body, "Prendre un colis {3DDC97}(serie x%d)\n", g_QStreak[playerid] + 1);
			else format(body, sizeof body, "Prendre un colis urgent\n");
			strcat(list, body, sizeof list); g_NpcTalkOpt[playerid][n++] = 5;
		}
	}
	if (g_Q[playerid] != Q_NONE) strcat(list, "{FFB454}Abandonner ma quete en cours\n", sizeof list), g_NpcTalkOpt[playerid][n++] = 7;
	strcat(list, "Discuter\nAu revoir", sizeof list);
	g_NpcTalkOpt[playerid][n++] = 6;
	g_NpcTalkOpt[playerid][n++] = 0;
	format(body, sizeof body, "%s", list);
	// replique du PNJ dans le chat (legende SA-MP limitee a 64 caracteres)
	new say[170];
	format(say, sizeof say, "{3B82FF}%s{FFFFFF} : \"%s\"", g_NpcName[i], line);
	SendClientMessage(playerid, 0xFFFFFFFF, say);
	return ShowPlayerDialog(playerid, NPC_DIALOG_TALK, DIALOG_STYLE_LIST, caption, body, "Choisir", "Fermer");
}

// ------------------------------------------------------------
//  Quetes : outils communs
// ------------------------------------------------------------
stock Q_Elapsed(playerid) return gettime() - g_QStart[playerid];

stock Q_DestroyVeh(playerid)
{
	if (g_QVeh[playerid] != INVALID_VEHICLE_ID)
	{
		DestroyVehicle(g_QVeh[playerid]);
		g_QVeh[playerid] = INVALID_VEHICLE_ID;
	}
	return 1;
}

stock Q_End(playerid, const reason[], success)
{
	if (g_Q[playerid] == Q_NONE) return 0;
	DisablePlayerRaceCheckpoint(playerid);
	if (g_Q[playerid] == Q_COURIER) RemovePlayerAttachedObject(playerid, 9);
	Gps_Clear(playerid);
	Obj_Hide(playerid);
	Q_DestroyVeh(playerid);
	if (reason[0]) Npc_Msg(playerid, success ? NPC_COL_OK : NPC_COL_ERR, reason);
	if (!success) GameTextForPlayer(playerid, "~r~QUETE ECHOUEE", 2500, 3);
	g_Q[playerid] = Q_NONE;
	g_QStep[playerid] = 0;
	g_QNpc[playerid] = -1;
	return 1;
}

stock Q_ShowPoint(playerid)
{
	new s = g_QStep[playerid];
	if (s >= g_QSteps[playerid]) return 0;
	new type = 0;
	new Float:nx = g_QPt[playerid][s][0], Float:ny = g_QPt[playerid][s][1], Float:nz = g_QPt[playerid][s][2];
	if (s + 1 < g_QSteps[playerid]) { nx = g_QPt[playerid][s + 1][0]; ny = g_QPt[playerid][s + 1][1]; nz = g_QPt[playerid][s + 1][2]; }
	if (g_Q[playerid] == Q_PILOT) type = (s + 1 >= g_QSteps[playerid]) ? 1 : 3;
	else type = (s + 1 >= g_QSteps[playerid]) ? 1 : 0;
	SetPlayerRaceCheckpoint(playerid, type, g_QPt[playerid][s][0], g_QPt[playerid][s][1], g_QPt[playerid][s][2], nx, ny, nz, (g_Q[playerid] == Q_PILOT) ? 14.0 : 6.0);
	Gps_Set(playerid, g_QPt[playerid][s][0], g_QPt[playerid][s][1]);
	return 1;
}

stock Q_Objectives(playerid)
{
	new sub[64], line[64], q = g_Q[playerid], el = Q_Elapsed(playerid);
	new left = g_QLimit[playerid] - el;
	if (left < 0) left = 0;
	format(sub, sizeof sub, "Chrono %d:%02d - reste %d:%02d", el / 60, el % 60, left / 60, left % 60);
	Obj_Begin(g_QuestName[q], sub);
	if (q == Q_SCOOTER)
	{
		new inVeh = (g_QVeh[playerid] != INVALID_VEHICLE_ID && IsPlayerInVehicle(playerid, g_QVeh[playerid]));
		Obj_Line("Monte sur le scooter", inVeh ? 2 : 1);
		for (new s = 0; s < g_QSteps[playerid] - 1 && s < 6; s++)
			Obj_Line(g_QPtName[playerid][s], (s < g_QStep[playerid]) ? 2 : ((s == g_QStep[playerid] && inVeh) ? 1 : 0));
		Obj_Line("Rends le scooter au guide", (g_QStep[playerid] >= g_QSteps[playerid] - 1) ? 1 : 0);
	}
	else if (q == Q_PILOT)
	{
		new inVeh = (g_QVeh[playerid] != INVALID_VEHICLE_ID && IsPlayerInVehicle(playerid, g_QVeh[playerid]));
		Obj_Line("Monte dans le Dodo", inVeh ? 2 : 1);
		format(line, sizeof line, "Passe dans les anneaux %d/%d", g_QStep[playerid] > g_QSteps[playerid] - 1 ? g_QSteps[playerid] - 1 : g_QStep[playerid], g_QSteps[playerid] - 1);
		Obj_Line(line, (g_QStep[playerid] >= g_QSteps[playerid] - 1) ? 2 : (inVeh ? 1 : 0));
		Obj_Line("Atterris en douceur (< 45 km/h)", (g_QStep[playerid] >= g_QSteps[playerid] - 1) ? 1 : 0);
		Obj_Line("Garde l'avion en bon etat", 1);
	}
	else if (q == Q_COURIER)
	{
		format(line, sizeof line, "Livre le colis : %s", g_QPtName[playerid][0]);
		Obj_Line(line, g_QStep[playerid] == 0 ? 1 : 2);
		Obj_Line("Ne traine pas : bonus si tu es rapide", 1);
	}
	return Obj_Send(playerid);
}

stock Q_CanStart(playerid)
{
	if (g_Q[playerid] != Q_NONE) { Npc_Msg(playerid, NPC_COL_ERR, "Termine (ou /abandonner) ta quete en cours d'abord."); return 0; }
	if (g_JobOn[playerid]) { Npc_Msg(playerid, NPC_COL_ERR, "Tu es en service : termine ton metier d'abord."); return 0; }
	if (Cj_Busy(playerid)) { Npc_Msg(playerid, NPC_COL_ERR, "Tu es en service (emploi) : /finemploi d'abord."); return 0; }
	return 1;
}

stock Q_SpawnVeh(playerid, model, Float:x, Float:y, Float:z, Float:a)
{
	new v = CreateVehicle(model, x, y, z, a, 1, 1, -1);
	if (v == INVALID_VEHICLE_ID || v <= 0 || v > MAX_VEHICLES) return INVALID_VEHICLE_ID;
	SetVehicleVirtualWorld(v, GetPlayerVirtualWorld(playerid));
	LinkVehicleToInterior(v, GetPlayerInterior(playerid));
	g_CwFuel[v] = 100.0;
	SetVehicleParamsEx(v, VEHICLE_PARAMS_OFF, VEHICLE_PARAMS_OFF, VEHICLE_PARAMS_OFF, VEHICLE_PARAMS_OFF, VEHICLE_PARAMS_OFF, VEHICLE_PARAMS_OFF, VEHICLE_PARAMS_OFF);   // [v20] moteur coupe
	SetVehicleNumberPlate(v, "CW-QUETE");
	return v;
}

// ---- Balade en scooter : 5 lieux les plus proches du guide ----
stock Q_StartScooter(playerid, i)
{
	if (!Q_CanStart(playerid)) return 0;
	// tri des lieux par distance au guide
	new idx[NPC_SPOTS], Float:d[NPC_SPOTS];
	for (new s = 0; s < NPC_SPOTS; s++)
	{
		idx[s] = s;
		new Float:dx = g_NpcSpot[s][0] - g_NpcPos[i][0], Float:dy = g_NpcSpot[s][1] - g_NpcPos[i][1];
		d[s] = floatsqroot(dx * dx + dy * dy);
	}
	for (new a = 0; a < NPC_SPOTS - 1; a++)
		for (new b = a + 1; b < NPC_SPOTS; b++)
			if (d[idx[b]] < d[idx[a]]) { new t = idx[a]; idx[a] = idx[b]; idx[b] = t; }
	new n = 0;
	for (new s = 0; s < NPC_SPOTS && n < 5; s++)
	{
		if (d[idx[s]] < 60.0) continue;          // trop pres du guide
		if (d[idx[s]] > 2600.0) break;
		g_QPt[playerid][n][0] = g_NpcSpot[idx[s]][0]; g_QPt[playerid][n][1] = g_NpcSpot[idx[s]][1]; g_QPt[playerid][n][2] = g_NpcSpot[idx[s]][2];
		format(g_QPtName[playerid][n], 28, "%s", g_NpcSpotName[idx[s]]);
		n++;
	}
	if (n < 3) return Npc_Msg(playerid, NPC_COL_ERR, "Guide : il n'y a rien a visiter par ici (PNJ trop loin des villes).");
	// retour au guide
	new Float:dx, Float:dy;
	Npc_Dir(g_NpcPos[i][3], dx, dy);
	g_QPt[playerid][n][0] = g_NpcPos[i][0] + dx * 4.0; g_QPt[playerid][n][1] = g_NpcPos[i][1] + dy * 4.0; g_QPt[playerid][n][2] = g_NpcPos[i][2];
	format(g_QPtName[playerid][n], 28, "Retour au guide");
	g_QSteps[playerid] = n + 1;
	new v = Q_SpawnVeh(playerid, 462, g_NpcPos[i][0] + dx * 3.0, g_NpcPos[i][1] + dy * 3.0, g_NpcPos[i][2] + 0.4, g_NpcPos[i][3] + 90.0);
	if (v == INVALID_VEHICLE_ID) return Npc_Msg(playerid, NPC_COL_ERR, "Guide : pas de scooter disponible (limite de vehicules).");
	g_Q[playerid] = Q_SCOOTER;
	g_QVeh[playerid] = v;
	g_QStep[playerid] = 0;
	g_QStart[playerid] = gettime();
	g_QLimit[playerid] = 60 * (4 + n);
	g_QNpc[playerid] = i;
	g_QOutSince[playerid] = 0;
	PutPlayerInVehicle(playerid, v, 0);
	Q_ShowPoint(playerid);
	Q_Objectives(playerid);
	GameTextForPlayer(playerid, "~b~BALADE EN SCOOTER~n~~w~suis le GPS", 3000, 3);
	Npc_Msg(playerid, NPC_COL, "Guide : \"Casque sur la tete ! Suis les points, je t'attends ici. Pas de casse avec mon scooter !\"");
	return 1;
}

// ---- Permis de pilote : quiz puis vol ----
stock Q_StartPilotQuiz(playerid, i)
{
	if (!Q_CanStart(playerid)) return 0;
	if (Gov_HasLicense(playerid, LIC_PLANE)) return Npc_Msg(playerid, NPC_COL_OK, "Instructeur : tu as deja ton permis, capitaine.");
	if (!Npc_Pay(playerid, Q_PILOT_FEE, "Ecole de pilotage"))
	{
		new m[96];
		format(m, sizeof m, "Instructeur : l'examen coute %d $ (liquide ou banque).", Q_PILOT_FEE);
		return Npc_Msg(playerid, NPC_COL_ERR, m);
	}
	g_QNpc[playerid] = i;
	g_QQuiz[playerid] = 0;
	g_QQuizOk[playerid] = 0;
	Npc_Msg(playerid, NPC_COL, "Instructeur : \"Theorie d'abord. 3 questions, il faut un sans-faute pour voler.\"");
	return Q_PilotQuestion(playerid);
}

stock Q_PilotQuestion(playerid)
{
	new cap[64];
	format(cap, sizeof cap, "{3B82FF}Permis de pilote {9AA0A8}- question %d/3", g_QQuiz[playerid] + 1);
	switch (g_QQuiz[playerid])
	{
		case 0: return ShowPlayerDialog(playerid, NPC_DIALOG_QUIZ, DIALOG_STYLE_TABLIST_HEADERS, cap, "{9AA0A8}Pour decoller, je dois :\nFace au vent, pleine puissance, je tire doucement vers 120 km/h\nPleine vitesse puis je tire le manche a fond d'un coup\nJe decolle en marche arriere", "Repondre", "");
		case 1: return ShowPlayerDialog(playerid, NPC_DIALOG_QUIZ, DIALOG_STYLE_TABLIST_HEADERS, cap, "{9AA0A8}En finale, avant l'atterrissage :\nJe garde 300 km/h jusqu'au sol\nJe reduis les gaz, nez legerement leve, roues dans l'axe\nJe coupe le moteur a 100 m de haut", "Repondre", "");
		default: return ShowPlayerDialog(playerid, NPC_DIALOG_QUIZ, DIALOG_STYLE_TABLIST_HEADERS, cap, "{9AA0A8}Un avion arrive en face sur ma trajectoire :\nJe fonce, il s'ecartera\nJe me deporte a droite et je garde le contact visuel\nJe monte a la verticale", "Repondre", "");
	}
	return 0;
}

stock Q_StartPilotFlight(playerid)
{
	new i = g_QNpc[playerid];
	if (i < 0 || i >= g_NpcCount) return 0;
	new Float:dx, Float:dy, Float:rx, Float:ry;
	Npc_Dir(g_NpcPos[i][3], dx, dy);
	rx = dy; ry = -dx;   // a droite de la piste
	new Float:sx = g_NpcPos[i][0] + dx * 14.0, Float:sy = g_NpcPos[i][1] + dy * 14.0, Float:sz = g_NpcPos[i][2];
	g_QGround[playerid] = sz;
	// tour de piste : montee, virage, vent arriere, base, finale, atterrissage
	new Float:pts[7][3];
	pts[0][0] = sx + dx * 380.0;               pts[0][1] = sy + dy * 380.0;               pts[0][2] = sz + 25.0;
	pts[1][0] = sx + dx * 900.0;               pts[1][1] = sy + dy * 900.0;               pts[1][2] = sz + 90.0;
	pts[2][0] = sx + dx * 1150.0 + rx * 450.0; pts[2][1] = sy + dy * 1150.0 + ry * 450.0; pts[2][2] = sz + 115.0;
	pts[3][0] = sx + dx * 450.0 + rx * 650.0;  pts[3][1] = sy + dy * 450.0 + ry * 650.0;  pts[3][2] = sz + 105.0;
	pts[4][0] = sx - dx * 350.0 + rx * 380.0;  pts[4][1] = sy - dy * 350.0 + ry * 380.0;  pts[4][2] = sz + 65.0;
	pts[5][0] = sx - dx * 280.0;               pts[5][1] = sy - dy * 280.0;               pts[5][2] = sz + 28.0;
	pts[6][0] = sx + dx * 160.0;               pts[6][1] = sy + dy * 160.0;               pts[6][2] = sz;
	new const names[7][28] = { "Decollage", "Montee", "Virage", "Vent arriere", "Etape de base", "Finale", "Atterrissage" };
	for (new p = 0; p < 7; p++)
	{
		g_QPt[playerid][p][0] = pts[p][0]; g_QPt[playerid][p][1] = pts[p][1]; g_QPt[playerid][p][2] = pts[p][2];
		format(g_QPtName[playerid][p], 28, "%s", names[p]);
	}
	g_QSteps[playerid] = 7;
	new v = Q_SpawnVeh(playerid, 593, sx, sy, sz + 1.2, g_NpcPos[i][3]);
	if (v == INVALID_VEHICLE_ID)
	{
		GivePlayerMoney(playerid, Q_PILOT_FEE);
		return Npc_Msg(playerid, NPC_COL_ERR, "Instructeur : aucun avion disponible, tu es rembourse.");
	}
	g_Q[playerid] = Q_PILOT;
	g_QVeh[playerid] = v;
	g_QStep[playerid] = 0;
	g_QStart[playerid] = gettime();
	g_QLimit[playerid] = 7 * 60;
	g_QOutSince[playerid] = 0;
	PutPlayerInVehicle(playerid, v, 0);
	Q_ShowPoint(playerid);
	Q_Objectives(playerid);
	GameTextForPlayer(playerid, "~b~EXAMEN DE VOL~n~~w~passe dans les anneaux", 3500, 3);
	Npc_Msg(playerid, NPC_COL, "Instructeur : \"Theorie validee ! Decollage dans l'axe, tour de piste par la droite, puis atterrissage devant moi.\"");
	return 1;
}

// ---- Colis urgent : une adresse a 300-1800 m, chrono selon la distance ----
stock Q_StartCourier(playerid, i)
{
	if (!Q_CanStart(playerid)) return 0;
	new cand[NPC_SPOTS], n = 0;
	for (new s = 0; s < NPC_SPOTS; s++)
	{
		new Float:dx = g_NpcSpot[s][0] - g_NpcPos[i][0], Float:dy = g_NpcSpot[s][1] - g_NpcPos[i][1];
		new Float:d = floatsqroot(dx * dx + dy * dy);
		if (d >= 300.0 && d <= 1800.0) cand[n++] = s;
	}
	if (!n) return Npc_Msg(playerid, NPC_COL_ERR, "Coursier : aucune adresse dans le coin pour le moment.");
	new s = cand[random(n)];
	new Float:dx = g_NpcSpot[s][0] - g_NpcPos[i][0], Float:dy = g_NpcSpot[s][1] - g_NpcPos[i][1];
	new Float:dist = floatsqroot(dx * dx + dy * dy);
	g_QPt[playerid][0][0] = g_NpcSpot[s][0]; g_QPt[playerid][0][1] = g_NpcSpot[s][1]; g_QPt[playerid][0][2] = g_NpcSpot[s][2];
	format(g_QPtName[playerid][0], 28, "%s", g_NpcSpotName[s]);
	g_QSteps[playerid] = 1;
	g_Q[playerid] = Q_COURIER;
	g_QStep[playerid] = 0;
	g_QStart[playerid] = gettime();
	g_QLimit[playerid] = floatround(dist / 9.0) + 50;
	g_QNpc[playerid] = i;
	g_QVeh[playerid] = INVALID_VEHICLE_ID;
	SetPlayerAttachedObject(playerid, 9, 1220, 6, 0.08, 0.04, -0.18, -80.0, 0.0, 0.0, 0.55, 0.55, 0.55);   // carton dans les mains
	Q_ShowPoint(playerid);
	Q_Objectives(playerid);
	new m[144];
	format(m, sizeof m, "Coursier : \"Colis pour %s. Tu as %d:%02d. Prends le vehicule que tu veux, mais fonce !\"", g_NpcSpotName[s], g_QLimit[playerid] / 60, g_QLimit[playerid] % 60);
	Npc_Msg(playerid, NPC_COL, m);
	GameTextForPlayer(playerid, "~b~COLIS URGENT", 2500, 3);
	return 1;
}

stock Q_Arrive(playerid)
{
	new q = g_Q[playerid];
	if (q == Q_NONE) return 0;
	PlayerPlaySound(playerid, 1058, 0.0, 0.0, 0.0);
	g_QStep[playerid]++;
	if (q == Q_COURIER)
	{
		RemovePlayerAttachedObject(playerid, 9);
		new el = Q_Elapsed(playerid), left = g_QLimit[playerid] - el;
		new Float:dx = g_QPt[playerid][0][0] - g_NpcPos[g_QNpc[playerid]][0], Float:dy = g_QPt[playerid][0][1] - g_NpcPos[g_QNpc[playerid]][1];
		new pay = 1500 + floatround(floatsqroot(dx * dx + dy * dy) * 1.2);
		new bonus = (left > 0) ? (left * 12) : 0;
		g_QStreak[playerid] = (g_QStreak[playerid] < 5) ? (g_QStreak[playerid] + 1) : 5;
		new total = (pay + bonus) * (10 + g_QStreak[playerid] * 2) / 10;
		GivePlayerMoney(playerid, total);
		Npc_QuestSave(playerid, Q_COURIER, el);
		new m[144];
		format(m, sizeof m, "Colis livre en %d:%02d ! +%d $ (course %d + pourboire %d, serie x%d). Retourne voir le coursier pour la suite.", el / 60, el % 60, total, pay, bonus, g_QStreak[playerid] + 1);
		GameTextForPlayer(playerid, "~g~COLIS LIVRE", 2500, 3);
		return Q_End(playerid, m, 1);
	}
	if (g_QStep[playerid] >= g_QSteps[playerid])
	{
		new el = Q_Elapsed(playerid), m[180];
		if (q == Q_SCOOTER)
		{
			new best, first = !Npc_QuestDone(playerid, Q_SCOOTER, best);
			new Float:hp;
			if (g_QVeh[playerid] != INVALID_VEHICLE_ID) GetVehicleHealth(g_QVeh[playerid], hp);
			Npc_QuestSave(playerid, Q_SCOOTER, el);
			if (first)
			{
				GivePlayerMoney(playerid, Q_SCOOTER_REWARD);
				format(m, sizeof m, "Guide : \"Bienvenue officiellement a Crestwood !\" Balade terminee en %d:%02d, +%d $.", el / 60, el % 60, Q_SCOOTER_REWARD);
			}
			else format(m, sizeof m, "Balade terminee en %d:%02d%s.", el / 60, el % 60, (best && el < best) ? (" - NOUVEAU RECORD") : (""));
			GameTextForPlayer(playerid, "~g~BALADE TERMINEE", 3000, 3);
			return Q_End(playerid, m, 1);
		}
		if (q == Q_PILOT)
		{
			new Float:hp = 0.0;
			if (g_QVeh[playerid] != INVALID_VEHICLE_ID) GetVehicleHealth(g_QVeh[playerid], hp);
			if (hp < 500.0) return Q_End(playerid, "Instructeur : \"Atterrissage trop brutal, l'avion est abime. Reviens quand tu veux retenter.\"", 0);
			new name[MAX_PLAYER_NAME + 1], q2[200];
			GetPlayerName(playerid, name, sizeof name);
			format(q2, sizeof q2, "INSERT OR REPLACE INTO gov_licenses (name, lic, ts) VALUES ('%s', %d, %d)", name, LIC_PLANE, gettime());
			db_free_result(db_query(g_JobDB, q2));
			Npc_QuestSave(playerid, Q_PILOT, el);
			RemovePlayerFromVehicle(playerid);
			format(m, sizeof m, "Instructeur : \"Superbe vol ! Felicitations, voici ton PERMIS DE PILOTE.\" (vol en %d:%02d, avion a %d %%)", el / 60, el % 60, floatround((hp - 250.0) / 7.5));
			GameTextForPlayer(playerid, "~g~PERMIS DE PILOTE OBTENU", 4000, 3);
			return Q_End(playerid, m, 1);
		}
	}
	Q_ShowPoint(playerid);
	Q_Objectives(playerid);
	return 1;
}

// ------------------------------------------------------------
//  Boucle (500 ms) : arrivee aux points, regles des quetes, braquages, securite
// ------------------------------------------------------------
public Npc_Tick()
{
	new now = gettime();
	for (new p = 0, m = GetMaxPlayers(); p < m; p++)
	{
		if (!IsPlayerConnected(p) || g_Q[p] == Q_NONE) continue;
		new q = g_Q[p], s = g_QStep[p];
		if (Q_Elapsed(p) > g_QLimit[p]) { Q_End(p, (q == Q_COURIER) ? ("Coursier : trop tard, le client a annule la commande.") : ("Temps ecoule : quete echouee."), 0); g_QStreak[p] = 0; continue; }
		if (now - g_QObjAt[p] >= 5) { g_QObjAt[p] = now; Q_Objectives(p); }   // chrono du HUD
		new Float:x, Float:y, Float:z;
		GetPlayerPos(p, x, y, z);
		if (q == Q_SCOOTER || q == Q_PILOT)
		{
			new v = g_QVeh[p];
			if (v == INVALID_VEHICLE_ID) { Q_End(p, "Le vehicule de la quete a disparu.", 0); continue; }
			if (!IsPlayerInVehicle(p, v))
			{
				if (!g_QOutSince[p]) { g_QOutSince[p] = now; GameTextForPlayer(p, "~y~REMONTE DANS LE VEHICULE~n~~w~30 secondes", 2500, 3); Q_Objectives(p); }
				else if (now - g_QOutSince[p] > 30) { Q_End(p, "Tu as abandonne le vehicule : quete echouee.", 0); continue; }
				continue;
			}
			if (g_QOutSince[p]) { g_QOutSince[p] = 0; Q_Objectives(p); }
			new Float:hp;
			GetVehicleHealth(v, hp);
			if (hp < 300.0) { Q_End(p, "Le vehicule est hors d'usage : quete echouee.", 0); continue; }
		}
		if (s >= g_QSteps[p]) continue;
		new Float:dx = x - g_QPt[p][s][0], Float:dy = y - g_QPt[p][s][1], Float:dz = z - g_QPt[p][s][2];
		if (q == Q_PILOT)
		{
			if (s + 1 >= g_QSteps[p])
			{
				// atterrissage : sur la piste, presque arrete
				new Float:vx, Float:vy, Float:vz;
				GetVehicleVelocity(g_QVeh[p], vx, vy, vz);
				new Float:kmh = floatsqroot(vx * vx + vy * vy + vz * vz) * 180.0;
				if (dx * dx + dy * dy < 45.0 * 45.0 && floatabs(z - g_QGround[p]) < 4.5 && kmh < 45.0) Q_Arrive(p);
			}
			else if (dx * dx + dy * dy + dz * dz < 24.0 * 24.0) Q_Arrive(p);
		}
		else if (dx * dx + dy * dy < 7.5 * 7.5 && floatabs(dz) < 6.0) Q_Arrive(p);
	}
	Npc_RobTick(now);
	// ambiance : les PNJ saluent les joueurs qui passent tout pres (au plus 1 fois / 2 min par joueur)
	if (++g_NpcTickN % 4 == 0) Npc_GreetTick(now);
	return 1;
}

stock Npc_GreetTick(now)
{
	for (new p = 0, m = GetMaxPlayers(); p < m; p++)
	{
		if (!IsPlayerConnected(p) || now - g_NpcGreetAt[p] < 120 || GetPlayerState(p) != PLAYER_STATE_ONFOOT) continue;
		new i = Npc_Nearest(p, 3.2);
		if (i < 0 || g_NpcRobBy[i] != INVALID_PLAYER_ID || g_NpcRole[i] == NPC_ROLE_GUARD) continue;
		g_NpcGreetAt[p] = now;
		new m2[128];
		switch (random(4))
		{
			case 0: format(m2, sizeof m2, "{9AA0A8}%s : \"Salut ! Bonne journee a Crestwood.\"", g_NpcName[i]);
			case 1: format(m2, sizeof m2, "{9AA0A8}%s : \"Hey, ca va ? Besoin de quelque chose ?\"", g_NpcName[i]);
			case 2: format(m2, sizeof m2, "{9AA0A8}%s te fait un signe de la tete.", g_NpcName[i]);
			default: format(m2, sizeof m2, "{9AA0A8}%s : \"Approche, je ne mords pas.\"", g_NpcName[i]);
		}
		SendClientMessage(p, 0xFFFFFFFF, m2);
		Npc_FacePlayer(i, p);
	}
	return 1;
}

stock Npc_IsArmed(playerid)
{
	new w = GetPlayerWeapon(playerid);
	return (w >= 22 && w <= 34) ? 1 : 0;
}

// le joueur vise-t-il le PNJ i ? (cible SA-MP, sinon : arme + touche viser + face au PNJ a < 7 m)
stock Npc_IsAimedBy(i, playerid)
{
	if (g_NpcActor[i] == INVALID_ACTOR_ID || !Npc_IsArmed(playerid)) return 0;
	if (GetPlayerState(playerid) != PLAYER_STATE_ONFOOT) return 0;
	if (GetPlayerVirtualWorld(playerid) != g_NpcVw[i]) return 0;
	if (!IsPlayerInRangeOfPoint(playerid, NPC_ROB_RANGE, g_NpcPos[i][0], g_NpcPos[i][1], g_NpcPos[i][2])) return 0;
	if (GetPlayerTargetActor(playerid) == g_NpcActor[i]) return 1;
	new keys, ud, lr;
	GetPlayerKeys(playerid, keys, ud, lr);
	if (!(keys & KEY_HANDBRAKE)) return 0;
	new Float:x, Float:y, Float:z, Float:a;
	GetPlayerPos(playerid, x, y, z);
	GetPlayerFacingAngle(playerid, a);
	new Float:want = atan2(g_NpcPos[i][1] - y, g_NpcPos[i][0] - x) - 90.0;
	new Float:diff = want - a;
	while (diff > 180.0) diff -= 360.0;
	while (diff < -180.0) diff += 360.0;
	return (floatabs(diff) < 35.0) ? 1 : 0;
}

stock Npc_RobAlert(i, robber)
{
	new m[144], zone[28];
	format(zone, sizeof zone, "%s", g_NpcName[i]);
	format(m, sizeof m, "[CENTRAL] Braquage en cours chez %s (vendeur). Unites demandees, position sur le GPS.", zone);
	for (new p = 0, mx = GetMaxPlayers(); p < mx; p++)
	{
		if (!IsPlayerConnected(p) || !Fac_IsLaw(p)) continue;
		SendClientMessage(p, 0xFF5A63FF, m);
		Gps_Set(p, g_NpcPos[i][0], g_NpcPos[i][1]);
	}
	SetPlayerWantedLevel(robber, NPC_ROB_WANTED);
	return 1;
}

stock Npc_RobTick(now)
{
	for (new i = 0; i < g_NpcCount; i++)
	{
		new role = g_NpcRole[i];
		if (role == NPC_ROLE_GUARD)
		{
			if (now - g_NpcWarnAt[i] < 15) continue;
			for (new p = 0, mx = GetMaxPlayers(); p < mx; p++)
			{
				if (!IsPlayerConnected(p) || !Npc_IsArmed(p) || Fac_IsLaw(p)) continue;
				if (GetPlayerVirtualWorld(p) != g_NpcVw[i] || !IsPlayerInRangeOfPoint(p, 9.0, g_NpcPos[i][0], g_NpcPos[i][1], g_NpcPos[i][2])) continue;
				new m[112];
				format(m, sizeof m, "%s (securite) : \"Range ton arme tout de suite, ou j'appelle la police.\"", g_NpcName[i]);
				SendClientMessage(p, 0xFFB454FF, m);
				Npc_FacePlayer(i, p);
				g_NpcWarnAt[i] = now;
				break;
			}
			continue;
		}
		if (role != NPC_ROLE_VENDOR) continue;
		new rb = g_NpcRobBy[i];
		if (rb == INVALID_PLAYER_ID)
		{
			if (g_NpcRobUntil[i] > now) continue;
			if (g_NpcRobUntil[i] && g_NpcRobUntil[i] <= now) { g_NpcRobUntil[i] = 0; Npc_UpdateLabel(i); }
			for (new p = 0, mx = GetMaxPlayers(); p < mx; p++)
			{
				if (!IsPlayerConnected(p) || Fac_IsLaw(p) || !Npc_IsAimedBy(i, p)) continue;
				// debut du braquage : mains en l'air
				g_NpcRobBy[i] = p;
				g_NpcRobProgress[i] = 0;
				g_NpcRobLost[i] = 0;
				Npc_FacePlayer(i, p);
				ApplyActorAnimation(g_NpcActor[i], "ROB_BANK", "SHP_HandsUp_Scr", 4.1, 0, 0, 0, 1, 0);
				new m[120];
				format(m, sizeof m, "%s : \"Ok ok ! Ne tire pas ! Je vide la caisse !\"", g_NpcName[i]);
				SendClientMessage(p, 0xFFB454FF, m);
				Npc_RobAlert(i, p);
				break;
			}
			continue;
		}
		// braquage en cours
		if (!IsPlayerConnected(rb) || !Npc_IsAimedBy(i, rb))
		{
			g_NpcRobLost[i]++;
			if (g_NpcRobLost[i] > 6)   // 3 s sans le tenir en joue
			{
				if (IsPlayerConnected(rb)) { GameTextForPlayer(rb, "~r~BRAQUAGE RATE", 2500, 3); SendClientMessage(rb, NPC_COL_ERR, "Le vendeur a declenche l'alarme silencieuse et ferme la caisse."); }
				g_NpcRobBy[i] = INVALID_PLAYER_ID;
				g_NpcRobUntil[i] = now + NPC_ROB_COOLDOWN / 2;
				Npc_ApplyAnim(i);
				Npc_UpdateLabel(i);
			}
			continue;
		}
		g_NpcRobLost[i] = 0;
		g_NpcRobProgress[i]++;
		new secs = g_NpcRobProgress[i] / 2;
		if (g_NpcRobProgress[i] % 2 == 0 && secs < NPC_ROB_HOLD)
		{
			new t[64];
			format(t, sizeof t, "~r~BRAQUAGE~n~~w~garde-le en joue %d s", NPC_ROB_HOLD - secs);
			GameTextForPlayer(rb, t, 1100, 3);
		}
		if (secs >= NPC_ROB_HOLD)
		{
			new amount = NPC_ROB_MIN + random(NPC_ROB_MAX - NPC_ROB_MIN + 1);
			GivePlayerMoney(rb, amount);
			ApplyActorAnimation(g_NpcActor[i], "ROB_BANK", "SHP_Rob_GiveCash", 4.1, 0, 0, 0, 0, 0);
			new m[120];
			format(m, sizeof m, "Braquage reussi : +%d $. La police arrive, file !", amount);
			SendClientMessage(rb, NPC_COL_OK, m);
			GameTextForPlayer(rb, "~g~CAISSE VIDEE", 2500, 3);
			g_NpcRobBy[i] = INVALID_PLAYER_ID;
			g_NpcRobUntil[i] = now + NPC_ROB_COOLDOWN;
			Npc_UpdateLabel(i);
			SetTimerEx("Npc_RobRelax", 6000, false, "i", i);
		}
	}
	return 1;
}

forward Npc_RobRelax(i);
public Npc_RobRelax(i)
{
	if (i < 0 || i >= g_NpcCount || g_NpcRobBy[i] != INVALID_PLAYER_ID) return 0;
	ApplyActorAnimation(g_NpcActor[i], "PED", "cower", 4.1, 1, 0, 0, 0, 0);
	SetTimerEx("Npc_RobRelax2", 9000, false, "i", i);
	return 1;
}

forward Npc_RobRelax2(i);
public Npc_RobRelax2(i)
{
	if (i < 0 || i >= g_NpcCount || g_NpcRobBy[i] != INVALID_PLAYER_ID) return 0;
	return Npc_ApplyAnim(i);
}

// ------------------------------------------------------------
//  Evenements
// ------------------------------------------------------------
stock Npc_OnConnect(playerid)
{
	g_Q[playerid] = Q_NONE;
	g_QVeh[playerid] = INVALID_VEHICLE_ID;
	g_QNpc[playerid] = -1;
	g_QStreak[playerid] = 0;
	g_NpcTalk[playerid] = -1;
	g_NpcGreetAt[playerid] = gettime();   // pas de salut pendant la 1re minute
	return 1;
}

stock Npc_OnDisconnect(playerid)
{
	Q_DestroyVeh(playerid);
	g_Q[playerid] = Q_NONE;
	for (new i = 0; i < g_NpcCount; i++)
	{
		if (g_NpcRobBy[i] == playerid) { g_NpcRobBy[i] = INVALID_PLAYER_ID; g_NpcRobUntil[i] = gettime() + NPC_ROB_COOLDOWN / 2; Npc_ApplyAnim(i); Npc_UpdateLabel(i); }
	}
	return 1;
}

stock Npc_OnDeath(playerid)
{
	if (g_Q[playerid] != Q_NONE) { Q_End(playerid, "Tu es tombe : quete echouee.", 0); g_QStreak[playerid] = 0; }
	RemovePlayerAttachedObject(playerid, 9);
	return 1;
}

stock Npc_OnVehicleDeath(vehicleid)
{
	for (new p = 0, m = GetMaxPlayers(); p < m; p++)
	{
		if (IsPlayerConnected(p) && g_Q[p] != Q_NONE && g_QVeh[p] == vehicleid)
		{
			g_QVeh[p] = INVALID_VEHICLE_ID;
			Q_End(p, "Le vehicule de la quete a ete detruit : quete echouee.", 0);
		}
	}
	return 1;
}

stock Npc_OnRaceCheckpoint(playerid)
{
	if (g_Q[playerid] == Q_NONE || g_Q[playerid] == Q_PILOT) return 0;   // l'avion valide par la boucle (atterrissage)
	new s = g_QStep[playerid];
	if (s >= g_QSteps[playerid]) return 0;
	if (IsPlayerInRangeOfPoint(playerid, 12.0, g_QPt[playerid][s][0], g_QPt[playerid][s][1], g_QPt[playerid][s][2])) Q_Arrive(playerid);
	return 1;
}

// avion sans permis : avertissement (le controle reste du RP police)
stock Npc_OnStateChange(playerid, newstate)
{
	if (newstate != PLAYER_STATE_DRIVER) return 0;
	new v = GetPlayerVehicleID(playerid), m = GetVehicleModel(v);
	if (g_Q[playerid] == Q_PILOT && v == g_QVeh[playerid]) return 0;
	new air = (m == 460 || m == 476 || m == 511 || m == 512 || m == 513 || m == 519 || m == 520 || m == 553 || m == 577 || m == 592 || m == 593);
	if (air && !Gov_HasLicense(playerid, LIC_PLANE))
		SendClientMessage(playerid, 0xFFB454FF, "Tu pilotes SANS permis de pilote : amende et saisie si la police te controle. Passe le permis a l'ecole de pilotage.");
	return 1;
}

stock Npc_OnDialog(playerid, dialogid, response, listitem)
{
	if (dialogid == NPC_DIALOG_TALK)
	{
		new i = g_NpcTalk[playerid];
		g_NpcTalk[playerid] = -1;
		if (i >= 0 && i < g_NpcCount && g_NpcRobBy[i] == INVALID_PLAYER_ID) SetTimerEx("Npc_RobRelax2", 1500, false, "i", i);
		if (!response || i < 0 || i >= g_NpcCount || listitem < 0 || listitem >= 6) return 1;
		if (!IsPlayerInRangeOfPoint(playerid, NPC_RANGE + 2.0, g_NpcPos[i][0], g_NpcPos[i][1], g_NpcPos[i][2])) return Npc_Msg(playerid, NPC_COL_ERR, "Tu t'es eloigne du PNJ.");
		switch (g_NpcTalkOpt[playerid][listitem])
		{
			case 1: Shop_Open(playerid);
			case 2:
			{
				if (Bank_IsNearPoint(playerid)) Bank_Open(playerid);
				else Npc_Msg(playerid, NPC_COL_ERR, "Le guichet n'est pas relie a un point banque (staff : pose un point banque ici).");
			}
			case 3: Q_StartScooter(playerid, i);
			case 4:
			{
				new t[200];
				format(t, sizeof t, "{FFFFFF}L'examen coute {3B82FF}%d ${FFFFFF} (liquide ou banque).\n\n1. Quiz theorique (3 questions, sans-faute)\n2. Vol : decollage, tour de piste, atterrissage devant l'instructeur\n\nOn y va ?", Q_PILOT_FEE);
				g_QNpc[playerid] = i;
				ShowPlayerDialog(playerid, NPC_DIALOG_CONFIRM, DIALOG_STYLE_MSGBOX, "{3B82FF}Ecole de pilotage", t, "Payer", "Annuler");
			}
			case 5: Q_StartCourier(playerid, i);
			case 6:
			{
				new line[128], m[160];
				Npc_Line(i, line, sizeof line);
				format(m, sizeof m, "%s : \"%s\"", g_NpcName[i], line);
				Npc_Msg(playerid, NPC_COL_INFO, m);
			}
			case 7: { Q_End(playerid, "Quete abandonnee.", 0); g_QStreak[playerid] = 0; }
			case 8: Npc_IdForm(playerid);
			case 9: Ui_ShowId(playerid, playerid);
		}
		return 1;
	}
	if (dialogid == NPC_DIALOG_CONFIRM)
	{
		if (response && g_QNpc[playerid] >= 0) Q_StartPilotQuiz(playerid, g_QNpc[playerid]);
		return 1;
	}
	if (dialogid == NPC_DIALOG_QUIZ)
	{
		if (!response) { Npc_Msg(playerid, NPC_COL_ERR, "Examen abandonne (non rembourse)."); return 1; }
		new good = (g_QQuiz[playerid] == 0) ? 0 : 1;   // ligne de la bonne reponse
		if (listitem == good) g_QQuizOk[playerid]++;
		g_QQuiz[playerid]++;
		if (g_QQuiz[playerid] < 3) return Q_PilotQuestion(playerid);
		if (g_QQuizOk[playerid] >= 3) return Q_StartPilotFlight(playerid);
		new m[128];
		format(m, sizeof m, "Instructeur : \"%d/3, ce n'est pas suffisant. Revois tes bases et repaie l'examen pour retenter.\"", g_QQuizOk[playerid]);
		return Npc_Msg(playerid, NPC_COL_ERR, m);
	}
	if (dialogid == NPC_DIALOG_JOURNAL) return 1;
	return 0;
}

// ---- Etat civil : formulaire (panneau natif Android) puis signature ----
// GPS vers le PNJ du role le plus proche (0 = aucun)
stock Npc_GpsToRole(playerid, role)
{
	new best = -1, Float:bd = 999999.0, Float:x, Float:y, Float:z;
	GetPlayerPos(playerid, x, y, z);
	for (new i = 0; i < g_NpcCount; i++)
	{
		if (g_NpcRole[i] != role || g_NpcVw[i] != 0) continue;
		new Float:dx = x - g_NpcPos[i][0], Float:dy = y - g_NpcPos[i][1], Float:d = dx * dx + dy * dy;
		if (d < bd) { bd = d; best = i; }
	}
	if (best < 0) return 0;
	Gps_Set(playerid, g_NpcPos[best][0], g_NpcPos[best][1]);
	new m[144];
	format(m, sizeof m, "Etat civil : tu n'as pas encore de carte d'identite. Va voir %s au guichet (GPS active).", g_NpcName[best]);
	SendClientMessage(playerid, NPC_COL, m);
	return 1;
}

stock Npc_NearGov(playerid)
{
	new i = Npc_Nearest(playerid, NPC_RANGE + 2.0);
	return (i >= 0 && g_NpcRole[i] == NPC_ROLE_GOV) ? 1 : 0;
}

stock Npc_IdForm(playerid)
{
	new first[24], last[24], age, origin[32], json[300];
	Ui_SplitName(playerid, first, sizeof first, last, sizeof last);
	new has = Ui_LoadId(playerid, age, origin, sizeof origin);
	if (!has) { age = 25; format(origin, sizeof origin, "Los Santos"); }
	for (new k = 0; origin[k]; k++) if (origin[k] == '"' || origin[k] == 92) origin[k] = ' ';
	format(json, sizeof json, "{\"prenom\":\"%s\",\"nom\":\"%s\",\"sexe\":\"%s\",\"age\":%d,\"origine\":\"%s\",\"renew\":%d,\"fee\":%d,\"key\":\"%s_%s\"}",
		first, last, (g_IdGender[playerid] == 1) ? ("F") : ("M"), age, origin, has, has ? NPC_ID_FEE : 0, first, last);
	return ShowPlayerDialog(playerid, NPC_DIALOG_IDFORM, DIALOG_STYLE_MSGBOX, "@idform", json, "OK", "");
}

// /idsign <age> <lieu_de_naissance>  (envoye par le panneau natif apres la signature)
stock Npc_IdSign(playerid, const params[])
{
	if (!Npc_NearGov(playerid)) return Npc_Msg(playerid, NPC_COL_ERR, "Etat civil : presente-toi au guichet (agent de l'etat civil).");
	new idx = 0, sAge[8], sOri[40], origin[32];
	Bank_NextToken(params, idx, sAge, sizeof sAge);
	Bank_NextToken(params, idx, sOri, sizeof sOri);
	new age = strval(sAge);
	for (new k = 0; sOri[k]; k++) if (sOri[k] == '_') sOri[k] = ' ';
	Ui_CleanOrigin(sOri, origin, sizeof origin);
	if (age < 16 || age > 90) return Npc_Msg(playerid, NPC_COL_ERR, "Etat civil : age invalide (16 a 90 ans).");
	if (strlen(origin) < 2) format(origin, sizeof origin, "Los Santos");
	new oldAge, oldOri[32];
	new renew = Ui_LoadId(playerid, oldAge, oldOri, sizeof oldOri);
	if (renew && !Npc_Pay(playerid, NPC_ID_FEE, "Etat civil : carte d'identite"))
	{
		new m[96];
		format(m, sizeof m, "Etat civil : le renouvellement coute %d $.", NPC_ID_FEE);
		return Npc_Msg(playerid, NPC_COL_ERR, m);
	}
	new name[MAX_PLAYER_NAME + 1], q[260];
	GetPlayerName(playerid, name, sizeof name);
	format(q, sizeof q, "INSERT OR REPLACE INTO player_ids (name, age, origin, issued, signed) VALUES ('%s', %d, '%s', %d, 1)", name, age, origin, gettime());
	db_free_result(db_query(g_JobDB, q));
	Npc_Msg(playerid, NPC_COL_OK, "Etat civil : \"Signature enregistree. Voici votre carte nationale d'identite de la Republique de Crestwood.\"");
	PlayerPlaySound(playerid, 1058, 0.0, 0.0, 0.0);
	return Ui_ShowId(playerid, playerid);
}

stock Npc_Journal(playerid)
{
	new body[600], line[140], best;
	format(body, sizeof body, "{3B82FF}Quete\t{9AA0A8}Etat\t{9AA0A8}Record\n");
	for (new q = 1; q < Q_COUNT; q++)
	{
		new done = Npc_QuestDone(playerid, q, best);
		if (q == Q_PILOT && Gov_HasLicense(playerid, LIC_PLANE)) done = done ? done : 1;
		format(line, sizeof line, "%s\t%s\t%s%d:%02d\n", g_QuestName[q], done ? ((q == Q_COURIER) ? ("{3DDC97}Livreur confirme") : ("{3DDC97}Terminee")) : ("{9AA0A8}A decouvrir"), best ? ("") : ("-- "), best / 60, best % 60);
		strcat(body, line, sizeof body);
	}
	if (g_Q[playerid] != Q_NONE)
	{
		format(line, sizeof line, "{FFB454}En cours : %s\t{FFB454}%d:%02d\t\n", g_QuestName[g_Q[playerid]], Q_Elapsed(playerid) / 60, Q_Elapsed(playerid) % 60);
		strcat(body, line, sizeof body);
	}
	return ShowPlayerDialog(playerid, NPC_DIALOG_JOURNAL, DIALOG_STYLE_TABLIST_HEADERS, "{3B82FF}Journal des quetes", body, "Fermer", "");
}

stock Npc_OnCommand(playerid, const cmdtext[])
{
	if (!strcmp(cmdtext, "/quetes", true) || !strcmp(cmdtext, "/quests", true)) return Npc_Journal(playerid), 1;
	if (!strcmp(cmdtext, "/idsign ", true, 8)) return Npc_IdSign(playerid, cmdtext[8]), 1;
	if (!strcmp(cmdtext, "/abandonner", true))
	{
		if (g_Q[playerid] == Q_NONE) return Npc_Msg(playerid, NPC_COL_ERR, "Aucune quete en cours."), 1;
		Q_End(playerid, "Quete abandonnee.", 0);
		g_QStreak[playerid] = 0;
		return 1;
	}
	if (strcmp(cmdtext, "/pnj", true, 4)) return 0;
	if (Admin_Level(playerid) < 4) return 0;
	new idx = 4, sub[16], a1[16], a2[16];
	Bank_NextToken(cmdtext, idx, sub, sizeof sub);
	if (!sub[0])
	{
		Npc_Msg(playerid, NPC_COL, "PNJ (staff 4+) : panneau admin > Monde > PNJ, ou :");
		Npc_Msg(playerid, NPC_COL_INFO, "/pnj creer <role> <skin> <nom>   roles : vendeur banquier guide instructeur coursier securite etatcivil ambiance");
		Npc_Msg(playerid, NPC_COL_INFO, "/pnj liste  |  /pnj tp <id>  |  /pnj ici <id>  |  /pnj suppr <id>");
		Npc_Msg(playerid, NPC_COL_INFO, "/pnj role <id> <role>  |  /pnj anim <id> <0-11>  |  /pnj skin <id> <skin>  |  /pnj nom <id> <nom>");
		return 1;
	}
	Bank_NextToken(cmdtext, idx, a1, sizeof a1);
	new msg[144], ok, action[16], arg[48];
	if (!strcmp(sub, "liste", true))
	{
		for (new i = 0; i < g_NpcCount; i++)
		{
			format(msg, sizeof msg, "  #%d %s - %s (skin %d, anim %s) - %.0f, %.0f, %.0f", g_NpcDb[i], g_NpcName[i], g_NpcRoleName[g_NpcRole[i]], g_NpcSkin[i], g_NpcAnimName[g_NpcAnim[i]], g_NpcPos[i][0], g_NpcPos[i][1], g_NpcPos[i][2]);
			Npc_Msg(playerid, 0xFFFFFFFF, msg);
		}
		if (!g_NpcCount) Npc_Msg(playerid, NPC_COL_INFO, "Aucun PNJ.");
		return 1;
	}
	if (!strcmp(sub, "creer", true))
	{
		Bank_NextToken(cmdtext, idx, a2, sizeof a2);
		while (cmdtext[idx] == ' ') idx++;
		format(arg, sizeof arg, "%s %s %s", a1, a2[0] ? a2 : ("211"), cmdtext[idx]);
		format(action, sizeof action, "npcadd");
	}
	else
	{
		while (cmdtext[idx] == ' ') idx++;
		format(arg, sizeof arg, "%s", cmdtext[idx]);
		if (!strcmp(sub, "tp", true)) format(action, sizeof action, "npctp");
		else if (!strcmp(sub, "ici", true)) format(action, sizeof action, "npchere");
		else if (!strcmp(sub, "suppr", true)) format(action, sizeof action, "npcdel");
		else if (!strcmp(sub, "role", true)) format(action, sizeof action, "npcrole");
		else if (!strcmp(sub, "anim", true)) format(action, sizeof action, "npcanim");
		else if (!strcmp(sub, "skin", true)) format(action, sizeof action, "npcskin");
		else if (!strcmp(sub, "nom", true)) format(action, sizeof action, "npcname");
		else return Npc_Msg(playerid, NPC_COL_ERR, "Sous-commande inconnue : /pnj pour l'aide."), 1;
	}
	Npc_AdminAction(playerid, action, a1, arg, msg, sizeof msg, ok);
	return Npc_Msg(playerid, ok ? NPC_COL_OK : NPC_COL_ERR, msg), 1;
}
