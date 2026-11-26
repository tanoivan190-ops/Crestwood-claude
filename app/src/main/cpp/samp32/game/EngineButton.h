#pragma once

// [Crestwood v35] Bouton MOTEUR natif (dessine par le moteur avec ImGui, pas de HTML).
// Cercle de verre + symbole marche/arret (assets/models/buttons/engine_off.png / engine_on.png),
// visible pour le CONDUCTEUR d'un vehicule. Un appui envoie /moteur au serveur.
// Appui long (0,6 s) puis glisser = deplacer le bouton (position gardee dans crestwood_engine_btn.txt).
// [v38] visible seulement moteur coupe ; pour couper : appui long (5 s) sur le vehicule -> roue.
// Fichier IDENTIQUE dans samp/ (64 bits) et samp32/ (32 bits).
class ImGuiRenderer;

namespace EngineButton
{
    void Render(ImGuiRenderer* renderer);
    bool OnTouch(int type, int finger, int x, int y);
}
