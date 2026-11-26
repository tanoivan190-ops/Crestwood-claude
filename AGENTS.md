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
- v22f : corps du studio = 4 (skin 7) et 3 (skin 12) ; NativeClothesPanel migre 0/1/2 ; serveur CLOTH_SKIN_MALE 7 / FEMALE 12, Cloth_BaseSkin 163/289 -> 7, 9 -> 12. hudWebView LAYER_TYPE_NONE. Cache des pages si RAM > 3,5 Go. minimap MIN_FRAME_MS 50 si deviceMemory <= 4. hud-kb.js : vibration desactivee (localStorage cwKbBuzz=1 pour la remettre). hud-v22.css sections 6 (roue Menu centree) et 7 (contain sur .cw-kb). WorldMarkers kMaxDist 60, kLabelDist 32, kMaxLabels 6.

## v23 (17/11/2026) : MDT, armee, EMS, assembleur, arene, craft, recyclage
- include/Cw23.inc (apres <Cw22>), base crestwood_v23.db (c23_players, c23_fines, c23_stock). Hooks : init, connect, disconnect, OnPlayerSpawn (Cw23_OnSpawn EN DERNIER), OnPlayerDeath (inArena = Cw23_OnDeath -> Bank_OnDeath / Cw_OnDeath sautes), OnPlayerStateChange, commandes AVANT Law_OnCommand (/mdt = nouveau MDT, /centrale = ancienne tablette).
- Canal HUD generique : dialogue 9860 "@hud" -> SAMP.java hudJs("CwBus(json)") -> hud/cw23.js (modules mdt, army, asm, arena, craft). HUD -> serveur : sendChatMessage("/c23 <module> <action> ..."). A reutiliser pour toute nouvelle page : pas de WebView en plus.
- hud-kb.js : CwKb.openFor(input, valider) = clavier Crestwood sur n'importe quel champ du HUD.
- Arene : objets joueur (CreatePlayerObject) crees a l'entree, detruits a la sortie ; monde 777.

