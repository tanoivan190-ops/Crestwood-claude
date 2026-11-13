#pragma once

#include <cstdint>

// ============================================================
// SkinBodies - corps de creation de personnage (boutons du panneau / page clothes).
//
//   genre 0 : Homme    skin 163 (bmyboun)  catalogue homme  (brl_m7_*)
//   genre 1 : Femme    skin 9   (bfori)    catalogue femme  (brl_f9_*)
//   genre 2 : Homme 2  skin 289 (zero)     vetements Clothes.zip (ClothesCatalog.h)
//   genre 3 : Femme 2  skin 12  (bfyri)    corps female.dff du pack [AVZ]Customper
//   genre 4 : Homme 3  skin 7   (male01)   corps male.dff du pack [AVZ]Customper
//
// Les corps AVZ (Femme 2 / Homme 3) ont les memes materiaux que 9 / 163
// (fantasy_f_* / fantasy_m_*, body.*) : ils utilisent le meme catalogue et les memes textures.
// ============================================================

namespace SkinBodies
{
    constexpr uint16_t kMale    = 163;
    constexpr uint16_t kFemale  = 9;
    constexpr uint16_t kMale2   = 289;
    constexpr uint16_t kFemale2 = 12;
    constexpr uint16_t kMale3   = 7;

    constexpr int kGenderCount = 5;

    constexpr uint16_t ModelOfGender(int gender)
    {
        switch (gender) {
            case 1: return kFemale;
            case 2: return kMale2;
            case 3: return kFemale2;
            case 4: return kMale3;
            default: return kMale;
        }
    }

    constexpr int GenderOfModel(uint16_t model)
    {
        return model == kFemale ? 1 : model == kMale2 ? 2 : model == kFemale2 ? 3 : model == kMale3 ? 4 : 0;
    }

    constexpr bool IsValidGender(int gender) { return gender >= 0 && gender < kGenderCount; }
    constexpr bool IsFemaleGender(int gender) { return gender == 1 || gender == 3; }

    // Skins civils de creation (changement de genre autorise entre eux).
    constexpr bool IsCivilBody(uint16_t model)
    {
        return model == kMale || model == kFemale || model == kMale2 || model == kFemale2 || model == kMale3;
    }

    // Skin dont le catalogue de vetements est utilise (corps AVZ -> catalogue 9 / 163).
    constexpr uint16_t ClothesBase(uint16_t model)
    {
        return model == kFemale2 ? kFemale : model == kMale3 ? kMale : model;
    }
}
