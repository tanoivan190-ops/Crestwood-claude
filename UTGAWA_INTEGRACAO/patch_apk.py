"""Filtro de loja no editor de roupas do APK: com "shop":1..4 no JSON do 9551, a página só mostra
os campos que aquela loja vende. Aplica nas duas cópias (samp / samp32)."""
import sys, os
ROOT = sys.argv[1]   # .../app/src/main/cpp
def rep(path, old, new):
    s = open(path, encoding='utf-8').read()
    if new in s: return
    assert s.count(old) == 1, (path, old[:60], s.count(old))
    open(path, 'w', encoding='utf-8').write(s.replace(old, new))
for d in ('samp', 'samp32'):
    g = os.path.join(ROOT, d, 'gui')
    rep(os.path.join(g, 'ClothesCustomization.h'),
        '    static std::string BuildOptionsJson();',
        '''    static std::string BuildOptionsJson();

    // [UTGAWA] boutique ouverte par le serveur ("shop" du dialogue 9551) : 0 = editeur complet,
    // 1 vetements, 2 accessoires, 3 barbier, 4 tatouage -> BuildOptionsJson ne garde que ces champs.
    static void SetShopFilter(int shop);
    static int ShopFilter();
    static int ShopOfKey(const char* key);''')
    rep(os.path.join(g, 'ClothesCustomization.cpp'),
        '#include <algorithm>\n',
        '#include <algorithm>\n#include <atomic>\n')
    rep(os.path.join(g, 'ClothesCustomization.cpp'),
        '''std::string ClothesCustomization::BuildOptionsJson()
{
	std::vector<OptField> fields;
	CollectOptionFields(fields);
''', '''static std::atomic<int> g_utgShopFilter{0};

void ClothesCustomization::SetShopFilter(int shop) { g_utgShopFilter.store((shop >= 1 && shop <= 4) ? shop : 0); }
int ClothesCustomization::ShopFilter() { return g_utgShopFilter.load(); }

int ClothesCustomization::ShopOfKey(const char* key)
{
	if (!key) return 0;
	auto starts = [key](const char* p) { return std::strncmp(key, p, std::strlen(p)) == 0; };
	if (starts("top") || starts("bottom") || starts("shoes")) return 1;
	if (starts("hat") || starts("glasse") || starts("mask") || starts("ear") || starts("necklace") ||
	    starts("clock") || starts("backpack")) return 2;
	if (starts("hair") || starts("eyebrow") || starts("beard") || starts("makeup")) return 3;
	if (starts("tattoo")) return 4;
	return 0;
}

std::string ClothesCustomization::BuildOptionsJson()
{
	std::vector<OptField> fields;
	CollectOptionFields(fields);
	const int shop = g_utgShopFilter.load();
	if (shop != 0) {
		fields.erase(std::remove_if(fields.begin(), fields.end(),
			[shop](const OptField& o) { return ShopOfKey(o.key) != shop; }), fields.end());
	}
''')
    rep(os.path.join(g, 'ClothesCustomization.cpp'),
        '''	json += ",\\"canSwitchGender\\":" + std::string(SkinBodies::IsCivilBody(realModelId) ? "true" : "false");''',
        '''	json += ",\\"canSwitchGender\\":" + std::string((shop == 0 && SkinBodies::IsCivilBody(realModelId)) ? "true" : "false");
	json += ",\\"shop\\":" + std::to_string(shop);''')
    rep(os.path.join(g, 'CharacterCreator.cpp'),
        '''	if (dialogId == 9552) {
		g_serverCloseRequested.store(true);
		return;
	}''', '''	if (dialogId == 9552) {
		ClothesCustomization::SetShopFilter(0);
		g_serverCloseRequested.store(true);
		return;
	}
	// [UTGAWA] 9551 : boutique (1..4) ou editeur complet (0)
	if (dialogId == 9551) ClothesCustomization::SetShopFilter(JsonInt(json, "shop", 0));''')
print('apk ok')
