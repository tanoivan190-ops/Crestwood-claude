/* ============================================================
   CRESTWOOD RP v41 (21/11/2026) - FACTIONS RP COMPLETES
   ------------------------------------------------------------
   1) FACTIONS PAR DEFAUT : creees au demarrage, une seule fois chacune (table faction_seed) :
        Police (CPD), Sheriff (CCSO), EMS, Pompiers (CFD), FBI, Garde nationale (armee),
        Gouvernement, Justice (DOJ), Crestwood News, Mecaniciens (CAS), Taxi (CAB).
      Grades, salaires, skins, couleurs et sigles remplis. Une faction deja presente (meme nom,
      ou meme type) est reprise et seulement completee (salaires vides, sigle, skins).
      Une faction supprimee par le staff n'est PAS recreee, sauf avec /factionsdefaut (staff).
   2) POMPIERS (type "fire") : incendies aleatoires dans la ville (points par defaut + points du staff),
      le feu se propage, brule les joueurs proches, des victimes a secourir. On eteint a l'extincteur
      (vestiaire) ou a la lance du camion (Firetruck 407). Paie selon le travail de chacun.
      Vehicule qui explose -> feu de vehicule. Appel : telephone 18, /112 pompiers.
      /incendies /secourir  |  staff : /incendie [1-3] /stopincendies /feupoint [nom] /feupoints /feupointdel id
   3) JUSTICE (type "justice") : casier judiciaire automatique (prisons, amendes, verdicts),
      mandats d'arret / de perquisition, proces + verdict (acquitte, amende, prison), caution,
      avocats (honoraires, plaidoirie : -25 % de prison si l'avocat est present au verdict).
      /casier /moncasier /mandat /mandats /annulermandat /executermandat /proces /verdict
      /caution /payercaution /avocat /accepteravocat /demandercaution /effacercasier
   4) FBI : traceur GPS pose sur un vehicule (/traceur /traceurs /suivre /stopsuivre),
      ecoute telephonique d'un numero (/ecoute numero, SMS et appels), detection : /inspectervehicule.
   5) ARMEE / GOUVERNEMENT : zone militaire interdite (/zonemilitaire on|off|ici <rayon>),
      etat d'urgence (/etatdurgence on|off : salaires des services d'urgence x2, annonce a tous).
   Roue d'interaction : casier (forces de l'ordre / justice), proces (juge), traceur (FBI),
      chercher un traceur (proprietaire / mecanicien).
   Texte ASCII (pas d'accents dans les messages).
   ============================================================ */
#define FRP_COL_INFO          0x33CCFFFF
#define FRP_COL_OK            0x33CC33FF
#define FRP_COL_ERR           0xFF6347FF
#define FRP_COL_FIRE          0xFF7A1AFF
#define FRP_COL_JUS           0xA78BFAFF
#define FRP_COL_FBI           0x8E7CFFFF
#define FRP_DIALOG_LIST       9655

// ------------------------------------------------------------
//  1) factions par defaut
// ------------------------------------------------------------
#define FRP_DEFS              11

new const g_FrpKey[FRP_DEFS][10] = {"police", "sheriff", "ems", "fire", "fbi", "army", "gov", "justice", "news", "meca", "taxi"};
new const g_FrpName[FRP_DEFS][40] =
{
	"Crestwood Police Department", "Crestwood County Sheriff", "Crestwood Medical Services", "Crestwood Fire Department",
	"Federal Bureau of Investigation", "Garde nationale de Crestwood", "Gouvernement de Crestwood", "Departement de la Justice",
	"Crestwood News", "Crestwood Auto Service", "Crestwood Cab Company"
};
new const g_FrpShort[FRP_DEFS][8] = {"CPD", "CCSO", "EMS", "CFD", "FBI", "GN", "GOUV", "DOJ", "CWN", "CAS", "CAB"};
new const g_FrpType[FRP_DEFS] =
{
	FACTION_POLICE, FACTION_POLICE, FACTION_MEDIC, FACTION_FIRE, FACTION_FEDERAL, FACTION_ARMY,
	FACTION_GOVERNMENT, FACTION_JUSTICE, FACTION_NEWS, FACTION_MECHANIC, FACTION_TAXIDRIVER
};
new const g_FrpColor[FRP_DEFS] =
{
	0x3B82FFFF, 0xB8874AFF, 0xFF5A6EFF, 0xFF7A1AFF, 0x7C4DFFFF, 0x6B8E3AFF,
	0x9FB4C8FF, 0xA78BFAFF, 0xFFB020FF, 0x9AA5B1FF, 0xFFD000FF
};
new const g_FrpRanks[FRP_DEFS][180] =
{
	"Cadet|Officier|Officier II|Sergent|Lieutenant|Capitaine|Commandant|Chef de la police",
	"Deputy stagiaire|Deputy|Deputy senior|Sergent|Lieutenant|Capitaine|Undersheriff|Sheriff",
	"Stagiaire|Ambulancier|Ambulancier confirme|Infirmier|Medecin|Chirurgien|Chef de service|Directeur",
	"Recrue|Sapeur|Caporal|Sergent|Lieutenant|Capitaine|Commandant|Chef des pompiers",
	"Agent stagiaire|Agent special|Agent special senior|Superviseur|Agent responsable|Directeur adjoint|Directeur",
	"Soldat|Caporal|Sergent|Adjudant|Lieutenant|Capitaine|Commandant|Colonel|General",
	"Agent administratif|Secretaire|Garde du corps|Conseiller|Chef de cabinet|Adjoint au maire|Maire",
	"Greffier|Avocat stagiaire|Avocat|Procureur adjoint|Procureur|Juge|President du tribunal",
	"Stagiaire|Reporter|Cameraman|Journaliste|Redacteur|Redacteur en chef|Directeur",
	"Apprenti|Mecanicien|Depanneur|Mecanicien confirme|Chef atelier|Gerant",
	"Chauffeur stagiaire|Chauffeur|Chauffeur VIP|Chef equipe|Gerant|Patron"
};
new const g_FrpPays[FRP_DEFS][64] =
{
	"1800|2200|2600|3100|3700|4400|5200|6500",
	"1800|2200|2600|3100|3700|4400|5200|6500",
	"1700|2100|2500|3000|3700|4500|5300|6500",
	"1700|2100|2500|3000|3600|4300|5100|6200",
	"2400|3000|3600|4300|5100|6000|7500",
	"1800|2100|2500|2900|3500|4100|4800|5600|7000",
	"1800|2200|2700|3300|4200|5200|7500",
	"1800|2300|3000|3600|4400|5500|7000",
	"1400|1800|2100|2500|3000|3700|4800",
	"1300|1700|2000|2400|3000|4000",
	"1200|1500|1900|2300|2900|3800"
};
new const g_FrpSkins[FRP_DEFS][64] =
{
	"280|281|282|283|288|284|265|266|267|246",
	"283|288|282|310|311|246",
	"274|275|276|308",
	"277|278|279",
	"286|285|163|164|165|166|141",
	"287|191|179|61",
	"147|150|163|164|165|166|17",
	"147|57|59|141|150|240",
	"187|188|141|17",
	"50|268|8|42",
	"255|262|263|44"
};

// n-ieme morceau d'un texte separe par '|' ; renvoie 0 s'il n'existe pas
stock Frp_Part(const src[], n, out[], size)
{
	new cur = 0, j = 0, i = 0;
	out[0] = 0;
	for (; src[i] != 0; i++)
	{
		if (src[i] == '|') { if (cur == n) break; cur++; j = 0; continue; }
		if (cur == n && j < size - 1) { out[j++] = src[i]; out[j] = 0; }
	}
	return (cur == n && j > 0) ? 1 : 0;
}

stock Frp_PartCount(const src[])
{
	if (!src[0]) return 0;
	new n = 1;
	for (new i = 0; src[i] != 0; i++) if (src[i] == '|') n++;
	return n;
}

stock Frp_Seeded(k)
{
	new q[96], ok = 0;
	format(q, sizeof q, "SELECT k FROM faction_seed WHERE k = '%s'", g_FrpKey[k]);
	new DBResult:r = db_query(g_FacDB, q);
	if (r != DBResult:0)
	{
		ok = db_num_rows(r) > 0;
		db_free_result(r);
	}
	return ok;
}

stock Frp_MarkSeeded(k)
{
	new q[96];
	format(q, sizeof q, "INSERT OR REPLACE INTO faction_seed (k) VALUES ('%s')", g_FrpKey[k]);
	db_free_result(db_query(g_FacDB, q));
	return 1;
}

// faction deja presente pour la definition k (meme nom, ou meme type si ce type n'a qu'une definition)
stock Frp_FindExisting(k)
{
	for (new f = 1; f < MAX_FACTIONS; f++) if (g_FxExists[f] && !strcmp(g_FxName[f], g_FrpName[k], true)) return f;
	for (new j = 0; j < k; j++) if (g_FrpType[j] == g_FrpType[k]) return 0;   // 2e definition du meme type (Sheriff) : par nom seulement
	for (new f = 1; f < MAX_FACTIONS; f++) if (g_FxExists[f] && g_FxType[f] == g_FrpType[k]) return f;
	return 0;
}

stock Frp_FreeSlot()
{
	for (new f = 1; f < MAX_FACTIONS; f++) if (!g_FxExists[f]) return f;
	return 0;
}

// full = 1 : faction neuve (tout est ecrit) ; 0 : faction reprise (on ne complete que le vide)
stock Frp_Fill(fac, k, full)
{
	new part[40], cnt = Frp_PartCount(g_FrpRanks[k]);
	if (cnt > MAX_FACTION_RANKS) cnt = MAX_FACTION_RANKS;
	if (full || !strcmp(g_FxShort[fac], "None", true) || !g_FxShort[fac][0]) format(g_FxShort[fac], 24, "%s", g_FrpShort[k]);
	if (full || g_FxColor[fac] == 0xCCCCCCFF) g_FxColor[fac] = g_FrpColor[k];
	if (full) g_FxRankCount[fac] = cnt;
	new pays = 0;
	for (new r = 1; r <= g_FxRankCount[fac]; r++) if (g_FxPay[fac][r] > 0) pays = 1;
	for (new r = 1; r <= g_FxRankCount[fac] && r <= cnt; r++)
	{
		new def[16];
		format(def, sizeof def, "Grade %d", r);
		if ((full || !strcmp(g_FxRankName[fac][r], def, true) || !g_FxRankName[fac][r][0]) && Frp_Part(g_FrpRanks[k], r - 1, part, sizeof part)) format(g_FxRankName[fac][r], 32, "%s", part);
		if ((full || !pays) && Frp_Part(g_FrpPays[k], r - 1, part, sizeof part)) g_FxPay[fac][r] = strval(part);
	}
	// grades au-dela de la liste (faction reprise avec plus de grades) : salaire du dernier grade connu
	if (!full && !pays)
		for (new r = cnt + 1; r <= g_FxRankCount[fac]; r++) g_FxPay[fac][r] = g_FxPay[fac][cnt];
	new hasSkin = 0;
	for (new s = 0; s < MAX_FACTION_SKINS; s++) if (g_FxSkin[fac][s]) hasSkin = 1;
	if (full || !hasSkin)
		for (new s = 0; s < MAX_FACTION_SKINS; s++) g_FxSkin[fac][s] = Frp_Part(g_FrpSkins[k], s, part, sizeof part) ? strval(part) : 0;
	Fac_SaveFaction(fac);
	for (new r = 1; r <= g_FxRankCount[fac]; r++) Fac_SaveRank(fac, r);
	for (new s = 0; s < MAX_FACTION_SKINS; s++) Fac_SaveSkin(fac, s);
	return 1;
}

// force = 1 : recree aussi les factions par defaut supprimees
stock Frp_SeedAll(force)
{
	if (g_FacDB == DB:0) return 0;
	db_free_result(db_query(g_FacDB, "CREATE TABLE IF NOT EXISTS faction_seed (k TEXT PRIMARY KEY)"));
	new made = 0;
	for (new k = 0; k < FRP_DEFS; k++)
	{
		if (!force && Frp_Seeded(k)) continue;
		new fac = Frp_FindExisting(k);
		if (!fac)
		{
			fac = Frp_FreeSlot();
			if (!fac) { print("[FACTIONS] plus de place pour les factions par defaut (MAX_FACTIONS)."); break; }
			Fac_Setup(fac, g_FrpName[k], g_FrpType[k]);
			Frp_Fill(fac, k, 1);
			printf("[FACTIONS] creee : #%d %s (%s)", fac, g_FrpName[k], g_FrpShort[k]);
			made++;
		}
		else Frp_Fill(fac, k, 0);
		Frp_MarkSeeded(k);
	}
	return made;
}

// ------------------------------------------------------------
//  utilitaires
// ------------------------------------------------------------
stock Frp_Online(type)
{
	new n = 0;
	for (new i = 0, m = GetMaxPlayers(); i < m; i++) if (IsPlayerConnected(i) && g_AuthLogged[i] && Fac_Type(i) == type) n++;
	return n;
}

stock Frp_NotifyType(type, const kind[], const title[], const text[], gps, Float:x, Float:y)
{
	for (new i = 0, m = GetMaxPlayers(); i < m; i++)
	{
		if (!IsPlayerConnected(i) || !g_AuthLogged[i] || Fac_Type(i) != type) continue;
		Cw_Notify(i, kind, title, text, 8000);
		if (gps) Gps_Set(i, x, y);
	}
	return 1;
}