## v24 (17/11/2026) : MDT ordinateur, armurerie des vestiaires, hopital EMS, corps femme
- MDT = fenetre facon macOS (classe .mac sur #cw23 quand mod === "mdt") : barre de titre + 3 feux (rouge/jaune ferment), barre laterale (onglets avec icones), contenu a droite, 94vw x 90vh. Les autres modules gardent le panneau de droite. Structure commune : header + .cw23-main (nav + body).
- Vestiaire : Locker_GearJson(type) = liste [cle, libelle, info] par type de faction, envoyee dans "@locker" ("gear", "stock"). Action JS "gear <cle>" -> /lockeract gear <cle> -> Locker_Gear (memes controles faction / distance / delai). Armes a feu des factions armees = munitions prises dans g_C23Stock (C23_TakeStock / C23_StockOf dans Cw23.inc). SampLockerBridge (2 SAMP.java) accepte "gear" (arg [a-z]).
- Admin : creation de vestiaire -> demande TOUJOURS la faction (liste, ou numero si la liste des factions n'est pas chargee).
- include/HopitalEMS.inc : 599 objets du .map MTA (objets "scale" != 1 ignores) + batiment 5403/5402 + ascenseur du toit (pickups 1318, MapHopital_OnPickup). Charge seulement si scriptfiles/mappings/hopital_ems_ok.txt (/mapdata hopital on). Le batiment du pack est chiffre (centralstore) : ne pas tenter de le dechiffrer, demander les fichiers en clair.
- Vetements corps femme (12) : BrlTextureShader::SetPlayerTexture pose les regles en kAnyShaderModelId (avant : figees sur le modele du moment -> rien sur le nouveau corps). ClothesCustomization::CommitWantedTextures vide les regles + le cache "deja pose" quand le corps change (g_lastLocalBody / g_lastRemoteBody). samp ET samp32.
- Bouton d'interaction du HUD reduit (hud-v22.css section 3 : icone 30-38 px, texte 9-11 px).
- Piege Pawn : strcmp("", "on") == 0 -> toujours tester arg[0] avant (corrige dans /mapdata).

## v25 (17/11/2026) : MDT pro + logo CPD, location de velo, interaction auto, icones, telephone en main
- MDT : nouvelle fenetre hud/mdt.js + mdt.css (#cwMdt, CwMdt.onData). cw23.js renvoie m === "mdt" vers CwMdt. Ecran de connexion (logo img/cw_police.webp, une fois par session), tableau de bord (unites, avis, amendes, radars), citoyens, vehicules, recherches, radio, cameras, radar. Mdt_Home envoie en plus id, rank, fines, radars, online, units [[id,nom,faction,enVehicule]].
- Extension generique : window.CwBusExt[m] = fonction (ex. bike.js). A utiliser pour toute nouvelle page HUD.
- include/Cw25.inc (apres <Cw23>) : location de velo BMX (table c25_bikes dans crestwood_v23.db, admin bikeadd/bikedel/bikegoto/bikelist, cle d'interaction 200, marqueur WorldMarkers type 19 = pictogramme velo dans samp ET samp32), 500 $ / 1 h, rendu automatique en roulant sur un point, amende 750 $ dans c23_fines sinon. Page hud/bike.js + bike.css (photo img/bmx.webp).
- Interaction automatique (Interact_Refresh) : en ARRIVANT sur un marqueur (cle precedente 0), cles 10 30 50 60 70 71 90 110 150 160 190 200 -> Interact_Do. Pas maison (120/121), vehicule, joueur, police, braquage, PNJ. Re-arme quand on s'eloigne de 3 m (g_InteractArmed / g_InteractAutoPos), 2,5 s mini entre deux.
- Telephone en main : HudWebView.phoneListener (static) appele a chaque ouverture / fermeture de la page telephone -> /telmain 1|0 -> SPECIAL_ACTION_USECELLPHONE (on peut marcher). Les 2 SAMP.java posent le listener.
- HUD : icones des 3 boutons (Menu = grille, Reglages = engrenage, Telephone = smartphone) + hud-v25.css ; faim = burger.svg (anneau + compteur). Telephone : widgets d'accueil (calendrier + Crestwood), dock sans libelles.

## v26 (17/11/2026) : plantages, streaming, demarrage sans Nightcode
- Diagnostic : CrashLog.logPreviousExit() (Android 11+) ecrit dans crestwood_log.txt POURQUOI la session precedente s'est arretee (MANQUE DE MEMOIRE, PLANTAGE NATIF + trace, ANR...). 64 bits : appele par startSession (GTASA) ; 32 bits : apres CrashLog.bind dans SAMP.onCreate. samp/main.cpp : g_logMirror copie aussi le journal du gestionnaire de signaux dans crestwood_log.txt.
- Streaming des mappings : MS_MAX_OBJ 9000 (avant 6800), plafond par joueur selon la RAM envoyee par l'appli (/cwdev <Mo> avant /authact open) : < 2 Go 620, 2-3,5 Go 820, > 3,5 Go 940 (avant 720 pour tous). Ms_Cap / Ms_SetDevice.
- Vestiaires : Locker_Apply / RpX_WearUniform / RpX_RemoveUniform ferment d'abord la page HTML, puis skin (350 ms, actions speciales et anims coupees), puis apparence (900 ms plus tard). Avant : skin + textures + navigation de la WebView au meme instant.
- Textes 3D "Station-service" / "Location de vehicules" (meme style que le magasin de vetements) : Svc_BuildLabels (Cw25.inc), reconstruits dans Marks_SendAll.
- Panneau admin > Commandes : ajout de /maisons /pontons /garages /garageadd /garagespawn /garagedel /donnerveh /nourrir /sondage /meteo /points /mapdata /mapdata hopital /pnj liste|creer|tp|suppr.
- Launcher : IntroOverlay reecrit (5 s : photo du joueur sur la Dodge, logo cw_boot_logo, barre bleue, plus de "Nightcode"). Page JOUER : fond intro_slide_1.
- Telephone : icones d'apps (Messages) arrondies meme avec le badge non lus (overflow visible).

## v27 (17/11/2026) : meme streaming pour tous
- Streamer Pawn (Ms) : plafond commun MS_PLAYER_CAP 950 pour TOUS (plus d'adaptation a la RAM, /cwdev ne fait que logger), chargement MS_LOAD_MARGIN 320 / dechargement 400, distance d'affichage mini MS_MIN_DRAW 350, une case qui depasse le plafond est chargee en partie (avant : sautee). MS_MAX_OBJ 11000, MS_MAX_MAT 6000, MS_MAX_ZONES 900.
- Appli : distance d'affichage (LOD) imposee a 130 % (CW_DRAW_DIST, 2 SAMP.java), curseur retire de la page Reglages.

## v28 (17/11/2026) : plantage en arrivant sur un mapping, 120 Hz, vetements par-dessus la tenue
- Streamer : plafond commun 800 (950 plantait), chargement 260 / dechargement 340, MS_MIN_DRAW 300, MS_PER_TICK 40 (pics plus doux). LOD impose 100 % (CW_DRAW_DIST).
- requestMaxRefreshRate() dans les 2 SAMP.onCreate : mode d'ecran le plus rapide (120/90 Hz) a la meme resolution ; le moteur est deja limite a 120 FPS (ApplyFPSPatch).
- ApplySouth (samp + samp32) : la tenue d'origine du corps est TOUJOURS cachee (familles torso / pants-short-bikini / boot-slipper) et la peau remise partout ; seuls les vetements "south" choisis s'affichent (sans rien : sous-vetements). Ancienne note : choisir une piece cache toute la famille (fantasy_*_torso*, pants/short/bikini, boot/slipper) : plus de vetement par-dessus le sweat / jean de base.

## v29 (17/11/2026) : style "jeu" (GTA V / FiveM) sur les panneaux
- common/cw-pro.css section [v29] : fond image common/img/bg-city-1.webp + voile, police CwGame (common/fonts), titres italique gras, coins 9-12 px, boutons principaux bleus. Exclus : pages html[data-flat] et html[data-sg] (inventaire : le joueur a prefere l'ancien style, garde tel quel).

## v30 (19/11/2026)
- Marqueurs 3D (WorldMarkers.cpp 32/64) : visibles < 12 m, noms < 5 m, textes ~fs*0.4. Textes 3D d'interaction a 4 m.
- Vetements : reset unique (`Cw_OnceDB(db, cle)` + table `cw_meta`), creation = visage/cheveux seulement (`CLOTH_FIRST_CLOTHING`), s'habiller seulement en magasin (`Cloth_CanDress` / `Cs_NearShop` dans Cw21.inc).
- Streaming : profils par RAM (`Ms_SetProfile`, /cwdev) ; client : budget streaming 300-384 Mo et distance 65/80 % si <= 4 Go.
- Compilation (x86_64) : pawncc 3.10.10 construit depuis les sources, `pawncc basicgm1.pwn -iinclude -i../../tools/pawn-windows/include -d0` -> 0 erreur, 8 warnings.

## v30c (20/11/2026)
- Les DEUX classes Java doivent recevoir chaque page : `com.gta.game.SAMP` (64 bits) ET `com.gta.game32.SAMP` (32 bits). Store + pages generiques portes dans game/SAMP.java.
- Pages generiques : `include/Pages.inc` (`Pg_Send`, dialogues 9970/9971/9972, titre `@page <nom>`, retour `/pageact <nom> <action> [args]`), pages `assets/ticket|repair|bodycam` (+ `common/cw-page.css/js`). Ajouter un nom a `CW_PAGES` (les deux SAMP.java) pour une nouvelle page.
- Prefixes deja pris : `Rep_` = reports (basicgm1.pwn) -> le garage utilise `Rpr_`. Un nom en double fait planter pawncc (segfault dans newfunc), pas d'erreur claire.
- Pawn : `strcmp` renvoie 0 si une chaine est vide -> toujours tester `x[0]` avant.
- Animations MTA : `include/MtaAnims.inc` (genere), cles `m1..m151` (anim-mini.js). Bloc "SNM" remplace par `assets/cwmodels/snm.ifp` (ModLoader lit aussi `SAMP/cwmodels/*.ifp`). `CPlayerPed::ApplyAnimation` ignore une animation absente (`CwAnimExists`, 32/64).

## v31 (20/11/2026)
- Fin de OnPlayerCommandText : `Cmp_OnCommand` puis `Cmp_Unknown` (message FR + scriptfiles/commandes_inconnues.log). Commandes envoyees par l'appli sans handler : ajouter dans include/Compat.inc.
- Corps 7/12 : `CwPreloadBodiesTick` (SouthModelLoader.cpp 32/64) installe le corps des que `CStreaming::GetInfo(id).IsLoaded()` (jamais bloquant).
- Compteur : hud/speedo-cw = copie de original-speedometer + speedo-plus.css/js.
- Telephone : phone/apps-v31.js (carte Uber : /phoneact pos|gps ; Instagram : /phoneact ig -> jeton, puis HTTP direct vers server/instagram/ig-server.js). Pawn : include/Insta.inc (HTTP natif, IG_SECRET). Java SampPhone : serverHost(), actions ig/pos/gps (les deux SAMP.java).
- Clavier : hud-kb.js desactive par defaut (useHtml), CwKb.open/openFor renvoient false -> clavier Android.
- Nouvelles commandes citoyennes : toujours les ajouter dans Aide_Cat (basicgm1.pwn).

## v32 (20/11/2026)
- PLUS de modeles/animations personnalises du MTA dans l'appli (assets/cwmodels supprime, ApplyCwModels retire) : plantages. Ne pas les remettre sans test sur telephone.
- samp32 utilise FLog (pas Log). Toujours verifier les deux dossiers samp/samp32.
- Streaming client + streamer serveur = versions d'origine (zip Crestwood-claude-22). MS_MAX_OBJ 12500.
- Corps South : APK en priorite (prepareClothesModel, cache crestwood-clothes-v2 + apk.stamp) ; CwPreloadBodiesTick reessaie toutes les 3 s ; modeles 7/12 demandes au demarrage (game.cpp).
- Uber : include/Uber.inc (dispatch au plus proche, taximetre C23_Send {"m":"meter"}), phone/apps-v32.js, hud/uber-meter.js ; Java SampPhone action "uber" (sub/arg).

## v33-v34 (20/11/2026)
- Bodycam : g_BcOn=1 a la connexion, Bc_SpecAgain (300/1200 ms), Bc_ViewPos utilise par Ms_Tick (streamer autour de la cible regardee).
- Camera du telephone : dans le telephone (v35 a retire le plein ecran). Java pixelCopyTry = 3 essais. rgCameraNativeStatus ignore les messages hors camera (le panneau recopie ses scripts dans le HUD cache via setMirror).
- Telephone : style iPhone 17 Pro dans <style id="cw34-iphone17"> (phone/index.html), #lockClockBox.
- Uber : phone/apps-v34.js remplace uberHTML/uberBodyHTML ; lit CwUber.state (apps-v31) et CwUberApp.state (apps-v32). Tarifs copies de Uber.inc (TARIF) : les garder synchronises.
- Boutique : apercu vehicule = PlayerTextDraw font 5 (Store_PvShow/Store_PvHide), coordonnees en pour-mille -> x0.640 / x0.448. SampStoreBridge accepte pv/pvoff (les deux SAMP.java).

## v35 (20/11/2026)
- .hud-root est en pointer-events:none : tout element cliquable du HUD doit remettre pointer-events:auto (bug camera, hud/hud-v35.css).
- Anzo ne veut PAS de HTML pour les controles : boutons natifs = textures APK (models/buttons/v_*.png, Crestwood_ButtonIcon dans Widgets/WidgetGta.cpp) ou ImGui (EngineButton, CombatButtons). Generateur : tools/buttons/gen_buttons.py <dossier> <car.png>.
- EngineButton : game/EngineButton.cpp identique samp/samp32, Render dans UI::drawList, OnTouch dans AND_TouchEvent_hook.
- Homme South (7) : ApplySouth garde la tenue d'origine du modele pour chaque zone sans vetement choisi (topStyle 0, bottomStyle 0/3, shoesStyle 0).
- Moteurs MTA Mobile (libGTASA/libMobileSA) : incompatibles (autre build de libGTASA, protocole MTA) -> ne pas les integrer.

## v36 (20/11/2026) : debut de la sortie du HTML
- DECISION d'Anzo : TOUT l'affichage (HUD, telephone, pages) passe en vues Android natives (layouts XML), puis suppression de la WebView. Ne plus creer de nouvelle page HTML.
- Code natif partage 32/64 : package com.crestwood.ui (app/src/main/java/com/crestwood/ui), R = com.gta.game.R.
- NativeHud.attach(activity, hudParent, hudMain) ajoute res/layout/cw_native_hud.xml SOUS hudMain ; suit la visibilite de hudMain (panneau ouvert = cache).
- Donnees : SAMP.updateHudState(floats) -> NativeHud.setPlayer ; updateHudMoney -> setMoney ; dialogue 9516 (@hudv) -> setInfo(JSONObject) ; updateHudNeeds -> setNeeds.
- La page hud/index.html recoit NativeHud.HIDE_HTML_JS a onPageFinished (classe cw-native-hud, iframe du compteur retire).
- Fait : SpeedoView, InfoPanelView. A faire (etape 1) : minimap, notifications, chat, boutons HUD, menu rapide.
- v37 : MinimapView (port de minimap.js + hud-ring.js) + CrestGps (port de gps-route.js, assets/hud/gps/roads.bin genere depuis roads.js). La minimap HTML est neutralisee par HIDE_HTML_JS (Minimap/MinimapBridge vides). SampHudBridge.nativeMinimap(bool) suit l'option de la roue. replayMapState() pousse aussi blips + GPS au HUD natif.

## v38 (21/11/2026)
- gameSettingDefault("cw_buttons") = 1 dans les DEUX SAMP.java (64 bits etait a 0 -> boutons restyles invisibles en 64 bits).
- PlayerTap : vehicule = appui long kVehHoldMs (5 s), aussi depuis l'interieur (LocalAny) ; joueur = appui court comme avant.
- EngineButton visible seulement si moteur coupe (et pas velo 481/509/510).
- SAMP.cwLog(String) : journal natif -> crestwood_log.txt (60 lignes max). C++ : CwNativeLog(fmt, ...) dans gui/SouthModelLoader.cpp.

## v39 (21/11/2026)
- isLowEndDevice() (RAM <= ~4,2 Go ou isLowRamDevice) dans les deux SAMP.java : 60 Hz + fps_limit 60 par defaut.
- updateHudState(floats) : JSON "lite" pour la page HTML quand nativeHud existe et que le HUD principal est affiche.

## v40 (21/11/2026)
- ATTENTION : CTxdStore::ms_pTxdPool (game/TxdStore.h) est une variable du client JAMAIS remplie. Utiliser GameTxdPool() (gui/SouthModelLoader.cpp, dlsym _ZN9CTxdStore11ms_pTxdPoolE).
- RwHelper GetAnimHierarchyFromSkinClump = hierarchie de l'ATOME (apres SetClump seulement). Pour un clump fraichement lu : FindClumpHierarchy (frames).
- Recreer un ped apres remplacement de modele : CPed::SetModelIndex (pas CreateRwObject seul : os vides -> crash).

## v41 (21/11/2026) - factions RP
- include/Frp.inc (inclus a la FIN de basicgm1.pwn). Types FACTION_FIRE 13 / FACTION_JUSTICE 14 (FACTION_TYPE_MAX 14) : penser a g_FxTypeNames, g_RpXUniSkin, admin.js FAC_TYPES.
- Seed des factions : table faction_seed (crestwood_factions.db). Donnees justice/pompiers/reglages : crestwood_justice.db.
- CreateDynamicObject de basicgm est un SHIM (Ms_Add, pas de DestroyDynamicObject) : objets temporaires = CreateObject/DestroyObject.
- Hooks : Pol_Jail -> Jus_OnJail, Fine_Give -> Jus_Record, Phone_Sms/Accept -> Frp_OnSms/OnCall, FacX_Pay -> Frp_PayMult, Ui menus -> Frp_Ui*.

## v42 (21/11/2026) - creation + interactions
- include/Cw41.inc (inclus en dernier). Studio : Crea_Begin (Cloth_OpenFirst) / Crea_End (Cloth_DoSave 1re fois). 18981 = bloc 1x25x25 fin sur X.
- Vetements de depart : southTop = index 39, southBottom = 40 (g_ClothKeys), valeurs = rang catalogue South + 1.
- Menu joueur : 5e champ peut commencer par "req_yes:...;req_no:...;" (Int_ReqItems). PlayerTap : ParseOptions 24 max, colonnes auto.
- Actions client : a-z et _ seulement, 16 car. max (ValidAction) ; serveur : action[16].

## v43 (21/11/2026)
- Chat tape -> "/tc <cmd>" (hud.js submit, SAMP.java submitChatInput, chat.cpp). Serveur : Tc_Handle (Cw41.inc). NE PAS prefixer les commandes internes des pages.
- Panneau de faction : serveur dans Cw41.inc (Fp_*), dialogues "@fac" 9680/9681/9682, tables faction_bank/log/events (crestwood_factions.db). Nouvelles actions page : tool, invite (liste blanche Java 32/64).
- Textes 3D : textlabelpool fontSize*0.40 ; WorldMarkers : rond 0.45 m, kMaxDist 22.

## v44 (21/11/2026)
- include/Cw44.inc (inclus en dernier) : casino (Cas_), bourse (Bs_/St_), evenements (Ev_). Base crestwood_v44.db.
- Nouvelles pages generiques : ajouter le nom dans CW_PAGES (SAMP.java 32 ET 64) + router dans Pg_OnCommand (Cw44_PageAction).
- Marqueur avec nom : Mk_AddN(type, x, y, z, nom). Type 20 = casino (WorldMarkers.cpp 32/64).
- JOB_COUNT = 9 (JOB_SWEEPER, JOB_TRUCKER, JOB_CASH, JOB_POST) ; assets/job/job.js JOBS 6-9.
- Attention : "state" est un mot reserve en Pawn (ne pas l'utiliser comme nom de parametre).

## v45 (21/11/2026)
- include/Cw45.inc : scene de connexion (Lg_Begin/Lg_End), VW 7000+id, objets CreateObject detruits au spawn. AddPlayerClass skin 7. OnPlayerRequestClass -> Lg_Begin si pas connecte.
- Client : spawn()->setVisible(false) (localplayer.cpp 32/64, main.cpp).
- Theme pages : verre blanc + police CwGame (common/cw-font.css). Notifs : hud/hud-v45.css, palette SA_COLORS foncee.
- Phone : apps-v45.js (statuts WhatsApp, override window.waStatusPaneHTML), ig-server.js routes /status /statuses /simg. Selfie = centre ecran (hud.js pcShoot).

## v46 (21/11/2026)
- include/Cw46.inc (inclus apres Cw45) : Gs_ stats (gs), page generique "groupe" (Grp_), coffre des gangs (gbank/glog, Gang_Income 30 min), identifiants du jour (Dt_, dialogue 9518 "@tags" -> "1|id:tag,..."), tutoriel (Tt_, HUD 9860 {"m":"tuto"}, commandes /tuto accept|decline|next|skip|gps|stop).
- Accroches : Pol arrest (Cw46_OnArrest), verdict Frp, Tk_Give (OnFine), /reanimer (OnRevive), Fire_End (OnFire), FacX_Pay (OnSalary), OnPlayerDeath (OnDeath), Pages router (Cw46_PageAction), Rp_Go "grp"/"tuto".
- Client : CwSettings.cpp Crestwood_SetPlayerTags / Crestwood_FormatPlayerTag, playertags.cpp, main.cpp JNI setPlayerTags (32/64), SAMP.java 9518 + CW_PAGES "groupe".
- Assets : groupe/index.html, hud/tuto.js + tuto.css + assets/ringtone.mp3 + tuto-ok.mp3, clothes/clothes-v46.css (silhouettes masquees), cw-page.css tons (color-mix, data-tone="dark"), admin.js CMDS v46.

## v47 (23/11/2026)
- Pages protegees : app/build.gradle.kts (cwEncryptAssets apres merge*Assets, propriete crestwood.protectPages) + com/crestwood/ui/CwAssets.java (shouldInterceptRequest dans SAMP 64/32, MainActivity, LoadingScreen, DialogManager ; NativeClothesPanel lit catalogue.json via CwAssets.open). Format "CWX1"+IV+AES/CTR, cle SHA-256 de la graine (identique Gradle/Java).
- MTA : tools/mta/mta_vers_apk.py (+cwtxd.py) -> assets/mta ; natif gui/MtaModels.cpp (32 et 64, appele par SouthModelLoader::PreloadBodies) ; Java SAMP.mtaModelsList/mtaStateDir/prepareMtaModel ; garde files/crestwood-mta/guard.txt + off.txt. Ne PAS charger au demarrage.
- MapFile.inc : commentaire en fin de ligne autorise dans liste.txt.
- HUD : InfoPanelView reecrit (haut gauche), hud/hud-v47.css. Styles perso : <page>/theme.css, login/v47.css, cw-welcome.js.
- include/Cw47.inc : interactions (Cw47_PlayerItems/VehItems/UiAction/Accept/ReqLabel), REQ 20+.
- v47b : HUD de la v46 remis (InfoPanelView en haut a droite, cw_native_hud.xml) + animations seulement (aucune couleur ajoutee). hud-v47.css = animations (proprietes scale/translate, pas transform, a cause des !important de hud-v45.css).
- REGLE Anzo : toujours mettre a jour le panel admin (admin.js CMDS) et /aide a chaque nouvelle commande.
- v49 : pages HTML existantes remises en v44 (ne plus les restyler sans demande explicite). cw-page.css v44 + bloc opti (pas de backdrop-filter). Dt_ broadcast groupe (g_DtDirty, C46_Tick).
- v50 : connexion Google. com.crestwood.ui.CwGoogleAuth (Credential Manager + Firebase Auth, FirebaseApp nomme "crestwood" initialise a la main depuis les resValues generees par cwFirebaseConfig() dans app/build.gradle.kts a partir de google-services.json ; pas de plugin google-services). SampAuth.google/googleAccount/googleSignOut (SAMP 32+64). Node server/instagram/gauth.js (/gauth/verify RS256 + /gauth/redeem ticket). Pawn include/Cw50.inc (Gg_*, google_links, /gunlink), Auth_LoginOk commun. Signature fixe app/crestwood-debug.jks. Page login refaite (login.css), demande explicite d'Anzo.
- v54 : applicationId com.crestwoodroleplay (projet Firebase crestwoodrp-3e53d), chemins natifs/journaux mis a jour, CwGoogleAuth.res() via getResourcePackageName.
- v51 : Google uniquement. Launcher : assets/play/ (bridge AndroidPlay dans MainActivity, CwGoogleAuth.signIn). Pseudo de connexion = saved_nickname (ecrit par SAMP.rememberCharacterName au salon) sinon Citoyen_XXXX ; le serveur renomme (Gg_BecomeCharacter : SetPlayerName + Bank_LoadPlayer/Gang_Load + reset des *_Loaded). Cw50.inc : cinematique Gg_Intro*, creation Gg_Create, ancien compte Gg_LinkOld, salon Sp_Lobby, spawn Sp_Preview/Sp_Go (SetSpawnInfo puis remise a zero dans Cw50_OnSpawn), spawn_last. g_LgLobby declare dans basicgm1.pwn (Cw45 l'utilise). Anti-lag C++ : BrlTextureShader (KeyEq/RuleKey/tex cache/PlayerIdForPed cache), FindObjectFromGtaPtrFast, ProcessAttach distance.
- v59 : ecran noir apres connexion. samp32 gere maintenant InterpolateCamera (RPC 82, scriptrpc.cpp + Camera.cpp). Cw50 : SetPlayerCameraPos/LookAt fixes avant chaque InterpolateCamera (Gg_IntroShot, Sp_Preview), Sp_SafeCam 1,5 s apres Sp_Go. SouthModelLoader : tenues 250/275 seulement si le modele est deja en memoire (plus de chargement force).
- v60 : pages chiffrees affichees en clair-chiffre (CWX1) : CwAssets.checkPage(view,url) dans chaque onPageFinished (5 WebViewClient) -> buildPage (HTML + CSS/JS dechiffres integres) + loadDataWithBaseURL ; intercept tolere les // et journalise les echecs dans crestwood_log.txt. Toute nouvelle WebView : intercept + checkPage.
- v61 : lanceur (LOADING_URL, PLAY_URL) et LoadingScreen chargent via CwAssets.load(view,url) (page en clair, loadDataWithBaseURL), plus de dependance a shouldInterceptRequest pour ces pages.
- Skill Hallmark (MIT, Nutlope) copie dans tools/hallmark/ (references, themes, scripts). Pages du jeu = portee "app" : `node tools/hallmark/scripts/sloplint.mjs <page.html> --scope app`. Seulement pour les NOUVELLES pages ou quand Anzo dit Hallmark ; jamais de restyle d'une page existante sans demande.
- v62 : Hallmark polish sur play, loading, login, locker (0 FAIL sloplint --scope app). Tampon Hallmark en ligne 1 des CSS, tokens :root (--c-*, --font-*, --c-focus). Le reste des pages : pas encore.
- v63 : toutes les navigations WebView du jeu (SAMP 32/64 hudMain/hudPanel, DialogManager) passent par CwAssets.load(view,url) ; pendingAuthJson n'est consomme que si window.DXAuth existe (sinon remis). Ne plus utiliser view.loadUrl() pour une page d'asset.
