/* ============================================================
   CRESTWOOD RP v22 (12/11/2026)
   ------------------------------------------------------------
   1) HOTBAR CRESTWOOD : les 5 emplacements rapides de l'inventaire s'affichent sur le HUD
      (onglet a droite de l'ecran, "+N" = objets prets). Toucher un emplacement = utiliser l'objet.
      Activation : AUTOMATIQUE. Si tu n'as encore rien range, tes consommables (eau, burger, bandage,
      vody...) sont ranges tout seuls. Pour choisir : Inventaire > un objet > "Emplacement rapide".
      /hotbar = masquer / afficher   /hotbar 1-5 = utiliser l'emplacement
      Serveur -> HUD : ShowPlayerDialog(9840, "@hotbar", {"slots":[["eau_kirene",3],...],"show":1,"jbl":0})
   2) PRIX : le staff (niveau 6) change le prix de chaque article de la boutique 24/7 depuis le
      panneau admin (onglet Serveur > "Prix de la boutique"). Sauvegarde : table shop_prices (inventaire).
   3) ANIMATIONS : le petit bouton du HUD ouvre une mini page a droite (anim-mini.js) -> /animq <cle> | stop.
      /anim <cle> et /stopanim marchent aussi au clavier.
   Inclus apres <Cw21>. Texte ASCII.
   ============================================================ */
#define HB_DIALOG             9840
#define HB_TICK_MS            2500
#define PRICE_MIN             1
#define PRICE_MAX             2000000

new g_HbLast[MAX_PLAYERS][200];
new g_HbHide[MAX_PLAYERS];
new g_HbAuto[MAX_PLAYERS];          // remplissage automatique deja tente
new g_AnimQLast[MAX_PLAYERS];       // anti-spam des animations rapides
forward Cw22_Tick();

stock Cw22_OnInit()
{
	if (g_InvDB != DB:0)
	{
		db_free_result(db_query(g_InvDB, "CREATE TABLE IF NOT EXISTS shop_prices (item TEXT PRIMARY KEY, price INTEGER)"));
		new DBResult:r = db_query(g_InvDB, "SELECT item, price FROM shop_prices"), it[24], f[16], n = 0;
		if (r != DBResult:0)
		{
			for (new i = 0, rows = db_num_rows(r); i < rows; i++)
			{
				db_get_field(r, 0, it, sizeof it);
				db_get_field(r, 1, f, sizeof f);
				new k = Shop_ItemIndex(it), p = strval(f);
				if (k != -1 && p >= PRICE_MIN && p <= PRICE_MAX) { g_ShopItems[k][siPrice] = p; n++; }
				db_next_row(r);
			}
			db_free_result(r);
		}
		if (n) printf("[PRIX] %d prix de boutique personnalises charges.", n);
	}
	SetTimer("Cw22_Tick", HB_TICK_MS, true);
	return 1;
}

stock Cw22_OnConnect(playerid)
{
	g_HbLast[playerid][0] = 0;
	g_HbHide[playerid] = 0;
	g_HbAuto[playerid] = 0;
	g_AnimQLast[playerid] = 0;
	return 1;
}

stock Cw22_OnSpawn(playerid)
{
	g_HbLast[playerid][0] = 0;   // renvoie la barre au prochain tour
	return 1;
}

// ------------------------------------------------------------
//  1) Hotbar
// ------------------------------------------------------------
stock Hb_AutoFill(playerid)
{
	g_HbAuto[playerid] = 1;
	if (g_InvDB == DB:0) return 0;
	if (!g_InvSlotLoaded[playerid]) Inv_LoadSlots(playerid);
	for (new k = 0; k < INV_SLOTS; k++) if (g_InvSlot[playerid][k] >= 0) return 0;   // deja choisi par le joueur
	static const pref[][] = { "eau_kirene", "hamburger", "bandage", "vody", "enceinte_jbl", "tablette_job", "gilet_tactique" };
	new name[MAX_PLAYER_NAME + 1], q[180], k = 0;
	GetPlayerName(playerid, name, sizeof name);
	for (new i = 0; i < sizeof(pref) && k < INV_SLOTS; i++)
	{
		new it = Inv_ItemIndex(pref[i]);
		if (it < 0 || Inv_GetQty(playerid, pref[i]) <= 0) continue;
		g_InvSlot[playerid][k] = it;
		k++;
		format(q, sizeof q, "INSERT INTO inv_slots (name, slot, item) VALUES ('%s', %d, '%s')", name, k, pref[i]);
		db_free_result(db_query(g_InvDB, q));
	}
	return k;
}

