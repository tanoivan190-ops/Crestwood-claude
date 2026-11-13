"""Port the read-only Lasouth model loader to Crestwood's engine types."""
from pathlib import Path
import hashlib
import json
import shutil
ROOT = Path(__file__).resolve().parents[1]
SOURCE = Path(r'C:\LASouth\atualizacao\Realized')
native = SOURCE / 'app/src/main/cpp/multiplayer/game/character'
assets = SOURCE / 'arquivos/root/root/cache-full/custom'
out = ROOT / 'app/src/main/assets/clothes/south'
hashes = json.loads((out/'source-hashes.json').read_text())
for gender in ('male','female'):
    for ext in ('dff','txd'):
        rel = f'{gender}skin/{gender}.{ext}'
        target = out / rel
        target.parent.mkdir(parents=True, exist_ok=True)
        shutil.copyfile(assets/rel, target)
        hashes[rel] = hashlib.sha256((assets/rel).read_bytes()).hexdigest()
header = (native/'CharacterModelLoader.h').read_text().replace('CCharacterModelLoader','SouthModelLoader')
code = (native/'CharacterModelLoader.cpp').read_text()
hashes['native/CharacterModelLoader.cpp'] = hashlib.sha256((native/'CharacterModelLoader.cpp').read_bytes()).hexdigest()
code = code.replace('CharacterModelLoader.h','SouthModelLoader.h').replace('CCharacterModelLoader','SouthModelLoader')
code = code.replace('#include "CharacterSlots.h"','').replace('game/Entity/Ped/Ped.h','game/Entity/CPedGTA.h')
import re
code = re.sub(r'\bCPed\b','CPedGTA',code)
code = code.replace('    const char* gender = CCharacterSlots::GetGenderName(modelId);', '    const char* gender = (modelId == 7 || modelId == 163) ? "male" : (modelId == 9 || modelId == 12) ? "female" : nullptr;')
code = code.replace('    snprintf(out, outSize, "%scustom/%sskin/", g_pszStorage ? g_pszStorage : "", gender);', '''    if (!pJavaWrapper || !javaVM) return false;
    JNIEnv* env = nullptr;
    if (javaVM->GetEnv(reinterpret_cast<void**>(&env), JNI_VERSION_1_6) != JNI_OK || !env) return false;
    jclass cls = env->GetObjectClass(pJavaWrapper->activity);
    const auto method = cls ? env->GetMethodID(cls, "prepareClothesModel", "(Ljava/lang/String;)Ljava/lang/String;") : nullptr;
    if (env->ExceptionCheck()) env->ExceptionClear();
    if (!method) { if (cls) env->DeleteLocalRef(cls); return false; }
    jstring g = env->NewStringUTF(gender);
    auto path = static_cast<jstring>(env->CallObjectMethod(pJavaWrapper->activity, method, g));
    env->DeleteLocalRef(g); env->DeleteLocalRef(cls);
    if (env->ExceptionCheck()) { env->ExceptionClear(); if (path) env->DeleteLocalRef(path); return false; }
    if (!path) return false;
    const char* chars = env->GetStringUTFChars(path, nullptr);
    if (!chars) { env->DeleteLocalRef(path); return false; }
    const int length = snprintf(out, outSize, "%s/", chars);
    env->ReleaseStringUTFChars(path, chars); env->DeleteLocalRef(path);
    if (length < 0 || static_cast<size_t>(length) >= outSize) return false;''')
code = code.replace('CCharacterSlots::IsSupportedModel(modelId)', '(modelId == 7 || modelId == 9 || modelId == 12 || modelId == 163)')
code = code.replace('        ped->CreateRwObject();', '''        // CEntityGTA::DeleteRwObject uses slot 9; CreateRwObject is slot 8.
        const auto table = *reinterpret_cast<uintptr_t**>(ped);
        reinterpret_cast<void (*)(CPedGTA*)>(table[8])(ped);''')
code = code.replace('#include "main.h"', '#include "main.h"\n#include "java/jniutil.h"\nextern CJavaWrapper* pJavaWrapper;')
for abi in ('samp','samp32'):
    base = ROOT / f'app/src/main/cpp/{abi}'
    (base/'gui/SouthModelLoader.h').write_text(header)
    (base/'gui/SouthModelLoader.cpp').write_text(code.replace('Log(', 'FLog(') if abi=='samp32' else code)
(out/'source-hashes.json').write_text(json.dumps(hashes,indent=2))
print('Imported shared male/female model assets and adapted the Lasouth model loader.')
