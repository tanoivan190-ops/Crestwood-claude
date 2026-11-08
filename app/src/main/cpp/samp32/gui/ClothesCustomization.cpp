#include "ClothesCustomization.h"
#include "SouthClothes.h"
#include "SouthModelLoader.h"
#include "SkinRegistry.h"
#include "ClothesCatalog.h"
#include "ClothesAvz.h"
#include "SkinBodies.h"

#include <algorithm>
#include <array>
#include <cstdio>
#include <cstring>
#include <string>
#include <unordered_map>
#include <vector>

#include "../main.h"
#include "../game/game.h"
#include "../game/playerped.h"
#include "../game/BrlTextureShader.h"
#include "../net/netgame.h"
#include "../vendor/imgui/imgui.h"

extern CGame* pGame;
extern CNetGame* pNetGame;

namespace {

	ClothesLook g_look;

	constexpr std::array<int, 7> kFaceCountByTone = {0, 6, 8, 8, 7, 5, 12};
	constexpr std::array<int, 7> kFemaleFaceCountByTone = {0, 6, 8, 8, 7, 6, 12};
	constexpr int kDffFaceSlots = 5;

	struct ClothingOption {
		const char* material;
		const char* texturePrefix;
		int maxValue;
	};

	struct FemaleClothingOption {
		std::array<const char*, 2> materials;
		std::array<const char*, 2> texturePrefixes;
		int materialCount;
		int maxValue;
	};

	// Tables homme (cheveux / hauts / bas / chaussures) : generees par tools/build_avz_catalog.py
	// (ClothesAvz.h). Les anciens styles gardent leur indice ; les nouveaux sont ajoutes a la fin.
	template <size_t N>
	constexpr std::array<ClothingOption, N> MakeOptions(const ClothesAvz::Opt (&src)[N])
	{
		std::array<ClothingOption, N> out{};
		for (size_t i = 0; i < N; ++i) out[i] = ClothingOption{ src[i].material, src[i].prefix, src[i].maxValue };
		return out;
	}

	constexpr auto kHairStyles = MakeOptions(ClothesAvz::kMaleHair);
	constexpr auto kTopStyles = MakeOptions(ClothesAvz::kMaleTop);
	constexpr auto kBottomStyles = MakeOptions(ClothesAvz::kMaleBottom);
	constexpr auto kShoesStyles = MakeOptions(ClothesAvz::kMaleShoes);

	constexpr std::array<ClothingOption, 3> kPoliceMaleHairStyles = {{
			{"fantasy_m_hair1", "fantasy_m_hair1", 6},
			{"fantasy_m_hair2", "fantasy_m_hair2", 6},
			{"fantasy_m_hair3", "fantasy_m_hair3", 5}
	}};

	constexpr std::array<ClothingOption, 2> kPoliceMaleTopStyles = {{
			{"fantasy_m_cop_torso2", "fantasy_m_cop_torso2", 1},
			{"fantasy_m_cop_torso4", "fantasy_m_cop_torso4", 2}
	}};

	constexpr std::array<ClothingOption, 2> kPoliceMaleBottomStyles = {{
			{"fantasy_m_underwear", "fantasy_m_underwear", 1},
			{"fantasy_m_cop_pants1", "fantasy_m_cop_pants1", 1}
	}};

	constexpr std::array<ClothingOption, 2> kPoliceMaleShoesStyles = {{
			{"fantasy_m_cop_boot1", "fantasy_m_cop_boot1", 1},
			{"fantasy_m_cop_boot2", "fantasy_m_cop_boot2", 1}
	}};

	constexpr std::array<FemaleClothingOption, 5> kFemaleHairStyles = {{
			{{"fantasy_f_hair1", nullptr}, {"fantasy_f_hair1", nullptr}, 1, 6},
			{{"fantasy_f_hair2", nullptr}, {"fantasy_f_hair2", nullptr}, 1, 6},
			{{"fantasy_f_hair3", nullptr}, {"fantasy_f_hair3", nullptr}, 1, 7},
			{{"fantasy_f_hair4", nullptr}, {"fantasy_f_hair4", nullptr}, 1, 7},
			{{"fantasy_f_hair5", nullptr}, {"fantasy_f_hair5", nullptr}, 1, 7}
	}};

	constexpr std::array<FemaleClothingOption, 4> kFemaleTopStyles = {{
			{{"fantasy_f_bikinitop", nullptr}, {"fantasy_f_bikinitop", nullptr}, 1, 12},
			{{"fantasy_f_torso5", nullptr}, {"fantasy_f_torso5", nullptr}, 1, 35},
			{{"fantasy_f_torso2", nullptr}, {"fantasy_f_torso2", nullptr}, 1, 20},
			{{"fantasy_f_torso6", nullptr}, {"fantasy_f_torso6", nullptr}, 1, 20}
	}};

	constexpr std::array<FemaleClothingOption, 4> kFemaleBottomStyles = {{
			{{"fantasy_f_bikinibotton", nullptr}, {"fantasy_f_bikinibotton", nullptr}, 1, 12},
			{{"fantasy_f_pants1", nullptr}, {"fantasy_f_pants1", nullptr}, 1, 16},
			{{"fantasy_f_pants5", nullptr}, {"fantasy_f_pants5", nullptr}, 1, 40},
			{{"fantasy_f_short1", nullptr}, {"fantasy_f_short1", nullptr}, 1, 11}
	}};

	constexpr std::array<FemaleClothingOption, 4> kFemaleShoesStyles = {{
			{{"fantasy_f_boot3", nullptr}, {"fantasy_f_boot3", nullptr}, 1, 16},
			{{"fantasy_f_boot5", nullptr}, {"fantasy_f_boot5", nullptr}, 1, 16},
			{{"fantasy_f_slipper1", nullptr}, {"fantasy_f_slipper1", nullptr}, 1, 24},
			{{"fantasy_f_slipper3", nullptr}, {"fantasy_f_slipper3", nullptr}, 1, 21}
	}};

	constexpr std::array<FemaleClothingOption, 5> kPoliceFemaleHairStyles = {{
			{{"fantasy_f_hair1", nullptr}, {"fantasy_f_hair1", nullptr}, 1, 6},
			{{"fantasy_f_hair2", nullptr}, {"fantasy_f_hair2", nullptr}, 1, 6},
			{{"fantasy_f_hair3", nullptr}, {"fantasy_f_hair3", nullptr}, 1, 7},
			{{"fantasy_f_hair4", nullptr}, {"fantasy_f_hair4", nullptr}, 1, 7},
			{{"fantasy_f_hair5", nullptr}, {"fantasy_f_hair5", nullptr}, 1, 7}
	}};

	constexpr std::array<FemaleClothingOption, 2> kPoliceFemaleTopStyles = {{
			{{"fantasy_f_cop_torso2", nullptr}, {"fantasy_f_cop_torso2", nullptr}, 1, 1},
			{{"fantasy_f_cop_torso4", nullptr}, {"fantasy_f_cop_torso4", nullptr}, 1, 2}
	}};

	constexpr std::array<FemaleClothingOption, 2> kPoliceFemaleBottomStyles = {{
			{{"fantasy_f_cop_pants1", nullptr}, {"fantasy_f_cop_pants1", nullptr}, 1, 1},
			{{"fantasy_f_cop_short1", nullptr}, {"fantasy_f_cop_short1", nullptr}, 1, 1}
	}};

	constexpr std::array<FemaleClothingOption, 2> kPoliceFemaleShoesStyles = {{
			{{"fantasy_f_cop_boot1", nullptr}, {"fantasy_f_cop_boot1", nullptr}, 1, 1},
			{{"fantasy_f_cop_boot2", nullptr}, {"fantasy_f_cop_boot2", nullptr}, 1, 1}
	}};

	constexpr std::array<const char*, 9> kTorsoMaterials = {
			"body_torso1", "body_torso2", "body_torso3", "body_torso4",
			"body_armright", "body_armleft", "body_shoulderleft", "body_shoulderright",
			"body_hand"
	};

	constexpr std::array<const char*, 12> kLegMaterials = {
			"body_legleft1", "body_legleft2", "body_legleft3", "body_legleft4", "body_legleft5", "body_legleft6",
			"body_legright1", "body_legright2", "body_legright3", "body_legright4", "body_legright5", "body_legright6"
	};

	constexpr std::array<const char*, 4> kFeetMaterials = {
			"body_feetleft", "body_feetright", "body_feets", "body_feets2"
	};

	constexpr std::array<const char*, 9> kFemaleTorsoMaterials = {
			"body_torso1", "body_torso2", "body_torso3", "body_torso4", "body_armright",
			"body_armleft", "body_shoulderleft", "body_shoulderright", "body_hand"
	};

	constexpr std::array<const char*, 12> kFemaleLegMaterials = {
			"body_legleft1", "body_legleft2", "body_legleft3", "body_legleft4", "body_legleft5", "body_legleft6",
			"body_legright1", "body_legright2", "body_legright3", "body_legright4", "body_legright5", "body_legright6"
	};

	constexpr std::array<const char*, 4> kFemaleFeetMaterials = {
			"body_feetleft", "body_feetright", "body_feets", "body_feets2"
	};

	constexpr std::array<const char*, 9> kPoliceMaleTorsoMaterials = {
			"body_torso1", "body_torso2", "body_torso3", "body_torso4", "body_armright",
			"body_armleft", "body_shoulderleft", "body_shoulderright", "body_hand"
	};

	constexpr std::array<const char*, 12> kPoliceMaleLegMaterials = {
			"body_legleft1", "body_legleft2", "body_legleft3", "body_legleft4", "body_legleft5", "body_legleft6",
			"body_legright1", "body_legright2", "body_legright3", "body_legright4", "body_legright5", "body_legright6"
	};

	constexpr std::array<const char*, 4> kPoliceMaleFeetMaterials = {
			"body_feetleft", "body_feetright", "body_feets", "body_feets2"
	};

	constexpr std::array<const char*, 25> kBodySkinMaterials = {
			"body_torso1", "body_torso2", "body_torso3", "body_torso4",
			"body_armright", "body_armleft", "body_shoulderleft", "body_shoulderright", "body_hand",
			"body_legleft1", "body_legleft2", "body_legleft3", "body_legleft4", "body_legleft5", "body_legleft6",
			"body_legright1", "body_legright2", "body_legright3", "body_legright4", "body_legright5", "body_legright6",
			"body_feetleft", "body_feetright", "body_feets", "body_feets2"
	};

	constexpr std::array<const char*, 16> kAllClothingMaterials = {
			"fantasy_m_hair1", "fantasy_m_hair2", "fantasy_m_hair3",
			"fantasy_m_torso1", "fantasy_m_torso2", "fantasy_m_torso3",
			"fantasy_m_pants1", "fantasy_m_short1", "fantasy_m_underwear",
			"fantasy_m_boot1", "fantasy_m_boot2", "fantasy_m_boot3", "fantasy_m_boot4",
			"fantasy_m_slipper1", "fantasy_m_slipper2", "fantasy_m_slipper3"
	};

	constexpr int kMaleEyebrowStyleCount = 5;
	constexpr int kMaleBeardStyleCount = 5;
	constexpr int kMaleEyebrowColorCount = 54;
	constexpr int kMaleBeardColorCount = 54;
	constexpr int kMaleTattooArmCount = 38;
	constexpr int kMaleTattooLegCount = 26;
	constexpr int kMaleTattooBackCount = 16;
	constexpr int kMaleTattooFrontCount = 25;
	constexpr int kFemaleMakeupStyleCount = 5;
	constexpr int kFemaleMakeupColorCount = 26;
	constexpr const char* kClearTexture = "south/clear.png";

	void ClampAccessories(bool female);
	void WantAccessories(std::unordered_map<std::string, std::string>& wanted, bool female);

	constexpr std::array<const char*, kMaleEyebrowStyleCount> kMaleEyebrowMaterials = {
			"body_eyebrown1", "body_eyebrown2", "body_eyebrown3", "body_eyebrown4", "body_eyebrown5"
	};

	constexpr std::array<const char*, kMaleBeardStyleCount> kMaleBeardMaterials = {
			"body_beards1", "body_beards2", "body_beards3", "body_beards4", "body_beards5"
	};

	constexpr std::array<const char*, kFemaleMakeupStyleCount> kFemaleMakeupMaterials = {
			"body_makeup1", "body_makeup2", "body_makeup3", "body_makeup4", "body_makeup5"
	};

	constexpr std::array<std::array<const char*, 3>, kDffFaceSlots> kFaceMaterialSlots = {{
			{"body_head1", "body_eyebrown1", "body_beards1"},
			{"body_head2", "body_eyebrown2", "body_beards2"},
			{"body_head3", "body_eyebrown3", "body_beards3"},
			{"body_head4", "body_eyebrown4", "body_beards4"},
			{"body_head5", "body_eyebrown5", "body_beards5"}
	}};

	constexpr std::array<const char*, kDffFaceSlots * 3> kAllFaceMaterials = {
			"body_head1", "body_eyebrown1", "body_beards1",
			"body_head2", "body_eyebrown2", "body_beards2",
			"body_head3", "body_eyebrown3", "body_beards3",
			"body_head4", "body_eyebrown4", "body_beards4",
			"body_head5", "body_eyebrown5", "body_beards5"
	};

	constexpr const char* kFemaleStableFaceMaterial = "body_head5";
	constexpr std::array<const char*, kDffFaceSlots> kPoliceMaleFaceMaterials = {
			"body_head1", "body_head2", "body_head3", "body_head4", "body_head5"
	};

