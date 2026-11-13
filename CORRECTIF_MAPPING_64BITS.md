# Correctif du mapping — client 64 bits

Ce correctif cible le traitement des objets SA-MP reçus par RPC dans `app/src/main/cpp/samp/` (client arm64).

## Changements
- Correction de la borne du pool : `MAX_OBJECTS` est un nombre d'éléments, donc les seuls indices valides sont `0` à `999` (et non `1000`).
- Vérification des identifiants avant tout accès au tableau d'objets.
- Refus d'enregistrer un objet si GTA n'a pas créé son entité (`m_pEntity` nul).
- Vérifications des lectures des champs essentiels du RPC de création d'objet et des longueurs des noms de matériaux.
- Vérification des pointeurs d'objet avant le traitement périodique et le traitement des matériaux texte.
- Aucun objet de mapping ni aucune coordonnée n'a été supprimé ou déplacé par ce correctif.

## Validation
- Les modifications ont été vérifiées statiquement et l'archive ZIP a été contrôlée.
- La compilation Android/NDK et un essai en jeu n'ont pas été effectués dans cet environnement : aucun SDK/NDK Android n'est configuré ici.
