# Notes projet CrestwoodRP

## Serveur SA-MP
- Le vrai gamemode est `server/basicgm/basicgm1.pwn` (12748 lignes) — PAS `server/GmLurion/gamemodes/GM.pwn` (ancien, plus petit).
- `basicgm1.pwn` est encodé Latin-1/CP1252 : `read_file`/`search_grep` échouent avec « Failed to decode utf-8 ». Lire/patcher via `python3` en décodant en `latin-1`.
- Compilation (fonctionne dans le sandbox ARM64) :
  `cd server/basicgm && LD_LIBRARY_PATH=../../../tools/pawnc-aarch64 ../../../tools/pawnc-aarch64/pawncc basicgm1.pwn -obasicgm1.amx -i../../../tools/samp-includes -p../../../tools/samp_compat_prefix.inc`
  - Compilateur : pawncc 3.10.10 (pawn-lang/compiler) compilé depuis les sources dans `tools/pawnc-aarch64/` (binaire aarch64 + libpawnc.so).
  - Includes SA-MP : `tools/samp-includes/` (jeu open.mp 1.6.7, équivalent 0.3.7).
  - `-p tools/samp_compat_prefix.inc` définit `SAMP_COMPAT` : sans lui, les tags open.mp (WEAPON:, KEY:…) font échouer 4 publics en « function heading differs from prototype ».
  - Résultat attendu : 0 erreur, ~38 warnings (bool/const, bénins), `.amx` ≈ 1 009 310 octets, file_version 8 (compatible SA-MP 0.3.7).
  - `basicgm1.amx.bak` = build d'avant le correctif caméra (rollback possible).
- L'authentification est intégrée au gamemode (dialogues 9520 ouverture / 9521 réponse / 9522 fermeture, préfixe `@auth`). Le client Android les intercepte dans `SAMP.java` → `handleAuthDialog` (page `assets/login/index.html`, pont `SampAuthBridge`).
- Flux de connexion : `Auth_OnRequestClass` (9520) → login via `/authact login …` → `Auth_Finish` (9522 + `SpawnPlayer` après 900 ms) → `OnPlayerSpawn`.

## Pièges résolus
- Joueurs figés au spawn : la caméra était posée exactement sur la position du joueur dans `OnPlayerRequestClass` (vecteur de regard nul) et jamais réinitialisée. Correctif : caméra décalée dans `OnPlayerRequestClass` + `SetCameraBehindPlayer(playerid)` en tête de `OnPlayerSpawn`.

- Menu RP (`server/basicgm/include/Rp.inc`) : inclus APRES tous les systemes dans `basicgm1.pwn` (juste avant `main()`). Protocole complet de la page `assets/rp/index.html`. `/rpact close` ne doit JAMAIS renvoyer de dialogue (sinon la page se rouvre en boucle et le joueur reste fige) ; une action inconnue ne fait rien.
- Niveaux staff : 0 a 10 (`ADMIN_LVL_MAX`), memes noms que `ROLES` dans `assets/admin/admin.js`. `Anzo_Smith` (`ADMIN_OWNER_NAME`) = niveau 10 (Fondateur) apres connexion. Un staff ne peut gerer que des niveaux inferieurs au sien.
- Apres toute modification de `basicgm1.pwn` / `Rp.inc` : RECOMPILER l'.amx (commande ci-dessus), l'.amx du zip n'est pas regenere automatiquement.

## Client Android (STRUCTURE UNIFIEE, comme CAMPINA - 31/10/2026)
- UN SEUL client natif : `app/src/main/cpp/samp` (ex-samp32), moteur GTA SA 2.10 `libGTASA.so` en armeabi-v7a ET arm64-v8a. Offsets via `VER_x32 ? 32 : 64`.
  Le client 2.11 (libGame.so, ancien cpp/samp 64 bits) est SUPPRIME : il plantait a la connexion (libopenal.so / mali).