stock Frp_SendType(type, color, const text[])
{
	for (new i = 0, m = GetMaxPlayers(); i < m; i++) if (IsPlayerConnected(i) && g_AuthLogged[i] && Fac_Type(i) == type) SendClientMessage(i, color, text);
	return 1;
}

stock Frp_PayBank(p, amount, const descr[])
{
	if (amount <= 0 || !IsPlayerConnected(p)) return 0;
	if (!g_BankLoaded[p]) { GivePlayerMoney(p, amount); return 1; }
	if (g_BankBalance[p] > BANK_MAX_MONEY - amount) return 0;
	g_BankBalance[p] += amount;
	Bank_AddHistory(p, descr, amount);
	Bank_SavePlayer(p);
	return 1;
}

// grade du joueur >= (nombre de grades - back) : chefs de la faction
stock Frp_IsTop(playerid, back)
{
	new f = Fac_Get(playerid);
	if (!f) return 0;
	new need = Fac_RankCount(f) - back;
	if (need < 1) need = 1;
	return g_FacRank[playerid] >= need;
}

stock Frp_Staff(playerid) return Admin_Level(playerid) >= FACTION_LVL_MANAGE;

// mot suivant de la commande
stock Frp_Tok(const s[], &idx, out[], size)
{
	while (s[idx] == ' ') idx++;
	new j = 0;
	while (s[idx] != 0 && s[idx] != ' ' && j < size - 1) out[j++] = s[idx++];
	out[j] = 0;
	return j;
}

stock Frp_Rest(const s[], idx, out[], size)
{
	while (s[idx] == ' ') idx++;
	format(out, size, "%s", s[idx]);
	return strlen(out);
}

// nom sur pour SQL : pas d'apostrophe, de guillemet ni d'espace (sinon vide)
stock Frp_SafeName(const in[], out[], size)
{
	new j = 0;
	for (new i = 0; in[i] != 0 && j < size - 1; i++)
	{
		new c = in[i];
		if (c == 39 || c == '"' || c == '\\' || c <= ' ') { out[0] = 0; return 0; }
		out[j++] = c;
	}
	out[j] = 0;
	return j;
}

// id / morceau de pseudo d'un joueur connecte, sinon Prenom_Nom tel quel ; renvoie l'id (ou INVALID_PLAYER_ID)
stock Frp_Target(const tok[], name[], size)
{
	name[0] = 0;
	new t = Fx_Find(tok);
	if (t != INVALID_PLAYER_ID) { GetPlayerName(t, name, size); return t; }
	if (strfind(tok, "_") != -1) Frp_SafeName(tok, name, size);
	return INVALID_PLAYER_ID;
}

stock Frp_Spaces(const in[], out[], size)
{
	format(out, size, "%s", in);
	for (new i = 0; out[i]; i++) if (out[i] == '_') out[i] = ' ';
	return 1;
}

stock Float:Frp_Dist2(Float:ax, Float:ay, Float:bx, Float:by)
{
	return floatsqroot((ax - bx) * (ax - bx) + (ay - by) * (ay - by));
}

// ============================================================
//  2) POMPIERS
// ============================================================
#define FIRE_MAX_INC          3
#define FIRE_MAX_FLAMES       8
#define FIRE_FLAME_HP         100
#define FIRE_SPREAD_S         25
#define FIRE_BURNOUT_S        900
#define FIRE_RAND_MIN         720
#define FIRE_RAND_MAX         1500
#define FIRE_PAY_BASE         2500
#define FIRE_PAY_FLAME        350
#define FIRE_VICTIM_PAY       1500
#define FIRE_MAX_SPOTS        60
#define FIRE_VICTIMS          2

new g_FireOn[FIRE_MAX_INC];
new Float:g_FireX[FIRE_MAX_INC], Float:g_FireY[FIRE_MAX_INC], Float:g_FireZ[FIRE_MAX_INC];
new g_FireStart[FIRE_MAX_INC], g_FireSpread[FIRE_MAX_INC], g_FireSpawned[FIRE_MAX_INC];
new g_FirePlace[FIRE_MAX_INC][40];
new g_FlameObj[FIRE_MAX_INC][FIRE_MAX_FLAMES];
new g_FlameHP[FIRE_MAX_INC][FIRE_MAX_FLAMES];
new Float:g_FlameX[FIRE_MAX_INC][FIRE_MAX_FLAMES], Float:g_FlameY[FIRE_MAX_INC][FIRE_MAX_FLAMES], Float:g_FlameZ[FIRE_MAX_INC][FIRE_MAX_FLAMES];
new Text3D:g_FireLabel[FIRE_MAX_INC] = {Text3D:INVALID_3DTEXT_ID, ...};
new g_FireHelp[FIRE_MAX_INC][MAX_PLAYERS];
new g_VicOn[FIRE_MAX_INC][FIRE_VICTIMS], g_VicActor[FIRE_MAX_INC][FIRE_VICTIMS], g_VicBy[FIRE_MAX_INC][FIRE_VICTIMS];
new Float:g_VicX[FIRE_MAX_INC][FIRE_VICTIMS], Float:g_VicY[FIRE_MAX_INC][FIRE_VICTIMS], Float:g_VicZ[FIRE_MAX_INC][FIRE_VICTIMS];
new Text3D:g_VicLabel[FIRE_MAX_INC][FIRE_VICTIMS];
new g_FireNext;
new g_FrpTicks;

// points d'incendie : Los Santos par defaut + points ajoutes par le staff (/feupoint)
#define FIRE_DEF_SPOTS        18
new const g_FireDefName[FIRE_DEF_SPOTS][32] =
{
	"Burger Shot Marina", "Ten Green Bottles", "Cluckin Bell Willowfield", "Alhambra Idlewood", "Station Idlewood",
	"Unity Station", "Jefferson Motel", "Market Station", "Atrium Commerce", "Maison Grove Street", "Entrepot Ocean Docks",
	"Boutiques Rodeo", "Verona Mall", "Gym de Ganton", "Entrepot Willowfield", "Ammu-Nation Market", "El Corona", "Glen Park"
};
new const Float:g_FireDefPos[FIRE_DEF_SPOTS][3] =
{
	{810.6, -1616.0, 13.5}, {2310.0, -1643.5, 13.8}, {2397.8, -1898.6, 13.5}, {1836.7, -1682.4, 13.3}, {1941.6, -1772.9, 13.6},
	{1742.8, -1860.8, 13.6}, {2225.0, -1159.3, 25.8}, {819.6, -1345.0, 13.5}, {1727.0, -1637.0, 20.2}, {2495.3, -1687.5, 13.5},
	{2771.0, -2417.0, 13.6}, {461.7, -1500.8, 31.0}, {1128.7, -1452.4, 15.8}, {2229.9, -1721.6, 13.6}, {2525.0, -2120.0, 13.5},
	{1368.3, -1279.7, 13.5}, {1810.0, -1920.0, 13.5}, {1970.0, -1200.0, 25.6}
};
new g_SpotCount;
new g_SpotId[FIRE_MAX_SPOTS];
new g_SpotName[FIRE_MAX_SPOTS][32];
new Float:g_SpotPos[FIRE_MAX_SPOTS][3];

new DB:g_FrpDB;

stock Frp_Exec(const q[])
{
	if (g_FrpDB != DB:0) db_free_result(db_query(g_FrpDB, q));
	return 1;
}

stock Fire_LoadSpots()
{
	g_SpotCount = 0;
	if (g_FrpDB == DB:0) return 0;
	new DBResult:r = db_query(g_FrpDB, "SELECT id, name, x, y, z FROM fire_spots ORDER BY id");
	if (r == DBResult:0) return 0;
	new f[40];
	for (new n = db_num_rows(r), i = 0; i < n && g_SpotCount < FIRE_MAX_SPOTS; i++)
	{
		db_get_field(r, 0, f, sizeof f); g_SpotId[g_SpotCount] = strval(f);
		db_get_field(r, 1, g_SpotName[g_SpotCount], 32);
		db_get_field(r, 2, f, sizeof f); g_SpotPos[g_SpotCount][0] = floatstr(f);
		db_get_field(r, 3, f, sizeof f); g_SpotPos[g_SpotCount][1] = floatstr(f);
		db_get_field(r, 4, f, sizeof f); g_SpotPos[g_SpotCount][2] = floatstr(f);
		g_SpotCount++;
		db_next_row(r);
	}
	db_free_result(r);
	return g_SpotCount;
}

stock Fire_Active()
{
	new n = 0;
	for (new i = 0; i < FIRE_MAX_INC; i++) if (g_FireOn[i]) n++;
	return n;
}

stock Fire_Alive(i)
{
	new n = 0;
	for (new f = 0; f < FIRE_MAX_FLAMES; f++) if (g_FlameHP[i][f] > 0) n++;
	return n;
}

stock Fire_Label(i)
{
	new t[128];
	format(t, sizeof t, "{FF7A1A}INCENDIE\n{FFFFFF}%s\nFlammes : %d", g_FirePlace[i], Fire_Alive(i));
	if (g_FireLabel[i] == Text3D:INVALID_3DTEXT_ID) g_FireLabel[i] = Create3DTextLabel(t, 0xFFFFFFFF, g_FireX[i], g_FireY[i], g_FireZ[i] + 2.5, 60.0, 0, 0);
	else Update3DTextLabelText(g_FireLabel[i], 0xFFFFFFFF, t);
	return 1;
}

stock Fire_AddFlame(i, Float:x, Float:y, Float:z)
{
	for (new f = 0; f < FIRE_MAX_FLAMES; f++)
	{
		if (g_FlameHP[i][f] > 0) continue;
		g_FlameX[i][f] = x; g_FlameY[i][f] = y; g_FlameZ[i][f] = z;
		g_FlameHP[i][f] = FIRE_FLAME_HP;
		g_FlameObj[i][f] = CreateObject((random(3) == 0) ? 18688 : 18691, x, y, z - 1.6, 0.0, 0.0, 0.0, 220.0);
		if (g_FlameObj[i][f] == INVALID_OBJECT_ID) g_FlameObj[i][f] = 0;
		g_FireSpawned[i]++;
		return f;
	}
	return -1;
}

stock Fire_KillFlame(i, f)
{
	if (g_FlameHP[i][f] <= 0 && !g_FlameObj[i][f]) return 0;
	g_FlameHP[i][f] = 0;
	if (g_FlameObj[i][f]) { DestroyObject(g_FlameObj[i][f]); g_FlameObj[i][f] = 0; }
	return 1;
}

stock Fire_Victims(i, n)
{
	static const vs[] = {29, 78, 79, 134, 135, 137, 15, 20};
	for (new v = 0; v < FIRE_VICTIMS; v++)
	{
		g_VicOn[i][v] = 0;
		g_VicBy[i][v] = INVALID_PLAYER_ID;
		if (v >= n) continue;
		new Float:a = float(random(360)), Float:d = 5.0 + float(random(30)) / 10.0;
		g_VicX[i][v] = g_FireX[i] + d * floatsin(a, degrees);
		g_VicY[i][v] = g_FireY[i] + d * floatcos(a, degrees);
		g_VicZ[i][v] = g_FireZ[i];
		g_VicActor[i][v] = CreateActor(vs[random(sizeof vs)], g_VicX[i][v], g_VicY[i][v], g_VicZ[i][v], float(random(360)));
		if (g_VicActor[i][v] != INVALID_ACTOR_ID) ApplyActorAnimation(g_VicActor[i][v], "PED", "KO_shot_stom", 4.1, 0, 0, 0, 1, 0);
		g_VicLabel[i][v] = Create3DTextLabel("{FF6347}Victime\n{FFFFFF}/secourir", 0xFFFFFFFF, g_VicX[i][v], g_VicY[i][v], g_VicZ[i][v] + 0.4, 12.0, 0, 1);
		g_VicOn[i][v] = 1;
	}
	return 1;
}

stock Fire_KillVictim(i, v)
{
	if (!g_VicOn[i][v]) return 0;
	g_VicOn[i][v] = 0;
	if (g_VicActor[i][v] != INVALID_ACTOR_ID) DestroyActor(g_VicActor[i][v]);
	g_VicActor[i][v] = INVALID_ACTOR_ID;
	Delete3DTextLabel(g_VicLabel[i][v]);
	return 1;
}

