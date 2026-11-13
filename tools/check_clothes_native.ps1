$ErrorActionPreference = 'Stop'
$ndkBin = 'C:\Users\gyygg\AppData\Local\Android\Sdk\ndk\26.2.11394342\toolchains\llvm\prebuilt\windows-x86_64'
foreach ($arch in @(@('samp','aarch64-linux-android28','false'), @('samp32','armv7a-linux-androideabi28','true'))) {
    $basePath = "app/src/main/cpp/$($arch[0])"
    $arguments = @("--target=$($arch[1])", "--sysroot=$ndkBin/sysroot", '-Itools', '-std=c++20', '-fsyntax-only', '-w', "-DVER_x32=$($arch[2])", "-I$basePath", "-I$basePath/util", "-I$basePath/vendor", "-I$basePath/vendor/imgui", "-I$basePath/game/RW", "-I$basePath/game/Core", '-Iapp/src/main/cpp/opus/include')
    & "$ndkBin/bin/clang++.exe" @arguments "$basePath/gui/SouthModelLoader.cpp" "$basePath/gui/ClothesCustomization.cpp" "$basePath/gui/CharacterCreator.cpp" "$basePath/game/BrlTextureShader.cpp" "$basePath/net/netgame.cpp"
    if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
    Write-Output "PASS: $($arch[0]) native syntax"
}