stock Hb_Build(playerid, out[], size)
{
	new part[40], qty[INV_SLOTS];
	if (!g_InvSlotLoaded[playerid]) Inv_LoadSlots(playerid);
	// une seule requete pour les 5 emplacements (le tour passe toutes les 2,5 s)
	if (g_InvDB != DB:0)
	{
		new name[MAX_PLAYER_NAME + 1], q[140], it[24], f[12];
		GetPlayerName(playerid, name, sizeof name);
		format(q, sizeof q, "SELECT item, qty FROM inv_items WHERE name = '%s'", name);
		new DBResult:r = db_query(g_InvDB, q);
		if (r != DBResult:0)
		{
			for (new i = 0, rows = db_num_rows(r); i < rows; i++)
			{
				db_get_field(r, 0, it, sizeof it);
				db_get_field(r, 1, f, sizeof f);
				new idx = Inv_ItemIndex(it);
				for (new k = 0; k < INV_SLOTS; k++) if (idx >= 0 && g_InvSlot[playerid][k] == idx) qty[k] = strval(f);
				db_next_row(r);
			}
			db_free_result(r);
		}
	}
	format(out, size, "{\"slots\":[");
	for (new k = 0; k < INV_SLOTS; k++)
	{
		new it = g_InvSlot[playerid][k];
		if (it >= 0 && it < INV_ITEM_COUNT) format(part, sizeof part, "%s[\"%s\",%d]", k ? (",") : (""), g_InvItems[it], qty[k]);
		else format(part, sizeof part, "%s[\"\",0]", k ? (",") : (""));
		strcat(out, part, size);
	}
	format(part, sizeof part, "],\"show\":%d,\"jbl\":0}", g_HbHide[playerid] ? 0 : 1);
	strcat(out, part, size);
	return 1;
}

stock Hb_Refresh(playerid, force = 0)
{
	if (!IsPlayerConnected(playerid) || !g_AuthLogged[playerid] || IsPlayerNPC(playerid)) return 0;
	if (!g_HbAuto[playerid]) Hb_AutoFill(playerid);
	new json[200];
	Hb_Build(playerid, json, sizeof json);
	if (!force && !strcmp(json, g_HbLast[playerid], false) && g_HbLast[playerid][0]) return 0;
	format(g_HbLast[playerid], 200, "%s", json);
	return ShowPlayerDialog(playerid, HB_DIALOG, DIALOG_STYLE_MSGBOX, "@hotbar", json, "OK", "");
}

stock Hb_Use(playerid, slot)
{
	if (slot < 1 || slot > INV_SLOTS) return 1;
	if (!g_InvSlotLoaded[playerid]) Inv_LoadSlots(playerid);
	new it = g_InvSlot[playerid][slot - 1];
	if (it < 0) return Cw_Notify(playerid, "warning", "Hotbar", "Emplacement vide : range un objet depuis ton inventaire.", 3500), 1;
	if (IsPlayerInAnyVehicle(playerid) && it != Inv_ItemIndex("eau_kirene") && it != Inv_ItemIndex("hamburger") && it != Inv_ItemIndex("vody"))
		return Cw_Notify(playerid, "denied", "Hotbar", "Descends du vehicule pour utiliser cet objet.", 3500), 1;
	new have = Inv_GetQty(playerid, g_InvItems[it]);
	if (have <= 0) return Cw_Notify(playerid, "denied", "Hotbar", "Tu n'en as plus : passe a la boutique 24/7.", 3500), 1;
	if (!strcmp(g_InvItems[it], "enceinte_jbl")) { Jbl_Use(playerid); return 1; }
	new msg[128];
	if (Inv_Use(playerid, it, msg, sizeof msg))
	{
		Inv_SetQty(playerid, g_InvItems[it], have - 1);
		if (msg[0]) Cw_Notify(playerid, "success", "Hotbar", msg, 3500);
	}
	else if (msg[0]) Cw_Notify(playerid, "denied", "Hotbar", msg, 3500);
	Hb_Refresh(playerid, 1);
	return 1;
}

public Cw22_Tick()
{
	for (new p = 0, mx = GetMaxPlayers(); p < mx; p++)
	{
		if (IsPlayerConnected(p) && g_AuthLogged[p]) Hb_Refresh(p);
	}
	return 1;
}

// ------------------------------------------------------------
//  2) Prix de la boutique (panneau admin)
// ------------------------------------------------------------
stock Price_AdminNeed(const action[])
{
	if (!strcmp(action, "priceset", true) || !strcmp(action, "pricereset", true)) return 6;
	if (!strcmp(action, "pricelist", true)) return 2;
	return 0;
}

stock Price_AdminSend(playerid)
{
	new json[900], part[64];
	format(json, sizeof json, "{\"x\":\"prices\",\"items\":[");
	for (new i = 0; i < sizeof(g_ShopItems); i++)
	{
		format(part, sizeof part, "%s[%d,\"%s\",%d]", i ? (",") : (""), i, g_ShopItems[i][siItem], g_ShopItems[i][siPrice]);
		strcat(json, part, sizeof json);
	}
	strcat(json, "]}", sizeof json);
	return ShowPlayerDialog(playerid, ADMIN_DIALOG_UPDATE, DIALOG_STYLE_MSGBOX, "@admin", json, "OK", "");
}

