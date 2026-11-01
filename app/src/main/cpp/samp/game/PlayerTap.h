#pragma once

// Appui long (5 s) sur un joueur dans le monde 3D -> menu d'interaction du serveur
// (/interaction <id> <x> <y>, x/y = position du joueur a l'ecran en pour-mille).
// OnTouch est appele depuis AND_TouchEvent_hook (game/hooks.cpp) et n'intercepte pas la touche ;
// Render est appele a chaque image depuis UI::drawList (anneau de progression + minuterie).
// Fichier identique dans samp/ (64 bits) et samp32/ (32 bits).
class ImGuiRenderer;

namespace PlayerTap
{
    void OnTouch(int type, int finger, int x, int y);
    void Render(ImGuiRenderer* renderer);
}
