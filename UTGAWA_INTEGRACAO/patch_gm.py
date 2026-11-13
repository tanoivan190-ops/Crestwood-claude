"""Integra o sistema UTGAWA (lojas de roupas / acessórios / barbearia / tatuagem + criação) na GM
basicgm1.pwn do cliente (Crestwood RP). Arquivo em Latin-1, como o original.

Uso: python3 patch_gm.py <basicgm1.pwn de entrada> <saída>
"""
import sys, re

src, dst = sys.argv[1], sys.argv[2]
s = open(src, encoding='latin-1').read()
if 'UTGAWA_LOJAS' in s:
    sys.exit('já integrado')


def rep(old, new, count=1):
    global s
    n = s.count(old)
    if n != count:
        sys.exit('trecho encontrado %d vezes (esperado %d):\n%s' % (n, count, old))
    s = s.replace(old, new)


# ------------------------------------------------------------------ 1) 39 valores (25 + 14 acessórios)
rep('#define CLOTH_COUNT           25          // nombre de valeurs d\'apparence',
    '#define CLOTH_COUNT           39          // [UTGAWA] 25 de base + 14 accessoires (le client en envoie 39)\n'
    '#define CLOTH_BASE_COUNT      25          // anciennes sauvegardes : 25 valeurs (les accessoires restent par defaut)')
rep('''	"tattooBack", "tattooFront", "policeBelt", "policeVest", "policeHat"
};''', '''	"tattooBack", "tattooFront", "policeBelt", "policeVest", "policeHat",
	"hatStyle", "hatColor", "glasseStyle", "glasseColor", "maskStyle", "maskColor", "earStyle", "earColor",
	"necklaceStyle", "necklaceColor", "clockStyle", "clockColor", "backpackStyle", "backpackColor"
};''')
rep('''	0, 0, 0, 0,
	0, 0, 0, 0, 0
};''', '''	0, 0, 0, 0,
	0, 0, 0, 0, 0,
	0, 1, 0, 1, 0, 1, 0, 1,
	0, 1, 0, 1, 0, 1
};''')

# os corpos que o APK já suporta (SkinBodies.h): 163, 9, 289, 12, 7 + polícia 246 / 280
rep('	return (skin == 9 || skin == 163 || skin == 246 || skin == 280);',
    '	return (skin == 9 || skin == 163 || skin == 246 || skin == 280 || skin == 289 || skin == 12 || skin == 7);')

# Cloth_Load: aceita 25 ou 39 valores
rep('''	new name[MAX_PLAYER_NAME + 1], query[160], look[160], f[12];
	GetPlayerName(playerid, name, sizeof name);
	format(query, sizeof query, "SELECT skin, look FROM clothes WHERE name = '%s'", name);''',
    '''	new name[MAX_PLAYER_NAME + 1], query[160], look[320], f[12];
	GetPlayerName(playerid, name, sizeof name);
	format(query, sizeof query, "SELECT skin, look FROM clothes WHERE name = '%s'", name);''')
rep('''		if (n == CLOTH_COUNT && Cloth_IsValidSkin(skin))
		{
			for (new i = 0; i < CLOTH_COUNT; i++)''', '''		if (n >= CLOTH_BASE_COUNT && Cloth_IsValidSkin(skin))
		{
			for (new i = 0; i < n; i++)''')
# Cloth_SaveToDB
rep('''	new name[MAX_PLAYER_NAME + 1], query[320], look[160], part[8];
	GetPlayerName(playerid, name, sizeof name);
	look[0] = 0;
	for (new i = 0; i < CLOTH_COUNT; i++)
	{
		format(part, sizeof part, "%s%d", (i > 0 ? (" ") : ("")), g_ClothV[playerid][i]);
		strcat(look, part, sizeof look);
	}
	format(query, sizeof query, "INSERT OR REPLACE INTO clothes''', '''	new name[MAX_PLAYER_NAME + 1], query[480], look[320], part[8];
	GetPlayerName(playerid, name, sizeof name);
	look[0] = 0;
	for (new i = 0; i < CLOTH_COUNT; i++)
	{
		format(part, sizeof part, "%s%d", (i > 0 ? (" ") : ("")), g_ClothV[playerid][i]);
		strcat(look, part, sizeof look);
	}
	format(query, sizeof query, "INSERT OR REPLACE INTO clothes''')
