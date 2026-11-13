# Plan to Fix Crestwoodrp Issues

## 1. Fix Pending Texture Replacements (BrlTextureShader.cpp)
**Problem**: The shader logs "pending" for textures that can't be found, but still adds the rules, causing crashes.
**Solution**: Modify `SetRule` to NOT add rules where replacementTexture can't be resolved. Also add a cleanup mechanism.

## 2. Fix SIGSEGV Crashes
### 2.1 RealTimeShadowManager::ReturnRealTimeShadow
**Problem**: Function doesn't check if `shdw` is null before accessing `m_pOwner`.
**Solution**: Add null check for `shdw`.

### 2.2 CPhysical::~CPhysical
**Problem**: Empty destructor, but crash occurs. Likely missing cleanup of shadow data.
**Solution**: Add cleanup in destructor to clear shadow data.

### 2.3 CPlayerPed::~CPlayerPed
**Problem**: Similar to CPhysical.
**Solution**: Add cleanup for shadow data.

### 2.4 CPlayerInfo::Process
**Problem**: Hook exists but might not be installed correctly.
**Solution**: Ensure hook is installed and working.

### 2.5 CWorld::Process
**Problem**: Calls `pNetGame->GetPlayerPool()->ProcessAttachedObjects()` without null checks.
**Solution**: Add null checks for `pNetGame` and `pPlayerPool`.

## 3. Fix Chat Bubbles
**Problem**: Players can't see bubbles when someone writes.
**Solution**: Debug the bubble pool Draw method and ensure bubbles are being created and rendering correctly. Check if bubbles are being skipped due to position checks.

## Implementation Steps
1. Modify `BrlTextureShader.cpp` SetRule function
2. Modify `RealTimeShadowManager.cpp` ReturnRealTimeShadow function
3. Modify `CPhysical.h` and `CPhysical.cpp` destructor
4. Modify `CPedGTA.h` destructor (or CPlayerPedGta)
5. Verify CPlayerInfo::Process hook is working
6. Modify `World.cpp` ProcessPedsAfterPreRender to add null checks
7. Debug chat bubble rendering issues

## Files to Modify
- `app/src/main/cpp/samp/game/BrlTextureShader.cpp`
- `app/src/main/cpp/samp/game/RealTimeShadowManager.cpp`
- `app/src/main/cpp/samp/game/Entity/CPhysical.h`
- `app/src/main/cpp/samp/game/Entity/CPhysical.cpp`
- `app/src/main/cpp/samp/game/Entity/PlayerPedGta.h`
- `app/src/main/cpp/samp/game/Entity/PlayerPedGta.cpp`
- `app/src/main/cpp/samp/game/World.cpp`
- `app/src/main/cpp/samp/game/hooks.cpp` (verify hook)
- `app/src/main/cpp/samp/gui/gui.cpp` (bubble rendering check)

## Expected Outcome
- No more "pending" texture warnings
- No SIGSEGV crashes
- Chat bubbles visible when players write