stock Price_AdminAction(playerid, const action[], const tid[], const arg[], msg[], msgsize, &ok)
{
	if (!Price_AdminNeed(action)) return 0;
	if (!strcmp(action, "pricelist", true)) { Price_AdminSend(playerid); ok = 1; format(msg, msgsize, "Prix de la boutique."); return 1; }
	new i = strval(tid);
	if (i < 0 || i >= sizeof(g_ShopItems)) { format(msg, msgsize, "Article inconnu."); return 1; }
	new q[160], p;
	if (!strcmp(action, "pricereset", true))
	{
		format(q, sizeof q, "DELETE FROM shop_prices WHERE item = '%s'", g_ShopItems[i][siItem]);
		if (g_InvDB != DB:0) db_free_result(db_query(g_InvDB, q));
		format(msg, msgsize, "Prix de %s : retour au prix d'origine au prochain redemarrage.", g_ShopItems[i][siItem]);
		ok = 1;
		Price_AdminSend(playerid);
		return 1;
	}
	p = strval(arg);
	if (p < PRICE_MIN || p > PRICE_MAX) { format(msg, msgsize, "Prix invalide (%d a %d $).", PRICE_MIN, PRICE_MAX); return 1; }
	g_ShopItems[i][siPrice] = p;
	format(q, sizeof q, "INSERT OR REPLACE INTO shop_prices (item, price) VALUES ('%s', %d)", g_ShopItems[i][siItem], p);
	if (g_InvDB != DB:0) db_free_result(db_query(g_InvDB, q));
	new a[16];
	Cw_Fmt(p, a, sizeof a);
	format(msg, msgsize, "Nouveau prix de %s : %s $.", g_ShopItems[i][siItem], a);
	ok = 1;
	Price_AdminSend(playerid);
	return 1;
}

// ------------------------------------------------------------
//  3) Animations rapides (mini page du HUD : /animq <cle> | /animq stop ; /anim <cle> ; /stopanim)
// ------------------------------------------------------------
stock Anim_Quick(playerid, const key[])
{
	new now = GetTickCount();
	if (now - g_AnimQLast[playerid] < 400 && g_AnimQLast[playerid] != 0) return 1;
	g_AnimQLast[playerid] = now;
	if (!key[0]) return Cw_Notify(playerid, "info", "Animations", "Touche le bouton des animations (en bas a gauche) ou tape ~b~/anim nom~w~.", 4500), 1;
	if (!strcmp(key, "stop", true)) { Rp_AnimStop(playerid); return 1; }
	if (GetPlayerState(playerid) != PLAYER_STATE_ONFOOT) return Cw_Notify(playerid, "denied", "Animations", "Descends du vehicule d'abord.", 3000), 1;
	if (GetPlayerSpecialAction(playerid) == SPECIAL_ACTION_CUFFED) return Cw_Notify(playerid, "denied", "Animations", "Impossible : tu es menotte.", 3000), 1;
	for (new i = 0; i < RP_ANIM_COUNT; i++)
	{
		if (strcmp(key, g_RpAnim[i][0], true) != 0) continue;
		Rp_AnimStop(playerid);
		new mode = g_RpAnimMode[i];
		if (mode >= 100) SetPlayerSpecialAction(playerid, mode - 100);
		else ApplyAnimation(playerid, g_RpAnim[i][1], g_RpAnim[i][2], 4.1, mode, 0, 0, 0, 0);
		format(g_RpCur[playerid], 12, "%s", g_RpAnim[i][0]);
		return 1;
	}
	return Cw_Notify(playerid, "denied", "Animations", "Animation inconnue.", 3000), 1;
}

// ------------------------------------------------------------
stock Cw22_OnCommand(playerid, const cmdtext[])
{
	if (!g_AuthLogged[playerid]) return 0;
	new a;
	if ((a = Cw_Arg(cmdtext, "/animq")) != -1) return Anim_Quick(playerid, cmdtext[a]);
	if ((a = Cw_Arg(cmdtext, "/anim")) != -1) return Anim_Quick(playerid, cmdtext[a]);
	if (!strcmp(cmdtext, "/stopanim", true)) { Rp_AnimStop(playerid); return 1; }
	if ((a = Cw_Arg(cmdtext, "/hotbar")) != -1)
	{
		if (!cmdtext[a])
		{
			g_HbHide[playerid] = !g_HbHide[playerid];
			Hb_Refresh(playerid, 1);
			Cw_Notify(playerid, "info", "Hotbar", g_HbHide[playerid] ? ("Hotbar masquee. Retape /hotbar pour la revoir.") : ("Hotbar affichee a droite de l'ecran."), 3500);
			return 1;
		}
		return Hb_Use(playerid, strval(cmdtext[a]));
	}
	return 0;
}