# JSON maior + campo "shop" (0 = editor normal; 1..4 = loja aberta)
rep('''	new json[800], part[56];
	format(json, sizeof json, "{\\"id\\":%d,\\"skin\\":%d,\\"saved\\":%d,\\"msg\\":\\"%s\\"", ofid, g_ClothSkin[ofid], g_ClothSaved[ofid], msg);''',
    '''	new json[1400], part[56];
	format(json, sizeof json, "{\\"id\\":%d,\\"skin\\":%d,\\"saved\\":%d,\\"shop\\":%d,\\"msg\\":\\"%s\\"", ofid, g_ClothSkin[ofid], g_ClothSaved[ofid], (toid == ofid ? g_UtgShop[ofid] : 0), msg);''')
# Cloth_DoSave: acessórios opcionais + regras das lojas
rep('''		if (!Bank_NextToken(params, idx, tok, sizeof tok))
		{
			Cloth_Msg(playerid, CLOTH_COL_ERR, "Vetements : donnees incompletes, rien n'a ete enregistre.");
			return 0;
		}''', '''		if (!Bank_NextToken(params, idx, tok, sizeof tok))
		{
			if (i >= CLOTH_BASE_COUNT) { vals[i] = g_ClothV[playerid][i]; continue; }   // ancien client : 25 valeurs
			Cloth_Msg(playerid, CLOTH_COL_ERR, "Vetements : donnees incompletes, rien n'a ete enregistre.");
			return 0;
		}''')
rep('''#if CLOTH_PRICE > 0
	if (GetPlayerMoney(playerid) < CLOTH_PRICE)''', '''	// [UTGAWA] creation = gratuite ; ensuite seules les boutiques changent la tenue (et font payer)
	if (!UtgShop_OnSave(playerid, skin, vals)) return 0;
#if CLOTH_PRICE > 0
	if (GetPlayerMoney(playerid) < CLOTH_PRICE)''')
# /vetements e /clothact open: só na criação (ou staff)
rep('''		else if (!strcmp(action, "open", true) && g_AuthLogged[playerid])
		{
			if (!g_ClothLoaded[playerid]) Cloth_Load(playerid);
			Cloth_Send(playerid, CLOTH_DIALOG_OPEN);
		}''', '''		else if (!strcmp(action, "open", true) && g_AuthLogged[playerid])
		{
			if (!g_ClothLoaded[playerid]) Cloth_Load(playerid);
			if (!UtgShop_CanFreeEdit(playerid)) return 1;
			g_UtgShop[playerid] = 0;
			Cloth_Send(playerid, CLOTH_DIALOG_OPEN);
		}''')
rep('''		if (!g_AuthLogged[playerid]) return 1;
		if (!g_ClothLoaded[playerid]) Cloth_Load(playerid);
		Cloth_Send(playerid, CLOTH_DIALOG_OPEN);
		return 1;''', '''		if (!g_AuthLogged[playerid]) return 1;
		if (!g_ClothLoaded[playerid]) Cloth_Load(playerid);
		if (!UtgShop_CanFreeEdit(playerid)) return 1;
		g_UtgShop[playerid] = 0;
		Cloth_Send(playerid, CLOTH_DIALOG_OPEN);
		return 1;''')
rep('''		Cloth_Msg(playerid, CLOTH_COL_INFO, "Astuce : utilise /vetements pour personnaliser ton personnage.");''',
    '''		Cloth_Msg(playerid, CLOTH_COL_INFO, "Astuce : utilise /vetements pour creer ton personnage (gratuit). Ensuite : boutiques sur la carte.");''')