// size 1 a 3 (flammes 2 / 4 / 6), victims : nombre de victimes
stock Fire_Start(Float:x, Float:y, Float:z, const place[], size, victims)
{
	new i = -1;
	for (new k = 0; k < FIRE_MAX_INC; k++) if (!g_FireOn[k]) { i = k; break; }
	if (i == -1) return -1;
	if (size < 1) size = 1;
	if (size > 3) size = 3;
	g_FireOn[i] = 1;
	g_FireX[i] = x; g_FireY[i] = y; g_FireZ[i] = z;
	g_FireStart[i] = gettime(); g_FireSpread[i] = gettime(); g_FireSpawned[i] = 0;
	format(g_FirePlace[i], 40, "%s", place);
	for (new f = 0; f < FIRE_MAX_FLAMES; f++) { g_FlameHP[i][f] = 0; g_FlameObj[i][f] = 0; }
	for (new p = 0; p < MAX_PLAYERS; p++) g_FireHelp[i][p] = 0;
	Fire_AddFlame(i, x, y, z);
	for (new n = 1; n < size * 2; n++)
	{
		new Float:a = float(random(360)), Float:d = 1.5 + float(random(30)) / 10.0;
		Fire_AddFlame(i, x + d * floatsin(a, degrees), y + d * floatcos(a, degrees), z);
	}
	Fire_Victims(i, victims);
	Fire_Label(i);
	new m[180];
	format(m, sizeof m, "Incendie signale : ~r~%s~w~. Rends-toi sur place (GPS) : extincteur ou lance du camion.", place);
	Frp_NotifyType(FACTION_FIRE, "warning", "Pompiers - 18", m, 1, x, y);
	if (victims > 0)
	{
		format(m, sizeof m, "Incendie a %s : %d victime(s) a prendre en charge (GPS).", place, victims);
		Frp_NotifyType(FACTION_MEDIC, "warning", "EMS", m, 1, x, y);
	}
	format(m, sizeof m, "[CENTRALE 18] Incendie en cours : %s. Securisez le perimetre.", place);
	Law_SendAll(FRP_COL_FIRE, m);
	return i;
}

stock Fire_End(i, success)
{
	if (!g_FireOn[i]) return 0;
	for (new f = 0; f < FIRE_MAX_FLAMES; f++) Fire_KillFlame(i, f);
	for (new v = 0; v < FIRE_VICTIMS; v++) Fire_KillVictim(i, v);
	if (g_FireLabel[i] != Text3D:INVALID_3DTEXT_ID) { Delete3DTextLabel(g_FireLabel[i]); g_FireLabel[i] = Text3D:INVALID_3DTEXT_ID; }
	g_FireOn[i] = 0;
	new m[160], a[16];
	if (!success)
	{
		format(m, sizeof m, "Le feu a %s s'est eteint tout seul : le batiment est detruit.", g_FirePlace[i]);
		Frp_NotifyType(FACTION_FIRE, "denied", "Pompiers", m, 0, 0.0, 0.0);
		return 1;
	}
	new total = 0;
	for (new p = 0, mx = GetMaxPlayers(); p < mx; p++) if (IsPlayerConnected(p)) total += g_FireHelp[i][p];
	if (total < 1) total = 1;
	for (new p = 0, mx = GetMaxPlayers(); p < mx; p++)
	{
		if (!IsPlayerConnected(p) || g_FireHelp[i][p] <= 0) continue;
		if (success) Cw46_OnFire(p);   // [v46] stats
		new pay;
		if (Fac_Type(p) == FACTION_FIRE) pay = FIRE_PAY_BASE + (FIRE_PAY_FLAME * g_FireSpawned[i] * g_FireHelp[i][p]) / total;
		else pay = 300;
		Frp_PayBank(p, pay, "Intervention incendie");
		Cw_Fmt(pay, a, sizeof a);
		format(m, sizeof m, "Feu maitrise a %s. Prime : ~g~%s $~w~ (sur ton compte).", g_FirePlace[i], a);
		Cw_Notify(p, "success", "Pompiers", m, 7000);
	}
	format(m, sizeof m, "[CENTRALE 18] Incendie maitrise : %s.", g_FirePlace[i]);
	Frp_SendType(FACTION_FIRE, FRP_COL_FIRE, m);
	return 1;
}

stock Fire_StartRandom()
{
	new total = FIRE_DEF_SPOTS + g_SpotCount;
	for (new tries = 0; tries < 8; tries++)
	{
		new s = random(total), Float:x, Float:y, Float:z, nm[32];
		if (s < FIRE_DEF_SPOTS) { x = g_FireDefPos[s][0]; y = g_FireDefPos[s][1]; z = g_FireDefPos[s][2]; format(nm, sizeof nm, "%s", g_FireDefName[s]); }
		else { s -= FIRE_DEF_SPOTS; x = g_SpotPos[s][0]; y = g_SpotPos[s][1]; z = g_SpotPos[s][2]; format(nm, sizeof nm, "%s", g_SpotName[s]); }
		new busy = 0;
		for (new i = 0; i < FIRE_MAX_INC; i++) if (g_FireOn[i] && Frp_Dist2(x, y, g_FireX[i], g_FireY[i]) < 60.0) busy = 1;
		if (busy) continue;
		return Fire_Start(x, y, z, nm, 1 + random(3), random(FIRE_VICTIMS + 1));
	}
	return -1;
}

// extinction : appele toutes les 500 ms
stock Fire_Extinguish()
{
	if (!Fire_Active()) return 0;
	for (new p = 0, mx = GetMaxPlayers(); p < mx; p++)
	{
		if (!IsPlayerConnected(p) || !g_AuthLogged[p]) continue;
		new keys, ud, lr, st = GetPlayerState(p), Float:range, power;
		GetPlayerKeys(p, keys, ud, lr);
		if (st == PLAYER_STATE_ONFOOT)
		{
			if (GetPlayerWeapon(p) != 42 || !(keys & KEY_FIRE)) continue;
			range = 6.5; power = 9;
		}
		else if (st == PLAYER_STATE_DRIVER)
		{
			if (GetVehicleModel(GetPlayerVehicleID(p)) != 407 || !(keys & (KEY_FIRE | KEY_ACTION))) continue;
			range = 28.0; power = 18;
		}
		else continue;
		new Float:px, Float:py, Float:pz, Float:fx, Float:fy, Float:fz;
		GetPlayerPos(p, px, py, pz);
		GetPlayerCameraFrontVector(p, fx, fy, fz);
		new Float:fl = floatsqroot(fx * fx + fy * fy);
		if (fl < 0.01) fl = 0.01;
		for (new i = 0; i < FIRE_MAX_INC; i++)
		{
			if (!g_FireOn[i] || Frp_Dist2(px, py, g_FireX[i], g_FireY[i]) > range + 15.0) continue;
			new hit = 0;
			for (new f = 0; f < FIRE_MAX_FLAMES; f++)
			{
				if (g_FlameHP[i][f] <= 0) continue;
				new Float:dx = g_FlameX[i][f] - px, Float:dy = g_FlameY[i][f] - py, Float:d = floatsqroot(dx * dx + dy * dy);
				if (d > range || floatabs(g_FlameZ[i][f] - pz) > 8.0) continue;
				if (d > 2.5 && (dx * fx + dy * fy) / (d * fl) < 0.55) continue;   // il faut viser la flamme
				g_FlameHP[i][f] -= power;
				g_FireHelp[i][p]++;
				hit = 1;
				if (g_FlameHP[i][f] <= 0) Fire_KillFlame(i, f);
			}
			if (hit)
			{
				if (!Fire_Alive(i)) Fire_End(i, 1);
				else Fire_Label(i);
			}
		}
	}
	return 1;
}

// brulures, propagation, extinction naturelle : toutes les secondes
stock Fire_Second()
{
	new now = gettime();
	for (new i = 0; i < FIRE_MAX_INC; i++)
	{
		if (!g_FireOn[i]) continue;
		if (now - g_FireStart[i] > FIRE_BURNOUT_S) { Fire_End(i, 0); continue; }
		if (now - g_FireSpread[i] >= FIRE_SPREAD_S)
		{
			g_FireSpread[i] = now;
			new alive = Fire_Alive(i);
			if (alive > 0 && alive < FIRE_MAX_FLAMES)
			{
				new pick = random(alive), c = 0;
				for (new f = 0; f < FIRE_MAX_FLAMES; f++)
				{
					if (g_FlameHP[i][f] <= 0) continue;
					if (c++ != pick) continue;
					new Float:a = float(random(360)), Float:d = 2.0 + float(random(20)) / 10.0;
					Fire_AddFlame(i, g_FlameX[i][f] + d * floatsin(a, degrees), g_FlameY[i][f] + d * floatcos(a, degrees), g_FlameZ[i][f]);
					break;
				}
				Fire_Label(i);
			}
		}
	}
	// brulures
	for (new p = 0, mx = GetMaxPlayers(); p < mx; p++)
	{
		if (!IsPlayerConnected(p) || !g_AuthLogged[p] || GetPlayerState(p) != PLAYER_STATE_ONFOOT) continue;
		new Float:px, Float:py, Float:pz, burn = 0;
		GetPlayerPos(p, px, py, pz);
		for (new i = 0; i < FIRE_MAX_INC && !burn; i++)
		{
			if (!g_FireOn[i] || Frp_Dist2(px, py, g_FireX[i], g_FireY[i]) > 30.0) continue;
			for (new f = 0; f < FIRE_MAX_FLAMES; f++)
				if (g_FlameHP[i][f] > 0 && Frp_Dist2(px, py, g_FlameX[i][f], g_FlameY[i][f]) < 2.2 && floatabs(pz - g_FlameZ[i][f]) < 3.0) { burn = 1; break; }
		}
		if (!burn) continue;
		new Float:hp;
		GetPlayerHealth(p, hp);
		new Float:dmg = (Fac_Type(p) == FACTION_FIRE && g_RpXUni[p]) ? 1.5 : 5.0;
		SetPlayerHealth(p, hp - dmg);
	}
	// incendie aleatoire si des pompiers sont en ligne
	if (now >= g_FireNext)
	{
		g_FireNext = now + FIRE_RAND_MIN + random(FIRE_RAND_MAX - FIRE_RAND_MIN);
		if (Fire_Active() < FIRE_MAX_INC && Frp_Online(FACTION_FIRE) > 0) Fire_StartRandom();
	}
	return 1;
}

forward Fire_RescueDone(playerid, i, v);
public Fire_RescueDone(playerid, i, v)
{
	if (i < 0 || i >= FIRE_MAX_INC || v < 0 || v >= FIRE_VICTIMS) return 0;
	if (!g_VicOn[i][v] || g_VicBy[i][v] != playerid) return 0;
	if (!IsPlayerConnected(playerid) || !IsPlayerInRangeOfPoint(playerid, 3.0, g_VicX[i][v], g_VicY[i][v], g_VicZ[i][v])) { g_VicBy[i][v] = INVALID_PLAYER_ID; return 0; }
	ClearAnimations(playerid);
	Fire_KillVictim(i, v);
	Frp_PayBank(playerid, FIRE_VICTIM_PAY, "Victime secourue");
	Cw_Notify(playerid, "success", "Secours", "Victime stabilisee et evacuee. Prime : ~g~1 500 $~w~.", 6000);
	return 1;
}

stock Fire_CmdRescue(playerid)
{
	new t = Fac_Type(playerid);
	if (t != FACTION_FIRE && t != FACTION_MEDIC) return Cw_Notify(playerid, "denied", "Secours", "Reserve aux pompiers et a l'EMS.", 3000), 1;
	if (GetPlayerState(playerid) != PLAYER_STATE_ONFOOT) return Cw_Notify(playerid, "denied", "Secours", "Descends du vehicule.", 3000), 1;
	for (new i = 0; i < FIRE_MAX_INC; i++)
	{
		for (new v = 0; v < FIRE_VICTIMS; v++)
		{
			if (!g_VicOn[i][v] || !IsPlayerInRangeOfPoint(playerid, 2.5, g_VicX[i][v], g_VicY[i][v], g_VicZ[i][v])) continue;
			if (g_VicBy[i][v] != INVALID_PLAYER_ID && g_VicBy[i][v] != playerid && IsPlayerConnected(g_VicBy[i][v])) return Cw_Notify(playerid, "denied", "Secours", "Un collegue s'occupe deja de cette victime.", 3000), 1;
			g_VicBy[i][v] = playerid;
			ApplyAnimation(playerid, "MEDIC", "CPR", 4.1, 1, 0, 0, 0, 0, 1);
			SetTimerEx("Fire_RescueDone", 6000, false, "iii", playerid, i, v);
			Cw_Notify(playerid, "info", "Secours", "Massage cardiaque en cours... reste a cote de la victime (6 s).", 6000);
			return 1;
		}
	}
	return Cw_Notify(playerid, "denied", "Secours", "Aucune victime a cote de toi.", 3000), 1;
}

stock Fire_CmdList(playerid)
{
	if (Fac_Type(playerid) != FACTION_FIRE && Fac_Type(playerid) != FACTION_MEDIC && !Fac_IsLaw(playerid) && !Frp_Staff(playerid))
		return Cw_Notify(playerid, "denied", "Pompiers", "Reserve aux services d'urgence.", 3000), 1;
	new Float:px, Float:py, Float:pz, best = -1, Float:bd = 999999.0, m[144], n = 0;
	GetPlayerPos(playerid, px, py, pz);
	for (new i = 0; i < FIRE_MAX_INC; i++)
	{
		if (!g_FireOn[i]) continue;
		new Float:d = Frp_Dist2(px, py, g_FireX[i], g_FireY[i]);
		format(m, sizeof m, "[FEU %d] %s - %d flamme(s) - %d m - depuis %d min", i + 1, g_FirePlace[i], Fire_Alive(i), floatround(d), (gettime() - g_FireStart[i]) / 60);
		SendClientMessage(playerid, FRP_COL_FIRE, m);
		if (d < bd) { bd = d; best = i; }
		n++;
	}
	if (!n) return SendClientMessage(playerid, FRP_COL_INFO, "Aucun incendie en cours."), 1;
	Gps_Set(playerid, g_FireX[best], g_FireY[best]);
	return SendClientMessage(playerid, FRP_COL_INFO, "GPS place sur l'incendie le plus proche."), 1;
}

