/*
    ============================================================
    skins.inc - registre des skins Crestwood (noms + affichage en jeu)
    Garder synchro avec data/skins.dat et client SkinRegistry.h
    ============================================================
    /skins        (admin) liste des skins en dialogue, selection = applique
    /skin <id>    (admin) applique un skin et affiche son nom
    Noms en ASCII (pas d'accents) : le client SA-MP les affiche mal sinon.
*/

#if defined _skins_included
    #endinput
#endif
#define _skins_included

#define DIALOG_SKINS    7400
#define SKINS_COUNT     57

forward Skins_Apply(playerid, skinid);

new g_SkinIds[SKINS_COUNT] = {
    9, 10, 31, 38, 39, 53, 54, 56, 69, 76, 88, 89, 91, 92,
    129, 130, 131, 138, 139, 140, 141, 148, 150, 151, 157, 163, 169, 190,
    191, 194, 195, 196, 197, 198, 199, 201, 215, 218, 219, 224, 225, 226,
    231, 232, 245, 246, 251, 263, 280, 281, 282, 283, 284, 285, 286, 288, 289
};

// 0 = masculin, 1 = feminin
new g_SkinGender[SKINS_COUNT] = {
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 1, 1, 1, 0, 0
};

// 1 = supporte le systeme de clothes (peau/vetements)
new g_SkinClothes[SKINS_COUNT] = {
    1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1
};

// Skin choisi par le joueur (0 = aucun) : reapplique a chaque spawn de la session
new g_SkinChoice[MAX_PLAYERS];

new g_SkinNames[SKINS_COUNT][32] = {
    "Femme (base creation)",
    "Femme agee",
    "Habitante du desert",
    "Femme latina chic",
    "Femme latina mature",
    "Femme orientale chic",
    "Femme orientale mature",
    "Jeune femme orientale",
    "Jeune femme decontractee",
    "Hotesse de l air",
    "Femme blanche chic",
    "Femme blanche mature",
    "Femme riche",
    "Patineuse",
    "Fermiere 1",
    "Fermiere 2",
    "Jeune fermiere",
    "Baigneuse blanche",
    "Baigneuse noire",
    "Baigneuse latina",
    "Femme d affaires 1",
    "Femme d affaires 2",
    "Femme d affaires 3",
    "Jeune citadine 1",
    "Campagnarde",
    "Homme (base creation)",
    "Femme elegante",
    "Barbara",
    "Helena",
    "Millie",
    "Denise",
    "Fermiere agee",
    "Campagnarde agee",
    "Jeune fermiere 1",
    "Jeune fermiere 2",
    "Jeune citadine 2",
    "Femme noire chic",
    "Femme noire mature",
    "Femme noire riche",
    "Femme chic 2",
    "Femme mature 2",
    "Jeune femme 2",
    "Femme blanche chic 2",
    "Femme blanche mature 2",
    "Femme tenue de soiree",
    "Policiere (base)",
    "Maitre-nageuse",
    "Femme decontractee 3",
    "Policier LSPD (base)",
    "Policier SFPD",
    "Policier LVPD",
    "Sherif du comte",
    "Policiere LSPD",
    "Policiere SWAT",
    "Agente federale",
    "Pilote helico police",
    "Homme Crestwood (clothes)"
};

stock Skins_GetIndex(skinid)
{
    for (new i = 0; i < SKINS_COUNT; i++)
    {
        if (g_SkinIds[i] == skinid) return i;
    }
    return -1;
}

stock Skins_IsRegistered(skinid)
{
    return (Skins_GetIndex(skinid) != -1);
}

stock Skins_GetName(skinid, dest[], size = sizeof(dest))
{
    new idx = Skins_GetIndex(skinid);
    if (idx == -1) format(dest, size, "Skin %d", skinid);
    else           format(dest, size, "%s", g_SkinNames[idx]);
}

// Skin a utiliser au spawn : le choix du joueur s'il est valide, sinon le skin de base
stock Skins_Resolve(playerid, baseSkin)
{
    if (g_SkinChoice[playerid] != 0 && Skins_IsRegistered(g_SkinChoice[playerid])) return g_SkinChoice[playerid];
    return baseSkin;
}

// Demande du client (packet 253, cmd 4) : skin du registre, visible par tous
stock Skins_OnClientRequest(playerid, skinid)
{
    if (!Skins_IsRegistered(skinid)) return 0;
    Skins_Apply(playerid, skinid);
    return 1;
}

stock Skins_ShowDialog(playerid)
{
    new list[2400], line[64];
    for (new i = 0; i < SKINS_COUNT; i++)
    {
        format(line, sizeof(line), "%d - %s\n", g_SkinIds[i], g_SkinNames[i]);
        strcat(list, line);
    }
    ShowPlayerDialog(playerid, DIALOG_SKINS, DIALOG_STYLE_LIST, "Skins", list, "Appliquer", "Fermer");
}

stock Skins_Apply(playerid, skinid)
{
    g_SkinChoice[playerid] = skinid;
    new name[32], msg[96];
    SetPlayerSkin(playerid, skinid);
    Skins_GetName(skinid, name);
    format(msg, sizeof(msg), "Skin %d : %s", skinid, name);
    SendClientMessage(playerid, 0x7C5BFFFF, msg);
}

// Retourne 1 si la commande a ete geree (a appeler dans OnPlayerCommandText)
stock Skins_OnCommand(playerid, cmdtext[])
{
    if (!strcmp("/skins", cmdtext, true))
    {
        if (!IsPlayerAdmin(playerid))
        {
            SendClientMessage(playerid, 0xFF6060FF, "Commande reservee aux admins.");
            return 1;
        }
        Skins_ShowDialog(playerid);
        return 1;
    }
    if (!strcmp("/skin ", cmdtext, true, 6))
    {
        if (!IsPlayerAdmin(playerid))
        {
            SendClientMessage(playerid, 0xFF6060FF, "Commande reservee aux admins.");
            return 1;
        }
        new id = strval(cmdtext[6]);
        if (id < 0 || id > 311 || id == 74)
        {
            SendClientMessage(playerid, 0xFF6060FF, "Utilisation : /skin [0-311]");
            return 1;
        }
        Skins_Apply(playerid, id);
        return 1;
    }
    return 0;
}

// Retourne 1 si le dialogue a ete gere (a appeler dans OnDialogResponse)
stock Skins_OnDialogResponse(playerid, dialogid, response, listitem)
{
    if (dialogid != DIALOG_SKINS) return 0;
    if (!response) return 1;
    if (!IsPlayerAdmin(playerid)) return 1;
    if (listitem < 0 || listitem >= SKINS_COUNT) return 1;
    Skins_Apply(playerid, g_SkinIds[listitem]);
    return 1;
}