	std::string MobileTextureName(const char* suffix, int tone, int face = 0)
	{
		char name[64]{};
		if (face > 0) {
			std::snprintf(name, sizeof(name), "brl_m7_skin_t%02d_%s%02d", tone, suffix, face);
		}
		else {
			std::snprintf(name, sizeof(name), "brl_m7_skin_t%02d_%s", tone, suffix);
		}
		return name;
	}

	std::string FemaleMobileTextureName(const char* suffix, int tone, int face = 0)
	{
		char name[64]{};
		if (face > 0) {
			std::snprintf(name, sizeof(name), "brl_f9_skin_t%02d_%s%02d", tone, suffix, face);
		}
		else {
			std::snprintf(name, sizeof(name), "brl_f9_skin_t%02d_%s", tone, suffix);
		}
		return name;
	}

	std::string ClothingTextureName(const char* texturePrefix, int value)
	{
		char name[96]{};
		std::snprintf(name, sizeof(name), "brl_m7_clothes_%s_%02d", texturePrefix, value);
		return name;
	}

	std::string FemaleClothingTextureName(const char* texturePrefix, int value)
	{
		char name[96]{};
		std::snprintf(name, sizeof(name), "brl_f9_clothes_%s_%02d", texturePrefix, value);
		return name;
	}

	std::string PoliceMaleTextureName(const char* texturePrefix, int value)
	{
		char name[112]{};
		std::snprintf(name, sizeof(name), "brl_m271_police_%s_%02d", texturePrefix, value);
		return name;
	}

	std::string PoliceFemaleTextureName(const char* texturePrefix, int value)
	{
		char name[112]{};
		std::snprintf(name, sizeof(name), "brl_f272_police_%s_%02d", texturePrefix, value);
		return name;
	}

	std::string MaleEyebrowTextureName(int value)
	{
		char name[64]{};
		std::snprintf(name, sizeof(name), "brl_m7_eyebrown_%02d", value);
		return name;
	}

	std::string MaleBeardTextureName(int value)
	{
		char name[64]{};
		std::snprintf(name, sizeof(name), "brl_m7_beard_%02d", value);
		return name;
	}

	std::string MaleTattooTextureName(const char* area, int value)
	{
		char name[64]{};
		std::snprintf(name, sizeof(name), "brl_m7_tattoo_%s_%02d", area, value);
		return name;
	}

	// Clamp para campos uint8_t do ClothesLook (evita conflito de tipos no std::clamp).
	uint8_t ClampU8(int value, int lo, int hi)
	{
		return static_cast<uint8_t>(std::clamp(value, lo, hi));
	}

	void ClampSelection()
	{
		g_look.skinTone = ClampU8(g_look.skinTone, 1, 6);
		g_look.face = ClampU8(g_look.face, 1, std::min(kFaceCountByTone[g_look.skinTone], kDffFaceSlots));
		g_look.hairStyle = ClampU8(g_look.hairStyle, 1, static_cast<int>(kHairStyles.size()));
		// topStyle/shoesStyle: 0 = sem camisa / descalço (começa só de cueca)
		g_look.topStyle = ClampU8(g_look.topStyle, 0, static_cast<int>(kTopStyles.size()));
		g_look.bottomStyle = ClampU8(g_look.bottomStyle, 1, static_cast<int>(kBottomStyles.size()));
		g_look.shoesStyle = ClampU8(g_look.shoesStyle, 0, static_cast<int>(kShoesStyles.size()));
		g_look.hairColor = ClampU8(g_look.hairColor, 1, kHairStyles[g_look.hairStyle - 1].maxValue);
		if (g_look.topStyle > 0) {
			g_look.topColor = ClampU8(g_look.topColor, 1, kTopStyles[g_look.topStyle - 1].maxValue);
		}
		g_look.bottomColor = ClampU8(g_look.bottomColor, 1, kBottomStyles[g_look.bottomStyle - 1].maxValue);
		if (g_look.shoesStyle > 0) {
			g_look.shoesColor = ClampU8(g_look.shoesColor, 1, kShoesStyles[g_look.shoesStyle - 1].maxValue);
		}
		g_look.eyebrowStyle = ClampU8(g_look.eyebrowStyle, 0, kMaleEyebrowStyleCount);
		g_look.eyebrowColor = ClampU8(g_look.eyebrowColor, 1, kMaleEyebrowColorCount);
		g_look.beardStyle = ClampU8(g_look.beardStyle, 0, kMaleBeardStyleCount);
		g_look.beardColor = ClampU8(g_look.beardColor, 1, kMaleBeardColorCount);
		g_look.tattooArmLeft = ClampU8(g_look.tattooArmLeft, 0, kMaleTattooArmCount);
		g_look.tattooArmRight = ClampU8(g_look.tattooArmRight, 0, kMaleTattooArmCount);
		g_look.tattooLegLeft = ClampU8(g_look.tattooLegLeft, 0, kMaleTattooLegCount);
		g_look.tattooLegRight = ClampU8(g_look.tattooLegRight, 0, kMaleTattooLegCount);
		g_look.tattooBack = ClampU8(g_look.tattooBack, 0, kMaleTattooBackCount);
		g_look.tattooFront = ClampU8(g_look.tattooFront, 0, kMaleTattooFrontCount);
		ClampAccessories(false);
	}

	void ClampFemaleSelection()
	{
		g_look.skinTone = ClampU8(g_look.skinTone, 1, 6);
		g_look.face = ClampU8(g_look.face, 1, kFemaleFaceCountByTone[g_look.skinTone]);
		g_look.hairStyle = ClampU8(g_look.hairStyle, 1, static_cast<int>(kFemaleHairStyles.size()));
		g_look.topStyle = ClampU8(g_look.topStyle, 1, static_cast<int>(kFemaleTopStyles.size()));
		g_look.bottomStyle = ClampU8(g_look.bottomStyle, 1, static_cast<int>(kFemaleBottomStyles.size()));
		g_look.shoesStyle = ClampU8(g_look.shoesStyle, 1, static_cast<int>(kFemaleShoesStyles.size()));
		g_look.hairColor = ClampU8(g_look.hairColor, 1, kFemaleHairStyles[g_look.hairStyle - 1].maxValue);
		g_look.topColor = ClampU8(g_look.topColor, 1, kFemaleTopStyles[g_look.topStyle - 1].maxValue);
		g_look.bottomColor = ClampU8(g_look.bottomColor, 1, kFemaleBottomStyles[g_look.bottomStyle - 1].maxValue);
		g_look.shoesColor = ClampU8(g_look.shoesColor, 1, kFemaleShoesStyles[g_look.shoesStyle - 1].maxValue);
		g_look.makeupStyle = ClampU8(g_look.makeupStyle, 0, kFemaleMakeupStyleCount);
		g_look.makeupColor = ClampU8(g_look.makeupColor, 1, kFemaleMakeupColorCount);
		ClampAccessories(true);
	}

	void ClampPoliceMaleSelection()
	{
		g_look.skinTone = ClampU8(g_look.skinTone, 1, 6);
		g_look.face = ClampU8(g_look.face, 1, std::min(kFaceCountByTone[g_look.skinTone], kDffFaceSlots));
		g_look.hairStyle = ClampU8(g_look.hairStyle, 1, static_cast<int>(kPoliceMaleHairStyles.size()));
		g_look.topStyle = ClampU8(g_look.topStyle, 1, static_cast<int>(kPoliceMaleTopStyles.size()));
		g_look.bottomStyle = ClampU8(g_look.bottomStyle, 1, static_cast<int>(kPoliceMaleBottomStyles.size()));
		g_look.shoesStyle = ClampU8(g_look.shoesStyle, 1, static_cast<int>(kPoliceMaleShoesStyles.size()));
		g_look.hairColor = ClampU8(g_look.hairColor, 1, kPoliceMaleHairStyles[g_look.hairStyle - 1].maxValue);
		g_look.topColor = ClampU8(g_look.topColor, 1, kPoliceMaleTopStyles[g_look.topStyle - 1].maxValue);
		g_look.bottomColor = ClampU8(g_look.bottomColor, 1, kPoliceMaleBottomStyles[g_look.bottomStyle - 1].maxValue);
		g_look.shoesColor = ClampU8(g_look.shoesColor, 1, kPoliceMaleShoesStyles[g_look.shoesStyle - 1].maxValue);
		g_look.policeBelt = ClampU8(g_look.policeBelt, 0, 1);
		g_look.policeVest = ClampU8(g_look.policeVest, 0, 1);
		g_look.policeHat = ClampU8(g_look.policeHat, 0, 1);
	}

	void ClampPoliceFemaleSelection()
	{
		g_look.skinTone = ClampU8(g_look.skinTone, 1, 6);
		g_look.face = ClampU8(g_look.face, 1, kFemaleFaceCountByTone[g_look.skinTone]);
		g_look.hairStyle = ClampU8(g_look.hairStyle, 1, static_cast<int>(kPoliceFemaleHairStyles.size()));
		g_look.topStyle = ClampU8(g_look.topStyle, 1, static_cast<int>(kPoliceFemaleTopStyles.size()));
		g_look.bottomStyle = ClampU8(g_look.bottomStyle, 1, static_cast<int>(kPoliceFemaleBottomStyles.size()));
		g_look.shoesStyle = ClampU8(g_look.shoesStyle, 1, static_cast<int>(kPoliceFemaleShoesStyles.size()));
		g_look.hairColor = ClampU8(g_look.hairColor, 1, kPoliceFemaleHairStyles[g_look.hairStyle - 1].maxValue);
		g_look.topColor = ClampU8(g_look.topColor, 1, kPoliceFemaleTopStyles[g_look.topStyle - 1].maxValue);
		g_look.bottomColor = ClampU8(g_look.bottomColor, 1, kPoliceFemaleBottomStyles[g_look.bottomStyle - 1].maxValue);
		g_look.shoesColor = ClampU8(g_look.shoesColor, 1, kPoliceFemaleShoesStyles[g_look.shoesStyle - 1].maxValue);
		g_look.policeBelt = ClampU8(g_look.policeBelt, 0, 1);
		g_look.policeVest = ClampU8(g_look.policeVest, 0, 1);
		g_look.policeHat = ClampU8(g_look.policeHat, 0, 1);
	}

	// Contexte d'application : par defaut le ped local. ApplyRemote() le bascule
	// temporairement sur un joueur distant (modele force + regles de ce joueur).
	uint16_t g_ctxPlayer = 0xFFFF;
	uint16_t g_ctxModel = 0;
	std::unordered_map<uint16_t, std::unordered_map<std::string, std::string>> g_remoteApplied;

	// Vrai skin du ped (7, 12, 163...).
	uint16_t RealLocalModelId()
	{
		if (g_ctxModel != 0) return g_ctxModel;
		if (!pGame) return 0;
		CPlayerPed* ped = pGame->FindPlayerPed();
		if (!ped || !ped->m_pPed) return 0;
		return static_cast<uint16_t>(ped->m_pPed->m_nModelIndex);
	}

	// Skin dont on utilise le catalogue : Homme 3 (7) -> 163, Femme 2 (12) -> 9 (SkinBodies.h).
	uint16_t CurrentLocalModelId()
	{
		return SkinBodies::ClothesBase(RealLocalModelId());
	}

	bool CurrentLocalPedReady()
	{
		if (g_ctxModel != 0) return true;
		if (!pGame) return false;
		CPlayerPed* ped = pGame->FindPlayerPed();
		return ped && ped->m_pPed && ped->m_pPed->m_pRwObject;
	}

	bool IsFemaleSkin()
	{
		return CurrentLocalModelId() == 9;
	}

	bool IsPoliceFemaleSkin()
	{
		return CurrentLocalModelId() == 246;
	}

	bool IsPoliceMaleSkin()
	{
		return CurrentLocalModelId() == 280;
	}

	// Parties du corps masquees par un style ajoute (liste "body_xxx body_yyy" generee depuis le pack AVZ).
	template <size_t N>
	bool HiddenByAvzStyle(const ClothesAvz::Hide (&table)[N], int style, const std::string& material)
	{
		for (const ClothesAvz::Hide& h : table) {
			if (h.style != style || !h.parts[0]) continue;
			const std::string parts = std::string(" ") + h.parts + " ";
			if (parts.find(" " + material + " ") != std::string::npos) return true;
		}
		return false;
	}