- jniLibs arm64-v8a = libGTASA.so + libOpenAL64.so + libSCAnd.so (2.10 64 bits, pris de CAMPINA, debug retire avec llvm-strip).
- Java : une seule activite de jeu `com.gta.game32.SAMP` (le lanceur la lance toujours). `com.gta.game.SAMP/GTASA` et `com.rockstargames.gtasa` supprimes ; `com.gta.game.ui` + `HudWebView` restent (partages).
- Verifier une adresse 64 bits : `llvm-nm -D app/src/main/jniLibs/arm64-v8a/libGTASA.so` (le nom du symbole doit etre le meme qu'en 32 bits). Script d'audit utilise : comparaison nom32 == nom64 de chaque `VER_x32 ? a : b`.
- WebView : deux couches `hudMain` (HUD, jamais recharge) + `hudPanel` (panneaux) ; `navigate(url)` remplace `hudWebView.loadUrl` ; `hudWebView` = couche active. Les ponts SampHud/AndroidHUD/AndroidClothes recoivent leur couche (`owner`). `assets/common/cw-perf.css` en derniere feuille de chaque page (pas de backdrop-filter).
- Page RP : `rpClosedAtMs` ignore un dialogue 9690 recu moins de 2 s apres une fermeture par le joueur (anti-boucle, meme avec un ancien serveur).
- `HudWebView` : les touches traversent vers le jeu sauf zones HUD déclarées par JS (`SampHud.setTouchBounds`) ou quand `fullscreenPage`/`phonePage`/`interactive` sont actifs.

## Magasins de vetements + spawn RP
- Module `MAGASINS DE VETEMENTS` dans `basicgm1.pwn` (juste avant le module ADMINISTRATION) : table `clothshops` dans `crestwood_clothes.db`, pin = pickup 1239 + texte 3D, main d'interaction via `INTERACT_CLOTHSHOP` (cle 90, id HUD `clothshop`), ouverture = `Cloth_Send(playerid, CLOTH_DIALOG_OPEN)`.
- Admin : actions `cshoplist|cshopadd|cshopdel|cshopgoto` (niveau 4 mini) ; la liste est envoyee a part au panneau (`{"x":"cshop","shops":[...]}`, dialogue 9541) car le JSON principal est plein. Carte dans l'onglet Factions de `assets/admin/admin.js`.
- Spawn RP : `RP_SPAWN_*` + `Spawn_Apply()` (appelee au debut de `OnPlayerSpawn`, avant les modules qui peuvent la surcharger).
- Bouton d'interaction du HUD : main noire dans un contour bleu (bloc CSS en fin de `hud.css`).


## Controles tactiles v2 (joystick / tir / conduite)
- HUD : `assets/hud/touch-controls.js|css` + bloc "Controles tactiles Crestwood v2" dans `assets/hud/index.html`.
- Joystick flottant (zone `#tcStickZone`, gauche) -> `AndroidHUD.padStick(x, y, actif)` (-128..128) -> JNI `setHudStick` -> `HudPad_SetStick` (game/pad.cpp) : marche a pied, direction + inclinaison au volant. Pousser au-dela de la base = sprint.
- Boutons -> `vehicle:control` {control, pressed} -> `SAMP.dispatchVehicleControl` -> `setVehicleControl(id)` : 1-10 inchanges, 11 tir, 12 viser (bascule), 13/14 arme suivante/precedente, 15 monter passager, 20 mode controles Crestwood.
- Mode 20 (envoye au chargement du HUD) : les widgets tactiles natifs de SA (joystick, tir, viser, sprint, saut, conduite...) sont masques ET desactives (`Crestwood_IsReplacedWidget`, `Widgets/WidgetGta.cpp`, 64 et 32 bits). Si le HUD ne charge pas, les boutons natifs restent.
- Au volant : joystick ou fleches (bascule `#tcSteerMode`, memorisee dans localStorage `crestwoodSteerMode`).
- Les groupes de boutons ont un `data-hud-key` (tcCombat, tcPedals, tcExtra, tcVehActions, tcSteer) : deplacables dans l'editeur de HUD.
- La lib du moteur est `libGTASA.so` (32 et 64 bits) -> `CHook::GameLibName` dans `samp/vendor/armhook/patch.h`.

## Garages + marqueurs 3D + jauges minimap
- Serveur : module "GARAGES DES JOUEURS + MARQUEURS 3D" juste avant `main()` dans `basicgm1.pwn` (`crestwood_garage.db`). Interaction cle 90 (garage) / 100 (vetements), page `assets/garage/` ("@garage" 9750-9752, `/garact`), marqueurs "@marks" 9760.
- Client : `SAMP.handleGarageDialog` / `SampGarageBridge` / `handleWorldMarkers` -> JNI `setWorldMarkers("type,x,y,z,label;...")` -> `game/WorldMarkers.cpp` (fichier identique samp/ et samp32/, rendu depuis `UI::drawList`).
- HUD : `hud-ring.js|css` (arcs autour de la minimap sur les `data-status`, rond d'arme `#mmWeapon` -> `SampHud.weaponAction`). Icones d'armes blanches : `assets/hud/assets/weapons-white/` (generees depuis `weapons/`).
- Compilation Pawn testee avec pawncc 3.10.10 + samp-stdlib + pawn-stdlib (0 erreur, 5 warnings deja presents).

## Panneau admin : niveaux 0-10 + actions completes
- `Adm_Need(action)` (module "ADMINISTRATION (suite)" avant `main()`) = niveau requis, identique au `lvl` de `assets/admin/admin.js`. Verification unique en tete de `Admin_Action` ; les anciennes verifications internes sont neutralisees (`lvl < 0`).
- Niveaux 0..10 (`ADMIN_LVL_MAX`), migration auto des anciens niveaux (meta `levels10`). `Staff_SetLevel` : niveau 7 mini, en dessous du sien.
- `AdmX_Action` traite les actions ajoutees (ban duree/hors ligne/unban, gangs, cshop*, kill, god, jetpack, tpxyz, setvw/setint, screen, freezeall...). Donnees "x" (fac/gang/ban/log/cshop) envoyees a la reception de `replist` (ouverture du panneau).
- Peche (`Fish_*`, page fish 9740-9742) et maisons (`House_*`, page house 9730-9732) : modules juste avant `main()`, donnees dans `crestwood_garage.db`. `include/House.inc` (ancien, non inclus) n'est plus utilise.
- Verification rapide : toutes les actions `send("...")` / `k:` d'admin.js et toutes les commandes `c: "/..."` de l'onglet Commandes existent dans `basicgm1.pwn`.

## Generique / chargement / page JOUER (theme noir & blanc)
- Generique de demarrage : `launcher/ui/IntroOverlay.java` = diaporama `intro_slide_1|2` (Dodge SRT / moto, `res/drawable-nodpi`) en fondu enchaine + Ken Burns, legendes, bandes cinema, puis affiche finale `crestwood_intro.jpg` (le mec sur la voiture + logo). Duree totale 24 s (`SLIDE_PASSES * SLIDE_MS + FINALE_MS`), passable seulement apres 12 s (`SKIP_ALLOWED_AT_MS`), musique `raw/crestwood_intro` en boucle.
- Ecran de chargement (launcher + en jeu, 64 et 32 bits via `com.gta.game.ui.LoadingScreen`) : `assets/loading/` ; fond `scenes/crestwood-loading.webp` (affiche "le mec sur la voiture"), panneau en verre en bas pour garder le logo visible. Les ids HTML utilises par `loading.js` n'ont pas change.
- Page JOUER : `res/layout/main_activity.xml`, fond `launcher_home_background.jpg` (Brabus) en ImageView centerCrop + voile `drawable/launcher_background.xml`; boutons verre blanc (`menu_panel`, `menu_play_outline`, `social_button`, `glass_pill`).
- Icone : `drawable-nodpi/ic_launcher_foreground.png` (logo CREST WOOD RP) + `mipmap-*/ic_launcher(_round).webp`. L'ancien `drawable/ic_launcher_foreground.xml` (robot Android) a ete supprime.

## Boutiques UTGAWA + touches SA-MP du HUD + diagnostic mappings (30/10)
- Module `UTGAWA_LOJAS` dans `basicgm1.pwn` (avant FACTIONS) : boutiques vetements/accessoires/barbier/tatouage (`/criarroupa` ...), table `utg_shops` / `utg_owned`, `g_UtgShop[]` envoye en `"shop"` dans le JSON 9551. `UTGS_ADMIN_LVL` = 4. Les points vetements (t-shirt 3D, `Vet_Open`) ouvrent la boutique type 1 (`UtgShop_OpenAs(playerid, 1, -100 - slot)`). Doc : `LISEZMOI_BOUTIQUES_UTGAWA.txt`, scripts : `UTGAWA_INTEGRACAO/`.
- Barre native SA-MP (ButtonPanel TAB/ALT/H/Y/N...) masquee aussi en 32 bits ; chat natif 32 bits masque quand le HUD HTML est actif. AUCUN remplacement (Anzo ne veut pas de touches Y/N/H/ALT dans le HUD).
- Arme en main : ligne `#mmWeapon` (classe `weapon-ammo`) sous l argent dans `.hud-info-panel` (icone blanche + chargeur / reserve), cachee sans arme a feu ; argent en blanc (hud-ring.css). Plus de rond d arme dans la minimap.
- `CHook::UnFuck` aligne sur la vraie taille de page (`sysconf`, 4 Ko ou 16 Ko) au lieu de 4 Ko fixe (64 et 32 bits).
- `CStreaming::AddImageToList` : log "IMG ... presente / ABSENTE" dans svlog.txt et ouverture du nom en minuscules si seul celui-ci existe.
- `DataInstaller.REQUIRE_CURRENT_DATA_VERSION` : data dont le marqueur n'est pas `STATIC_VERSION` = retelechargee (changer URL + version a chaque modpack).

## Vetements Utgawa + Lasouth, main 3D, controles natifs (30/10/2026)
- Voir LISEZMOI_VETEMENTS_FUSION_ET_CONTROLES.txt. Lasouth RETIRE (meme pack qu Utgawa) : 39 valeurs d apparence, catalogue Utgawa seul. Selecteur Homme 1 (163) / Homme 2 (7) / Femme 1 (9) / Femme 2 (12).
- Main 3D : WorldMarkers::OnTouch (avant PlayerTap dans AND_TouchEvent_hook) -> /interagir. Boutiques Utgawa = cles 131..134 dans Interact_Key.
- Controles : touch-controls.js n'est plus charge ; id 20 "customControls" ignore dans pad.cpp ; ButtonPanel SA-MP visible.
- game/CombatButtons.cpp (identique 32/64) : VISER (HudPad 12) et TIRER passager (HudPad 11 + drive-by).
- Compilation GM hors sandbox ARM : pawncc 3.10.10 compile depuis github.com/pawn-lang/compiler + includes tools/pawn-windows/include (0.3.7).

## Vocal + telephone (30/10/2026)
- Voir LISEZMOI_MICRO_TELEPHONE.txt. GM : CW_VOICE (SampVoice 3.1, include/sampvoice.inc), canal DLStream par joueur, touche 0x42 ; module TELEPHONE (/phoneact, dialogue 9700 "@phone", crestwood_phone.db), objet "carte_sim" (inventaire index 8, 24/7).
- Client : Crestwood_EnsureRecord() dans voice(_new)/Plugin.cpp (relance BASS_RecordInit a l'appui) ; 64 bits sans anti-echo Speex.

## Streaming + mappings + systemes MTA (30/10/2026, v6)
- Streaming moteur : Crestwood_StreamingBudget() (RAM/4, 160..512 Mo) dans game/Streaming.cpp (32+64) ; hooks.cpp CStreaming__Init2_hook l'applique (32 bits : aussi ancienne variable 0x685FA0).
- GM : streamer integre v2 (grille 60 m, cap 850 objets/joueur, MS_PLAYER_CAP). Ne jamais depasser ~900 (limite client 1000 avec les objets globaux).
- Maps importees : stocks MapImp_* ; modeles personnalises derriere #define MAP_MODELES_DATA (0 par defaut) + dossier DATA_MAPPINGS (TXD renommes cw_* pour eviter les collisions). Voir LISEZMOI_MAPPINGS.txt.
- Modules Kit_* (dialog 9790, /kit, table kit_claimed) et Rob_* (dialog 9791, cles d'interaction 140..142, groupe 14 dans Interact_Do, objets globaux CreateObject 2930/1913).

## Vetements + skins dans l'APK (31/10/2026, v7)
- Lasouth remis (SouthClothes.h, SouthModelLoader, BrlTextureShader LoadSouthTexture, SAMP.java prepareClothesModel/loadClothesTexture) + groupes Utgawa (utgHat/utgGlasse/utgNecklace/utgTop) dans SouthClothes.h.
- Textures brl_* servies depuis l'APK via gui/UtgBrlMap.h (prioritaire sur la texdb). Generateur : tools/utg_apk/.
- 52 valeurs d apparence (client netgame/CharacterCreator, serveur CLOTH_COUNT 52, CLOTH_PIECE_FIRST 39). Vetements = APK uniquement (sauf police). 64 bits : ScrCreateObject/ScrSetObjectMaterial reactives (mappings). Barre SA-MP masquee.. Voir LISEZMOI_VETEMENTS_APK.txt.

## v9 (31/10/2026) : points 3D, RP PLUS, chat HTML, panneaux noirs
- Marqueurs : Mk_Add/Mk_Flush (9760 + 9761), types 1..15 (voir LISEZMOI_V9_RP_INTERFACE.txt), Cw_PushMarks en fin de GM. Pickups/textes 3D via Cw_Pickup/Cw_Label (CW_PICKUPS/CW_TEXT3D = 0).
- Module RP PLUS en fin de basicgm1.pwn (Rp_*), Fac_IsLaw (police/FBI/armee), vestiaires toutes factions (g_RpUni).
- Chat : Chat::addMessage -> CJavaWrapper::HudChatMessage -> SAMP.hudChatMessage -> HudApp.addChatMessage (hud-cw9.css).
- MAP_REMOVE_BUILDINGS 0. Panneaux recolores (tools/recolor_panels_noir.py).

## Tablette RP + codes promo (31/10/2026, v10)
- include/Rp.inc (tablette, prefixe Rp_) est maintenant inclus ; le module RP PLUS utilise le prefixe RpX_.
- include/Promo.inc : codes promo (vue "promo" de assets/rp, actions promouse/promoadd/promodel, liste blanche SAMP.java game+game32).

## v11 (31/10/2026) : inventaire v2, centrale forces de l'ordre
- Inventaire : /invact use|drop|give|slot|pick|refresh (SAMP.java game+game32), poids g_InvWeight, sol g_Gr*, raccourcis inv_slots.
- include/Law.inc : vue "law" de la tablette (Rp_Send), actions law* via Law_Action (Rp_Action), commandes /mdt /renfort /m /recherche /effacer /badge /barrage /dossier /alerte.

## v12 (31/10/2026) : services, banque FR, plus de bulles
- include/Svc.inc : essence (g_CwFuel, hook CreateVehicle -> Cw_CreateVehicle, Svc_Tick) + location ; page assets/svc (9810-9812, /svcact), SampSvcBridge dans les deux SAMP.java ; marqueurs 16/17.
- Bulles de chat coupees : serveur (plus de SetPlayerChatBubble) + client CW_CHAT_BUBBLES 0 (chat.cpp, netrpc.cpp, 32+64).
- Logo HUD masque (hud-cw9.css). Banque assets/bank reecrite en francais, meme API BankApp/SampBank.
- Compilation GM utilisee ici : /tmp/claude-0/pawnc/build/pawncc basicgm1.pwn -iinclude -i<pawn-windows/include 0.3.7> -d0 (5 warnings).
- v12g : adresses 64 bits verifiees contre `nm -D jniLibs/arm64-v8a/libGame.so` (5 fausses corrigees, dont RwCameraClear = crash des textes de mapping). Refaire cet audit apres tout portage de wrappers RW.

## v13 (31/10/2026) : panneau /mappings, points en jeu, corps dedies, identite
- include/Pts.inc : registre des mappings (MapReg_Begin/End autour de chaque MapObj_/MapImp_ dans MapObj_OnInit), portails (g_Entries dynamique, table map_portals), points (cw_points dans crestwood_points.db -> g_BankPos/g_Shops/g_SvcPumps/g_SvcRentPts/g_Jobs). Page assets/mapadmin ("@maps" 9830-9832, /mapact, SampMapsBridge dans les 2 SAMP.java).
- Plus aucun point fixe (banques, 24/7, pompes, locations, sieges d'emploi, entrees, braquage). Pts_LoadAll() en tete de OnGameModeInit.
- Entry_Teleport -> Pts_WaitStream : defige quand Ms_NearReady (zones < 25 m chargees) ou 7 s.
- MAP_REMOVE_BUILDINGS 1 (demande d'Anzo : les mappings doivent retirer les objets remplaces).
- Corps APK : SkinBodies kMale 289 / kFemale 12, charges au demarrage (SouthModelLoader::PreloadBodies dans MainLoop). ClothesBase -> catalogues 163/9. Serveur CLOTH_SKIN_MALE 289 / FEMALE 12, migration 163/7->289, 9->12.
- Identite : ecran "identity" de assets/login (genre, naissance, origine) -> /authact identity ; Id_* en fin de basicgm1.pwn ; pas de spawn tant que g_IdPending.

## v14 (31/10/2026) : LSPD Atlas, JBL, barre rapide
- Modeles d'objets remplaces depuis l'APK au demarrage : SouthModelLoader::ReplaceObjectModel / PreloadObjects (table kCwObjModels : 1862 lspd, 2226 jbl). assets/models/<nom>/model.dff (+ model.col facultatif) + tex/*.png|jpg + materials.txt (1 ligne par materiau du DFF). Les TXD PC (D3D9) ne se lisent pas sur mobile -> convertis avec tools/models/model_tex.py (txdpc.py de tools/utg_apk). Java prepareCustomModel + prefixe "models/" dans loadClothesTexture. BrlTextureShader::GetApkTexture (texture persistante).
- MapImp_PoliceLS = nouveau LSPD (Unity Station), StationService retire (meme zone).
- include/Jbl.inc : JBL (page assets/jbl, "@jbl" 9850-9852, /jblact) + barre rapide (dialogue "@hotbar" 9840 -> HUD hotbar.js, /hotbar n). Hot_SendSoon appele dans Inv_SetQty.

## v15 (31/10/2026) : encheres, zones vertes, invisibilite, RemoveBuilding 64 bits, parametres GTA V
- Voir LISEZMOI_V15_ENCHERES_ZONES_HUD.txt. Serveur : `include/Cw15.inc` (inclus apres Jbl), crochets `Cw_*` dans les callbacks, `Cw_AdminAction` en tete de `AdmX_Action`, `Cw_AdminNeed` en tete de `Adm_Need`, donnees panneau `{"x":"world"}` (onglet Monde d'admin.js).
- Encheres : tableaux `g_HouseAucBid/Bidder/End` declares dans le module IMMOBILIER (utilises par House_Send) ; `Auc_LoadAll()` en fin de `House_Load()`. Action page `/houseact bid <montant>` (liste blanche Java : "bid").
- Dialogues : 9516 "@hudv" (JSON fuel/gz/belt -> HudApp.updateHudInfo, carburant reinjecte dans updateHudState), 9517 "@hide" (ids caches -> natif setHiddenPlayers).
- Client : `RemoveBuilding()` (game/util.cpp) etait VIDE en 64 bits -> aucun batiment n'etait retire. Maintenant liste + `Crestwood_ShouldHideBuilding()` (filtre par modele) utilise par `CEntity_Render_hook` (32 et 64).
- `game/CwSettings.cpp` (identique 32/64) : `applyGameSetting(cle, valeur)` (1 FPS, 2 limite FPS, 3 distance LOD via dlsym CRenderer::ms_lodDistScale, 4 post-effets, 5 bouton micro, 6 icones boutons) + joueurs caches. Prefs Java "display_settings" cles "cw_*", reappliquees au 1er updateHudState.
- Icones des boutons natifs : `assets/models/buttons/*.png` chargees par `Crestwood_LoadApkTexture` (BrlTextureShader.cpp), posees dans `Crestwood_SkinWidget` (hooks Draw/Update de WidgetGta.cpp).
- Bouton micro : MainActivity ecrivait VoiceChatEnable=false -> VoiceButton::draw ne dessinait rien. Le dessin ne depend plus de ce reglage.

## v15b : garage partout, vetements supprimes
- Voir LISEZMOI_V15B_GARAGE_SANS_VETEMENTS.txt. Module VETEMENTS remplace par "SKIN DU PERSONNAGE" (memes noms Cloth_* pour le vestiaire / l'inscription, plus aucun dialogue "@clothes"). Module UTGAWA_LOJAS supprime. assets/clothes supprime. Java ignore 9550-9554. SouthModelLoader::PreloadBodies n'installe plus les corps (seulement les objets APK).
- Garage : Garage_Open partout (g_GarAt = garage proche ou -1), Gar_Take livre a cote du joueur hors garage (GAR_DELIVERY_FEE), Gar_Store "valet" a 25 m. JSON + "at","deliv","bank". Roue HUD : slot s9 "garage" -> menuAction -> /garage. Rp.inc : go garage.

## Vetements = RAR (31/10/2026)
- Module VETEMENTS de basicgm1.pwn = copie exacte de celui du RAR (46 valeurs, corps 163/9, /vetements /clothes /skin, /clothact). UTGAWA_LOJAS, points vetements (/vetadd...), cshop* du panneau admin : SUPPRIMES. Les sections Utgawa / v15b ci-dessus sont historiques.
- Client : C++ vetements du RAR (CharacterCreator, ClothesCustomization, SouthModelLoader + chargeur d'objets APK v14), assets/clothes du RAR, bouton TENUE du HUD (`#cwClothesBtn` -> menuAction "clothes" -> openClothesTest).
- Docs dans `docs/` (LISEZMOI_STRUCTURE_ET_CRASH.txt = etat actuel).
- Panneaux (31/10 soir) : fond flou natif (PixelCopy de GetSurfaceView -> ImageView `backdropView` sous hudMain, start/stopBackdrop dans navigate, seulement si `hudPanel.isFullscreenPage()`). Rayons des panneaux <= 4 px (script de conversion applique une fois, NE PAS relancer : il re-diminue l'opacite), `common/cw-pro.css` avant cw-perf.css dans chaque panneau.

## 31/10 soir : retour 2.11 en 64 bits
- L'essai "un seul client 2.10 pour 32+64" (structure CAMPINA) est ABANDONNE : la data du serveur est une data 2.11 (objets cdSize=0, crash au spawn).
- cpp/samp = 2.11 (libGame.so, arm64), cpp/samp32 = 2.10 (libGTASA.so, armeabi-v7a). Java : game.SAMP (64) + game32.SAMP (32) a garder synchronises (meme WebView deux couches, fond flou, vetements RAR).
- Plus aucun remplacement de modele au demarrage (SouthModelLoader::PreloadBodies retourne true). Icones de boutons APK desactivees par defaut.

## Correction build arm64 (31/10/2026, retour 2.11)
- Le source "retour 2.11" = ancienne structure : `cpp/samp` = client 64 bits (voice/, LogVoice), `cpp/samp32` = client 32 bits (voice_new/, FLog). Les sections "STRUCTURE UNIFIEE" plus haut sont historiques.
- Pousser ce source sur un repo qui contenait la structure unifiee laisse des fichiers fantomes dans `cpp/samp` (voice_new/, game_sa/, nv_event.cpp, game/CRenderTarget|ES2VertexBuffer|RQ_Commands|Shadows|multitouch|opengl.cpp) -> erreur `FLog` non declare. `samp/CMakeLists.txt` les exclut ; voir NETTOYAGE_REPO_GIT.txt pour les supprimer du repo.

## v16 (06/11/2026) : ecrans natifs (sans WebView), menu 3D joueurs, creation de tenue
- Vetements : `com.gta.game.ui.NativeClothesPanel` (Java natif, partage 64/32) remplace la page `assets/clothes` (gardee en secours, traduite FR). `SAMP.openClothesPage` -> `ensureNativeClothes()` ; memes commandes natives `clothesCommand(type,key,value)`. HUD masque pendant l'edition.
- Creation : serveur `g_ClothFirst` ; joueur sans tenue enregistree -> `Cloth_OpenFirst` (9551 avec `"first":1`) 3,6 s apres le spawn ; Java : `clothesCreation` = pas de bouton Annuler. `Id_DoSet` n'enregistre plus de tenue par defaut. `Cloth_Load` : sans tenue, corps = `Id_BodyOf`.
- Signature `CrestwoodTopBanner` (haut de l'ecran, entre hudMain et hudPanel, masquee quand un panneau est ouvert). Charte commune `CwUi` (gris/noir, filets, bleu #3B82FF, Montserrat).
- Menu 3D joueurs : `game/PlayerTap.cpp` (identique 32/64 sauf adresse CalcScreenCoors) : appui court sur un joueur -> `/interaction3d <id>` -> serveur `Ui_OpenPlayer3D` -> dialogue 9636 "@inter3d" `id|nom|police|gouv` -> Java `onInteract3D` -> JNI -> `PlayerTap::OnServerMenu`. Boutons -> `/uiact <action>`. `PlayerTap::OnTouch` retourne bool (touche consommee). L'ancien appui long 5 s est supprime.
- `navigate("about:...")` se rappelait a l'infini : corrige.
- HTML : `common/cw-pro.css` v3 (accent bleu, titres Montserrat via `common/fonts/`), garage recolore gris/noir/bleu.

## v17 (06/11/2026) : GARAGE PRO (natif + serveur)
- Client : `com.gta.game.ui.NativeGaragePanel` + `VehicleSilhouetteView` (silhouettes vectorielles peintes avec la couleur SA, palette 0..126). `SAMP.handleGarageDialog` -> `ensureNativeGarage()` (64 et 32 bits) ; `closeGaragePage` ferme d'abord le natif. Page `assets/garage` = secours (cles courtes normalisees dans garage.js).
- Commandes envoyees : `/garact <action> [id] [mot] [nombre]`, liste blanche Java (regex dans ensureNativeGarage).
- Serveur (module GARAGES de basicgm1.pwn) : colonnes ajoutees par ALTER (nick, fav, km, wear, ins, impound, shared, plate) dans `Gar_InitDB`. Actions : repair, service, refuel, insure 7|30, unimpound, nick, fav, paint c1 c2, plate, keys [id], lock, offer id prix (dialogue 9755 -> `Gar_OnDialog` dans OnDialogResponse). `Gar_GetRow(..., ownerOnly)` : 1 proprio, 2 double des cles. `Gar_Tick` (2 s) : km + usure (+1 / 15 km), surchauffe >= 85 %. Destruction : assure = gratuit, sinon `impound` = frais de fourriere. JSON vehicule a cles courtes (id,m,n,k,pl,hp,fu,o,s,p,km,w,in,im,f,c1,c2,sh,ow,lk,en), GAR_LIST_MAX 12.
- Verification Java faite avec des stubs Android (/home/claude/astub, hors projet) : les 5 classes ui compilent.

## v18 (07/11/2026) : PNJ, quetes, etat civil, tablette job, HUD, clavier, mappings
- Mappings 64 bits : `samp/game/hooks.cpp` patche aussi les vtables Render de CBuilding/CAnimatedBuilding/CDummy/CDummyObject (slots 0x81D3E0, 0x81D4A8, 0x81E3F8, 0x822B18, verifies par readelf) -> RemoveBuildingForPlayer marche enfin sur les batiments. Le 2e InlineHook CObject::Render (doublon) est retire. `samp/game/object.cpp` : SetAttachedObject/Vehicle, AttachToVehicle, AttachedToMovingEntity implementes (vides avant).
- Import de mappings sans recompiler : `include/MapFile.inc` lit `scriptfiles/mappings/liste.txt` (Pawn/Texture Studio + MTA .map). Les scripts Lua MTA ne peuvent PAS tourner (moteur SA-MP).
- PNJ : `include/Npc.inc` (roles 0-7, dont 7 Etat civil), panneau admin > Monde > PNJ, braquage des vendeurs, quetes scooter / permis avion / colis. Marqueurs type 18 = main noire dans un disque bleu (WorldMarkers.cpp, 32 et 64).
- Etat civil : PNJ role 7 -> dialogue 9880 "@idform" -> Java `NativeIdPanel` (formulaire + signature au doigt) -> `/idsign <age> <lieu>` -> `Npc_IdSign` (player_ids : issued, signed). Signature PNG gardee dans SharedPreferences "cw_signatures" (cle = pseudo en minuscules). Carte : `assets/ui/index.html` vue "id" (CNI Republique de Crestwood, recto/verso, MRZ, photo du HUD via `SampUi.photo()`, signature via `SampUi.signature(key)`).
- Tablette job : objet `tablette_job` (index 11, INV_ITEM_COUNT 12, 24/7 2 500 $). `include/JobPro.inc` : dialogues 9890/9891/9892 "@tablet" -> Java `NativeJobTablet` ; actions `/tabact`. Emplois du staff (table cjobs de crestwood_jobs.db, CJ_MAX 24, 16 points) : panneau admin > Monde > Emplois (`cj*`), joueurs `/emploi` au siege, `/finemploi`. Checkpoints de course partages avec les quetes (exclusifs : Q_CanStart / Cj_Start).
- HUD : bouton chat `#cwChatPin` colle a la minimap (toucher = ecrire, maintenir = masquer le chat) ; rangee sous la carte = roue / parametres / telephone (anciens boutons chat, tenue, RP retires -> roue). Roue a 16 entrees (2 anneaux, `.cw18-slot`, nom au centre) ; nouvelles actions Java menuAction : jobs, papers, anims, quests ; minimap et chat geres en JS. Styles : `hud/hud-v18.css` (charge apres cw-perf.css). Jauges de la minimap affinees (STROKE 3.5).
- Clavier de jeu : `hud/hud-kb.js` (AZERTY, accents en appui long, phrases enregistrees = macros dans localStorage "cw.kb.macros"). `setChatComposer` appelle `CwKb.open()` au lieu du clavier Android ; touche ⌨ = clavier Android pour ce message.
- Banque Atlas (MTA) : modeles convertis dans `tools/models/ready/`, mapping `scriptfiles/mappings/banque_atlas.txt` NON active (le remplacement de modeles est coupe depuis le 31/10).

## v19 (07/11/2026) : login, chargement, notifs GTA SA, Vie RP, perf
- Voir LISEZMOI_V19_LOGIN_CHARGEMENT_NOTIFS_VIE.txt. GM NON recompile dans le sandbox (pas de pawncc) : workflow `.github/workflows/pawn-build.yml` (construit pawncc 3.10.10 depuis les sources + artefact basicgm1.amx).
- Login (assets/login) refait : index.html / style.css / app.js. Meme pont : SampAuth.action(type,json) + ready ; DXAuth.openFromNative/handleResponse ; CwIdentity.open. Nouveau type "promocheck" ; "register" porte `promo`. Java (game + game32) : `/authact register u e p [CODE]`, `/authact promocheck CODE`.
- Serveur : Auth_DoRegister valide le code (Promo_SignupValidate) AVANT de creer le compte, Promo_SignupStore -> table promo_signup ; Promo_OnSpawn -> Promo_SignupSoon -> Promo_SignupApply (5 s) -> Promo_Redeem (coeur de l'ancien Promo_Use, qui l'appelle maintenant).
- Notifications : `Cw_Notify(playerid, type, titre, texte, duree)` (include/Vie.inc) -> 9644 "@note" -> hud.js "SA HELP" (boite d'aide GTA SA, codes ~g~ ~r~ ~y~ ~b~ ~p~ ~o~ ~w~ ~n~, file d'attente, bip WebAudio). CSS : hud/hud-v19.css (charge apres hud-v18.css).
- HUD : rangee .quick-actions collee a gauche (plus d'env(safe-area)), grille 3 colonnes, libelles `<small class="cw19-qa-label">`, bouton tcSteerMode cache ; reframeHud ignore la cle "quick".
- include/Vie.inc (inclus apres JobPro) : faim/soif (g_VieHunger/Thirst, envoyes dans "@hudv" par Cw_HudPush -> Java parse hunger/thirst dans 9516), niveau/XP (SetPlayerScore), /apparence + /examiner, loto (/loto, tirage a l'heure pile, crestwood_vie.db). Inv_Use : hamburger/eau/vody -> Vie_Eat/Vie_Drink. Hamburger ajoute a g_ShopItems + shop.js + items/hamburger.png.
- Mappings : MapImp_StationService reactive (+ RemoveBuilding 4019/4025 r=77), ROB_POINT_ACTIF 1.
- Perf/crash : onRenderProcessGone ajoute a DialogManager, LoadingScreen, MainActivity (le renderer WebView est partage : une seule WebView sans ce handler = appli fermee). updateHudState : le natif envoie deja a 10 Hz (main.cpp hudDelta>=0.10f) ; plafond Java 15/s (hudFlush) en filet. LoadingScreen libere sa page apres hide(). Minimap 30 fps.

## v19b (07/11/2026) : roue a familles, Ville.inc, charte "signaletique", 500k
- Promo Medium = 500000 $ (g_PromoMoney). Roue HUD : `.cw19-wheel` 6 `.cw19-fam` + `#cw19Panel` (`.cw19-group[data-fam]` / `.cw19-item[data-slot]`), slots `cmd:/x` -> Java menuAction n'envoie que WHEEL_COMMANDS (game + game32).
- include/Ville.inc (apres <Vie>) : courses de rue (/course, g_Rc*, Ville_Tick 1 s, SetPlayerRaceCheckpoint ; Ville_OnRaceCheckpoint appele dans OnPlayerEnterRaceCheckpoint apres Npc_) et mariage (/mariage /oui /non /divorce, table mariages dans g_VieDB, Ville_Spouse pour /examiner). Hooks init/connect/disconnect/spawn/command.
- Charte : common/cw-signal.css (html[data-sg], charge en dernier, 24 panneaux) ; dialog/ autonome (dialog.css + tone() dans dialog.js remappe or/jaune -> bleu). Plaque = `inset 0 0 0 2px #3B82FF, inset 0 0 0 3px rgba(255,255,255,.62)`. Pas de text-transform uppercase (sauf cartes d'identite .cni-*).
- Java : updateHudState(float...) arrondit (StringBuilder hudSb) pour que la deduplication (json.equals) marche a l'arret.

## v20 (10/11/2026) : dollars, cache des pages, langues, GPS 3D, roue vehicule
- Voir LISEZMOI_V20_DOLLARS_LANGUES_GPS3D_VEHICULES.txt. Devise "$" partout (script /home/claude/tools/money.py : FCFA -> $, base64 du telephone protegee).
- SAMP.navigate (game + game32) : cache de la derniere page de panneau (HudWebView.loadedUrl, PAGE_BRIDGE, isCacheablePage, reusePanelPage -> deliverPendingPanelJson + Bridge.ready()). onPageFinished injecte PANEL_KEYBOARD_JS. HudWebView.evaluateLocal = sans miroir vers le HUD.
- phone/index.html : images extraites dans phone/media/ (ne plus recoller de base64 dans la page).
- i18n : common/i18n.js (CwI18n.t / apply / live), common/cw-welcome.js (langue + reglement, cw.lang / cw.rules). Charges par loading, login, hud.
- Natif : PlayerTap.cpp roue vehicule (ActionGlyph, s_vehicle), WorldMarkers::SetGps + RenderGps (chevrons + balise), JNI setGpsWorld ; minimap.js sendGpsWorld -> AndroidHUD.setGpsRoute.
- Serveur : ManualVehicleEngineAndLights ; Ui_OpenVehicle3D (engine, lights, enter...) ; Ui_Action enter/lights/engine a distance ; Phone_AfterShopBuy (puce a la caisse) ; phoneact shopgps.

## v21 (12/11/2026) : camera drone, telephone qui memorise, vetements, garage, entree du jeu
- Voir LISEZMOI_V21_CAMERA_DRONE_VETEMENTS_GARAGE.txt. include/Cw21.inc (inclus apres <Ville>) : Pcam_Action (Phone_OnCommand "cam" -> selfie/drone/land, staff = portee illimitee + SetPlayerPosFindZ), /cadeau (table cadeau dans g_VieDB), /parrain (table parrains, bank_accounts de g_BankDB), /sondage + /voter, meteo auto (Wx_Fix aussi appele par adminact weather), /afk, /mapdata (scriptfiles/mappings/data_ok.txt -> g_MapData remplace #if MAP_MODELES_DATA), magasins de vetements (vetpoints de crestwood_garage.db : Cs_InteractKey 190 -> Cs_Use, Cs_PushMarks type 1, Cs_AppendBlips icone 45, Cs_AdminAction cshopadd/cshopdel/cshopgoto, Cs_AdminSend {"x":"cshop"}), GarAuto_Minute (vehicule de garage 30 min vide -> Gar_StoreVehicle). Hooks : init/connect/disconnect/spawn/death(Cw21_OnSpawn)/command.
- Vetements : seulement corps 0 (163) et 1 (9) ; NativeClothesPanel migre 2/3/4 -> onGender(0|1) ; serveur Cloth_BaseSkin (7/289 -> 163, 12 -> 9).
- Java (game + game32) : garage HTML par defaut (USE_NATIVE_GARAGE=false, SampGarage.cmd = meme regex que le natif), HeightProvider + onHeightChanged -> KB_LIFT_JS (manifest adjustNothing), PANEL_KEYBOARD_JS ecoute cw:pageshow (classe html.cw-re = rejoue l'animation d'entree de cw-signal.css), setOffscreenPreRaster si pageCacheEnabled. LoadingScreen / MainActivity : DOM storage ON + AndroidLoading.getPref/setPref (cw.lang / cw.rules). CrestwoodTopBanner = image login/assets/images/cw-logo.png. CwUi.RADIUS_DP = 12.
- Natif : PlayerTap.cpp roue vehicule centree (s_wheel = hit-test polaire), WorldMarkers.cpp Hand() redessinee. Verifier avec /tmp/cxxcheck.sh (les 2 dossiers ne different que par l'adresse ToScreen).
- HUD : hud-v21.css (camera dans le telephone, notifs rond "i", #cwPeek, formes arrondies, compteur .cw-note-count / .cw-note-more), hud.js notifs (count, urgent en tete, duree selon longueur, toucher = suivante, zone tactile "note").

## v22 (12/11/2026) : notifs "grand rond", marqueurs 3D bleus, hotbar, prix
- Voir LISEZMOI_V22_NOTIFS_HOTBAR_MARQUEURS_PRIX.txt. include/Cw22.inc (apres <Cw21>) : hotbar (Hb_Refresh -> 9840 "@hotbar" si le JSON change, tick 2,5 s, 1 requete inv_items par joueur ; Hb_AutoFill range les consommables si aucun emplacement ; /hotbar [1-5]) ; prix (table shop_prices dans g_InvDB, Price_AdminNeed/Send/Action : pricelist / priceset <index> <prix> / pricereset, {"x":"prices","items":[[i,"item",prix]]}).
- HUD : hud-v22.css (notifs sans carte, rond .cw-note-dot + SVG NOTE_ICON selon le type ; .cw22-hb onglet + emplacements ; .interact-tap-button en carre bleu). hotbar.js reconstruit l'onglet "+N".
- Natif WorldMarkers.cpp (samp + samp32) : Badge(dl, renderer, type, c, r, alpha, name, dist) = carre bleu + pointe + nom (TypeName) + distance ; Hand() = carre bleu + main ; le marqueur de la main est saute (handM) et son nom s'affiche a cote. kMaxDist 90 m.
- cw-signal.css v22 : habillage unique (variables --cw-glass) ; blazer charge cw-signal.
- v22b : hud/anim-mini.js (window.CwAnim.toggle, #cwAnimMini, zone tactile "animpanel") ouvert par .animations-toggle-button (hud.js) ; serveur Anim_Quick (/animq, /anim, /stopanim) sur g_RpAnim / g_RpAnimMode / g_RpCur de Rp.inc. hud-v22.css sections 4 (capsule .quick-actions) et 5 (mini page). Attention : .interact-tap-button garde display:none quand [hidden].
- v22d (stabilite) : HeightProvider (game, game32, nvidia) showAtLocation protege (token null / finishing, 10 essais, try/catch), dismiss dans SAMP.onDestroy ; setOffscreenPreRaster retire ; pageCacheEnabled = false si RAM totale < 4,6 Go ; CrashLog (game32) utilise aussi par game/GTASA (64 bits), fichier crestwood_log.txt.
