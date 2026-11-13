# Correctif Spawn + South DATA

- Le deblocage local du ped est maintenant rejoue a la fin de `CLocalPlayer::Spawn()` en ARM32 et ARM64, apres changement de skin et positionnement.
- Le gamemode serveur deplace `TogglePlayerControllable(playerid, 1)` a la fin du spawn normal. Le mode creation de personnage (`pdCharState == 0`) reste volontairement gele jusqu'a confirmation de creation.
- Les skins South sont recherches d'abord dans `/storage/emulated/0/data/clothes/south/maleskin/` et `/storage/emulated/0/data/clothes/south/femaleskin/` (fichiers `.dff` + `.txd`).
- Les textures/miniatures South sont recherchees d'abord dans `/storage/emulated/0/data/clothes/south/...`; les assets APK restent en repli pour compatibilite.
- Le catalogue HTML/JS et les fichiers de vetements n'ont pas ete modifies.

## Structure DATA attendue

```text
/storage/emulated/0/data/clothes/south/
  maleskin/male.dff
  maleskin/male.txd
  femaleskin/female.dff
  femaleskin/female.txd
  9/...
  163/...
  ... autres repertoires du catalogue South ...
```

Si les deux fichiers DFF/TXD d'un skin ne sont pas presents dans DATA, le client utilise le modele APK de secours.