# vestiário: 39 valores também
rep('''	new name[MAX_PLAYER_NAME + 1], query[320], look[160], part[8];
	GetPlayerName(playerid, name, sizeof name);
	look[0] = 0;
	for (new i = 0; i < CLOTH_COUNT; i++)
	{
		format(part, sizeof part, "%s%d", (i > 0 ? (" ") : ("")), g_ClothV[playerid][i]);
		strcat(look, part, sizeof look);
	}
	format(query, sizeof query, "INSERT OR REPLACE INTO locker_civil''', '''	new name[MAX_PLAYER_NAME + 1], query[480], look[320], part[8];
	GetPlayerName(playerid, name, sizeof name);
	look[0] = 0;
	for (new i = 0; i < CLOTH_COUNT; i++)
	{
		format(part, sizeof part, "%s%d", (i > 0 ? (" ") : ("")), g_ClothV[playerid][i]);
		strcat(look, part, sizeof look);
	}
	format(query, sizeof query, "INSERT OR REPLACE INTO locker_civil''')
rep('''		new name[MAX_PLAYER_NAME + 1], query[128], f[12], look[160];''', '''		new name[MAX_PLAYER_NAME + 1], query[128], f[12], look[320];''')
rep('''				if (n == CLOTH_COUNT && Cloth_IsValidSkin(skin) && !Locker_IsPoliceSkin(skin))
				{
					for (new i = 0; i < CLOTH_COUNT; i++)''', '''				if (n >= CLOTH_BASE_COUNT && Cloth_IsValidSkin(skin) && !Locker_IsPoliceSkin(skin))
				{
					for (new i = 0; i < n; i++)''')

# ------------------------------------------------------------------ 2) módulo das lojas
MODULE = open(sys.argv[3] if len(sys.argv) > 3 else 'utg_lojas_module.pwn', encoding='utf-8').read()
anchor = '''// ============================================================
//  FACTIONS + VESTIAIRES (POLICE)'''
rep(anchor, MODULE + '\n' + anchor)
# a variável é usada no Cloth_SendLook (antes do módulo): declara lá em cima
rep('''new g_ClothLastSave[MAX_PLAYERS];''', '''new g_ClothLastSave[MAX_PLAYERS];
new g_UtgShop[MAX_PLAYERS];          // [UTGAWA] boutique ouverte (0 = aucune, 1 vetements, 2 accessoires, 3 barbier, 4 tatouage)''')

# ------------------------------------------------------------------ 3) callbacks
rep('''	Cloth_OnInit();
	Locker_OnInit();''', '''	Cloth_OnInit();
	UtgShop_OnInit();
	Locker_OnInit();''')
rep('''	Cloth_OnExit();
	Bank_OnExit();''', '''	Cloth_OnExit();
	UtgShop_OnExit();
	Bank_OnExit();''')
rep('''	Cloth_OnConnect(playerid);
	Locker_OnConnect(playerid);''', '''	Cloth_OnConnect(playerid);
	UtgShop_OnConnect(playerid);
	Locker_OnConnect(playerid);''')
rep('''	Cloth_OnSpawn(playerid);
	Admin_OnSpawn(playerid);''', '''	Cloth_OnSpawn(playerid);
	UtgShop_OnSpawn(playerid);
	Admin_OnSpawn(playerid);''')
rep('''	if (Cloth_OnCommand(playerid, cmdtext)) return 1;
	if (Admin_OnCommand(playerid, cmdtext)) return 1;''', '''	if (UtgShop_OnCommand(playerid, cmdtext)) return 1;
	if (Cloth_OnCommand(playerid, cmdtext)) return 1;
	if (Admin_OnCommand(playerid, cmdtext)) return 1;''')
rep('''	Bank_OnPickup(playerid, pickupid);
	return 1;''', '''	Bank_OnPickup(playerid, pickupid);
	UtgShop_OnPickup(playerid, pickupid);
	return 1;''')

open(dst, 'w', encoding='latin-1', newline='').write(s)
print('ok')
