// ============================================================
//  [v18] IMPORT DE MAPPINGS SANS RECOMPILER  (include/MapFile.inc)
// ------------------------------------------------------------
//  Mets tes fichiers dans  scriptfiles/mappings/  et liste-les (un par ligne) dans
//      scriptfiles/mappings/liste.txt
//  Formats lus automatiquement, ligne par ligne :
//    - Pawn / Texture Studio / convertisseurs :
//        CreateObject(...)  CreateDynamicObject(...)  CreateDynamicObjectEx(...)
//        SetObjectMaterial / SetDynamicObjectMaterial (s'applique au dernier objet cree)
//        SetDynamicObjectMaterialText (idem)
//        RemoveBuildingForPlayer(playerid, modele, x, y, z, rayon)
//    - MTA:SA (.map de l'editeur MTA, XML) :
//        <object ... model="" posX="" posY="" posZ="" rotX="" rotY="" rotZ="" />
//        <removeWorldObject ... model="" lodModel="" radius="" posX="" posY="" posZ="" />
//  Chaque fichier devient un mapping du panneau /mappings (nom = nom du fichier).
//  Les objets passent par le streamer integre (plafond par joueur, aucun plugin).
//  Charge au demarrage du serveur : redemarre (ou gmx) apres avoir ajoute un fichier.
// ============================================================

#define MF_MAX_RB             900         // batiments retires (limite SA-MP : 1000 par joueur)
#define MF_LINE               640

new g_MfRbModel[MF_MAX_RB];
new Float:g_MfRb[MF_MAX_RB][4];
new g_MfRbCount;
new g_MfFiles;
new g_MfObjects;
new g_MfLast = -1;

// nombre suivant a partir de idx (saute tout ce qui n'est pas un debut de nombre) ; 0 = aucun
stock MF_NextNum(const s[], &idx, &Float:out)
{
	while (s[idx] && !((s[idx] >= '0' && s[idx] <= '9') || s[idx] == '-' || s[idx] == '.')) idx++;
	if (!s[idx]) return 0;
	new tok[32], j = 0;
	// hexadecimal (0x...)
	if (s[idx] == '0' && (s[idx + 1] == 'x' || s[idx + 1] == 'X'))
	{
		idx += 2;
		new v = 0;
		while (s[idx])
		{
			new c = s[idx], d = -1;
			if (c >= '0' && c <= '9') d = c - '0';
			else if (c >= 'a' && c <= 'f') d = c - 'a' + 10;
			else if (c >= 'A' && c <= 'F') d = c - 'A' + 10;
			if (d < 0) break;
			v = (v << 4) | d;
			idx++;
		}
		out = Float:v;   // bits bruts (couleur) : relu avec _:out
		return 2;
	}
	while (s[idx] && j < 31 && ((s[idx] >= '0' && s[idx] <= '9') || s[idx] == '-' || s[idx] == '.' || s[idx] == 'e' || s[idx] == 'E' || s[idx] == '+'))
		tok[j++] = s[idx++];
	tok[j] = 0;
	out = floatstr(tok);
	return 1;
}

// entier (decimal ou 0x...)
stock MF_NextInt(const s[], &idx)
{
	new Float:f, k = MF_NextNum(s, idx, f);
	if (k == 2) return _:f;
	return floatround(f, floatround_tozero);
}

// saute le 1er argument (variable : tmpobjid, playerid...) apres la parenthese
stock MF_SkipFirstArg(const s[], &idx)
{
	while (s[idx] && s[idx] != '(') idx++;
	while (s[idx] && s[idx] != ',') idx++;
	if (s[idx] == ',') idx++;
	return 1;
}

// chaine entre guillemets suivante
stock MF_NextStr(const s[], &idx, out[], size)
{
	while (s[idx] && s[idx] != '"') idx++;
	if (!s[idx]) { out[0] = 0; return 0; }
	idx++;
	new j = 0;
	while (s[idx] && s[idx] != '"' && j < size - 1) out[j++] = s[idx++];
	out[j] = 0;
	if (s[idx] == '"') idx++;
	return 1;
}