stock Frp_OnVehicleDeath(vehicleid)
{
	if (Frp_Online(FACTION_FIRE) < 1 || Fire_Active() >= FIRE_MAX_INC || random(2)) return 0;
	new Float:x, Float:y, Float:z;
	GetVehiclePos(vehicleid, x, y, z);
	if (x == 0.0 && y == 0.0) return 0;
	for (new i = 0; i < FIRE_MAX_INC; i++) if (g_FireOn[i] && Frp_Dist2(x, y, g_FireX[i], g_FireY[i]) < 40.0) return 0;
	return Fire_Start(x, y, z + 0.4, "Vehicule en feu", 1, 0);
}

// ============================================================
//  3) JUSTICE
// ============================================================
#define JUS_TRIAL_RANGE       18.0
#define JUS_JUDGE_PAY         1500
new g_JusTrial[MAX_PLAYERS] = {INVALID_PLAYER_ID, ...};   // juge -> accuse
new g_JusBail[MAX_PLAYERS];
new g_JusLawyer[MAX_PLAYERS] = {INVALID_PLAYER_ID, ...};  // client -> avocat
new g_JusOffer[MAX_PLAYERS] = {INVALID_PLAYER_ID, ...};   // client -> avocat qui propose
new g_JusOfferFee[MAX_PLAYERS];
new g_JusChecked[MAX_PLAYERS];

stock Jus_IsJudge(playerid)
{
	if (Frp_Staff(playerid)) return 1;
	return (Fac_Type(playerid) == FACTION_JUSTICE && Frp_IsTop(playerid, 1));
}

stock Jus_IsLawyer(playerid) return (Fac_Type(playerid) == FACTION_JUSTICE && g_FacRank[playerid] >= 2);
stock Jus_CanRead(playerid) return (Fac_IsLaw(playerid) || Fac_Type(playerid) == FACTION_JUSTICE || Frp_Staff(playerid));

stock Jus_Record(const name[], const kind[], const text[], const author[])
{
	new sn[MAX_PLAYER_NAME + 1], tx[100], au[MAX_PLAYER_NAME + 1], q[320];
	if (!Frp_SafeName(name, sn, sizeof sn)) return 0;
	Law_Clean(text, tx, sizeof tx);
	Law_Clean(author, au, sizeof au);
	format(q, sizeof q, "INSERT INTO casier (name, kind, text, author, ts) VALUES ('%s', '%s', '%s', '%s', %d)", sn, kind, tx, au, gettime());
	return Frp_Exec(q);
}

stock Jus_RecordId(target, const kind[], const text[], by)
{
	if (target < 0 || target >= MAX_PLAYERS || !IsPlayerConnected(target)) return 0;
	new n[MAX_PLAYER_NAME + 1], a[MAX_PLAYER_NAME + 1];
	GetPlayerName(target, n, sizeof n);
	if (by != INVALID_PLAYER_ID && IsPlayerConnected(by)) GetPlayerName(by, a, sizeof a); else format(a, sizeof a, "Systeme");
	return Jus_Record(n, kind, text, a);
}

// appele par Pol_Jail (toute incarceration)
stock Jus_OnJail(target, minutes)
{
	new t[48];
	format(t, sizeof t, "Incarceration : %d min", minutes);
	return Jus_RecordId(target, "prison", t, INVALID_PLAYER_ID);
}

stock Jus_ShowRecord(playerid, const name[])
{
	new sn[MAX_PLAYER_NAME + 1], q[160], body[1800], line[180], f[100], k[16], au[32], ts[16];
	if (!Frp_SafeName(name, sn, sizeof sn)) return Cw_Notify(playerid, "denied", "Casier", "Nom invalide (Prenom_Nom).", 3000), 1;
	format(q, sizeof q, "SELECT kind, text, author, ts FROM casier WHERE name = '%s' ORDER BY id DESC LIMIT 14", sn);
	body = "Type\tFait\tPar\n";
	new n = 0;
	if (g_FrpDB != DB:0)
	{
		new DBResult:r = db_query(g_FrpDB, q);
		if (r != DBResult:0)
		{
			for (new rows = db_num_rows(r); n < rows; n++)
			{
				db_get_field(r, 0, k, sizeof k);
				db_get_field(r, 1, f, sizeof f);
				db_get_field(r, 2, au, sizeof au);
				db_get_field(r, 3, ts, sizeof ts);
				new days = (gettime() - strval(ts)) / 86400;
				format(line, sizeof line, "%s\t%s (il y a %d j)\t%s\n", k, f, days, au);
				strcat(body, line, sizeof body);
				db_next_row(r);
			}
			db_free_result(r);
		}
	}
	if (!n) strcat(body, "-\tCasier vierge\t-\n", sizeof body);
	new title[64], sp[MAX_PLAYER_NAME + 1];
	Frp_Spaces(sn, sp, sizeof sp);
	format(title, sizeof title, "Casier judiciaire - %s", sp);
	return ShowPlayerDialog(playerid, FRP_DIALOG_LIST, DIALOG_STYLE_TABLIST_HEADERS, title, body, "Fermer", ""), 1;
}

stock Jus_HasWarrant(const name[], kind)
{
	new sn[MAX_PLAYER_NAME + 1], q[160], id = 0, f[12];
	if (g_FrpDB == DB:0 || !Frp_SafeName(name, sn, sizeof sn)) return 0;
	format(q, sizeof q, "SELECT id FROM mandats WHERE name = '%s' AND kind = %d AND active = 1 LIMIT 1", sn, kind);
	new DBResult:r = db_query(g_FrpDB, q);
	if (r != DBResult:0)
	{
		if (db_num_rows(r) > 0)
		{
			db_get_field(r, 0, f, sizeof f);
			id = strval(f);
		}
		db_free_result(r);
	}
	return id;
}

stock Jus_ApplyWarrant(t, const reason[], const judge[])
{
	if (t == INVALID_PLAYER_ID || !IsPlayerConnected(t)) return 0;
	if (GetPlayerWantedLevel(t) < 4) SetPlayerWantedLevel(t, 4);
	format(g_LawReason[t], 64, "Mandat d'arret : %s", reason);
	Law_Clean(g_LawReason[t], g_LawReason[t], 64);
	format(g_LawBy[t], MAX_PLAYER_NAME + 1, "%s", judge);
	new m[160], tn[MAX_PLAYER_NAME + 1];
	Fx_RPName(t, tn, sizeof tn);
	format(m, sizeof m, "[CENTRALE] %s (%d) est sous MANDAT D'ARRET : %s", tn, t, reason);
	Law_SendAll(FRP_COL_JUS, m);
	return 1;
}

stock Jus_CmdWarrant(playerid, const cmd[], idx)
{
	if (!Jus_IsJudge(playerid)) return Cw_Notify(playerid, "denied", "Justice", "Seul un juge peut signer un mandat.", 3000), 1;
	new kindTok[16], who[32], reason[80], name[MAX_PLAYER_NAME + 1];
	Frp_Tok(cmd, idx, kindTok, sizeof kindTok);
	Frp_Tok(cmd, idx, who, sizeof who);
	Frp_Rest(cmd, idx, reason, sizeof reason);
	new kind = !strcmp(kindTok, "arret", true) ? 1 : (!strcmp(kindTok, "perquisition", true) ? 2 : 0);
	if (!kind || !who[0] || strlen(reason) < 3) return SendClientMessage(playerid, FRP_COL_INFO, "/mandat arret|perquisition <id ou Prenom_Nom> <motif>"), 1;
	new t = Frp_Target(who, name, sizeof name);
	if (!name[0]) return Cw_Notify(playerid, "denied", "Justice", "Personne introuvable (id en ligne ou Prenom_Nom).", 3000), 1;
	new rs[80], jn[MAX_PLAYER_NAME + 1], q[320], m[180], sp[MAX_PLAYER_NAME + 1];
	Law_Clean(reason, rs, sizeof rs);
	GetPlayerName(playerid, jn, sizeof jn);
	format(q, sizeof q, "INSERT INTO mandats (name, kind, reason, judge, ts, active) VALUES ('%s', %d, '%s', '%s', %d, 1)", name, kind, rs, jn, gettime());
	Frp_Exec(q);
	format(m, sizeof m, "Mandat %s : %s", (kind == 1) ? ("d'arret") : ("de perquisition"), rs);
	Jus_Record(name, "mandat", m, jn);
	Frp_Spaces(name, sp, sizeof sp);
	format(m, sizeof m, "[JUSTICE] Mandat %s signe contre %s : %s (/mandats)", (kind == 1) ? ("d'ARRET") : ("de PERQUISITION"), sp, rs);
	Law_SendAll(FRP_COL_JUS, m);
	Frp_SendType(FACTION_JUSTICE, FRP_COL_JUS, m);
	if (kind == 1 && t != INVALID_PLAYER_ID) Jus_ApplyWarrant(t, rs, jn);
	return Cw_Notify(playerid, "success", "Justice", "Mandat signe et transmis aux forces de l'ordre.", 4000), 1;
}

stock Jus_CmdWarrants(playerid)
{
	if (!Jus_CanRead(playerid)) return Cw_Notify(playerid, "denied", "Justice", "Reserve aux forces de l'ordre et a la justice.", 3000), 1;
	new body[1800], line[200], f[80], nm[32], kd[4], jd[32], id[12];
	body = "N\tPersonne\tType\tMotif\n";
	new n = 0;
	if (g_FrpDB != DB:0)
	{
		new DBResult:r = db_query(g_FrpDB, "SELECT id, name, kind, reason, judge FROM mandats WHERE active = 1 ORDER BY id DESC LIMIT 15");
		if (r != DBResult:0)
		{
			for (new rows = db_num_rows(r); n < rows; n++)
			{
				db_get_field(r, 0, id, sizeof id);
				db_get_field(r, 1, nm, sizeof nm);
				db_get_field(r, 2, kd, sizeof kd);
				db_get_field(r, 3, f, sizeof f);
				db_get_field(r, 4, jd, sizeof jd);
				new on = Cw_PlayerByName(nm), sp[32];
				Frp_Spaces(nm, sp, sizeof sp);
				format(line, sizeof line, "#%s\t%s%s\t%s\t%s (juge %s)\n", id, sp, (on != INVALID_PLAYER_ID) ? (" {33CC33}[en ligne]{FFFFFF}") : (""), (strval(kd) == 1) ? ("Arret") : ("Perquisition"), f, jd);
				strcat(body, line, sizeof body);
				db_next_row(r);
			}
			db_free_result(r);
		}
	}
	if (!n) strcat(body, "-\tAucun mandat actif\t-\t-\n", sizeof body);
	return ShowPlayerDialog(playerid, FRP_DIALOG_LIST, DIALOG_STYLE_TABLIST_HEADERS, "Mandats actifs", body, "Fermer", ""), 1;
}

// action = 0 annuler (juge), 1 executer (forces de l'ordre)
stock Jus_CmdWarrantClose(playerid, const cmd[], idx, action)
{
	new tok[12];
	Frp_Tok(cmd, idx, tok, sizeof tok);
	new id = strval(tok);
	if (id <= 0) return SendClientMessage(playerid, FRP_COL_INFO, action ? ("/executermandat <numero> (voir /mandats)") : ("/annulermandat <numero>")), 1;
	if (!action && !Jus_IsJudge(playerid)) return Cw_Notify(playerid, "denied", "Justice", "Seul un juge peut annuler un mandat.", 3000), 1;
	if (action && !Fac_IsLaw(playerid)) return Cw_Notify(playerid, "denied", "Justice", "Reserve aux forces de l'ordre.", 3000), 1;
	new q[160], nm[32], kd[4], rs[80], found = 0;
	format(q, sizeof q, "SELECT name, kind, reason FROM mandats WHERE id = %d AND active = 1", id);
	new DBResult:r = (g_FrpDB != DB:0) ? db_query(g_FrpDB, q) : DBResult:0;
	if (r != DBResult:0)
	{
		if (db_num_rows(r) > 0)
		{
			db_get_field(r, 0, nm, sizeof nm);
			db_get_field(r, 1, kd, sizeof kd);
			db_get_field(r, 2, rs, sizeof rs);
			found = 1;
		}
		db_free_result(r);
	}
	if (!found) return Cw_Notify(playerid, "denied", "Justice", "Mandat introuvable ou deja clos.", 3000), 1;
	new t = Cw_PlayerByName(nm), kind = strval(kd), by[MAX_PLAYER_NAME + 1], m[160], sp[32];
	GetPlayerName(playerid, by, sizeof by);
	if (action && kind == 1)
	{
		if (t == INVALID_PLAYER_ID) return Cw_Notify(playerid, "denied", "Justice", "La personne n'est pas en ville.", 3000), 1;
		new Float:x, Float:y, Float:z;
		GetPlayerPos(t, x, y, z);
		if (!IsPlayerInRangeOfPoint(playerid, 5.0, x, y, z) || !g_PolCuffed[t]) return Cw_Notify(playerid, "denied", "Justice", "Il faut que la personne soit menottee a cote de toi.", 3500), 1;
	}
	format(q, sizeof q, "UPDATE mandats SET active = 0 WHERE id = %d", id);
	Frp_Exec(q);
	Frp_Spaces(nm, sp, sizeof sp);
	if (action)
	{
		Jus_Record(nm, "mandat", (kind == 1) ? ("Mandat d'arret execute") : ("Perquisition effectuee"), by);
		format(m, sizeof m, "[CENTRALE] Mandat #%d execute (%s) par %s.", id, sp, by);
		if (kind == 1 && t != INVALID_PLAYER_ID) SendClientMessage(t, FRP_COL_JUS, "Mandat d'arret execute : tu passeras devant un juge (/proces) ou en cellule.");
	}
	else
	{
		Jus_Record(nm, "mandat", "Mandat annule par le juge", by);
		format(m, sizeof m, "[JUSTICE] Mandat #%d (%s) annule par %s.", id, sp, by);
		if (kind == 1 && t != INVALID_PLAYER_ID && !Jus_HasWarrant(nm, 1)) { SetPlayerWantedLevel(t, 0); g_LawReason[t][0] = 0; }
	}
	Law_SendAll(FRP_COL_JUS, m);
	Frp_SendType(FACTION_JUSTICE, FRP_COL_JUS, m);
	return 1;
}

