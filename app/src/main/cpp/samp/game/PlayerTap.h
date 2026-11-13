#pragma once

// [Crestwood] Interaction joueurs dans le monde 3D :
//   appui court sur un joueur -> "/interaction3d <id>" ; le serveur repond (dialogue 9636 "@inter3d",
//   texte "id|nom|police|gouv") -> OnServerMenu -> menu natif ancre au joueur, dessine chaque image.
// OnTouch est appele depuis AND_TouchEvent_hook (game/hooks.cpp) : retourne true quand la touche
// appartient au menu (elle ne va alors ni au jeu ni a ImGui).
// Render est appele a chaque image depuis UI::drawList.
// Fichier identique dans samp/ (64 bits) et samp32/ (32 bits).
class ImGuiRenderer;

namespace PlayerTap
{
    bool OnTouch(int type, int finger, int x, int y);
    void Render(ImGuiRenderer* renderer);
    void OnServerMenu(const char* data);   // thread Java : memorise, ouvert par Render
}