	bool IsBodyMaterialHidden(const char* material)
	{
		const std::string value = material ? material : "";
		if (IsFemaleSkin()) {
			std::string femaleValue = value;
			std::replace(femaleValue.begin(), femaleValue.end(), '.', '_');
			if (g_look.bottomStyle == 2 || g_look.bottomStyle == 3) {
				if (femaleValue == "body_legright1" || femaleValue == "body_legleft1" ||
						femaleValue == "body_legright2" || femaleValue == "body_legleft2" ||
						femaleValue == "body_legright3" || femaleValue == "body_legleft3") return true;
			}
			if (g_look.bottomStyle == 4) {
				if (femaleValue == "body_legleft1" || femaleValue == "body_legright1") return true;
			}
			if (g_look.bottomStyle != 3) {
				if (femaleValue == "body_legleft6" || femaleValue == "body_legright6") return true;
			}
			if (g_look.shoesStyle == 1 || g_look.shoesStyle == 2 || g_look.shoesStyle == 4) {
				if (femaleValue == "body_feets") return true;
			}
			if (g_look.shoesStyle == 2) {
				if (femaleValue == "body_legright5" || femaleValue == "body_legleft5") return true;
			}
			if (g_look.topStyle == 2) {
				if (femaleValue == "body_torso1" || femaleValue == "body_torso3" ||
						femaleValue == "body_shoulderleft" || femaleValue == "body_shoulderright" ||
						femaleValue == "body_armright" || femaleValue == "body_armleft") return true;
			}
			if (g_look.topStyle == 4) {
				if (femaleValue == "body_torso4" || femaleValue == "body_torso3" ||
						femaleValue == "body_legleft1" || femaleValue == "body_legright1") return true;
			}
			return false;
		}

		if (IsPoliceFemaleSkin()) {
			std::string femaleValue = value;
			std::replace(femaleValue.begin(), femaleValue.end(), '.', '_');
			if (g_look.bottomStyle == 1) {
				if (femaleValue == "body_legleft1" || femaleValue == "body_legright1" ||
						femaleValue == "body_legright2" || femaleValue == "body_legleft2" ||
						femaleValue == "body_legright3" || femaleValue == "body_legleft3" ||
						femaleValue == "body_legleft4" || femaleValue == "body_legright4") return true;
			}
			if (g_look.bottomStyle == 2) {
				if (femaleValue == "body_legleft1" || femaleValue == "body_legright1" ||
						femaleValue == "body_legright2" || femaleValue == "body_legleft2") return true;
			}
			if (g_look.shoesStyle >= 1 && g_look.shoesStyle <= static_cast<int>(kPoliceFemaleShoesStyles.size())) {
				if (femaleValue == "body_legright5" || femaleValue == "body_legleft5" || femaleValue == "body_feets") return true;
			}
			if (g_look.topStyle == 1) {
				if (femaleValue == "body_torso1" || femaleValue == "body_torso3" ||
						femaleValue == "body_shoulderleft" || femaleValue == "body_shoulderright" ||
						femaleValue == "body_torso4") return true;
			}
			if (g_look.topStyle == 2) {
				if (femaleValue == "body_torso1" || femaleValue == "body_shoulderleft" ||
						femaleValue == "body_shoulderright" || femaleValue == "body_torso3" ||
						femaleValue == "body_torso4" || femaleValue == "body_armright" ||
						femaleValue == "body_armleft" || femaleValue == "fantasy_tatto_arm_l" ||
						femaleValue == "fantasy_tatto_arm_r") return true;
			}
			return false;
		}

		if (IsPoliceMaleSkin()) {
			std::string policeMaleValue = value;
			std::replace(policeMaleValue.begin(), policeMaleValue.end(), '.', '_');
			if (g_look.topStyle == 1) {
				if (policeMaleValue == "body_shoulderleft" || policeMaleValue == "body_shoulderright" ||
						policeMaleValue == "body_torso1" || policeMaleValue == "fantasy_tatto_torso_back" ||
						policeMaleValue == "fantasy_tatto_torso_front") return true;
			}
			if (g_look.topStyle == 2) {
				if (policeMaleValue == "body_shoulderleft" || policeMaleValue == "body_shoulderright" ||
						policeMaleValue == "body_armleft" || policeMaleValue == "body_armright" ||
						policeMaleValue == "fantasy_tatto_arm_l" || policeMaleValue == "fantasy_tatto_arm_r") return true;
			}
			if (g_look.bottomStyle == 2) {
				if (policeMaleValue == "body_legleft1" || policeMaleValue == "body_legleft2" ||
						policeMaleValue == "body_legleft3" || policeMaleValue == "body_legright1" ||
						policeMaleValue == "body_legright2" || policeMaleValue == "body_legright3" ||
						policeMaleValue == "fantasy_tatto_leg_l_3" || policeMaleValue == "fantasy_tatto_leg_r_3") return true;
			}
			if (g_look.shoesStyle >= 1 && g_look.shoesStyle <= 2) {
				if (policeMaleValue == "body_feetright" || policeMaleValue == "body_feetleft") return true;
			}
			return false;
		}

		std::string maleValue = value;
		std::replace(maleValue.begin(), maleValue.end(), '.', '_');
		if (g_look.topStyle == 3) {
			if (maleValue == "body_shoulderright" || maleValue == "body_shoulderleft" ||
					maleValue == "body_armleft" || maleValue == "body_armright") return true;
		}
		if (g_look.topStyle == 2) {
			if (maleValue == "body_shoulderright" || maleValue == "body_shoulderleft" || maleValue == "body_torso1") return true;
		}
		if (g_look.bottomStyle == 1) {
			if (maleValue == "body_legright1" || maleValue == "body_legleft1" ||
					maleValue == "body_legleft2" || maleValue == "body_legright2" ||
					maleValue == "body_legleft3" || maleValue == "body_legright3") return true;
		}
		if (g_look.bottomStyle == 2) {
			if (maleValue == "body_legleft1" || maleValue == "body_legright1") return true;
		}
		if (g_look.shoesStyle >= 1 && g_look.shoesStyle <= 4) {
			if (maleValue == "body_feetright" || maleValue == "body_feetleft") return true;
		}
		if (HiddenByAvzStyle(ClothesAvz::kMaleTopHide, g_look.topStyle, maleValue)) return true;
		if (HiddenByAvzStyle(ClothesAvz::kMaleBottomHide, g_look.bottomStyle, maleValue)) return true;
		return false;
	}

	void WantTexture(std::unordered_map<std::string, std::string>& wanted, const char* material, const std::string& texture)
	{
		if (!material || texture.empty()) return;
		wanted[material] = texture;
	}

	void WantList(std::unordered_map<std::string, std::string>& wanted, const auto& materials, const std::string& texture)
	{
		for (const char* material : materials) {
			if (!IsBodyMaterialHidden(material)) {
				WantTexture(wanted, material, texture);
			}
		}
	}

	void WantClearList(std::unordered_map<std::string, std::string>& wanted, const auto& materials)
	{
		for (const char* material : materials) {
			WantTexture(wanted, material, kClearTexture);
		}
	}

	void WantClothing(std::unordered_map<std::string, std::string>& wanted, const ClothingOption& option, int color)
	{
		const int safeColor = std::clamp(color, 1, option.maxValue);
		WantTexture(wanted, option.material, ClothingTextureName(option.texturePrefix, safeColor));
	}

	void WantPoliceMaleClothing(std::unordered_map<std::string, std::string>& wanted, const ClothingOption& option, int color)
	{
		const int safeColor = std::clamp(color, 1, option.maxValue);
		WantTexture(wanted, option.material, PoliceMaleTextureName(option.texturePrefix, safeColor));
	}

	void WantFemaleClothing(std::unordered_map<std::string, std::string>& wanted, const FemaleClothingOption& option, int color)
	{
		const int safeColor = std::clamp(color, 1, option.maxValue);
		for (int i = 0; i < option.materialCount; ++i) {
			const char* material = option.materials[i];
			const char* texturePrefix = option.texturePrefixes[i] ? option.texturePrefixes[i] : material;
			WantTexture(wanted, material, FemaleClothingTextureName(texturePrefix, safeColor));
		}
	}

	void WantPoliceFemaleClothing(std::unordered_map<std::string, std::string>& wanted, const FemaleClothingOption& option, int color)
	{
		const int safeColor = std::clamp(color, 1, option.maxValue);
		for (int i = 0; i < option.materialCount; ++i) {
			const char* material = option.materials[i];
			const char* texturePrefix = option.texturePrefixes[i] ? option.texturePrefixes[i] : material;
			WantTexture(wanted, material, PoliceFemaleTextureName(texturePrefix, safeColor));
		}
	}

	void ApplyMaleFaceAccessoriesAndTattoos(std::unordered_map<std::string, std::string>& wanted)
	{
		for (const char* material : kMaleEyebrowMaterials) {
			WantTexture(wanted, material, kClearTexture);
		}
		if (g_look.eyebrowStyle > 0) {
			WantTexture(wanted, kMaleEyebrowMaterials[g_look.eyebrowStyle - 1], MaleEyebrowTextureName(g_look.eyebrowColor));
		}

		for (const char* material : kMaleBeardMaterials) {
			WantTexture(wanted, material, kClearTexture);
		}
		if (g_look.beardStyle > 0) {
			WantTexture(wanted, kMaleBeardMaterials[g_look.beardStyle - 1], MaleBeardTextureName(g_look.beardColor));
		}

		WantTexture(wanted, "fantasy_tatto_arm_l", g_look.tattooArmLeft > 0 ? MaleTattooTextureName("arm", g_look.tattooArmLeft) : kClearTexture);
		WantTexture(wanted, "fantasy_tatto_arm_r", g_look.tattooArmRight > 0 ? MaleTattooTextureName("arm", g_look.tattooArmRight) : kClearTexture);
		WantTexture(wanted, "fantasy_tatto_leg_l_3", g_look.tattooLegLeft > 0 ? MaleTattooTextureName("leg", g_look.tattooLegLeft) : kClearTexture);
		WantTexture(wanted, "fantasy_tatto_leg_r_3", g_look.tattooLegRight > 0 ? MaleTattooTextureName("leg", g_look.tattooLegRight) : kClearTexture);
		WantTexture(wanted, "fantasy_tatto_torso_back", g_look.tattooBack > 0 ? MaleTattooTextureName("back", g_look.tattooBack) : kClearTexture);
		WantTexture(wanted, "fantasy_tatto_torso_front", g_look.tattooFront > 0 ? MaleTattooTextureName("front", g_look.tattooFront) : kClearTexture);
	}

	void ApplyFemaleFaceAccessoriesAndTattoos(std::unordered_map<std::string, std::string>& wanted)
	{
		for (const char* material : kMaleEyebrowMaterials) {
			WantTexture(wanted, material, kClearTexture);
		}
		if (g_look.eyebrowStyle > 0) {
			WantTexture(wanted, kMaleEyebrowMaterials[g_look.eyebrowStyle - 1], MaleEyebrowTextureName(g_look.eyebrowColor));
		}

		for (const char* material : kFemaleMakeupMaterials) {
			WantTexture(wanted, material, kClearTexture);
		}
		if (g_look.makeupStyle > 0) {
			char name[64]{};
			std::snprintf(name, sizeof(name), "brl_f9_makeup_%02d", g_look.makeupColor);
			WantTexture(wanted, kFemaleMakeupMaterials[g_look.makeupStyle - 1], name);
		}

		WantTexture(wanted, "fantasy_tatto_arm_l", g_look.tattooArmLeft > 0 ? MaleTattooTextureName("arm", g_look.tattooArmLeft) : kClearTexture);
		WantTexture(wanted, "fantasy_tatto_arm_r", g_look.tattooArmRight > 0 ? MaleTattooTextureName("arm", g_look.tattooArmRight) : kClearTexture);
		WantTexture(wanted, "fantasy_tatto_leg_l_3", g_look.tattooLegLeft > 0 ? MaleTattooTextureName("leg", g_look.tattooLegLeft) : kClearTexture);
		WantTexture(wanted, "fantasy_tatto_leg_r_3", g_look.tattooLegRight > 0 ? MaleTattooTextureName("leg", g_look.tattooLegRight) : kClearTexture);
		WantTexture(wanted, "fantasy_tatto_torso_back", g_look.tattooBack > 0 ? std::string("brl_f9_tattoo_back_") + (g_look.tattooBack < 10 ? "0" : "") + std::to_string(g_look.tattooBack) : kClearTexture);
		WantTexture(wanted, "fantasy_tatto_torso_front2", g_look.tattooFront > 0 ? std::string("brl_f9_tattoo_front_") + (g_look.tattooFront < 10 ? "0" : "") + std::to_string(g_look.tattooFront) : kClearTexture);
	}

	void ApplyPoliceFemaleFaceAccessories(std::unordered_map<std::string, std::string>& wanted)
	{
		for (const char* material : kMaleEyebrowMaterials) {
			WantTexture(wanted, material, kClearTexture);
		}
		if (g_look.eyebrowStyle > 0) {
			WantTexture(wanted, kMaleEyebrowMaterials[g_look.eyebrowStyle - 1], MaleEyebrowTextureName(g_look.eyebrowColor));
		}

		for (const char* material : kFemaleMakeupMaterials) {
			WantTexture(wanted, material, kClearTexture);
		}
		if (g_look.makeupStyle > 0) {
			char name[64]{};
			std::snprintf(name, sizeof(name), "brl_f9_makeup_%02d", g_look.makeupColor);
			WantTexture(wanted, kFemaleMakeupMaterials[g_look.makeupStyle - 1], name);
		}

		WantTexture(wanted, "fantasy_tatto_arm_l", g_look.tattooArmLeft > 0 ? MaleTattooTextureName("arm", g_look.tattooArmLeft) : kClearTexture);
		WantTexture(wanted, "fantasy_tatto_arm_r", g_look.tattooArmRight > 0 ? MaleTattooTextureName("arm", g_look.tattooArmRight) : kClearTexture);
	}

	// Textures actuellement posees sur le ped local (videe par Reset() quand le modele change).
	std::unordered_map<std::string, std::string> g_appliedTextures;

	// Textures de vêtements (par opposition à la peau / au corps) : cherchées dans la texdb
	// sous ces préfixes (voir ClothingTextureName & co). Introuvables => pièce masquée.
	const std::string kClearTextureName = kClearTexture;

	bool IsClothingTexture(const std::string& name)
	{
		return name.rfind("brl_m7_clothes_", 0) == 0 || name.rfind("brl_f9_clothes_", 0) == 0 || name.rfind("brl_cw_clothes_", 0) == 0 ||
			name.rfind("brl_m271_police_", 0) == 0 || name.rfind("brl_f272_police_", 0) == 0;
	}