stock Jus_Near(playerid, t, Float:range)
{
	if (t < 0 || t >= MAX_PLAYERS || !IsPlayerConnected(t) || t == playerid) return 0;
	new Float:x, Float:y, Float:z;
	GetPlayerPos(t, x, y, z);
	return IsPlayerInRangeOfPoint(playerid, range, x, y, z) && GetPlayerVirtualWorld(t) == GetPlayerVirtualWorld(playerid);
}

stock Jus_Around(playerid, Float:range, color, const text[])
{
	new Float:x, Float:y, Float:z;
	GetPlayerPos(playerid, x, y, z);
	for (new i = 0, m = GetMaxPlayers(); i < m; i++)
		if (IsPlayerConnected(i) && GetPlayerVirtualWorld(i) == GetPlayerVirtualWorld(playerid) && IsPlayerInRangeOfPoint(i, range, x, y, z)) SendClientMessage(i, color, text);
	return 1;
}

stock Jus_OpenTrial(playerid, t)
{
	if (!Jus_IsJudge(playerid)) return Cw_Notify(playerid, "denied", "Justice", "Seul un juge preside un proces.", 3000), 1;
	if (!Jus_Near(playerid, t, JUS_TRIAL_RANGE)) return Cw_Notify(playerid, "denied", "Justice", "L'accuse doit etre dans la salle (18 m).", 3000), 1;
	g_JusTrial[playerid] = t;
	new jn[MAX_PLAYER_NAME + 1], tn[MAX_PLAYER_NAME + 1], m[180];
	Fx_RPName(playerid, jn, sizeof jn);
	Fx_RPName(t, tn, sizeof tn);
	format(m, sizeof m, "** Le tribunal est en session. Le juge %s preside l'affaire : Etat de Crestwood contre %s. **", jn, tn);
	Jus_Around(playerid, 30.0, FRP_COL_JUS, m);
	if (g_JusLawyer[t] != INVALID_PLAYER_ID && IsPlayerConnected(g_JusLawyer[t]))
	{
		Fx_RPName(g_JusLawyer[t], jn, sizeof jn);
		format(m, sizeof m, "** Defense assuree par Maitre %s. **", jn);
		Jus_Around(playerid, 30.0, FRP_COL_JUS, m);
	}
	SendClientMessage(playerid, FRP_COL_INFO, "Verdict : /verdict <id> acquitte | amende <montant> | prison <minutes> [motif]");
	return 1;
}

stock Jus_CmdVerdict(playerid, const cmd[], idx)
{
	if (!Jus_IsJudge(playerid)) return Cw_Notify(playerid, "denied", "Justice", "Seul un juge rend un verdict.", 3000), 1;
	new tok[16], what[16], val[16], motif[80];
	Frp_Tok(cmd, idx, tok, sizeof tok);
	Frp_Tok(cmd, idx, what, sizeof what);
	new t = Fx_Find(tok);
	if (t == INVALID_PLAYER_ID || !what[0]) return SendClientMessage(playerid, FRP_COL_INFO, "/verdict <id> acquitte | amende <montant> | prison <minutes> [motif]"), 1;
	if (!Jus_Near(playerid, t, JUS_TRIAL_RANGE)) return Cw_Notify(playerid, "denied", "Justice", "L'accuse doit etre dans la salle.", 3000), 1;
	new amount = 0;
	if (strcmp(what, "acquitte", true)) { Frp_Tok(cmd, idx, val, sizeof val); amount = strval(val); }
	Frp_Rest(cmd, idx, motif, sizeof motif);
	if (strlen(motif) < 3) format(motif, sizeof motif, "Decision du tribunal");
	new jn[MAX_PLAYER_NAME + 1], tn[MAX_PLAYER_NAME + 1], m[200], rec[100];
	Fx_RPName(playerid, jn, sizeof jn);
	Fx_RPName(t, tn, sizeof tn);
	if (!strcmp(what, "acquitte", true))
	{
		format(m, sizeof m, "** VERDICT : %s est ACQUITTE(E). Le juge %s leve les charges. **", tn, jn);
		format(rec, sizeof rec, "Acquitte : %s", motif);
		if (GetPlayerWantedLevel(t) > 0) { SetPlayerWantedLevel(t, 0); g_LawReason[t][0] = 0; }
	}
	else if (!strcmp(what, "amende", true))
	{
		if (amount < 100 || amount > 50000) return Cw_Notify(playerid, "denied", "Justice", "Amende : 100 a 50 000 $.", 3000), 1;
		Fine_Give(playerid, t, amount, motif);
		format(m, sizeof m, "** VERDICT : %s est condamne(e) a %d $ d'amende (%s). **", tn, amount, motif);
		format(rec, sizeof rec, "Condamne : %d $ d'amende (%s)", amount, motif);
	}
	else if (!strcmp(what, "prison", true))
	{
		if (amount < 1 || amount > 120) return Cw_Notify(playerid, "denied", "Justice", "Prison : 1 a 120 minutes.", 3000), 1;
		new law = g_JusLawyer[t];
		if (law != INVALID_PLAYER_ID && IsPlayerConnected(law) && Jus_Near(playerid, law, JUS_TRIAL_RANGE) && amount > 1)
		{
			amount = amount - amount / 4;
			Jus_Around(playerid, 30.0, FRP_COL_JUS, "** La plaidoirie de la defense est retenue : peine reduite de 25 %. **");
		}
		format(m, sizeof m, "** VERDICT : %s est condamne(e) a %d minute(s) de prison (%s). **", tn, amount, motif);
		format(rec, sizeof rec, "Condamne : %d min de prison (%s)", amount, motif);
		if (g_PolCuffed[t]) Pol_Uncuff(t);
		Pol_Jail(t, amount);
		Cw46_OnVerdict(playerid);   // [v46] stats
		SetPlayerWantedLevel(t, 0);
		g_LawReason[t][0] = 0;
	}
	else return SendClientMessage(playerid, FRP_COL_INFO, "/verdict <id> acquitte | amende <montant> | prison <minutes> [motif]"), 1;
	Jus_Around(playerid, 30.0, FRP_COL_JUS, m);
	Jus_RecordId(t, "verdict", rec, playerid);
	g_JusTrial[playerid] = INVALID_PLAYER_ID;
	g_JusLawyer[t] = INVALID_PLAYER_ID;
	if (Fac_Type(playerid) == FACTION_JUSTICE && g_GovTreasury >= JUS_JUDGE_PAY)
	{
		g_GovTreasury -= JUS_JUDGE_PAY;
		Gov_SaveSetting("treasury", g_GovTreasury);
		Frp_PayBank(playerid, JUS_JUDGE_PAY, "Audience du tribunal");
		Cw_Notify(playerid, "success", "Justice", "Audience close. Vacation : ~g~1 500 $~w~ (caisse de l'Etat).", 5000);
	}
	return 1;
}

stock Jus_CmdBail(playerid, const cmd[], idx)
{
	if (!Jus_IsJudge(playerid)) return Cw_Notify(playerid, "denied", "Justice", "Seul un juge fixe une caution.", 3000), 1;
	new tok[16];
	Frp_Tok(cmd, idx, tok, sizeof tok);
	new t = Fx_Find(tok);
	Frp_Tok(cmd, idx, tok, sizeof tok);
	new amount = strval(tok);
	if (t == INVALID_PLAYER_ID || amount < 500 || amount > 500000) return SendClientMessage(playerid, FRP_COL_INFO, "/caution <id> <montant 500 - 500 000> (detenu en cellule)"), 1;
	if (g_PolJailUntil[t] <= gettime()) return Cw_Notify(playerid, "denied", "Justice", "Cette personne n'est pas detenue.", 3000), 1;
	g_JusBail[t] = amount;
	new a[16], m[160], tn[MAX_PLAYER_NAME + 1];
	Cw_Fmt(amount, a, sizeof a);
	format(m, sizeof m, "Le juge fixe ta caution a ~g~%s $~w~ : paye avec ~b~/payercaution~w~ pour sortir.", a);
	Cw_Notify(t, "info", "Justice", m, 9000);
	Fx_RPName(t, tn, sizeof tn);
	format(m, sizeof m, "Caution de %s $ fixee pour %s.", a, tn);
	Jus_RecordId(t, "caution", m, playerid);
	return Cw_Notify(playerid, "success", "Justice", m, 4000), 1;
}

stock Jus_CmdPayBail(playerid)
{
	new amount = g_JusBail[playerid];
	if (amount <= 0 || g_PolJailUntil[playerid] <= gettime()) return Cw_Notify(playerid, "denied", "Justice", "Aucune caution fixee pour toi (demande a ton avocat : /demandercaution).", 4000), 1;
	if (GetPlayerMoney(playerid) >= amount) GivePlayerMoney(playerid, -amount);
	else if (g_BankLoaded[playerid] && g_BankBalance[playerid] >= amount) { g_BankBalance[playerid] -= amount; Bank_AddHistory(playerid, "Caution judiciaire", -amount); Bank_SavePlayer(playerid); }
	else return Cw_Notify(playerid, "denied", "Justice", "Tu n'as pas assez d'argent (liquide ou banque).", 3500), 1;
	if (g_GovTreasury < 2000000000 - amount) g_GovTreasury += amount;
	Gov_SaveSetting("treasury", g_GovTreasury);
	g_JusBail[playerid] = 0;
	Jus_RecordId(playerid, "caution", "Liberation sous caution", INVALID_PLAYER_ID);
	Pol_Release(playerid);
	return Cw_Notify(playerid, "success", "Justice", "Caution payee : tu es libre. Reste a la disposition de la justice.", 6000), 1;
}

stock Jus_CmdLawyer(playerid, const cmd[], idx)
{
	if (!Jus_IsLawyer(playerid)) return Cw_Notify(playerid, "denied", "Justice", "Il faut etre avocat (Justice, grade 2+).", 3000), 1;
	new tok[16];
	Frp_Tok(cmd, idx, tok, sizeof tok);
	new t = Fx_Find(tok);
	Frp_Tok(cmd, idx, tok, sizeof tok);
	new fee = strval(tok);
	if (t == INVALID_PLAYER_ID || t == playerid || fee < 0 || fee > 50000) return SendClientMessage(playerid, FRP_COL_INFO, "/avocat <id> <honoraires 0 - 50 000>"), 1;
	if (!Jus_Near(playerid, t, 6.0) && g_PolJailUntil[t] <= gettime()) return Cw_Notify(playerid, "denied", "Justice", "Rapproche-toi de ton client (ou il doit etre en cellule).", 3000), 1;
	g_JusOffer[t] = playerid;
	g_JusOfferFee[t] = fee;
	new ln[MAX_PLAYER_NAME + 1], m[160], a[16];
	Fx_RPName(playerid, ln, sizeof ln);
	Cw_Fmt(fee, a, sizeof a);
	format(m, sizeof m, "Maitre %s propose d'etre ton avocat (%s $). Tape ~b~/accepteravocat~w~.", ln, a);
	Cw_Notify(t, "info", "Justice", m, 9000);
	return Cw_Notify(playerid, "success", "Justice", "Proposition envoyee.", 3000), 1;
}

stock Jus_CmdAcceptLawyer(playerid)
{
	new l = g_JusOffer[playerid], fee = g_JusOfferFee[playerid];
	if (l == INVALID_PLAYER_ID || !IsPlayerConnected(l) || !Jus_IsLawyer(l)) return Cw_Notify(playerid, "denied", "Justice", "Aucune proposition d'avocat.", 3000), 1;
	if (fee > 0)
	{
		if (GetPlayerMoney(playerid) >= fee) GivePlayerMoney(playerid, -fee);
		else if (g_BankLoaded[playerid] && g_BankBalance[playerid] >= fee) { g_BankBalance[playerid] -= fee; Bank_AddHistory(playerid, "Honoraires d'avocat", -fee); Bank_SavePlayer(playerid); }
		else return Cw_Notify(playerid, "denied", "Justice", "Pas assez d'argent pour les honoraires.", 3000), 1;
		Frp_PayBank(l, fee, "Honoraires d'avocat");
	}
	g_JusLawyer[playerid] = l;
	g_JusOffer[playerid] = INVALID_PLAYER_ID;
	new n[MAX_PLAYER_NAME + 1], m[128];
	Fx_RPName(playerid, n, sizeof n);
	format(m, sizeof m, "%s t'a choisi comme avocat. Present au verdict : -25 %% de prison. /demandercaution %d", n, playerid);
	Cw_Notify(l, "success", "Justice", m, 7000);
	return Cw_Notify(playerid, "success", "Justice", "Avocat engage.", 3000), 1;
}

