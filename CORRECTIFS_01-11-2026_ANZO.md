# Correctifs vêtements et icônes — 01/11/2026

## Corps alternatifs homme/femme
- Conservation des modèles AVZ alternatifs : homme 7 et femme 12, avec les DFF/TXD du pack South.
- Les modèles 7/12 continuent d'utiliser les catalogues de matériaux partagés 163/9 pour appliquer les vêtements.
- Départ homme : torse sans haut, bas `fantasy_m_underwear`, sans chaussures.
- Départ femme : haut/bas de base de type bikini/lingerie, sans chaussures. Le sélecteur autorise ensuite le choix des chaussures.
- L'application des textures reste synchronisée dans `cpp/samp` et `cpp/samp32`.

## Icônes des commandes HUD
- Les icônes personnalisées restent activées par défaut dans les deux variantes natives.
- Le chargement ne se bloque plus définitivement après dix échecs temporaires. En cas d'échec JNI/asset pendant le démarrage, un nouvel essai est effectué toutes les 120 frames.
- Les fichiers PNG des boutons (poing, balle, voiture, chaussure, accroupi, saut et micro) ont été vérifiés.

## Vérifications effectuées
- Contrôle statique des routages des corps alternatifs et des valeurs de départ.
- Contrôle statique des deux variantes natives.
- Vérification du format et de l'ouverture des PNG.
- Vérification de la présence du pont Java de chargement des assets.

La compilation Android et le test sur téléphone ne sont pas déclarés comme effectués dans cet environnement. Il faudra compiler puis vérifier en jeu sur l'appareil.
