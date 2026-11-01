#pragma once

// ============================================================
// ClothesAvz - extension du catalogue BRL avec le pack [AVZ]Customper.
// FICHIER GENERE par tools/build_avz_catalog.py : ne pas modifier a la main.
//
//   * Homme (skin 163) : plus de styles haut / bas / cheveux + variantes "marques",
//     et 7 emplacements d'accessoires (chapeau, lunettes, masque, boucles, collier, montre, sac).
//   * Femme (skin 9)   : accessoires (masque, boucles, montre, sac).
//
// Texture cherchee par nom dans la texdb du jeu :
//     brl_m7_clothes_<materiau>_<NN>   (homme)      brl_f9_clothes_<materiau>_<NN>   (femme)
// Texture introuvable => piece masquee (jamais la texture vanilla).
// ============================================================

namespace ClothesAvz
{
    constexpr int kAccessoryValueCount = 14;   // 7 emplacements x (style, couleur) dans ClothesLook / serveur

    struct Opt  { const char* material; const char* prefix; int maxValue; };   // style de vetement
    struct Hide { int style; const char* parts; };                              // parties du corps masquees (style 1-based)
    struct Acc  { const char* material; int variants; };                        // un style d'accessoire = un materiau du dff
    struct Slot { const char* key; const char* label; const char* colorLabel; const char* tab;
                  const Acc* styles; int count; };

    constexpr Opt kMaleHair[] = {
        { "fantasy_m_hair1", "fantasy_m_hair1", 6 },
        { "fantasy_m_hair2", "fantasy_m_hair2", 6 },
        { "fantasy_m_hair3", "fantasy_m_hair3", 5 },
        { "fantasy_m_hair4", "fantasy_m_hair4", 5 },
    };
    constexpr Opt kMaleTop[] = {
        { "fantasy_m_torso1", "fantasy_m_torso1", 27 },
        { "fantasy_m_torso2", "fantasy_m_torso2", 42 },
        { "fantasy_m_torso3", "fantasy_m_torso3", 39 },
        { "fantasy_m_torso4", "fantasy_m_torso4", 32 },
    };
    constexpr Opt kMaleBottom[] = {
        { "fantasy_m_pants1", "fantasy_m_pants1", 17 },
        { "fantasy_m_short1", "fantasy_m_short1", 34 },
        { "fantasy_m_underwear", "fantasy_m_underwear", 17 },
        { "fantasy_m_pants2", "fantasy_m_pants2", 31 },
        { "fantasy_m_pants3", "fantasy_m_pants3", 11 },
        { "fantasy_m_pants4", "fantasy_m_pants4", 40 },
        { "fantasy_m_short2", "fantasy_m_short2", 16 },
        { "fantasy_m_short3", "fantasy_m_short3", 40 },
    };
    constexpr Opt kMaleShoes[] = {
        { "fantasy_m_boot1", "fantasy_m_boot1", 18 },
        { "fantasy_m_boot2", "fantasy_m_boot2", 55 },
        { "fantasy_m_boot3", "fantasy_m_boot3", 23 },
        { "fantasy_m_boot4", "fantasy_m_boot4", 40 },
        { "fantasy_m_slipper1", "fantasy_m_slipper1", 24 },
        { "fantasy_m_slipper2", "fantasy_m_slipper2", 19 },
        { "fantasy_m_slipper3", "fantasy_m_slipper3", 21 },
    };
    constexpr Hide kMaleTopHide[] = {
        { 4, "body_armleft body_armright body_shoulderleft body_shoulderright body_torso1 body_torso2 body_torso3" },
    };
    constexpr Hide kMaleBottomHide[] = {
        { 4, "body_legleft1 body_legleft2 body_legleft3 body_legleft4 body_legright1 body_legright2 body_legright3 body_legright4" },
        { 5, "body_legleft1 body_legleft2 body_legright1 body_legright2" },
        { 6, "body_legleft1 body_legleft2 body_legleft3 body_legright1 body_legright2 body_legright3" },
        { 7, "body_legleft1 body_legright1" },
        { 8, "body_legleft1 body_legright1" },
    };

    constexpr Acc kMaleHat[] = { { "fantasy_m_hat1", 20 } };
    constexpr Acc kMaleGlasse[] = { { "fantasy_m_glasse1", 8 }, { "fantasy_m_glasse2", 9 } };
    constexpr Acc kMaleMask[] = { { "fantasy_m_mask1", 17 }, { "fantasy_m_mask2", 20 } };
    constexpr Acc kMaleEar[] = { { "fantasy_m_ear1", 5 }, { "fantasy_m_ear2", 3 }, { "fantasy_m_ear3", 3 }, { "fantasy_m_ear4", 4 } };
    constexpr Acc kMaleNecklace[] = { { "fantasy_m_necklace1", 20 } };
    constexpr Acc kMaleClock[] = { { "fantasy_m_clock1", 4 }, { "fantasy_m_clock2", 6 } };
    constexpr Acc kMaleBackpack[] = { { "fantasy_m_backpack1", 29 }, { "fantasy_m_backpack2", 40 }, { "fantasy_m_backpack3", 4 } };
    constexpr Slot kMaleSlots[] = {
        { "hat", "Chapeau", "Couleur du chapeau", "tete", kMaleHat, 1 },
        { "glasse", "Lunettes", "Couleur des lunettes", "tete", kMaleGlasse, 2 },
        { "mask", "Masque", "Couleur du masque", "tete", kMaleMask, 2 },
        { "ear", "Boucles d'oreilles", "Couleur des boucles", "bijoux", kMaleEar, 4 },
        { "necklace", "Collier", "Couleur du collier", "bijoux", kMaleNecklace, 1 },
        { "clock", "Montre poignet", "Couleur de la montre", "bijoux", kMaleClock, 2 },
        { "backpack", "Sac à dos", "Couleur du sac", "sacs", kMaleBackpack, 3 },
    };
    constexpr int kMaleSlotCount = 7;

    constexpr Acc kFemaleMask[] = { { "fantasy_f_mask1", 17 }, { "fantasy_f_mask2", 20 } };
    constexpr Acc kFemaleEar[] = { { "fantasy_f_ear1", 3 }, { "fantasy_f_ear2", 4 }, { "fantasy_f_ear3", 1 }, { "fantasy_f_ear4", 3 } };
    constexpr Acc kFemaleClock[] = { { "fantasy_f_clock1", 4 }, { "fantasy_f_clock2", 6 } };
    constexpr Acc kFemaleBackpack[] = { { "fantasy_f_backpack1", 29 }, { "fantasy_f_backpack2", 40 } };
    constexpr Slot kFemaleSlots[] = {
        { "mask", "Masque", "Couleur du masque", "tete", kFemaleMask, 2 },
        { "ear", "Boucles d'oreilles", "Couleur des boucles", "bijoux", kFemaleEar, 4 },
        { "clock", "Montre poignet", "Couleur de la montre", "bijoux", kFemaleClock, 2 },
        { "backpack", "Sac à dos", "Couleur du sac", "sacs", kFemaleBackpack, 2 },
    };
    constexpr int kFemaleSlotCount = 4;
}