stock Jus_CmdAskBail(playerid, const cmd[], idx)
{
	new tok[16];
	Frp_Tok(cmd, idx, tok, sizeof tok);
	new t = Fx_Find(tok);
	if (t == INVALID_PLAYER_ID || g_JusLawyer[t] != playerid) return Cw_Notify(playerid, "denied", "Justice", "/demandercaution <id de ton client>", 3000), 1;
	if (g_PolJailUntil[t] <= gettime()) return Cw_Notify(playerid, "denied", "Justice", "Ton client n'est pas detenu.", 3000), 1;
	new ln[MAX_PLAYER_NAME + 1], tn[MAX_PLAYER_NAME + 1], m[180], n = 0;
	Fx_RPName(playerid, ln, sizeof ln);
	Fx_RPName(t, tn, sizeof tn);
	format(m, sizeof m, "[JUSTICE] Maitre %s demande une caution pour %s (ID %d) : /caution %d <montant>", ln, tn, t, t);
	for (new i = 0, mx = GetMaxPlayers(); i < mx; i++) if (IsPlayerConnected(i) && i != playerid && Jus_IsJudge(i) && Fac_Type(i) == FACTION_JUSTICE) { SendClientMessage(i, FRP_COL_JUS, m); n++; }
	return Cw_Notify(playerid, n ? ("success") : ("denied"), "Justice", n ? ("Demande transmise aux juges.") : ("Aucun juge en ville pour le moment."), 3500), 1;
}

stock Jus_CmdClearRecord(playerid, const cmd[], idx)
{
	if (!Frp_Staff(playerid) && !(Fac_Type(playerid) == FACTION_JUSTICE && Frp_IsTop(playerid, 0))) return Cw_Notify(playerid, "denied", "Justice", "Reserve au president du tribunal.", 3000), 1;
	new who[32], name[MAX_PLAYER_NAME + 1], q[128];
	Frp_Tok(cmd, idx, who, sizeof who);
	Frp_Target(who, name, sizeof name);
	if (!name[0]) return SendClientMessage(playerid, FRP_COL_INFO, "/effacercasier <id ou Prenom_Nom>"), 1;
	format(q, sizeof q, "DELETE FROM casier WHERE name = '%s'", name);
	Frp_Exec(q);
	return Cw_Notify(playerid, "success", "Justice", "Casier judiciaire efface.", 3000), 1;
}

// ============================================================
//  4) FBI : traceurs GPS + ecoutes
// ============================================================
#define FBI_TRACKERS          10
#define FBI_TRACK_S           1800
#define FBI_TAPS              6
#define FBI_TAP_S             1200
new g_TrkVeh[FBI_TRACKERS], g_TrkEnd[FBI_TRACKERS], g_TrkBy[FBI_TRACKERS][MAX_PLAYER_NAME + 1];
new g_FbiFollow[MAX_PLAYERS] = {-1, ...};
new g_TapNum[FBI_TAPS][16], g_TapEnd[FBI_TAPS];
new g_ScanBusy[MAX_PLAYERS];

stock Fbi_Is(playerid) return Fac_Type(playerid) == FACTION_FEDERAL || Frp_Staff(playerid);

stock Fbi_NearestVehicle(playerid, Float:range)
{
	new Float:px, Float:py, Float:pz, best = 0, Float:bd = range;
	GetPlayerPos(playerid, px, py, pz);
	for (new v = 1; v <= MAX_VEHICLES; v++)
	{
		if (!GetVehicleModel(v) || GetVehicleVirtualWorld(v) != GetPlayerVirtualWorld(playerid)) continue;
		new Float:x, Float:y, Float:z;
		GetVehiclePos(v, x, y, z);
		new Float:d = floatsqroot((x - px) * (x - px) + (y - py) * (y - py) + (z - pz) * (z - pz));
		if (d < bd) { bd = d; best = v; }
	}
	return best;
}

stock Fbi_TrackerOf(v)
{
	for (new k = 0; k < FBI_TRACKERS; k++) if (g_TrkVeh[k] == v && g_TrkEnd[k] > gettime()) return k;
	return -1;
}

stock Fbi_Plant(playerid, v)
{
	if (!Fbi_Is(playerid)) return Cw_Notify(playerid, "denied", "FBI", "Reserve au FBI.", 3000), 1;
	if (GetPlayerState(playerid) != PLAYER_STATE_ONFOOT) return Cw_Notify(playerid, "denied", "FBI", "Il faut etre a pied.", 3000), 1;
	if (!v) v = Fbi_NearestVehicle(playerid, 4.0);
	if (!v) return Cw_Notify(playerid, "denied", "FBI", "Aucun vehicule a cote de toi.", 3000), 1;
	if (Fbi_TrackerOf(v) != -1) return Cw_Notify(playerid, "denied", "FBI", "Ce vehicule a deja un traceur.", 3000), 1;
	new slot = -1, now = gettime();
	for (new k = 0; k < FBI_TRACKERS; k++) if (g_TrkEnd[k] <= now) { slot = k; break; }
	if (slot == -1) return Cw_Notify(playerid, "denied", "FBI", "Plus de traceurs disponibles (10 max).", 3000), 1;
	g_TrkVeh[slot] = v;
	g_TrkEnd[slot] = now + FBI_TRACK_S;
	GetPlayerName(playerid, g_TrkBy[slot], MAX_PLAYER_NAME + 1);
	ApplyAnimation(playerid, "BOMBER", "BOM_Plant", 4.1, 0, 0, 0, 0, 0, 1);
	new m[128];
	format(m, sizeof m, "Traceur #%d pose sur %s (30 min). /traceurs puis /suivre %d.", slot + 1, g_VehName[GetVehicleModel(v) - 400], slot + 1);
	Cw_Notify(playerid, "success", "FBI", m, 6000);
	format(m, sizeof m, "[FBI] Traceur #%d pose sur un(e) %s.", slot + 1, g_VehName[GetVehicleModel(v) - 400]);
	Frp_SendType(FACTION_FEDERAL, FRP_COL_FBI, m);
	return 1;
}

stock Fbi_List(playerid)
{
	if (!Fbi_Is(playerid)) return Cw_Notify(playerid, "denied", "FBI", "Reserve au FBI.", 3000), 1;
	new now = gettime(), n = 0, m[144], Float:px, Float:py, Float:pz;
	GetPlayerPos(playerid, px, py, pz);
	for (new k = 0; k < FBI_TRACKERS; k++)
	{
		if (g_TrkEnd[k] <= now || !GetVehicleModel(g_TrkVeh[k])) continue;
		new Float:x, Float:y, Float:z;
		GetVehiclePos(g_TrkVeh[k], x, y, z);
		format(m, sizeof m, "[TRACEUR %d] %s - %d m - encore %d min - pose par %s", k + 1, g_VehName[GetVehicleModel(g_TrkVeh[k]) - 400], floatround(Frp_Dist2(px, py, x, y)), (g_TrkEnd[k] - now) / 60, g_TrkBy[k]);
		SendClientMessage(playerid, FRP_COL_FBI, m);
		n++;
	}
	if (!n) SendClientMessage(playerid, FRP_COL_INFO, "Aucun traceur actif. Pose-en un : /traceur (a cote d'un vehicule).");
	else SendClientMessage(playerid, FRP_COL_INFO, "/suivre <numero> : GPS mis a jour toutes les 4 s  |  /stopsuivre");
	return 1;
}

stock Fbi_Follow(playerid, const cmd[], idx)
{
	if (!Fbi_Is(playerid)) return Cw_Notify(playerid, "denied", "FBI", "Reserve au FBI.", 3000), 1;
	new tok[8];
	Frp_Tok(cmd, idx, tok, sizeof tok);
	new k = strval(tok) - 1;
	if (k < 0 || k >= FBI_TRACKERS || g_TrkEnd[k] <= gettime() || !GetVehicleModel(g_TrkVeh[k])) return SendClientMessage(playerid, FRP_COL_INFO, "/suivre <numero du traceur> (voir /traceurs)"), 1;
	g_FbiFollow[playerid] = k;
	return Cw_Notify(playerid, "success", "FBI", "Suivi actif : le GPS suit le vehicule (/stopsuivre).", 4000), 1;
}

forward Fbi_ScanDone(playerid, v);
public Fbi_ScanDone(playerid, v)
{
	g_ScanBusy[playerid] = 0;
	if (!IsPlayerConnected(playerid)) return 0;
	ClearAnimations(playerid);
	new k = Fbi_TrackerOf(v);
	new chance = (Fac_Type(playerid) == FACTION_MECHANIC) ? 90 : 55;
	if (k == -1 || random(100) >= chance) return Cw_Notify(playerid, "info", "Vehicule", "Inspection terminee : rien de suspect.", 4000), 1;
	g_TrkEnd[k] = 0;
	g_TrkVeh[k] = 0;
	for (new p = 0, m = GetMaxPlayers(); p < m; p++) if (g_FbiFollow[p] == k) g_FbiFollow[p] = -1;
	Frp_SendType(FACTION_FEDERAL, FRP_COL_FBI, "[FBI] Un traceur a ete decouvert et arrache : signal perdu.");
	return Cw_Notify(playerid, "warning", "Vehicule", "Tu trouves un ~r~traceur GPS~w~ sous le chassis et tu l'arraches !", 6000), 1;
}

stock Fbi_Scan(playerid, v)
{
	if (g_ScanBusy[playerid]) return 1;
	if (GetPlayerState(playerid) != PLAYER_STATE_ONFOOT) return Cw_Notify(playerid, "denied", "Vehicule", "Il faut etre a pied.", 3000), 1;
	if (!v) v = Fbi_NearestVehicle(playerid, 4.0);
	if (!v) return Cw_Notify(playerid, "denied", "Vehicule", "Aucun vehicule a cote de toi.", 3000), 1;
	if (!Ui_OwnsVehicle(playerid, v) && Fac_Type(playerid) != FACTION_MECHANIC) return Cw_Notify(playerid, "denied", "Vehicule", "Seulement ton vehicule (ou mecanicien).", 3000), 1;
	g_ScanBusy[playerid] = 1;
	ApplyAnimation(playerid, "CAR", "Fixn_Car_Loop", 4.1, 1, 0, 0, 0, 0, 1);
	SetTimerEx("Fbi_ScanDone", 5000, false, "ii", playerid, v);
	return Cw_Notify(playerid, "info", "Vehicule", "Inspection du vehicule (5 s)...", 5000), 1;
}

stock Fbi_Tap(playerid, const cmd[], idx)
{
	if (!Fbi_Is(playerid) || (Fac_Type(playerid) == FACTION_FEDERAL && g_FacRank[playerid] < 2)) return Cw_Notify(playerid, "denied", "FBI", "Reserve aux agents speciaux (grade 2+).", 3000), 1;
	new num[16], now = gettime(), m[128];
	Frp_Tok(cmd, idx, num, sizeof num);
	if (!strcmp(num, "liste", true) || !num[0])
	{
		new n = 0;
		for (new k = 0; k < FBI_TAPS; k++) if (g_TapEnd[k] > now) { format(m, sizeof m, "[ECOUTE] %s - encore %d min", g_TapNum[k], (g_TapEnd[k] - now) / 60); SendClientMessage(playerid, FRP_COL_FBI, m); n++; }
		if (!n) SendClientMessage(playerid, FRP_COL_INFO, "/ecoute <numero> (20 min)  |  /ecoute stop <numero>");
		return 1;
	}
	if (!strcmp(num, "stop", true))
	{
		Frp_Tok(cmd, idx, num, sizeof num);
		for (new k = 0; k < FBI_TAPS; k++) if (g_TapEnd[k] > now && !strcmp(g_TapNum[k], num)) g_TapEnd[k] = 0;
		return Cw_Notify(playerid, "success", "FBI", "Ecoute arretee.", 3000), 1;
	}
	for (new i = 0; num[i]; i++) if (num[i] < '0' || num[i] > '9') return SendClientMessage(playerid, FRP_COL_INFO, "/ecoute <numero>"), 1;
	new slot = -1;
	for (new k = 0; k < FBI_TAPS; k++) if (g_TapEnd[k] <= now) { slot = k; break; }
	if (slot == -1) return Cw_Notify(playerid, "denied", "FBI", "Trop d'ecoutes en cours (6 max).", 3000), 1;
	format(g_TapNum[slot], 16, "%s", num);
	g_TapEnd[slot] = now + FBI_TAP_S;
	format(m, sizeof m, "[FBI] Ecoute ouverte sur le %s (20 min).", num);
	Frp_SendType(FACTION_FEDERAL, FRP_COL_FBI, m);
	return 1;
}

stock Fbi_Tapped(const num[])
{
	if (!num[0]) return 0;
	new now = gettime();
	for (new k = 0; k < FBI_TAPS; k++) if (g_TapEnd[k] > now && !strcmp(g_TapNum[k], num)) return 1;
	return 0;
}