    int SouthModel() {
        const auto model = CurrentLocalModelId();
        return model == 163 ? 7 : model == 9 ? 9 : 0;
    }
    uint8_t* SouthValue(const std::string& key) {
        if (key == "southTop") return &g_look.southTop;
        if (key == "southBottom") return &g_look.southBottom;
        if (key == "southShoes") return &g_look.southShoes;
        if (key == "southBackpack") return &g_look.southBackpack;
        if (key == "southMask") return &g_look.southMask;
        if (key == "southEar") return &g_look.southEar;
        if (key == "southClock") return &g_look.southClock;
        return nullptr;
    }
    const SouthClothes::Group* SouthGroup(const std::string& key) {
        for (const auto& group : SouthClothes::groups)
            if (group.model == SouthModel() && key == group.key) return &group;
        return nullptr;
    }
    void SouthParts(const char* list, const auto& callback) {
        std::string parts = list;
        size_t start = 0;
        while (start < parts.size()) {
            const auto end = parts.find(',', start);
            std::string part = parts.substr(start, end == std::string::npos ? end : end - start);
            if (part.rfind("hairs_", 0) == 0) part = part.substr(6);
            std::replace(part.begin(), part.end(), '.', '_');
            if (!part.empty()) callback(part);
            if (end == std::string::npos) break;
            start = end + 1;
        }
    }
    void ApplySouth(std::unordered_map<std::string, std::string>& wanted) {
        if (!SouthModel()) return;
        for (const auto& group : SouthClothes::groups) {
            if (group.model != SouthModel()) continue;
            for (int i = 0; i < group.count; ++i)
                if (!wanted.contains(group.items[i].material)) wanted[group.items[i].material] = kClearTexture;
        }
        std::unordered_map<std::string, std::string> body;
        const bool female = SouthModel() == 9;
        const auto torso = female ? FemaleMobileTextureName("torso", g_look.skinTone) : MobileTextureName("torso", g_look.skinTone);
        const auto leg = female ? FemaleMobileTextureName("leg", g_look.skinTone) : MobileTextureName("leg", g_look.skinTone);
        const auto feet = female ? FemaleMobileTextureName("feet", g_look.skinTone) : MobileTextureName("feet", g_look.skinTone);
        for (auto material : kTorsoMaterials) body[material] = torso;
        for (auto material : kLegMaterials) body[material] = leg;
        for (auto material : kFeetMaterials) body[material] = feet;
        std::vector<const SouthClothes::Item*> selected;
        for (const auto& group : SouthClothes::groups) {
            if (group.model != SouthModel()) continue;
            const auto id = *SouthValue(group.key);
            if (!id || id > group.count) continue; // 0 preserves the original saved outfit
            const auto& item = group.items[id - 1];
            const std::string texture = std::string("south/") + item.texture;
            if (!BrlTextureShader::HasTexture(texture)) continue;
            selected.push_back(&item);
            // Restore the body covered by the old garment before the new DFS rules.
            const std::string key = group.key;
            if (key == "southTop") for (auto m : kTorsoMaterials) wanted[m] = body[m];
            if (key == "southBottom") for (auto m : kLegMaterials) wanted[m] = body[m];
            if (key == "southShoes") {
                for (auto m : kFeetMaterials) wanted[m] = body[m];
                wanted["body_legleft5"] = leg; wanted["body_legright5"] = leg;
            }
            for (int i = 0; i < group.count; ++i) wanted[group.items[i].material] = kClearTexture;
            SouthParts(item.apply, [&](const std::string& part) {
                const auto it = body.find(part);
                if (it != body.end()) wanted[part] = it->second;
            });
            wanted[item.material] = texture;
        }
        // Union of occluded parts: one accessory cannot reveal skin under another garment.
        for (const auto* item : selected)
            SouthParts(item->remove, [&](const std::string& part) { wanted[part] = kClearTexture; });
    }

	void CommitWantedTextures(std::unordered_map<std::string, std::string> wanted)
	{
        ApplySouth(wanted);
		const bool remote = g_ctxPlayer != 0xFFFF;
		auto& applied = remote ? g_remoteApplied[g_ctxPlayer] : g_appliedTextures;
		auto clearTex = [&](const std::string& material) {
			if (remote) BrlTextureShader::ClearPlayerTexture(g_ctxPlayer, material);
			else BrlTextureShader::ClearLocalPlayerTexture(material);
		};

		for (auto it = applied.begin(); it != applied.end();) {
			if (!wanted.contains(it->first)) {
				clearTex(it->first);
				it = applied.erase(it);
			}
			else {
				++it;
			}
		}

		static int s_diagCommits = 0;
		const bool diag = s_diagCommits < 6;
		if (diag) { s_diagCommits++; FLog("[vetements] application de %u textures (joueur %u, modele %u)", (unsigned)wanted.size(), (unsigned)g_ctxPlayer, (unsigned)g_ctxModel); }
		for (const auto& [material, wantedTexture] : wanted) {
			if (diag) FLog("[vetements]   %s <- %s (%s)", material.c_str(), wantedTexture.c_str(), BrlTextureShader::HasTexture(wantedTexture) ? "trouvee" : "INTROUVABLE");
			// Vêtement dont la texture principale est introuvable : on n'affiche JAMAIS
			// la texture vanilla du modèle, on masque la pièce (texture transparente).
			const std::string& texture = (IsClothingTexture(wantedTexture) && !BrlTextureShader::HasTexture(wantedTexture))
				? kClearTextureName : wantedTexture;
			if (texture == kClearTexture && !BrlTextureShader::HasTexture(texture)) {
				clearTex(material);
				applied.erase(material);
				continue;
			}
			auto it = applied.find(material);
			if (it == applied.end() || it->second != texture) {
				if (remote) BrlTextureShader::SetPlayerTexture(g_ctxPlayer, material, texture);
				else BrlTextureShader::SetLocalPlayerTexture(material, texture);
				applied[material] = texture;
			}
		}
	}

	// ------------------------------------------------------------
	// Accessoires du pack AVZ (chapeau, lunettes, masque, boucles, collier, montre, sac)
	// ------------------------------------------------------------
	struct AccBinding {
		const char* key;        // cle du slot dans ClothesAvz::Slot
		const char* styleKey;   // cle JSON / option (identique cote serveur : g_ClothKeys)
		const char* colorKey;
		uint8_t ClothesLook::* style;
		uint8_t ClothesLook::* color;
	};

	constexpr AccBinding kAccBindings[] = {
		{ "hat",      "hatStyle",      "hatColor",      &ClothesLook::hatStyle,      &ClothesLook::hatColor },
		{ "glasse",   "glasseStyle",   "glasseColor",   &ClothesLook::glasseStyle,   &ClothesLook::glasseColor },
		{ "mask",     "maskStyle",     "maskColor",     &ClothesLook::maskStyle,     &ClothesLook::maskColor },
		{ "ear",      "earStyle",      "earColor",      &ClothesLook::earStyle,      &ClothesLook::earColor },
		{ "necklace", "necklaceStyle", "necklaceColor", &ClothesLook::necklaceStyle, &ClothesLook::necklaceColor },
		{ "clock",    "clockStyle",    "clockColor",    &ClothesLook::clockStyle,    &ClothesLook::clockColor },
		{ "backpack", "backpackStyle", "backpackColor", &ClothesLook::backpackStyle, &ClothesLook::backpackColor },
	};

	const AccBinding* FindAccBinding(const char* key)
	{
		for (const AccBinding& b : kAccBindings) {
			if (std::strcmp(b.key, key) == 0) return &b;
		}
		return nullptr;
	}

	template <class Fn>
	void ForEachAccessory(bool female, Fn&& fn)
	{
		const ClothesAvz::Slot* slots = female ? ClothesAvz::kFemaleSlots : ClothesAvz::kMaleSlots;
		const int count = female ? ClothesAvz::kFemaleSlotCount : ClothesAvz::kMaleSlotCount;
		for (int i = 0; i < count; ++i) {
			if (const AccBinding* b = FindAccBinding(slots[i].key)) fn(slots[i], *b);
		}
	}

	void ClampAccessories(bool female)
	{
		// Emplacements absents de ce skin (ex. chapeau pour la femme) : remis a zero.
		for (const AccBinding& b : kAccBindings) {
			bool present = false;
			ForEachAccessory(female, [&](const ClothesAvz::Slot& slot, const AccBinding& sb) { if (&sb == &b) present = true; (void)slot; });
			if (!present) { g_look.*(b.style) = 0; g_look.*(b.color) = 1; }
		}
		ForEachAccessory(female, [&](const ClothesAvz::Slot& slot, const AccBinding& b) {
			uint8_t& style = g_look.*(b.style);
			uint8_t& color = g_look.*(b.color);
			style = ClampU8(style, 0, slot.count);
			color = style > 0 ? ClampU8(color, 1, slot.styles[style - 1].variants) : ClampU8(color, 1, 1);
		});
	}

	void WantAccessories(std::unordered_map<std::string, std::string>& wanted, bool female)
	{
		ForEachAccessory(female, [&](const ClothesAvz::Slot& slot, const AccBinding& b) {
			const int style = g_look.*(b.style);
			if (style <= 0 || style > slot.count) return;
			const ClothesAvz::Acc& acc = slot.styles[style - 1];
			const int color = std::clamp(static_cast<int>(g_look.*(b.color)), 1, acc.variants);
			WantTexture(wanted, acc.material,
				female ? FemaleClothingTextureName(acc.material, color) : ClothingTextureName(acc.material, color));
		});
	}

	void ApplyFemaleCurrent()
	{
		ClampFemaleSelection();

		std::unordered_map<std::string, std::string> wanted;
		const std::string torso = FemaleMobileTextureName("torso", g_look.skinTone);
		const std::string leg = FemaleMobileTextureName("leg", g_look.skinTone);
		const std::string feet = FemaleMobileTextureName("feet", g_look.skinTone);
		const std::string head = FemaleMobileTextureName("head", g_look.skinTone, g_look.face);

		WantList(wanted, kFemaleTorsoMaterials, torso);
		WantList(wanted, kFemaleLegMaterials, leg);
		WantList(wanted, kFemaleFeetMaterials, feet);
		WantTexture(wanted, kFemaleStableFaceMaterial, head);

		WantFemaleClothing(wanted, kFemaleHairStyles[g_look.hairStyle - 1], g_look.hairColor);
		WantFemaleClothing(wanted, kFemaleTopStyles[g_look.topStyle - 1], g_look.topColor);
		WantFemaleClothing(wanted, kFemaleBottomStyles[g_look.bottomStyle - 1], g_look.bottomColor);
		WantFemaleClothing(wanted, kFemaleShoesStyles[g_look.shoesStyle - 1], g_look.shoesColor);
		ApplyFemaleFaceAccessoriesAndTattoos(wanted);
		WantAccessories(wanted, true);

		CommitWantedTextures(wanted);
	}

	void ApplyPoliceMaleCurrent()
	{
		ClampPoliceMaleSelection();

		std::unordered_map<std::string, std::string> wanted;
		const std::string torso = MobileTextureName("torso", g_look.skinTone);
		const std::string leg = MobileTextureName("leg", g_look.skinTone);
		const std::string feet = MobileTextureName("feet", g_look.skinTone);
		const std::string head = MobileTextureName("head", g_look.skinTone, g_look.face);

		WantList(wanted, kPoliceMaleTorsoMaterials, torso);
		WantList(wanted, kPoliceMaleLegMaterials, leg);
		WantList(wanted, kPoliceMaleFeetMaterials, feet);
		WantTexture(wanted, kPoliceMaleFaceMaterials[g_look.face - 1], head);

		WantPoliceMaleClothing(wanted, kPoliceMaleTopStyles[g_look.topStyle - 1], g_look.topColor);
		WantPoliceMaleClothing(wanted, kPoliceMaleBottomStyles[g_look.bottomStyle - 1], g_look.bottomColor);
		WantPoliceMaleClothing(wanted, kPoliceMaleShoesStyles[g_look.shoesStyle - 1], g_look.shoesColor);
		if (g_look.policeBelt == 1) {
			WantPoliceMaleClothing(wanted, {"fantasy_m_cop_belt1", "fantasy_m_cop_belt1", 1}, 1);
		}
		if (g_look.policeVest == 1) {
			WantPoliceMaleClothing(wanted, {"fantasy_m_cop_vest3", "fantasy_m_cop_vest3", 1}, 1);
		}
		if (g_look.policeHat == 1) {
			WantTexture(wanted, "fantasy_m_cop_hat3", "BRLboina1");
		}
		else {
			WantClothing(wanted, kPoliceMaleHairStyles[g_look.hairStyle - 1], g_look.hairColor);
		}
		ApplyMaleFaceAccessoriesAndTattoos(wanted);

		CommitWantedTextures(wanted);
	}