// valeur d'un attribut XML : nom="valeur"
stock MF_Attr(const s[], const name[], out[], size)
{
	new pat[24];
	format(pat, sizeof pat, " %s=\"", name);
	new p = strfind(s, pat, true);
	if (p < 0) { out[0] = 0; return 0; }
	p += strlen(pat);
	new j = 0;
	while (s[p] && s[p] != '"' && j < size - 1) out[j++] = s[p++];
	out[j] = 0;
	return 1;
}

stock Float:MF_AttrF(const s[], const name[])
{
	new v[32];
	if (!MF_Attr(s, name, v, sizeof v)) return 0.0;
	return floatstr(v);
}

stock MF_AddRemove(model, Float:x, Float:y, Float:z, Float:r)
{
	if (g_MfRbCount >= MF_MAX_RB || model == 0) return 0;
	new i = g_MfRbCount++;
	g_MfRbModel[i] = model;
	g_MfRb[i][0] = x; g_MfRb[i][1] = y; g_MfRb[i][2] = z; g_MfRb[i][3] = (r <= 0.0) ? 0.25 : r;
	return 1;
}

stock MF_ParseLine(const line[])
{
	new idx = 0;
	// ---------- MTA (XML) ----------
	if (strfind(line, "<object", true) >= 0)
	{
		new m[12];
		if (!MF_Attr(line, "model", m, sizeof m)) return 0;
		new model = strval(m);
		if (model < 300) return 0;
		new o = Ms_Add(model, MF_AttrF(line, "posX"), MF_AttrF(line, "posY"), MF_AttrF(line, "posZ"),
			MF_AttrF(line, "rotX"), MF_AttrF(line, "rotY"), MF_AttrF(line, "rotZ"), 300.0);
		if (o >= 0) { g_MfLast = o; g_MfObjects++; }
		return 1;
	}
	if (strfind(line, "<removeWorldObject", true) >= 0)
	{
		new m[12], lod[12];
		MF_Attr(line, "model", m, sizeof m);
		MF_Attr(line, "lodModel", lod, sizeof lod);
		new Float:x = MF_AttrF(line, "posX"), Float:y = MF_AttrF(line, "posY"), Float:z = MF_AttrF(line, "posZ"), Float:r = MF_AttrF(line, "radius");
		MF_AddRemove(strval(m), x, y, z, r);
		if (strval(lod) > 0) MF_AddRemove(strval(lod), x, y, z, r);
		return 1;
	}
	// ---------- Pawn ----------
	if (strfind(line, "//") == 0) return 0;
	new p = strfind(line, "RemoveBuildingForPlayer(", true);
	if (p >= 0)
	{
		idx = p;
		MF_SkipFirstArg(line, idx);
		new model = MF_NextInt(line, idx), Float:x, Float:y, Float:z, Float:r;
		MF_NextNum(line, idx, x); MF_NextNum(line, idx, y); MF_NextNum(line, idx, z); MF_NextNum(line, idx, r);
		MF_AddRemove(model, x, y, z, r);
		return 1;
	}
	p = strfind(line, "MaterialText(", true);
	if (p >= 0)
	{
		if (g_MfLast < 0) return 0;
		idx = p;
		MF_SkipFirstArg(line, idx);
		new mi = MF_NextInt(line, idx), text[96], font[24];
		MF_NextStr(line, idx, text, sizeof text);
		new size = MF_NextInt(line, idx);
		MF_NextStr(line, idx, font, sizeof font);
		new fsize = MF_NextInt(line, idx), bold = MF_NextInt(line, idx), fcol = MF_NextInt(line, idx), bcol = MF_NextInt(line, idx), align = MF_NextInt(line, idx);
		SetDynamicObjectMaterialText(g_MfLast, mi, text, size ? size : OBJECT_MATERIAL_SIZE_256x128, font[0] ? font : ("Arial"), fsize ? fsize : 24, bold, fcol, bcol, align);
		return 1;
	}
	p = strfind(line, "ObjectMaterial(", true);
	if (p >= 0)
	{
		if (g_MfLast < 0) return 0;
		idx = p;
		MF_SkipFirstArg(line, idx);
		new mi = MF_NextInt(line, idx), model = MF_NextInt(line, idx), txd[24], tex[32];
		MF_NextStr(line, idx, txd, sizeof txd);
		MF_NextStr(line, idx, tex, sizeof tex);
		new col = MF_NextInt(line, idx);
		SetDynamicObjectMaterial(g_MfLast, mi, model, txd, tex, col);
		return 1;
	}
	p = strfind(line, "CreateDynamicObjectEx(", true);
	if (p < 0) p = strfind(line, "CreateDynamicObject(", true);
	if (p < 0) p = strfind(line, "CreateObject(", true);
	if (p >= 0)
	{
		idx = p;
		while (line[idx] && line[idx] != '(') idx++;
		new model = MF_NextInt(line, idx), Float:v[6];
		for (new k = 0; k < 6; k++) if (!MF_NextNum(line, idx, v[k])) return 0;
		if (model < 300) return 0;
		new o = Ms_Add(model, v[0], v[1], v[2], v[3], v[4], v[5], 300.0);
		if (o >= 0) { g_MfLast = o; g_MfObjects++; }
		return 1;
	}
	return 0;
}