// appele par Phone_Sms / Phone_Accept
stock Frp_OnSms(const from[], const to[], const text[])
{
	if (!Fbi_Tapped(from) && !Fbi_Tapped(to)) return 0;
	new m[180];
	format(m, sizeof m, "[ECOUTE] SMS %s -> %s : %s", from, to, text);
	return Frp_SendType(FACTION_FEDERAL, FRP_COL_FBI, m);
}

stock Frp_OnCall(const a[], const b[])
{
	if (!Fbi_Tapped(a) && !Fbi_Tapped(b)) return 0;
	new m[128];
	format(m, sizeof m, "[ECOUTE] Appel en cours entre le %s et le %s.", a, b);
	return Frp_SendType(FACTION_FEDERAL, FRP_COL_FBI, m);
}

// ============================================================
//  5) ARMEE / ETAT D'URGENCE
// ============================================================
new g_ArmyZoneOn, Float:g_ArmyZoneX, Float:g_ArmyZoneY, Float:g_ArmyZoneR;
new g_ArmyWarn[MAX_PLAYERS];
new g_Emergency;

stock Frp_GetSetting(const k[], def)
{
	new q[96], v = def, f[24];
	if (g_FrpDB == DB:0) return def;
	format(q, sizeof q, "SELECT v FROM frp_settings WHERE k = '%s'", k);
	new DBResult:r = db_query(g_FrpDB, q);
	if (r != DBResult:0)
	{
		if (db_num_rows(r) > 0)
		{
			db_get_field(r, 0, f, sizeof f);
			v = strval(f);
		}
		db_free_result(r);
	}
	return v;
}

stock Frp_SetSetting(const k[], v)
{
	new q[120];
	format(q, sizeof q, "INSERT OR REPLACE INTO frp_settings (k, v) VALUES ('%s', %d)", k, v);
	return Frp_Exec(q);
}

stock Army_Can(playerid) return Frp_Staff(playerid) || (Fac_Type(playerid) == FACTION_ARMY && Frp_IsTop(playerid, 2));

stock Army_CmdZone(playerid, const cmd[], idx)
{
	if (!Army_Can(playerid)) return Cw_Notify(playerid, "denied", "Armee", "Reserve aux officiers superieurs de l'armee.", 3000), 1;
	new tok[12], m[128];
	Frp_Tok(cmd, idx, tok, sizeof tok);
	if (!strcmp(tok, "on", true)) g_ArmyZoneOn = 1;
	else if (!strcmp(tok, "off", true)) g_ArmyZoneOn = 0;
	else if (!strcmp(tok, "ici", true))
	{
		new Float:z;
		GetPlayerPos(playerid, g_ArmyZoneX, g_ArmyZoneY, z);
		Frp_Tok(cmd, idx, tok, sizeof tok);
		new r = strval(tok);
		if (r < 30 || r > 400) r = 120;
		g_ArmyZoneR = float(r);
		g_ArmyZoneOn = 1;
	}
	else
	{
		format(m, sizeof m, "Zone militaire : %s, rayon %d m. /zonemilitaire on | off | ici <rayon 30-400>", g_ArmyZoneOn ? ("ACTIVE") : ("inactive"), floatround(g_ArmyZoneR));
		return SendClientMessage(playerid, FRP_COL_INFO, m), 1;
	}
	Frp_SetSetting("army_on", g_ArmyZoneOn);
	Frp_SetSetting("army_x", floatround(g_ArmyZoneX));
	Frp_SetSetting("army_y", floatround(g_ArmyZoneY));
	Frp_SetSetting("army_r", floatround(g_ArmyZoneR));
	format(m, sizeof m, "[ARMEE] Zone militaire %s (rayon %d m).", g_ArmyZoneOn ? ("ACTIVEE : intrusion = avis de recherche") : ("desactivee"), floatround(g_ArmyZoneR));
	Frp_SendType(FACTION_ARMY, 0x6B8E3AFF, m);
	if (g_ArmyZoneOn) Gps_Set(playerid, g_ArmyZoneX, g_ArmyZoneY);
	return 1;
}

stock Army_ZoneTick()
{
	if (!g_ArmyZoneOn) return 0;
	for (new p = 0, mx = GetMaxPlayers(); p < mx; p++)
	{
		if (!IsPlayerConnected(p) || !g_AuthLogged[p] || IsPlayerNPC(p)) continue;
		if (Fac_Type(p) == FACTION_ARMY || Admin_Level(p) >= 1 || GetPlayerVirtualWorld(p) != 0 || GetPlayerInterior(p) != 0) { g_ArmyWarn[p] = 0; continue; }
		new Float:x, Float:y, Float:z;
		GetPlayerPos(p, x, y, z);
		if (Frp_Dist2(x, y, g_ArmyZoneX, g_ArmyZoneY) > g_ArmyZoneR || z > 150.0) { g_ArmyWarn[p] = 0; continue; }
		// passager d'un militaire : autorise
		new v = GetPlayerVehicleID(p), escorted = 0;
		if (v) for (new q = 0; q < mx; q++) if (q != p && IsPlayerConnected(q) && GetPlayerVehicleID(q) == v && Fac_Type(q) == FACTION_ARMY) { escorted = 1; break; }
		if (escorted) continue;
		g_ArmyWarn[p]++;
		if (g_ArmyWarn[p] == 1) Cw_Notify(p, "warning", "Zone militaire", "~r~ZONE MILITAIRE INTERDITE~w~. Fais demi-tour immediatement !", 6000);
		else if (g_ArmyWarn[p] == 4)
		{
			if (GetPlayerWantedLevel(p) < 3) SetPlayerWantedLevel(p, 3);
			format(g_LawReason[p], 64, "Intrusion en zone militaire");
			format(g_LawBy[p], MAX_PLAYER_NAME + 1, "Armee");
			new n[MAX_PLAYER_NAME + 1], m[128];
			Fx_RPName(p, n, sizeof n);
			format(m, sizeof m, "Intrus dans la zone militaire : %s (%d). Position au GPS.", n, p);
			Frp_NotifyType(FACTION_ARMY, "warning", "Armee", m, 1, x, y);
			Cw_Notify(p, "denied", "Zone militaire", "Tu es entre de force : ~r~avis de recherche~w~ (3 etoiles).", 6000);
		}
	}
	return 1;
}

stock Frp_CmdEmergency(playerid, const cmd[], idx)
{
	if (!Frp_Staff(playerid) && !(Fac_Type(playerid) == FACTION_GOVERNMENT && Frp_IsTop(playerid, 1)) && !(Fac_Type(playerid) == FACTION_ARMY && Frp_IsTop(playerid, 1)))
		return Cw_Notify(playerid, "denied", "Etat d'urgence", "Reserve au maire, a son adjoint et a l'etat-major.", 3000), 1;
	new tok[8];
	Frp_Tok(cmd, idx, tok, sizeof tok);
	if (!strcmp(tok, "on", true)) g_Emergency = 1;
	else if (!strcmp(tok, "off", true)) g_Emergency = 0;
	else return SendClientMessage(playerid, FRP_COL_INFO, g_Emergency ? ("Etat d'urgence EN COURS. /etatdurgence off") : ("/etatdurgence on | off")), 1;
	Frp_SetSetting("emergency", g_Emergency);
	if (g_Emergency) Cw_NotifyAll("warning", "ETAT D'URGENCE", "Le gouvernement declare l'~r~etat d'urgence~w~ : restez chez vous, obeissez aux forces de l'ordre et a l'armee.", 12000);
	else Cw_NotifyAll("success", "Etat d'urgence", "L'etat d'urgence est leve. Merci de votre cooperation.", 9000);
	return 1;
}

// FacX_Pay : salaires doubles pendant l'etat d'urgence (forces de l'ordre, armee, EMS, pompiers)
stock Frp_PayMult(playerid, pay)
{
	if (!g_Emergency) return pay;
	new t = Fac_Type(playerid);
	if (Fac_IsLaw(playerid) || t == FACTION_MEDIC || t == FACTION_FIRE) return pay * 2;
	return pay;
}

// ============================================================
//  aide, roue, minuteries, hooks
// ============================================================
stock Frp_AideText(playerid, t[], size)
{
	new ft = Fac_Type(playerid), st = Frp_Staff(playerid);
	strcat(t, "{FF7A1A}/112 pompiers <message>\t{FFFFFF}Appeler les pompiers (ou le 18 au telephone)\n", size);
	strcat(t, "{A78BFA}/moncasier\t{FFFFFF}Ton casier judiciaire\n", size);
	strcat(t, "{A78BFA}/payercaution\t{FFFFFF}Payer la caution fixee par le juge (sortie de prison)\n", size);
	strcat(t, "{A78BFA}/accepteravocat\t{FFFFFF}Accepter l'avocat qui te propose ses services\n", size);
	strcat(t, "{9AA5B1}/inspectervehicule\t{FFFFFF}Chercher un traceur GPS sur ton vehicule\n", size);
	if (ft == FACTION_FIRE || ft == FACTION_MEDIC || st)
		strcat(t, "{FF7A1A}/incendies  /secourir\t{FFFFFF}Incendies en cours (GPS)  |  Sauver une victime\n{FF7A1A}Extincteur / Firetruck\t{FFFFFF}Vise les flammes et tire (lance du camion : bouton tir)\n", size);
	if (Jus_CanRead(playerid))
		strcat(t, "{A78BFA}/casier <id|Prenom_Nom>\t{FFFFFF}Casier judiciaire\n{A78BFA}/mandats  /executermandat <n>\t{FFFFFF}Mandats actifs  |  executer (suspect menotte)\n", size);
	if (ft == FACTION_JUSTICE || st)
		strcat(t, "{A78BFA}/mandat arret|perquisition <qui> <motif>\t{FFFFFF}Signer un mandat (juge)\n{A78BFA}/proces <id>  /verdict <id> ...\t{FFFFFF}Audience : acquitte | amende <$> | prison <min>\n{A78BFA}/caution <id> <$>  /annulermandat <n>\t{FFFFFF}Caution d'un detenu  |  annuler un mandat\n{A78BFA}/avocat <id> <$>  /demandercaution <id>\t{FFFFFF}Avocat : honoraires, demande de caution\n{A78BFA}/effacercasier <qui>\t{FFFFFF}Effacer un casier (president)\n", size);
	if (ft == FACTION_FEDERAL || st)
		strcat(t, "{8E7CFF}/traceur  /traceurs\t{FFFFFF}Poser un traceur GPS  |  liste\n{8E7CFF}/suivre <n>  /stopsuivre\t{FFFFFF}Suivre un vehicule au GPS\n{8E7CFF}/ecoute <numero>\t{FFFFFF}Ecoute telephonique 20 min (liste, stop)\n", size);
	if (ft == FACTION_ARMY || ft == FACTION_GOVERNMENT || st)
		strcat(t, "{6B8E3A}/zonemilitaire on|off|ici <r>\t{FFFFFF}Zone interdite (officiers)\n{6B8E3A}/etatdurgence on|off\t{FFFFFF}Etat d'urgence (maire, etat-major)\n", size);
	if (st)
		strcat(t, "{C2A2DA}/factionsdefaut\t{FFFFFF}Recreer les factions par defaut manquantes\n{C2A2DA}/incendie [1-3]  /stopincendies\t{FFFFFF}Declencher / eteindre les incendies\n{C2A2DA}/feupoint <nom>  /feupoints  /feupointdel <id>\t{FFFFFF}Points d'incendie\n", size);
	new l = strlen(t);
	if (l > 0 && t[l - 1] == '\n') t[l - 1] = 0;
	return 1;
}

// roue du joueur (Ui_OpenPlayer3D)
stock Frp_UiPlayerItems(playerid, target, txt[], size)
{
	#pragma unused target
	if (Jus_CanRead(playerid)) strcat(txt, ";jus_rec:Casier judiciaire", size);
	if (Jus_IsJudge(playerid) && Fac_Type(playerid) == FACTION_JUSTICE) strcat(txt, ";jus_trial:Ouvrir le procès", size);
	return 1;
}

// roue du vehicule (Ui_OpenVehicle3D, a pied)
stock Frp_UiVehItems(playerid, v, txt[], size)
{
	if (Fac_Type(playerid) == FACTION_FEDERAL && Fbi_TrackerOf(v) == -1) strcat(txt, "fbi_trk:Poser un traceur;", size);
	if (Ui_OwnsVehicle(playerid, v) || Fac_Type(playerid) == FACTION_MECHANIC) strcat(txt, "trk_scan:Chercher un traceur;", size);
	return 1;
}

stock Frp_UiAction(playerid, const action[])
{
	if (!strcmp(action, "jus_rec", true))
	{
		new t = g_UiTarget[playerid], n[MAX_PLAYER_NAME + 1];
		if (!Jus_CanRead(playerid) || t < 0 || t >= MAX_PLAYERS || !IsPlayerConnected(t)) return 1;
		GetPlayerName(t, n, sizeof n);
		return Jus_ShowRecord(playerid, n);
	}
	if (!strcmp(action, "jus_trial", true)) return Jus_OpenTrial(playerid, g_UiTarget[playerid]);
	if (!strcmp(action, "fbi_trk", true)) return Fbi_Plant(playerid, g_UiVeh[playerid]);
	if (!strcmp(action, "trk_scan", true)) return Fbi_Scan(playerid, g_UiVeh[playerid]);
	return 0;
}

