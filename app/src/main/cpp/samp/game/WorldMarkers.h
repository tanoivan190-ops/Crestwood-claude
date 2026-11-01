#pragma once

// Marqueurs 3D flottants dessines dans le monde (style GTA V RP) :
//  - type 1 "clothes" : t-shirt blanc translucide qui flotte et tourne (magasins de vetements)
//  - type 2 "garage"  : epingle blanche + anneau au sol (garages)
//  - type 3 "pin"     : epingle generique
//  [Crestwood] Magasins de vetements (type 1) : quand le joueur est sur le point, une MAIN
//  d'interaction s'affiche dans le monde 3D au-dessus du t-shirt ; un appui dessus envoie
//  /interagir au serveur (qui ouvre la boutique la plus proche). OnTouch est appele depuis
//  AND_TouchEvent_hook (game/hooks.cpp) et retourne true si la touche est pour la main.
// Liste envoyee par le serveur (dialogue 9760 "@marks", converti par SAMP.java) via JNI :
//   "type,x,y,z,label;type,x,y,z,label;..."   (label optionnel, sans ',' ni ';')
// Render est appele a chaque image depuis UI::drawList (meme repere que PlayerTap).
// Fichier IDENTIQUE dans samp/ (64 bits) et samp32/ (32 bits).
class ImGuiRenderer;

namespace WorldMarkers
{
    void Set(const char* data);
    void Render(ImGuiRenderer* renderer);
    bool OnTouch(int type, int finger, int x, int y);
}