	void ApplyPoliceFemaleCurrent()
	{
		ClampPoliceFemaleSelection();

		std::unordered_map<std::string, std::string> wanted;
		const std::string torso = FemaleMobileTextureName("torso", g_look.skinTone);
		const std::string leg = FemaleMobileTextureName("leg", g_look.skinTone);
		const std::string feet = FemaleMobileTextureName("feet", g_look.skinTone);
		const std::string head = FemaleMobileTextureName("head", g_look.skinTone, g_look.face);

		if (g_look.bottomStyle == 1) {
			WantClearList(wanted, std::array<const char*, 8>{
					"body_legleft1", "body_legright1", "body_legright2", "body_legleft2",
					"body_legright3", "body_legleft3", "body_legleft4", "body_legright4"
			});
		}
		else if (g_look.bottomStyle == 2) {
			WantClearList(wanted, std::array<const char*, 4>{
					"body_legleft1", "body_legright1", "body_legright2", "body_legleft2"
			});
		}

		if (g_look.shoesStyle >= 1 && g_look.shoesStyle <= static_cast<int>(kPoliceFemaleShoesStyles.size())) {
			WantClearList(wanted, std::array<const char*, 3>{
					"body_legright5", "body_legleft5", "body_feets"
			});
		}

		if (g_look.topStyle == 1) {
			WantClearList(wanted, std::array<const char*, 5>{
					"body_torso1", "body_torso3", "body_shoulderleft",
					"body_shoulderright", "body_torso4"
			});
		}
		else if (g_look.topStyle == 2) {
			WantClearList(wanted, std::array<const char*, 8>{
					"body_torso1", "body_shoulderleft", "body_shoulderright",
					"body_torso3", "body_torso4", "body_armright",
					"body_armleft", "fantasy_tatto_arm_l"
			});
			WantClearList(wanted, std::array<const char*, 1>{"fantasy_tatto_arm_r"});
		}

		WantList(wanted, kFemaleTorsoMaterials, torso);
		WantList(wanted, kFemaleLegMaterials, leg);
		WantList(wanted, kFemaleFeetMaterials, feet);
		WantTexture(wanted, kFemaleStableFaceMaterial, head);

		WantFemaleClothing(wanted, kPoliceFemaleHairStyles[g_look.hairStyle - 1], g_look.hairColor);
		WantPoliceFemaleClothing(wanted, kPoliceFemaleTopStyles[g_look.topStyle - 1], g_look.topColor);
		WantPoliceFemaleClothing(wanted, kPoliceFemaleBottomStyles[g_look.bottomStyle - 1], g_look.bottomColor);
		WantPoliceFemaleClothing(wanted, kPoliceFemaleShoesStyles[g_look.shoesStyle - 1], g_look.shoesColor);
		if (g_look.policeBelt == 1) {
			WantTexture(wanted, "fantasy_m_cop_belt1", PoliceFemaleTextureName("fantasy_m_cop_belt1", 1));
		}
		if (g_look.policeVest == 1) {
			WantPoliceFemaleClothing(wanted, {{"fantasy_f_cop_vest3", nullptr}, {"fantasy_f_cop_vest3", nullptr}, 1, 1}, 1);
		}
		if (g_look.policeHat == 1) {
			WantTexture(wanted, "fantasy_f_cop_hat3", "BRLboina1");
		}
		else {
			WantFemaleClothing(wanted, kPoliceFemaleHairStyles[g_look.hairStyle - 1], g_look.hairColor);
		}
		ApplyPoliceFemaleFaceAccessories(wanted);

		CommitWantedTextures(wanted);
	}

	// ------------------------------------------------------------
	// Skin personnalise Crestwood (ClothesCatalog.h) : vetements du pack Clothes.zip.
	// Le jeu cherche chaque texture par son nom : brl_cw_clothes_<cle>_<NN>.
	// ------------------------------------------------------------
	void ClampCustomSelection()
	{
		g_look.face = ClampU8(g_look.face, 1, ClothesCatalog::kFaceCount);
		g_look.beardStyle = ClampU8(g_look.beardStyle, 0, ClothesCatalog::Count(ClothesCatalog::kBeard));
		// eyebrowStyle (inutilise sur ce skin) sert a commander la montre : 0 = aucune
		g_look.eyebrowStyle = ClampU8(g_look.eyebrowStyle, 0, ClothesCatalog::Count(ClothesCatalog::kWatch));
		g_look.hairStyle = ClampU8(g_look.hairStyle, 1, ClothesCatalog::Count(ClothesCatalog::kHair));
		g_look.hairColor = ClampU8(g_look.hairColor, 1, ClothesCatalog::kHair[g_look.hairStyle - 1].variants);
		g_look.topStyle = ClampU8(g_look.topStyle, 0, ClothesCatalog::Count(ClothesCatalog::kTop));
		if (g_look.topStyle > 0) {
			g_look.topColor = ClampU8(g_look.topColor, 1, ClothesCatalog::kTop[g_look.topStyle - 1].variants);
		}
		// bas 0 = sous-vetement (le corps nu du modele porte deja un calecon)
		g_look.bottomStyle = ClampU8(g_look.bottomStyle, 0, ClothesCatalog::Count(ClothesCatalog::kBottom));
		if (g_look.bottomStyle > 0) {
			g_look.bottomColor = ClampU8(g_look.bottomColor, 1, ClothesCatalog::kBottom[g_look.bottomStyle - 1].variants);
		}
		g_look.shoesStyle = ClampU8(g_look.shoesStyle, 0, ClothesCatalog::Count(ClothesCatalog::kShoes));
		if (g_look.shoesStyle > 0) {
			g_look.shoesColor = ClampU8(g_look.shoesColor, 1, ClothesCatalog::kShoes[g_look.shoesStyle - 1].variants);
		}
	}

	void WantCatalogStyle(std::unordered_map<std::string, std::string>& wanted, const ClothesCatalog::Style& style, int variant)
	{
		char name[96]{};
		ClothesCatalog::TextureName(name, sizeof(name), style.key, std::clamp(variant, 1, style.variants));
		for (int i = 0; i < style.materialCount; ++i) {
			WantTexture(wanted, style.materials[i], name);
		}
	}

	// selected : 0 = rien, 1..N = style choisi. Le dff contient toutes les pieces en meme
	// temps : on masque d'abord tous les materiaux du groupe, puis on habille le style choisi
	// (plusieurs styles peuvent partager le meme materiau).
	template <size_t N>
	void WantCatalogGroup(std::unordered_map<std::string, std::string>& wanted, const ClothesCatalog::Style (&styles)[N], int selected, int variant)
	{
		for (size_t i = 0; i < N; ++i) {
			for (int m = 0; m < styles[i].materialCount; ++m) {
				WantTexture(wanted, styles[i].materials[m], kClearTexture);
			}
		}
		if (selected >= 1 && selected <= static_cast<int>(N)) {
			WantCatalogStyle(wanted, styles[selected - 1], variant);
		}
	}

	void ApplyCustomCurrent()
	{
		ClampCustomSelection();

		std::unordered_map<std::string, std::string> wanted;
		char face[96]{};
		ClothesCatalog::TextureName(face, sizeof(face), ClothesCatalog::kFaceKey, g_look.face);
		WantTexture(wanted, ClothesCatalog::kFaceMaterial, face);

		WantCatalogGroup(wanted, ClothesCatalog::kHair, g_look.hairStyle, g_look.hairColor);
		WantCatalogGroup(wanted, ClothesCatalog::kBeard, g_look.beardStyle, 1);
		WantCatalogGroup(wanted, ClothesCatalog::kTop, g_look.topStyle, g_look.topColor);
		WantCatalogGroup(wanted, ClothesCatalog::kBottom, g_look.bottomStyle, g_look.bottomColor);
		WantCatalogGroup(wanted, ClothesCatalog::kShoes, g_look.shoesStyle, g_look.shoesColor);
		WantCatalogGroup(wanted, ClothesCatalog::kWatch, g_look.eyebrowStyle, 1);

		CommitWantedTextures(wanted);
	}

	// ------------------------------------------------------------
	// Interface du panneau : onglets + sélecteurs tactiles
	// ------------------------------------------------------------
	// 0 visage, 1 cheveux, 2 habits, 3 tatouages, 4 police, 5 accessoires
	int g_panelTab = 0;

	// Chaque sélecteur appartient à un onglet, déduit de son libellé : le code
	// de chaque skin (homme, femme, police) reste donc une simple liste de lignes.
	int TabOfLabel(const char* label)
	{
		if (std::strstr(label, "Tatouage")) return 3;
		if (std::strstr(label, "Chapeau") || std::strstr(label, "du chapeau") || std::strstr(label, "Lunettes") || std::strstr(label, "des lunettes") ||
			std::strstr(label, "Masque") || std::strstr(label, "du masque") || std::strstr(label, "Boucles") || std::strstr(label, "des boucles") ||
			std::strstr(label, "Collier") || std::strstr(label, "du collier") || std::strstr(label, "poignet") || std::strstr(label, "de la montre") ||
			std::strstr(label, "Sac") || std::strstr(label, "du sac")) return 5;
		if (std::strstr(label, "Arme") || std::strstr(label, "Gilet") || std::strstr(label, "Béret")) return 4;
		if (std::strstr(label, "cheveux")) return 1;
		if (std::strstr(label, "Montre") || std::strstr(label, "Hauts") || std::strstr(label, "du haut") || std::strcmp(label, "Bas") == 0 ||
			std::strstr(label, "du bas") || std::strstr(label, "Chaussures") || std::strstr(label, "des chaussures")) return 2;
		return 0; // peau, visage, maquillage, sourcils, barbe
	}

	void FormatValue(char* buf, size_t n, int value, int minValue, int maxValue)
	{
		if (minValue == 0 && maxValue == 1) {
			std::snprintf(buf, n, "%s", value ? "Oui" : "Non");
		}
		else if (minValue == 0 && value == 0) {
			std::snprintf(buf, n, "Aucun");
		}
		else {
			std::snprintf(buf, n, "%d / %d", value, maxValue);
		}
	}

	bool DrawSelectorInternal(const char* label, uint8_t& value, int minValue, int maxValue, bool applyOnChange)
	{
		if (TabOfLabel(label) != g_panelTab) return false;

		bool changed = false;
		ImGui::PushID(label);

		const float btnW = 58.0f;
		const float btnH = 50.0f;
		const float gap = 8.0f;
		const float avail = ImGui::GetContentRegionAvail().x;
		const float boxW = std::max(avail - btnW * 2.0f - gap * 2.0f, 80.0f);

		ImGui::TextColored(ImVec4(0.62f, 0.69f, 0.66f, 1.0f), "%s", label);

		ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 10.0f);
		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.06f, 0.14f, 0.10f, 1.0f));
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.0f, 0.42f, 0.24f, 1.0f));
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.0f, 0.72f, 0.38f, 1.0f));

		if (ImGui::Button("<", ImVec2(btnW, btnH))) {
			value = value <= minValue ? maxValue : value - 1;
			changed = true;
		}
		ImGui::SameLine(0.0f, gap);

		// Zone centrale : valeur + barre de progression
		{
			const ImVec2 p = ImGui::GetCursorScreenPos();
			ImDrawList* dl = ImGui::GetWindowDrawList();
			dl->AddRectFilled(p, ImVec2(p.x + boxW, p.y + btnH), IM_COL32(8, 18, 14, 255), 10.0f);
			dl->AddRect(p, ImVec2(p.x + boxW, p.y + btnH), IM_COL32(0, 210, 120, 70), 10.0f, 0, 1.0f);

			char txt[24];
			FormatValue(txt, sizeof(txt), value, minValue, maxValue);
			const ImVec2 ts = ImGui::CalcTextSize(txt);
			dl->AddText(ImVec2(p.x + (boxW - ts.x) * 0.5f, p.y + (btnH - ts.y) * 0.5f - 4.0f),
				IM_COL32(245, 250, 247, 255), txt);

			if (maxValue > minValue) {
				const float frac = static_cast<float>(value - minValue) / static_cast<float>(maxValue - minValue);
				const float x0 = p.x + 14.0f;
				const float x1 = p.x + boxW - 14.0f;
				const float y = p.y + btnH - 9.0f;
				dl->AddRectFilled(ImVec2(x0, y), ImVec2(x1, y + 3.0f), IM_COL32(255, 255, 255, 28), 2.0f);
				dl->AddRectFilled(ImVec2(x0, y), ImVec2(x0 + (x1 - x0) * frac, y + 3.0f), IM_COL32(0, 215, 120, 255), 2.0f);
			}
			ImGui::Dummy(ImVec2(boxW, btnH));
		}

		ImGui::SameLine(0.0f, gap);
		if (ImGui::Button(">", ImVec2(btnW, btnH))) {
			value = value >= maxValue ? minValue : value + 1;
			changed = true;
		}

		ImGui::PopStyleColor(3);
		ImGui::PopStyleVar();
		ImGui::Dummy(ImVec2(0.0f, 8.0f));
		ImGui::PopID();

		if (changed && applyOnChange) {
			ClothesCustomization::ApplyCurrent();
		}
		return changed;
	}

	void DrawSelector(const char* label, uint8_t& value, int minValue, int maxValue)
	{
		DrawSelectorInternal(label, value, minValue, maxValue, true);
	}

	void DrawSelectorResettingColor(const char* label, uint8_t& style, uint8_t& color, int minValue, int maxValue)
	{
		if (DrawSelectorInternal(label, style, minValue, maxValue, false)) {
			color = 1;
			ClothesCustomization::ApplyCurrent();
		}
	}

	// Affiche le NOM du vetement choisi sous son selecteur (meme onglet que lui).
	void DrawNameLine(const char* selectorLabel, const char* text)
	{
		if (TabOfLabel(selectorLabel) != g_panelTab) return;
		ImGui::TextColored(ImVec4(0.0f, 0.85f, 0.45f, 1.0f), "%s", text);
		ImGui::Dummy(ImVec2(0.0f, 4.0f));
	}

	// Barre d'onglets (boutons de largeur égale, adaptés au tactile)
	// Selecteurs ImGui des accessoires (onglet ACCESS.)
	void DrawAccessorySelectors(bool female)
	{
		ForEachAccessory(female, [&](const ClothesAvz::Slot& slot, const AccBinding& b) {
			uint8_t& style = g_look.*(b.style);
			uint8_t& color = g_look.*(b.color);
			DrawSelectorResettingColor(slot.label, style, color, 0, slot.count);
			if (style > 0) {
				DrawSelector(slot.colorLabel, color, 1, slot.styles[style - 1].variants);
			}
		});
	}

	void DrawTabBar(bool withPolice, bool withAccessories)
	{
		struct Tab { const char* name; int id; };
		Tab tabs[6];
		int count = 0;
		tabs[count++] = { "VISAGE", 0 };
		tabs[count++] = { "CHEVEUX", 1 };
		tabs[count++] = { "HABITS", 2 };
		tabs[count++] = { "TATOUAGES", 3 };
		if (withAccessories) tabs[count++] = { "ACCESS.", 5 };
		if (withPolice) tabs[count++] = { "POLICE", 4 };
		bool known = false;
		for (int i = 0; i < count; ++i) known = known || tabs[i].id == g_panelTab;
		if (!known) g_panelTab = 0;

		const float gap = 6.0f;
		const float tabW = (ImGui::GetContentRegionAvail().x - gap * (count - 1)) / count;

		ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 10.0f);
		for (int i = 0; i < count; ++i) {
			if (i > 0) ImGui::SameLine(0.0f, gap);
			const bool active = g_panelTab == tabs[i].id;
			ImGui::PushStyleColor(ImGuiCol_Button, active ? ImVec4(0.0f, 0.72f, 0.38f, 1.0f) : ImVec4(0.05f, 0.10f, 0.08f, 1.0f));
			ImGui::PushStyleColor(ImGuiCol_ButtonHovered, active ? ImVec4(0.0f, 0.78f, 0.42f, 1.0f) : ImVec4(0.0f, 0.30f, 0.18f, 1.0f));
			ImGui::PushStyleColor(ImGuiCol_Text, active ? ImVec4(0.0f, 0.0f, 0.0f, 1.0f) : ImVec4(0.80f, 0.86f, 0.83f, 1.0f));
			if (ImGui::Button(tabs[i].name, ImVec2(tabW, 42.0f))) g_panelTab = tabs[i].id;
			ImGui::PopStyleColor(3);
		}
		ImGui::PopStyleVar();
		ImGui::Dummy(ImVec2(0.0f, 6.0f));
	}
}

