CORRECTIF VETEMENTS ET ICONES HUD — 01/11/2026

1) VETEMENTS / APERCU 3D
- Les corps AVZ homme 7 et femme 12 sont maintenant rediriges vers leurs tables de vetements partagees (163 et 9) au moment d'appliquer les textures.
- Avant ce correctif, ils etaient declares compatibles, mais ApplyCurrent() ne traitait explicitement que les modeles 163 et 9 : les choix du catalogue pouvaient donc changer dans l'interface sans etre appliques au personnage.
- Le correctif est applique aux sources natives 32 bits et 64 bits.
- Les valeurs de depart de base sont conservees : homme en sous-vetements (haut retire, bas underwear, sans chaussures) ; femme avec les pieces de base de type lingerie/bikini deja definies dans le catalogue.

2) ICONES HUD
- Les PNG models/buttons (poing, balle, voiture, chaussure, accroupi, saut, micro) sont presents dans les assets et valides en PNG RGBA 128x128.
- Le drapeau g_cwCustomButtons etait desactive par defaut dans le client 64 bits, alors qu'il etait active dans le client 32 bits. Les deux architectures activent maintenant les icones Crestwood par defaut.
- Le reglage utilisateur existant continue de pouvoir desactiver ces icones.

VALIDATION
- Controle statique des deux copies natives : le routage des corps 7/12 et l'etat par defaut des icones sont identiques en 32/64 bits.
- Presence et format PNG des icones verifies.
- Compilation Android non effectuee : Gradle devait telecharger Gradle 8.13 depuis services.gradle.org, mais l'environnement de compilation n'avait pas de connexion DNS/reseau.
- Le test final sur appareil Android reste necessaire pour confirmer l'actualisation visuelle en jeu et le chargement des icones.