forward Frp_Tick();
public Frp_Tick()
{
	g_FrpTicks++;
	Fire_Extinguish();
	if (g_FrpTicks % 2) return 1;
	Fire_Second();
	if (g_FrpTicks % 4 == 0) Army_ZoneTick();
	if (g_FrpTicks % 8 == 0)
	{
		new now = gettime();
		for (new p = 0, mx = GetMaxPlayers(); p < mx; p++)
		{
			new k = g_FbiFollow[p];
			if (k < 0 || !IsPlayerConnected(p)) continue;
			if (g_TrkEnd[k] <= now || !GetVehicleModel(g_TrkVeh[k])) { g_FbiFollow[p] = -1; Cw_Notify(p, "denied", "FBI", "Signal du traceur perdu.", 4000); continue; }
			new Float:x, Float:y, Float:z;
			GetVehiclePos(g_TrkVeh[k], x, y, z);
			Gps_Set(p, x, y);
		}
	}
	return 1;
}

stock Frp_OnInit()
{
	g_FrpDB = db_open("crestwood_justice.db");
	if (g_FrpDB != DB:0)
	{
		Frp_Exec("CREATE TABLE IF NOT EXISTS casier (id INTEGER PRIMARY KEY AUTOINCREMENT, name TEXT COLLATE NOCASE, kind TEXT, text TEXT, author TEXT, ts INTEGER)");
		Frp_Exec("CREATE TABLE IF NOT EXISTS mandats (id INTEGER PRIMARY KEY AUTOINCREMENT, name TEXT COLLATE NOCASE, kind INTEGER, reason TEXT, judge TEXT, ts INTEGER, active INTEGER DEFAULT 1)");
		Frp_Exec("CREATE TABLE IF NOT EXISTS fire_spots (id INTEGER PRIMARY KEY AUTOINCREMENT, name TEXT, x REAL, y REAL, z REAL)");
		Frp_Exec("CREATE TABLE IF NOT EXISTS frp_settings (k TEXT PRIMARY KEY, v INTEGER)");
	}
	else print("[FRP] ERREUR : impossible d'ouvrir crestwood_justice.db");
	new made = Frp_SeedAll(0);
	printf("[FRP] factions par defaut : %d creee(s).", made);
	Fire_LoadSpots();
	for (new i = 0; i < FIRE_MAX_INC; i++) for (new v = 0; v < FIRE_VICTIMS; v++) g_VicActor[i][v] = INVALID_ACTOR_ID;
	g_FireNext = gettime() + 600;
	g_ArmyZoneOn = Frp_GetSetting("army_on", 0);
	g_ArmyZoneX = float(Frp_GetSetting("army_x", floatround(C23_BASE_X)));
	g_ArmyZoneY = float(Frp_GetSetting("army_y", floatround(C23_BASE_Y)));
	g_ArmyZoneR = float(Frp_GetSetting("army_r", 120));
	g_Emergency = Frp_GetSetting("emergency", 0);
	SetTimer("Frp_Tick", 500, true);
	return 1;
}

stock Frp_OnConnect(playerid)
{
	g_JusTrial[playerid] = INVALID_PLAYER_ID;
	g_JusBail[playerid] = 0;
	g_JusLawyer[playerid] = INVALID_PLAYER_ID;
	g_JusOffer[playerid] = INVALID_PLAYER_ID;
	g_JusChecked[playerid] = 0;
	g_FbiFollow[playerid] = -1;
	g_ArmyWarn[playerid] = 0;
	g_ScanBusy[playerid] = 0;
	for (new i = 0; i < FIRE_MAX_INC; i++) g_FireHelp[i][playerid] = 0;
	return 1;
}

stock Frp_OnDisconnect(playerid)
{
	for (new p = 0, m = GetMaxPlayers(); p < m; p++)
	{
		if (g_JusLawyer[p] == playerid) g_JusLawyer[p] = INVALID_PLAYER_ID;
		if (g_JusOffer[p] == playerid) g_JusOffer[p] = INVALID_PLAYER_ID;
		if (g_JusTrial[p] == playerid) g_JusTrial[p] = INVALID_PLAYER_ID;
	}
	for (new i = 0; i < FIRE_MAX_INC; i++) for (new v = 0; v < FIRE_VICTIMS; v++) if (g_VicBy[i][v] == playerid) g_VicBy[i][v] = INVALID_PLAYER_ID;
	return Frp_OnConnect(playerid);
}

stock Frp_OnSpawn(playerid)
{
	if (g_JusChecked[playerid] || !g_AuthLogged[playerid]) return 0;
	g_JusChecked[playerid] = 1;
	new n[MAX_PLAYER_NAME + 1], q[160], rs[80], jd[32];
	GetPlayerName(playerid, n, sizeof n);
	if (!Jus_HasWarrant(n, 1)) return 0;
	format(q, sizeof q, "SELECT reason, judge FROM mandats WHERE name = '%s' AND kind = 1 AND active = 1 ORDER BY id DESC LIMIT 1", n);
	new DBResult:r = db_query(g_FrpDB, q);
	if (r != DBResult:0)
	{
		if (db_num_rows(r) > 0)
		{
			db_get_field(r, 0, rs, sizeof rs);
			db_get_field(r, 1, jd, sizeof jd);
		}
		db_free_result(r);
	}
	return Jus_ApplyWarrant(playerid, rs, jd);
}

stock Frp_OnCommand(playerid, const cmdtext[])
{
	if (!g_AuthLogged[playerid]) return 0;
	new idx;
	// --- pompiers ---
	if (Fx_IsCmd(cmdtext, "/incendies", idx) || Fx_IsCmd(cmdtext, "/feux", idx)) return Fire_CmdList(playerid);
	if (Fx_IsCmd(cmdtext, "/secourir", idx)) return Fire_CmdRescue(playerid);
	if (Fx_IsCmd(cmdtext, "/incendie", idx))
	{
		if (!Frp_Staff(playerid)) return 0;
		new tok[8], Float:x, Float:y, Float:z;
		Frp_Tok(cmdtext, idx, tok, sizeof tok);
		GetPlayerPos(playerid, x, y, z);
		if (Fire_Start(x + 3.0, y, z, "Incendie (staff)", tok[0] ? strval(tok) : 2, 1) == -1) return Cw_Notify(playerid, "denied", "Pompiers", "Deja 3 incendies en cours.", 3000), 1;
		return 1;
	}
	if (Fx_IsCmd(cmdtext, "/stopincendies", idx))
	{
		if (!Frp_Staff(playerid)) return 0;
		for (new i = 0; i < FIRE_MAX_INC; i++) if (g_FireOn[i]) Fire_End(i, 0);
		return Cw_Notify(playerid, "success", "Pompiers", "Tous les incendies sont eteints.", 3000), 1;
	}
	if (Fx_IsCmd(cmdtext, "/feupoint", idx))
	{
		if (!Frp_Staff(playerid)) return 0;
		new nm[32], sn[32], q[200], Float:x, Float:y, Float:z;
		Frp_Rest(cmdtext, idx, nm, sizeof nm);
		Law_Clean(nm, sn, sizeof sn);
		if (strlen(sn) < 3) return SendClientMessage(playerid, FRP_COL_INFO, "/feupoint <nom du lieu> (a ta position, devant le batiment)"), 1;
		GetPlayerPos(playerid, x, y, z);
		format(q, sizeof q, "INSERT INTO fire_spots (name, x, y, z) VALUES ('%s', %f, %f, %f)", sn, x, y, z);
		Frp_Exec(q);
		Fire_LoadSpots();
		return Cw_Notify(playerid, "success", "Pompiers", "Point d'incendie ajoute.", 3000), 1;
	}
	if (Fx_IsCmd(cmdtext, "/feupoints", idx))
	{
		if (!Frp_Staff(playerid)) return 0;
		new m[96];
		format(m, sizeof m, "%d points par defaut + %d points du staff :", FIRE_DEF_SPOTS, g_SpotCount);
		SendClientMessage(playerid, FRP_COL_INFO, m);
		for (new s = 0; s < g_SpotCount; s++) { format(m, sizeof m, " #%d %s", g_SpotId[s], g_SpotName[s]); SendClientMessage(playerid, FRP_COL_FIRE, m); }
		return 1;
	}
	if (Fx_IsCmd(cmdtext, "/feupointdel", idx))
	{
		if (!Frp_Staff(playerid)) return 0;
		new tok[8], q[80];
		Frp_Tok(cmdtext, idx, tok, sizeof tok);
		format(q, sizeof q, "DELETE FROM fire_spots WHERE id = %d", strval(tok));
		Frp_Exec(q);
		Fire_LoadSpots();
		return Cw_Notify(playerid, "success", "Pompiers", "Point supprime.", 3000), 1;
	}
	// --- justice ---
	if (Fx_IsCmd(cmdtext, "/moncasier", idx)) { new n[MAX_PLAYER_NAME + 1]; GetPlayerName(playerid, n, sizeof n); return Jus_ShowRecord(playerid, n); }
	if (Fx_IsCmd(cmdtext, "/casier", idx))
	{
		if (!Jus_CanRead(playerid)) return Cw_Notify(playerid, "denied", "Justice", "Reserve aux forces de l'ordre et a la justice. Ton casier : /moncasier", 3500), 1;
		new who[32], name[MAX_PLAYER_NAME + 1];
		Frp_Tok(cmdtext, idx, who, sizeof who);
		Frp_Target(who, name, sizeof name);
		if (!name[0]) return SendClientMessage(playerid, FRP_COL_INFO, "/casier <id ou Prenom_Nom>"), 1;
		return Jus_ShowRecord(playerid, name);
	}
	if (Fx_IsCmd(cmdtext, "/mandat", idx)) return Jus_CmdWarrant(playerid, cmdtext, idx);
	if (Fx_IsCmd(cmdtext, "/mandats", idx)) return Jus_CmdWarrants(playerid);
	if (Fx_IsCmd(cmdtext, "/annulermandat", idx)) return Jus_CmdWarrantClose(playerid, cmdtext, idx, 0);
	if (Fx_IsCmd(cmdtext, "/executermandat", idx)) return Jus_CmdWarrantClose(playerid, cmdtext, idx, 1);
	if (Fx_IsCmd(cmdtext, "/proces", idx))
	{
		new tok[16];
		Frp_Tok(cmdtext, idx, tok, sizeof tok);
		new t = Fx_Find(tok);
		if (t == INVALID_PLAYER_ID) return SendClientMessage(playerid, FRP_COL_INFO, "/proces <id de l'accuse>"), 1;
		return Jus_OpenTrial(playerid, t);
	}
	if (Fx_IsCmd(cmdtext, "/verdict", idx)) return Jus_CmdVerdict(playerid, cmdtext, idx);
	if (Fx_IsCmd(cmdtext, "/caution", idx)) return Jus_CmdBail(playerid, cmdtext, idx);
	if (Fx_IsCmd(cmdtext, "/payercaution", idx)) return Jus_CmdPayBail(playerid);
	if (Fx_IsCmd(cmdtext, "/avocat", idx)) return Jus_CmdLawyer(playerid, cmdtext, idx);
	if (Fx_IsCmd(cmdtext, "/accepteravocat", idx)) return Jus_CmdAcceptLawyer(playerid);
	if (Fx_IsCmd(cmdtext, "/demandercaution", idx)) return Jus_CmdAskBail(playerid, cmdtext, idx);
	if (Fx_IsCmd(cmdtext, "/effacercasier", idx)) return Jus_CmdClearRecord(playerid, cmdtext, idx);
	// --- FBI ---
	if (Fx_IsCmd(cmdtext, "/traceur", idx)) return Fbi_Plant(playerid, 0);
	if (Fx_IsCmd(cmdtext, "/traceurs", idx)) return Fbi_List(playerid);
	if (Fx_IsCmd(cmdtext, "/suivre", idx)) return Fbi_Follow(playerid, cmdtext, idx);
	if (Fx_IsCmd(cmdtext, "/stopsuivre", idx)) { g_FbiFollow[playerid] = -1; return Cw_Notify(playerid, "info", "FBI", "Suivi arrete.", 2500), 1; }
	if (Fx_IsCmd(cmdtext, "/ecoute", idx)) return Fbi_Tap(playerid, cmdtext, idx);
	if (Fx_IsCmd(cmdtext, "/inspectervehicule", idx)) return Fbi_Scan(playerid, 0);
	// --- armee / gouvernement ---
	if (Fx_IsCmd(cmdtext, "/zonemilitaire", idx)) return Army_CmdZone(playerid, cmdtext, idx);
	if (Fx_IsCmd(cmdtext, "/etatdurgence", idx)) return Frp_CmdEmergency(playerid, cmdtext, idx);
	// --- staff ---
	if (Fx_IsCmd(cmdtext, "/factionsdefaut", idx))
	{
		if (!Frp_Staff(playerid)) return 0;
		new made = Frp_SeedAll(1), m[96];
		format(m, sizeof m, "Factions par defaut verifiees : %d recreee(s). Panneau admin > Actualiser.", made);
		return Cw_Notify(playerid, "success", "Factions", m, 5000), 1;
	}
	return 0;
}
