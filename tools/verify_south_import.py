"""Verify copied assets, saved field order, and read-only source integrity."""
from pathlib import Path
import hashlib
import json
import re
import struct

ROOT = Path(__file__).resolve().parents[1]
SOURCE = Path(r'C:\LASouth\atualizacao\Realized')
custom = SOURCE / 'arquivos/root/root/cache-full/custom'
out = ROOT / 'app/src/main/assets/clothes/south'
hashes = json.loads((out/'source-hashes.json').read_text())
for name, expected in hashes.items():
    src = SOURCE/'app/src/main/cpp/multiplayer/game/character'/name[7:] if name.startswith('native/') else custom/name
    assert hashlib.sha256(src.read_bytes()).hexdigest() == expected, f'Source changed: {src}'
    if name.startswith('textures/'):
        assert hashlib.sha256((out/name[9:]).read_bytes()).hexdigest() == expected
    elif name.endswith(('.dff','.txd')):
        assert hashlib.sha256((out/name).read_bytes()).hexdigest() == expected
catalogue = json.loads((out/'catalogue.json').read_text(encoding='utf-8'))
assert sum(len(g['items']) for g in catalogue) == 1223
for group in catalogue:
    assert len(group['items']) <= 255
    for item in group['items']:
        assert (out/item['texture']).is_file()
server = (ROOT/'server/basicgm/basicgm1.pwn').read_bytes().decode('latin-1')
keys = re.findall(r'"([A-Za-z]+)"', server.split('new const g_ClothKeys[CLOTH_COUNT][20] =',1)[1].split('};',1)[0])
assert len(keys) == 46
for abi in ('samp','samp32'):
    creator = (ROOT/f'app/src/main/cpp/{abi}/gui/CharacterCreator.cpp').read_text(encoding='utf-8')
    values = re.findall(r'l\.([A-Za-z]+)',creator.split('const uint8_t v[46] =',1)[1].split('};',1)[0])
    assert values == keys, f'{abi} saved fields differ from Pawn'
    native = (ROOT/f'app/src/main/cpp/{abi}/net/netgame.cpp').read_text(encoding='utf-8')
    values = re.findall(r'l\.([A-Za-z]+)',native.split('const uint8_t v[46] =',1)[1].split('};',1)[0])
    assert values == keys
for filename in ('ClothesCustomization.h','ClothesCustomization.cpp','SouthClothes.h'):
    assert (ROOT/f'app/src/main/cpp/samp/gui/{filename}').read_bytes() == (ROOT/f'app/src/main/cpp/samp32/gui/{filename}').read_bytes(), filename
java = [(ROOT/f'app/src/main/java/com/gta/{p}/SAMP.java').read_text(encoding='utf-8') for p in ('game','game32')]
for method,next_method in [('prepareClothesModel','loadClothesTexture'),('loadClothesTexture','clothesOpen')]:
    blocks=[s.split('public '+('synchronized String ' if method=='prepareClothesModel' else 'byte[] ')+method,1)[1].split('public '+('byte[] ' if next_method=='loadClothesTexture' else 'void ')+next_method,1)[0] for s in java]
    assert blocks[0] == blocks[1], method
amx = (ROOT/'server/basicgm/basicgm1.amx').read_bytes()
assert struct.unpack_from('<H',amx,4)[0] == 0xf1e0 and amx[6] == 8
print(f'PASS: {len(hashes)} source hashes unchanged, 1223 copied pieces, 46 fields aligned, both clients synchronized, AMX v8 ({len(amx)} bytes).')