stock MF_LoadFile(const name[])
{
	new path[96];
	format(path, sizeof path, "mappings/%s", name);
	new File:f = fopen(path, io_read);
	if (!f) { printf("[MAPPINGS] fichier introuvable : scriptfiles/%s", path); return 0; }
	new line[MF_LINE], label[32], before = g_MfObjects, rbBefore = g_MfRbCount;
	// nom du mapping = nom du fichier sans extension
	new j = 0;
	for (new i = 0; name[i] && name[i] != '.' && j < 31; i++) label[j++] = (name[i] == '_') ? ' ' : name[i];
	label[j] = 0;
	MapReg_Begin(label);
	g_MfLast = -1;
	while (fread(f, line))
	{
		MF_ParseLine(line);
	}
	fclose(f);
	MapReg_End();
	g_MfFiles++;
	printf("[MAPPINGS] %s : %d objets, %d batiments retires.", name, g_MfObjects - before, g_MfRbCount - rbBefore);
	return 1;
}

stock MapFile_LoadAll()
{
	g_MfRbCount = 0; g_MfFiles = 0; g_MfObjects = 0;
	new File:list = fopen("mappings/liste.txt", io_read);
	if (!list) { print("[MAPPINGS] aucun fichier scriptfiles/mappings/liste.txt (import de mappings non utilise)."); return 0; }
	new line[96], name[64];
	while (fread(list, line))
	{
		new j = 0;
		for (new i = 0; line[i] && j < 63; i++)
		{
			new c = line[i];
			if (c == '\r' || c == '\n') break;
			if (c == ' ' && !j) continue;
			if (c == '/' || c == 92) { j = 0; break; }   // pas de chemins : fichiers du dossier uniquement
			name[j++] = c;
		}
		name[j] = 0;
		if (!name[0] || name[0] == '#' || strfind(name, "..") >= 0) continue;
		MF_LoadFile(name);
	}
	fclose(list);
	printf("[MAPPINGS] import : %d fichier(s), %d objets, %d batiments retires.", g_MfFiles, g_MfObjects, g_MfRbCount);
	return g_MfFiles;
}

stock MapFile_RemoveBuildings(playerid)
{
	for (new i = 0; i < g_MfRbCount; i++)
		RemoveBuildingForPlayer(playerid, g_MfRbModel[i], g_MfRb[i][0], g_MfRb[i][1], g_MfRb[i][2], g_MfRb[i][3]);
	return 1;
}