ClothesLook& ClothesCustomization::Look()
{
	return g_look;
}

void ClothesCustomization::Reset(int gender)
{
	g_appliedTextures.clear();
	g_look = ClothesLook{};
	// Padrões do gênero
	g_look.skinTone = 1;
	g_look.face = 1;
	g_look.hairStyle = 1;
	g_look.hairColor = 1;
	g_look.topStyle = 1;
	g_look.topColor = 1;
	g_look.bottomStyle = 1;
	g_look.bottomColor = 1;
	g_look.shoesStyle = 1;
	g_look.shoesColor = 1;
	g_look.eyebrowStyle = 0;
	g_look.eyebrowColor = 1;
	if (gender == 2) {
		// Homme Crestwood (skin 289 / zero, vetements Clothes.zip) : demarre en sous-vetement
		g_look.topStyle = 0;
		g_look.bottomStyle = 0;
		g_look.shoesStyle = 0;
		g_look.beardStyle = 0;
		g_look.eyebrowStyle = 0;
	}
	else if (SkinBodies::IsFemaleGender(gender)) {   // Femme (9) / Femme 2 (12)
		g_look.beardStyle = 0;
		g_look.makeupStyle = 0;
		g_look.makeupColor = 1;
	}
	else {
		// Personagem novo masculino começa só de cueca: sem camisa (top 0),
		// cueca (estilo 3 = fantasy_m_underwear) e descalço (shoes 0).
		g_look.topStyle = 0;
		g_look.bottomStyle = 3;
		g_look.shoesStyle = 0;
	}
	// Limpa as regras antigas do jogador local antes de aplicar o novo visual
	BrlTextureShader::ClearPlayer(0xFFFF);
}

void ClothesCustomization::ClearLocal()
{
	BrlTextureShader::ClearPlayer(0xFFFF);
}

bool ClothesCustomization::IsSupportedSkin(uint16_t modelId)
{
	return SkinBodies::IsCivilBody(modelId) || modelId == 246 || modelId == 280;
}

bool ClothesCustomization::IsSupportedLocalSkin()
{
	return IsSupportedSkin(CurrentLocalModelId());
}

void ClothesCustomization::ApplyCurrent()
{
    const uint16_t realModel = RealLocalModelId();
    if (SkinBodies::IsCivilBody(realModel) && realModel != 289) {
        std::string error;
        // Shared model is loaded once per body, on the game thread, for local and remote players.
        const bool loaded = SouthModelLoader::LoadForModel(realModel, error);
        {   // [diag] une ligne par resultat different (evite de saturer le journal)
            static std::string s_lastDiag;
            const std::string diag = std::string(loaded ? "ok" : "ECHEC") + " : " + error;
            if (diag != s_lastDiag) { s_lastDiag = diag; FLog("[vetements] mega-skin modele %u -> %s", (unsigned)realModel, diag.c_str()); }
        }
        if (!loaded) return;
    }
	const uint16_t modelId = CurrentLocalModelId();
	if (!IsSupportedSkin(modelId) || !CurrentLocalPedReady()) {
		return;
	}

	if (modelId == 9) {
		ApplyFemaleCurrent();
		return;
	}

	if (modelId == 246) {
		ApplyPoliceFemaleCurrent();
		return;
	}

	if (modelId == 280) {
		ApplyPoliceMaleCurrent();
		return;
	}

	if (modelId == ClothesCatalog::kCustomMaleSkinId) {
		ApplyCustomCurrent();
		return;
	}

	if (modelId != 163) return;

	ClampSelection();
	const std::string torso = MobileTextureName("torso", g_look.skinTone);
	const std::string leg = MobileTextureName("leg", g_look.skinTone);
	const std::string feet = MobileTextureName("feet", g_look.skinTone);
	const std::string head = MobileTextureName("head", g_look.skinTone, g_look.face);

	std::unordered_map<std::string, std::string> wanted;

	WantList(wanted, kTorsoMaterials, torso);
	WantList(wanted, kLegMaterials, leg);
	WantList(wanted, kFeetMaterials, feet);
	WantTexture(wanted, kFaceMaterialSlots[g_look.face - 1][0], head);

	if (g_look.hatStyle == 0) {   // un chapeau remplace les cheveux (comme dans le pack AVZ)
		WantClothing(wanted, kHairStyles[g_look.hairStyle - 1], g_look.hairColor);
	}
	if (g_look.topStyle > 0) {
		WantClothing(wanted, kTopStyles[g_look.topStyle - 1], g_look.topColor);
	}
	if (g_look.bottomStyle > 0) {
		WantClothing(wanted, kBottomStyles[g_look.bottomStyle - 1], g_look.bottomColor);
	}
	if (g_look.shoesStyle > 0) {
		WantClothing(wanted, kShoesStyles[g_look.shoesStyle - 1], g_look.shoesColor);
	}
	ApplyMaleFaceAccessoriesAndTattoos(wanted);
	WantAccessories(wanted, false);

	CommitWantedTextures(wanted);
}

void ClothesCustomization::ApplyRemote(uint16_t playerId, const ClothesLook& look, uint16_t modelId)
{
	if (playerId == 0xFFFF) return;
	if (!IsSupportedSkin(modelId)) {
		ClearRemote(playerId);
		return;
	}
	const ClothesLook saved = g_look;
	g_ctxPlayer = playerId;
	g_ctxModel = modelId;
	g_look = look;
	ApplyCurrent();
	g_look = saved;
	g_ctxPlayer = 0xFFFF;
	g_ctxModel = 0;
}

void ClothesCustomization::ClearRemote(uint16_t playerId)
{
	auto it = g_remoteApplied.find(playerId);
	if (it != g_remoteApplied.end()) {
		for (const auto& entry : it->second) {
			BrlTextureShader::ClearPlayerTexture(playerId, entry.first);
		}
		g_remoteApplied.erase(it);
	}
	BrlTextureShader::ClearPlayer(playerId);
}

bool ClothesCustomization::RequestSkin(uint16_t modelId)
{
	if (!SkinRegistry::Find(modelId) || !pNetGame) {
		return false;
	}
	// Le serveur valide puis fait SetPlayerSkin : tous les joueurs voient le skin.
	pNetGame->SendCharSkinRequest(modelId);
	return true;
}

namespace {
	// Liste des skins du registre affichée dans le panneau (noms + ID).
	void RenderSkinPicker(uint16_t currentModelId)
	{
		ImGui::Separator();
		if (!ImGui::CollapsingHeader("Skins disponibles")) {
			return;
		}
		ImGui::TextWrapped("Touchez un skin pour le choisir. Tous les joueurs le verront.");
		for (size_t i = 0; i < SkinRegistry::kCount; ++i) {
			const SkinRegistry::Entry& e = SkinRegistry::kEntries[i];
			char label[112]{};
			std::snprintf(label, sizeof(label), "%s%d - %s##skin_%d",
				e.id == currentModelId ? "> " : "", static_cast<int>(e.id), e.name, static_cast<int>(e.id));
			if (ImGui::Button(label, ImVec2(-1.0f, 40.0f))) {
				ClothesCustomization::RequestSkin(e.id);
			}
		}
	}
}

