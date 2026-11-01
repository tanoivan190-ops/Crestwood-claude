#pragma once

// [Crestwood] Boutons de combat NATIFS (dessines par le moteur, ImGui), memes dans samp/ (64 bits)
// et samp32/ (32 bits) :
//  - VISER  : a pied avec une arme a feu, bouton toujours affiche. Un appui = visee permanente
//             (on peut viser AVANT de tirer avec le bouton de tir natif), un 2e appui = fin de visee.
//  - TIRER  : passager d'un vehicule avec une mitraillette (Uzi, MP5, Tec-9) : maintenir = drive-by
//             (passe automatiquement en mode drive-by SA-MP puis tire). Le conducteur utilise les
//             boutons natifs de tir lateral de San Andreas (reactives).
// Les touches passent par les hooks CPad de game/pad.cpp (HudPad_SetControl 11 tir / 12 visee),
// donc elles sont synchronisees au serveur comme de vraies touches.
// OnTouch est appele depuis AND_TouchEvent_hook (game/hooks.cpp) et retourne true si la touche
// est pour un de ces boutons (elle n'est alors pas transmise au jeu).
class ImGuiRenderer;

namespace CombatButtons
{
    void Render(ImGuiRenderer* renderer);
    bool OnTouch(int type, int finger, int x, int y);
    bool WantsPassengerDriveBy();   // lu par CLocalPlayer::Process (passager)
}
