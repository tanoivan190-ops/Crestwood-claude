//
// Shared character model loading — one mega-DFF + TXD set per gender.
//
// This is loaded ONCE per gender for the whole session, not per player. Every
// player of that gender renders from the same RpClump and the same texture
// dictionary; per-player differences are material texture swaps only. That is
// what keeps 100-200 players affordable — the geometry and base textures cost
// the same for 1 player as for 200.
//
// Assets live under <storage>/custom/{male,female}skin/ :
//   *.txd  — merged into the ped model's texture dictionary
//   *.dff  — the mega clump (all clothing meshes, one geometry, N materials)
//

#pragma once

#include <string>

#include "game/common.h"

class SouthModelLoader {
public:
    // Loads and installs the model for this gender. Idempotent — a second call
    // returns true immediately. err always carries a human-readable step trace.
    static bool LoadForModel(int modelId, std::string& err);
    static bool IsLoaded(int modelId);

    // appele par main.cpp : plus aucun remplacement de modele (LSPD / JBL MTA retires le 31/10)
    static bool PreloadBodies();

    // Absolute skin asset directory for a model id (may not exist yet).
    static bool GetSkinDir(int modelId, char* out, size_t outSize);

    // Frees the cached source clump for a gender (does not un-replace the
    // model — the game keeps rendering the installed clone).
    static void ReleaseSource(int modelId);
    static void Shutdown();
};