void ClothesCustomization::RenderPanel()
{
	const uint16_t modelId = CurrentLocalModelId();
	const bool femaleSkin = modelId == 9;
	const bool policeFemaleSkin = modelId == 246;
	const bool policeMaleSkin = modelId == 280;
	const bool customSkin = modelId == ClothesCatalog::kCustomMaleSkinId;
	const bool supportedSkin = IsSupportedSkin(modelId);

	DrawTabBar(policeFemaleSkin || policeMaleSkin, (femaleSkin || modelId == 163) && !customSkin);
	ImGui::BeginChild("clothes_scroll", ImVec2(0.0f, 0.0f), false, ImGuiWindowFlags_AlwaysVerticalScrollbar);

	// Nom du skin affiché en jeu (registre SkinRegistry.h)
	const uint16_t realModelId = RealLocalModelId();
	const SkinRegistry::Entry* skinInfo = SkinRegistry::Find(realModelId);
	{
		char fallback[16]{};
		ImGui::TextColored(ImVec4(0.0f, 0.85f, 0.45f, 1.0f), "SKIN : %s (ID %d)",
			SkinRegistry::Name(realModelId, fallback, sizeof(fallback)), static_cast<int>(realModelId));
		ImGui::Separator();
	}

	if (!supportedSkin) {
		if (skinInfo) {
			ImGui::TextWrapped("Skin fixe : la personnalisation de la peau et des vêtements n'est disponible que pour les skins de base (7, 9, 12, 163, 246, 280 et 289).");
		}
		else {
			ImGui::TextWrapped("Skin ID %d absent du registre. Ajoutez-le dans SkinRegistry.h, data/skins.dat et skins.inc.", static_cast<int>(realModelId));
		}
	}
	else if (customSkin) {
		namespace CC = ClothesCatalog;
		ClampCustomSelection();
		DrawSelector("Texture du visage", g_look.face, 1, CC::kFaceCount);
		DrawNameLine("Texture du visage", CC::kFaceNames[g_look.face - 1]);
		DrawSelectorResettingColor("Style de barbe", g_look.beardStyle, g_look.beardColor, 0, CC::Count(CC::kBeard));
		DrawNameLine("Style de barbe", g_look.beardStyle > 0 ? CC::kBeard[g_look.beardStyle - 1].name : "Sans barbe");
		DrawSelectorResettingColor("Style de cheveux", g_look.hairStyle, g_look.hairColor, 1, CC::Count(CC::kHair));
		DrawNameLine("Style de cheveux", CC::kHair[g_look.hairStyle - 1].name);
		DrawSelector("Couleur des cheveux", g_look.hairColor, 1, CC::kHair[g_look.hairStyle - 1].variants);
		DrawNameLine("Couleur des cheveux", CC::kHair[g_look.hairStyle - 1].variantNames[g_look.hairColor - 1]);
		DrawSelectorResettingColor("Hauts", g_look.topStyle, g_look.topColor, 0, CC::Count(CC::kTop));
		DrawNameLine("Hauts", g_look.topStyle > 0 ? CC::kTop[g_look.topStyle - 1].name : "Torse nu");
		if (g_look.topStyle > 0) {
			DrawSelector("Couleur du haut", g_look.topColor, 1, CC::kTop[g_look.topStyle - 1].variants);
			DrawNameLine("Couleur du haut", CC::kTop[g_look.topStyle - 1].variantNames[g_look.topColor - 1]);
		}
		DrawSelectorResettingColor("Bas", g_look.bottomStyle, g_look.bottomColor, 0, CC::Count(CC::kBottom));
		DrawNameLine("Bas", g_look.bottomStyle > 0 ? CC::kBottom[g_look.bottomStyle - 1].name : "Sous-vêtement");
		if (g_look.bottomStyle > 0) {
			DrawSelector("Couleur du bas", g_look.bottomColor, 1, CC::kBottom[g_look.bottomStyle - 1].variants);
			DrawNameLine("Couleur du bas", CC::kBottom[g_look.bottomStyle - 1].variantNames[g_look.bottomColor - 1]);
		}
		DrawSelectorResettingColor("Chaussures", g_look.shoesStyle, g_look.shoesColor, 0, CC::Count(CC::kShoes));
		DrawNameLine("Chaussures", g_look.shoesStyle > 0 ? CC::kShoes[g_look.shoesStyle - 1].name : "Pieds nus");
		if (g_look.shoesStyle > 0) {
			DrawSelector("Couleur des chaussures", g_look.shoesColor, 1, CC::kShoes[g_look.shoesStyle - 1].variants);
			DrawNameLine("Couleur des chaussures", CC::kShoes[g_look.shoesStyle - 1].variantNames[g_look.shoesColor - 1]);
		}
		DrawSelector("Montre", g_look.eyebrowStyle, 0, CC::Count(CC::kWatch));
		DrawNameLine("Montre", g_look.eyebrowStyle > 0 ? CC::kWatch[g_look.eyebrowStyle - 1].name : "Sans montre");
	}
	else if (femaleSkin) {
		DrawSelector("Couleur de peau", g_look.skinTone, 1, 6);
		ClampFemaleSelection();
		DrawSelector("Texture du visage", g_look.face, 1, kFemaleFaceCountByTone[g_look.skinTone]);
		DrawSelectorResettingColor("Style de maquillage", g_look.makeupStyle, g_look.makeupColor, 0, kFemaleMakeupStyleCount);
		if (g_look.makeupStyle > 0) {
			DrawSelector("Couleur du maquillage", g_look.makeupColor, 1, kFemaleMakeupColorCount);
		}
		DrawSelectorResettingColor("Style de sourcils", g_look.eyebrowStyle, g_look.eyebrowColor, 0, kMaleEyebrowStyleCount);
		if (g_look.eyebrowStyle > 0) {
			DrawSelector("Couleur des sourcils", g_look.eyebrowColor, 1, kMaleEyebrowColorCount);
		}
		DrawSelectorResettingColor("Style de cheveux", g_look.hairStyle, g_look.hairColor, 1, static_cast<int>(kFemaleHairStyles.size()));
		DrawSelector("Couleur des cheveux", g_look.hairColor, 1, kFemaleHairStyles[g_look.hairStyle - 1].maxValue);
		DrawSelectorResettingColor("Hauts", g_look.topStyle, g_look.topColor, 1, static_cast<int>(kFemaleTopStyles.size()));
		DrawSelector("Couleur du haut", g_look.topColor, 1, kFemaleTopStyles[g_look.topStyle - 1].maxValue);
		DrawSelectorResettingColor("Bas", g_look.bottomStyle, g_look.bottomColor, 1, static_cast<int>(kFemaleBottomStyles.size()));
		DrawSelector("Couleur du bas", g_look.bottomColor, 1, kFemaleBottomStyles[g_look.bottomStyle - 1].maxValue);
		DrawSelectorResettingColor("Chaussures", g_look.shoesStyle, g_look.shoesColor, 1, static_cast<int>(kFemaleShoesStyles.size()));
		DrawSelector("Couleur des chaussures", g_look.shoesColor, 1, kFemaleShoesStyles[g_look.shoesStyle - 1].maxValue);
		DrawSelector("Tatouage bras gauche", g_look.tattooArmLeft, 0, kMaleTattooArmCount);
		DrawSelector("Tatouage bras droit", g_look.tattooArmRight, 0, kMaleTattooArmCount);
		DrawSelector("Tatouage jambe gauche", g_look.tattooLegLeft, 0, kMaleTattooLegCount);
		DrawSelector("Tatouage jambe droite", g_look.tattooLegRight, 0, kMaleTattooLegCount);
		DrawSelector("Tatouage dos", g_look.tattooBack, 0, kMaleTattooBackCount);
		DrawSelector("Tatouage ventre", g_look.tattooFront, 0, kMaleTattooFrontCount);
		DrawAccessorySelectors(true);
	}
	else if (policeFemaleSkin) {
		DrawSelector("Couleur de peau", g_look.skinTone, 1, 6);
		ClampPoliceFemaleSelection();
		DrawSelector("Texture du visage", g_look.face, 1, kFemaleFaceCountByTone[g_look.skinTone]);
		DrawSelectorResettingColor("Style de maquillage", g_look.makeupStyle, g_look.makeupColor, 0, kFemaleMakeupStyleCount);
		if (g_look.makeupStyle > 0) {
			DrawSelector("Couleur du maquillage", g_look.makeupColor, 1, kFemaleMakeupColorCount);
		}
		DrawSelectorResettingColor("Style de sourcils", g_look.eyebrowStyle, g_look.eyebrowColor, 0, kMaleEyebrowStyleCount);
		if (g_look.eyebrowStyle > 0) {
			DrawSelector("Couleur des sourcils", g_look.eyebrowColor, 1, kMaleEyebrowColorCount);
		}
		DrawSelectorResettingColor("Style de cheveux", g_look.hairStyle, g_look.hairColor, 1, static_cast<int>(kPoliceFemaleHairStyles.size()));
		DrawSelector("Couleur des cheveux", g_look.hairColor, 1, kPoliceFemaleHairStyles[g_look.hairStyle - 1].maxValue);
		DrawSelectorResettingColor("Hauts", g_look.topStyle, g_look.topColor, 1, static_cast<int>(kPoliceFemaleTopStyles.size()));
		DrawSelector("Couleur du haut", g_look.topColor, 1, kPoliceFemaleTopStyles[g_look.topStyle - 1].maxValue);
		DrawSelectorResettingColor("Bas", g_look.bottomStyle, g_look.bottomColor, 1, static_cast<int>(kPoliceFemaleBottomStyles.size()));
		DrawSelector("Couleur du bas", g_look.bottomColor, 1, kPoliceFemaleBottomStyles[g_look.bottomStyle - 1].maxValue);
		DrawSelectorResettingColor("Chaussures", g_look.shoesStyle, g_look.shoesColor, 1, static_cast<int>(kPoliceFemaleShoesStyles.size()));
		DrawSelector("Couleur des chaussures", g_look.shoesColor, 1, kPoliceFemaleShoesStyles[g_look.shoesStyle - 1].maxValue);
		DrawSelector("Arme à la ceinture", g_look.policeBelt, 0, 1);
		DrawSelector("Gilet", g_look.policeVest, 0, 1);
		DrawSelector("Béret", g_look.policeHat, 0, 1);
		DrawSelector("Tatouage bras gauche", g_look.tattooArmLeft, 0, kMaleTattooArmCount);
		DrawSelector("Tatouage bras droit", g_look.tattooArmRight, 0, kMaleTattooArmCount);
	}
	else if (policeMaleSkin) {
		DrawSelector("Couleur de peau", g_look.skinTone, 1, 6);
		ClampPoliceMaleSelection();
		DrawSelector("Forme du visage", g_look.face, 1, std::min(kFaceCountByTone[g_look.skinTone], kDffFaceSlots));
		DrawSelectorResettingColor("Style de sourcils", g_look.eyebrowStyle, g_look.eyebrowColor, 0, kMaleEyebrowStyleCount);
		if (g_look.eyebrowStyle > 0) {
			DrawSelector("Couleur des sourcils", g_look.eyebrowColor, 1, kMaleEyebrowColorCount);
		}
		DrawSelectorResettingColor("Style de barbe", g_look.beardStyle, g_look.beardColor, 0, kMaleBeardStyleCount);
		if (g_look.beardStyle > 0) {
			DrawSelector("Couleur de la barbe", g_look.beardColor, 1, kMaleBeardColorCount);
		}
		DrawSelectorResettingColor("Style de cheveux", g_look.hairStyle, g_look.hairColor, 1, static_cast<int>(kPoliceMaleHairStyles.size()));
		DrawSelector("Couleur des cheveux", g_look.hairColor, 1, kPoliceMaleHairStyles[g_look.hairStyle - 1].maxValue);
		DrawSelectorResettingColor("Hauts", g_look.topStyle, g_look.topColor, 1, static_cast<int>(kPoliceMaleTopStyles.size()));
		DrawSelector("Couleur du haut", g_look.topColor, 1, kPoliceMaleTopStyles[g_look.topStyle - 1].maxValue);
		DrawSelectorResettingColor("Bas", g_look.bottomStyle, g_look.bottomColor, 1, static_cast<int>(kPoliceMaleBottomStyles.size()));
		DrawSelector("Couleur du bas", g_look.bottomColor, 1, kPoliceMaleBottomStyles[g_look.bottomStyle - 1].maxValue);
		DrawSelectorResettingColor("Chaussures", g_look.shoesStyle, g_look.shoesColor, 1, static_cast<int>(kPoliceMaleShoesStyles.size()));
		DrawSelector("Couleur des chaussures", g_look.shoesColor, 1, kPoliceMaleShoesStyles[g_look.shoesStyle - 1].maxValue);
		DrawSelector("Arme à la ceinture", g_look.policeBelt, 0, 1);
		DrawSelector("Gilet", g_look.policeVest, 0, 1);
		DrawSelector("Béret", g_look.policeHat, 0, 1);
		DrawSelector("Tatouage bras gauche", g_look.tattooArmLeft, 0, kMaleTattooArmCount);
		DrawSelector("Tatouage bras droit", g_look.tattooArmRight, 0, kMaleTattooArmCount);
		DrawSelector("Tatouage jambe gauche", g_look.tattooLegLeft, 0, kMaleTattooLegCount);
		DrawSelector("Tatouage jambe droite", g_look.tattooLegRight, 0, kMaleTattooLegCount);
		DrawSelector("Tatouage dos", g_look.tattooBack, 0, kMaleTattooBackCount);
		DrawSelector("Tatouage ventre", g_look.tattooFront, 0, kMaleTattooFrontCount);
	}
	else {
		DrawSelector("Couleur de peau", g_look.skinTone, 1, 6);
		ClampSelection();
		DrawSelector("Forme du visage", g_look.face, 1, std::min(kFaceCountByTone[g_look.skinTone], kDffFaceSlots));
		DrawSelectorResettingColor("Style de sourcils", g_look.eyebrowStyle, g_look.eyebrowColor, 0, kMaleEyebrowStyleCount);
		if (g_look.eyebrowStyle > 0) {
			DrawSelector("Couleur des sourcils", g_look.eyebrowColor, 1, kMaleEyebrowColorCount);
		}
		DrawSelectorResettingColor("Style de barbe", g_look.beardStyle, g_look.beardColor, 0, kMaleBeardStyleCount);
		if (g_look.beardStyle > 0) {
			DrawSelector("Couleur de la barbe", g_look.beardColor, 1, kMaleBeardColorCount);
		}
		DrawSelectorResettingColor("Style de cheveux", g_look.hairStyle, g_look.hairColor, 1, static_cast<int>(kHairStyles.size()));
		DrawSelector("Couleur des cheveux", g_look.hairColor, 1, kHairStyles[g_look.hairStyle - 1].maxValue);
		// 0 = sem camisa (personagem começa só de cueca)
		DrawSelectorResettingColor("Hauts", g_look.topStyle, g_look.topColor, 0, static_cast<int>(kTopStyles.size()));
		if (g_look.topStyle > 0) {
			DrawSelector("Couleur du haut", g_look.topColor, 1, kTopStyles[g_look.topStyle - 1].maxValue);
		}
		DrawSelectorResettingColor("Bas", g_look.bottomStyle, g_look.bottomColor, 1, static_cast<int>(kBottomStyles.size()));
		DrawSelector("Couleur du bas", g_look.bottomColor, 1, kBottomStyles[g_look.bottomStyle - 1].maxValue);
		// 0 = descalço
		DrawSelectorResettingColor("Chaussures", g_look.shoesStyle, g_look.shoesColor, 0, static_cast<int>(kShoesStyles.size()));
		if (g_look.shoesStyle > 0) {
			DrawSelector("Couleur des chaussures", g_look.shoesColor, 1, kShoesStyles[g_look.shoesStyle - 1].maxValue);
		}
		DrawSelector("Tatouage bras gauche", g_look.tattooArmLeft, 0, kMaleTattooArmCount);
		DrawSelector("Tatouage bras droit", g_look.tattooArmRight, 0, kMaleTattooArmCount);
		DrawSelector("Tatouage jambe gauche", g_look.tattooLegLeft, 0, kMaleTattooLegCount);
		DrawSelector("Tatouage jambe droite", g_look.tattooLegRight, 0, kMaleTattooLegCount);
		DrawSelector("Tatouage dos", g_look.tattooBack, 0, kMaleTattooBackCount);
		DrawSelector("Tatouage ventre", g_look.tattooFront, 0, kMaleTattooFrontCount);
		DrawAccessorySelectors(false);
	}
	RenderSkinPicker(modelId);
	ImGui::EndChild();
}

// ============================================================
// Panneau HTML des vetements : liste des options + application
// (meme logique et memes bornes que RenderPanel ci-dessus)
// ============================================================
namespace {

	struct OptField {
		const char* key;
		const char* label;
		const char* tab;      // haut / bas / chaussures / cheveux / visage / tatouages / police
		uint8_t*    ref;
		int         minV;
		int         maxV;
		uint8_t*    colorRef; // non nul : remis a 1 quand ce style change
	};

