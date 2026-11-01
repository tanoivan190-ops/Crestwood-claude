#pragma once
// Adresse d'une fonction de libGTASA.so par son SYMBOLE (dlsym), au lieu d'une adresse ecrite en dur.
// Les adresses en dur de ces appels venaient d'une AUTRE compilation de libGTASA 2.10 (decalage de 0x50,
// 0x80...) : avec la libGTASA.so fournie, elles tombaient au milieu d'autres fonctions -> plantage du 32 bits.
// dlsym renvoie deja l'adresse Thumb (bit 0 a 1) : appel direct.
#include <dlfcn.h>
#include <cstdint>

void FLog(const char* fmt, ...);

inline uintptr_t GtaSymAddr(const char* name)
{
	static void* h = nullptr;
	if (!h) h = dlopen("libGTASA.so", RTLD_NOW | RTLD_NOLOAD);
	void* p = h ? dlsym(h, name) : nullptr;
	if (!p) FLog("[gta_sym] symbole introuvable : %s", name);
	return (uintptr_t)p;
}

// Resolu une seule fois par endroit d'appel
#define GTA_SYM(name) ([]() -> uintptr_t { static uintptr_t a = GtaSymAddr(name); return a; }())