	// Cheveux + haut + bas + chaussures (tables homme/femme/police : meme structure, champ maxValue).
	template <class H, class T, class B, class S>
	void AddBaseClothes(std::vector<OptField>& f, const H& hair, const T& top, const B& bottom, const S& shoes,
		int topMin, int shoesMin)
	{
		f.push_back({ "hairStyle", "Style de cheveux", "cheveux", &g_look.hairStyle, 1, static_cast<int>(hair.size()), &g_look.hairColor });
		f.push_back({ "hairColor", "Couleur des cheveux", "cheveux", &g_look.hairColor, 1, hair[g_look.hairStyle - 1].maxValue, nullptr });
		f.push_back({ "topStyle", "Haut", "haut", &g_look.topStyle, topMin, static_cast<int>(top.size()), &g_look.topColor });
		if (g_look.topStyle > 0) {
			f.push_back({ "topColor", "Couleur du haut", "haut", &g_look.topColor, 1, top[g_look.topStyle - 1].maxValue, nullptr });
		}
		f.push_back({ "bottomStyle", "Bas", "bas", &g_look.bottomStyle, 1, static_cast<int>(bottom.size()), &g_look.bottomColor });
		f.push_back({ "bottomColor", "Couleur du bas", "bas", &g_look.bottomColor, 1, bottom[g_look.bottomStyle - 1].maxValue, nullptr });
		f.push_back({ "shoesStyle", "Chaussures", "chaussures", &g_look.shoesStyle, shoesMin, static_cast<int>(shoes.size()), &g_look.shoesColor });
		if (g_look.shoesStyle > 0) {
			f.push_back({ "shoesColor", "Couleur des chaussures", "chaussures", &g_look.shoesColor, 1, shoes[g_look.shoesStyle - 1].maxValue, nullptr });
		}
	}

	void AddArmTattoos(std::vector<OptField>& f)
	{
		f.push_back({ "tattooArmLeft", "Tatouage bras gauche", "tatouages", &g_look.tattooArmLeft, 0, kMaleTattooArmCount, nullptr });
		f.push_back({ "tattooArmRight", "Tatouage bras droit", "tatouages", &g_look.tattooArmRight, 0, kMaleTattooArmCount, nullptr });
	}

	void AddAllTattoos(std::vector<OptField>& f)
	{
		AddArmTattoos(f);
		f.push_back({ "tattooLegLeft", "Tatouage jambe gauche", "tatouages", &g_look.tattooLegLeft, 0, kMaleTattooLegCount, nullptr });
		f.push_back({ "tattooLegRight", "Tatouage jambe droite", "tatouages", &g_look.tattooLegRight, 0, kMaleTattooLegCount, nullptr });
		f.push_back({ "tattooBack", "Tatouage dos", "tatouages", &g_look.tattooBack, 0, kMaleTattooBackCount, nullptr });
		f.push_back({ "tattooFront", "Tatouage ventre", "tatouages", &g_look.tattooFront, 0, kMaleTattooFrontCount, nullptr });
	}

	void AddPoliceGear(std::vector<OptField>& f)
	{
		f.push_back({ "policeBelt", "Arme à la ceinture", "police", &g_look.policeBelt, 0, 1, nullptr });
		f.push_back({ "policeVest", "Gilet", "police", &g_look.policeVest, 0, 1, nullptr });
		f.push_back({ "policeHat", "Béret", "police", &g_look.policeHat, 0, 1, nullptr });
	}

	void AddMaleEyebrowsAndBeard(std::vector<OptField>& f)
	{
		f.push_back({ "eyebrowStyle", "Style de sourcils", "visage", &g_look.eyebrowStyle, 0, kMaleEyebrowStyleCount, &g_look.eyebrowColor });
		if (g_look.eyebrowStyle > 0) {
			f.push_back({ "eyebrowColor", "Couleur des sourcils", "visage", &g_look.eyebrowColor, 1, kMaleEyebrowColorCount, nullptr });
		}
		f.push_back({ "beardStyle", "Style de barbe", "visage", &g_look.beardStyle, 0, kMaleBeardStyleCount, &g_look.beardColor });
		if (g_look.beardStyle > 0) {
			f.push_back({ "beardColor", "Couleur de la barbe", "visage", &g_look.beardColor, 1, kMaleBeardColorCount, nullptr });
		}
	}

	void AddFemaleFace(std::vector<OptField>& f, bool police)
	{
		f.push_back({ "skinTone", "Couleur de peau", "visage", &g_look.skinTone, 1, 6, nullptr });
		if (police) ClampPoliceFemaleSelection(); else ClampFemaleSelection();
		f.push_back({ "face", "Texture du visage", "visage", &g_look.face, 1, kFemaleFaceCountByTone[g_look.skinTone], nullptr });
		f.push_back({ "makeupStyle", "Style de maquillage", "visage", &g_look.makeupStyle, 0, kFemaleMakeupStyleCount, &g_look.makeupColor });
		if (g_look.makeupStyle > 0) {
			f.push_back({ "makeupColor", "Couleur du maquillage", "visage", &g_look.makeupColor, 1, kFemaleMakeupColorCount, nullptr });
		}
		f.push_back({ "eyebrowStyle", "Style de sourcils", "visage", &g_look.eyebrowStyle, 0, kMaleEyebrowStyleCount, &g_look.eyebrowColor });
		if (g_look.eyebrowStyle > 0) {
			f.push_back({ "eyebrowColor", "Couleur des sourcils", "visage", &g_look.eyebrowColor, 1, kMaleEyebrowColorCount, nullptr });
		}
	}

	void AddAccessoryFields(std::vector<OptField>& f, bool female)
	{
		ClampAccessories(female);
		ForEachAccessory(female, [&](const ClothesAvz::Slot& slot, const AccBinding& b) {
			uint8_t& style = g_look.*(b.style);
			uint8_t& color = g_look.*(b.color);
			f.push_back({ b.styleKey, slot.label, slot.tab, &style, 0, slot.count, &color });
			if (style > 0) {
				f.push_back({ b.colorKey, slot.colorLabel, slot.tab, &color, 1, slot.styles[style - 1].variants, nullptr });
			}
		});
	}

	void CollectOptionFields(std::vector<OptField>& f)
	{
		const uint16_t modelId = CurrentLocalModelId();
        for (const auto& group : SouthClothes::groups) {
            if (group.model == SouthModel()) f.push_back({group.key, group.label, group.tab, SouthValue(group.key), 0, group.count, nullptr});
        }

		if (modelId == ClothesCatalog::kCustomMaleSkinId) {
			namespace CC = ClothesCatalog;
			ClampCustomSelection();
			f.push_back({ "face", "Texture du visage", "visage", &g_look.face, 1, CC::kFaceCount, nullptr });
			f.push_back({ "beardStyle", "Style de barbe", "visage", &g_look.beardStyle, 0, CC::Count(CC::kBeard), &g_look.beardColor });
			f.push_back({ "eyebrowStyle", "Montre", "haut", &g_look.eyebrowStyle, 0, CC::Count(CC::kWatch), nullptr });
			f.push_back({ "hairStyle", "Style de cheveux", "cheveux", &g_look.hairStyle, 1, CC::Count(CC::kHair), &g_look.hairColor });
			f.push_back({ "hairColor", "Couleur des cheveux", "cheveux", &g_look.hairColor, 1, CC::kHair[g_look.hairStyle - 1].variants, nullptr });
			f.push_back({ "topStyle", "Haut", "haut", &g_look.topStyle, 0, CC::Count(CC::kTop), &g_look.topColor });
			if (g_look.topStyle > 0) {
				f.push_back({ "topColor", "Couleur du haut", "haut", &g_look.topColor, 1, CC::kTop[g_look.topStyle - 1].variants, nullptr });
			}
			f.push_back({ "bottomStyle", "Bas", "bas", &g_look.bottomStyle, 0, CC::Count(CC::kBottom), &g_look.bottomColor });
			if (g_look.bottomStyle > 0) {
				f.push_back({ "bottomColor", "Couleur du bas", "bas", &g_look.bottomColor, 1, CC::kBottom[g_look.bottomStyle - 1].variants, nullptr });
			}
			f.push_back({ "shoesStyle", "Chaussures", "chaussures", &g_look.shoesStyle, 0, CC::Count(CC::kShoes), &g_look.shoesColor });
			if (g_look.shoesStyle > 0) {
				f.push_back({ "shoesColor", "Couleur des chaussures", "chaussures", &g_look.shoesColor, 1, CC::kShoes[g_look.shoesStyle - 1].variants, nullptr });
			}
		}
		else if (modelId == 9) {
			AddFemaleFace(f, false);
			AddBaseClothes(f, kFemaleHairStyles, kFemaleTopStyles, kFemaleBottomStyles, kFemaleShoesStyles, 1, 1);
			AddAllTattoos(f);
			AddAccessoryFields(f, true);
		}
		else if (modelId == 246) {
			AddFemaleFace(f, true);
			AddBaseClothes(f, kPoliceFemaleHairStyles, kPoliceFemaleTopStyles, kPoliceFemaleBottomStyles, kPoliceFemaleShoesStyles, 1, 1);
			AddPoliceGear(f);
			AddArmTattoos(f);
		}
		else if (modelId == 280) {
			ClampPoliceMaleSelection();
			f.push_back({ "skinTone", "Couleur de peau", "visage", &g_look.skinTone, 1, 6, nullptr });
			ClampPoliceMaleSelection();
			f.push_back({ "face", "Forme du visage", "visage", &g_look.face, 1, std::min(kFaceCountByTone[g_look.skinTone], kDffFaceSlots), nullptr });
			AddMaleEyebrowsAndBeard(f);
			AddBaseClothes(f, kPoliceMaleHairStyles, kPoliceMaleTopStyles, kPoliceMaleBottomStyles, kPoliceMaleShoesStyles, 1, 1);
			AddPoliceGear(f);
			AddAllTattoos(f);
		}
		else if (modelId == 163) {
			ClampSelection();
			f.push_back({ "skinTone", "Couleur de peau", "visage", &g_look.skinTone, 1, 6, nullptr });
			ClampSelection();
			f.push_back({ "face", "Forme du visage", "visage", &g_look.face, 1, std::min(kFaceCountByTone[g_look.skinTone], kDffFaceSlots), nullptr });
			AddMaleEyebrowsAndBeard(f);
			AddBaseClothes(f, kHairStyles, kTopStyles, kBottomStyles, kShoesStyles, 0, 0);
			AddAllTattoos(f);
			AddAccessoryFields(f, false);
		}
	}

	void AppendJsonString(std::string& out, const char* s)
	{
		out += '"';
		for (; s && *s; ++s) {
			if (*s == '"' || *s == '\\') out += '\\';
			out += *s;
		}
		out += '"';
	}
}

std::string ClothesCustomization::BuildOptionsJson()
{
	std::vector<OptField> fields;
	CollectOptionFields(fields);

	const uint16_t modelId = CurrentLocalModelId();
	const uint16_t realModelId = RealLocalModelId();
	std::string json = "{\"model\":" + std::to_string(static_cast<int>(realModelId));
	// genre : 0 Homme, 1 Femme, 2 Homme 2 (289), 3 Femme 2 (12), 4 Homme 3 (7) - voir SkinBodies.h
	json += ",\"gender\":" + std::to_string(realModelId == 246 ? 1 : SkinBodies::GenderOfModel(realModelId));
	json += ",\"canSwitchGender\":" + std::string(SkinBodies::IsCivilBody(realModelId) ? "true" : "false");
	json += ",\"supported\":" + std::string(IsSupportedSkin(modelId) ? "true" : "false");
	json += ",\"fields\":[";
	for (size_t i = 0; i < fields.size(); ++i) {
		const OptField& o = fields[i];
		if (i > 0) json += ',';
		json += "{\"key\":";
		AppendJsonString(json, o.key);
		json += ",\"label\":";
		AppendJsonString(json, o.label);
		json += ",\"tab\":";
		AppendJsonString(json, o.tab);
		json += ",\"min\":" + std::to_string(o.minV);
		json += ",\"max\":" + std::to_string(o.maxV);
		json += ",\"val\":" + std::to_string(static_cast<int>(*o.ref));
        if (const auto* group = SouthGroup(o.key)) {
            json += ",\"item\":";
            const auto id = *o.ref;
            AppendJsonString(json, id > 0 && id <= group->count ? group->items[id - 1].name : "Visual anterior");
        }
		json += '}';
	}
	json += "]}";
	return json;
}

bool ClothesCustomization::SetOption(const std::string& key, int value)
{
	if (!IsSupportedSkin(CurrentLocalModelId())) return false;

	std::vector<OptField> fields;
	CollectOptionFields(fields);
	for (const OptField& o : fields) {
		if (key != o.key) continue;
		const int v = std::clamp(value, o.minV, o.maxV);
		if (*o.ref != v) {
			*o.ref = static_cast<uint8_t>(v);
			if (o.colorRef) *o.colorRef = 1; // nouveau style -> premiere couleur
		}
        if (key == "topStyle" || key == "topColor") g_look.southTop = 0;
        if (key == "bottomStyle" || key == "bottomColor") g_look.southBottom = 0;
        if (key == "shoesStyle" || key == "shoesColor") g_look.southShoes = 0;
        if (key == "backpackStyle" || key == "backpackColor") g_look.southBackpack = 0;
        if (key == "maskStyle" || key == "maskColor") g_look.southMask = 0;
        if (key == "earStyle" || key == "earColor") g_look.southEar = 0;
        if (key == "clockStyle" || key == "clockColor") g_look.southClock = 0;
		ApplyCurrent();
		return true;
	}
	return false;
}
