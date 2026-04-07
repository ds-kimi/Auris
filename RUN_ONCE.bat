@echo off
setlocal enabledelayedexpansion

:: ============================================================
:: build_shaders.bat
:: Full setup from a fresh git clone. Run from the module/ dir.
::
:: Requirements:
::   - Git
::   - CMake
::   - Vulkan SDK (glslc in PATH or installed to C:\VulkanSDK)
::   - Visual Studio with VC tools
:: ============================================================

set MODULE_DIR=%~dp0
:: Repo root is either the module dir itself (bat copied to root)
:: or one level up (bat stays in module/ subdir)
if exist "%MODULE_DIR%.git" (
    set REPO_ROOT=%MODULE_DIR%
) else (
    set REPO_ROOT=%MODULE_DIR%..\
)
set WHISPER_DIR=%MODULE_DIR%vendor\whisper.cpp
set GMCOMMON_DIR=%REPO_ROOT%garrysmod_common
set SHADERS_DIR=%WHISPER_DIR%\ggml\src\ggml-vulkan\vulkan-shaders
set GEN_SRC=%SHADERS_DIR%\vulkan-shaders-gen.cpp
set GEN_EXE=%MODULE_DIR%vulkan-shaders-gen.exe
set SPV_DIR=%MODULE_DIR%shader_spv
set CPP_DIR=%MODULE_DIR%shader_cpp
set OUT_CPP=%WHISPER_DIR%\ggml\src\ggml-vulkan\ggml-vulkan-shaders.cpp
set OUT_HPP=%WHISPER_DIR%\ggml\src\ggml-vulkan\ggml-vulkan-shaders.hpp
set VULKAN_CPP=%WHISPER_DIR%\ggml\src\ggml-vulkan\ggml-vulkan.cpp

:: ── [1/8] Auto-detect MSVC ───────────────────────────────────
where cl.exe >nul 2>&1
if errorlevel 1 (
    set VSWHERE="%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
    if not exist !VSWHERE! (
        echo ERROR: cl.exe not found and vswhere.exe missing.
        exit /b 1
    )
    for /f "usebackq tokens=*" %%I in (`!VSWHERE! -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do set VS_PATH=%%I
    if not defined VS_PATH (
        echo ERROR: No Visual Studio with VC tools found.
        exit /b 1
    )
    call "!VS_PATH!\VC\Auxiliary\Build\vcvars64.bat" >nul
)

:: ── [2/8] Git submodules ─────────────────────────────────────
echo [2/8] Initialising git submodules...
pushd "%REPO_ROOT%"
git submodule update --init --recursive
if errorlevel 1 ( echo ERROR: git submodule failed & popd & exit /b 1 )
popd

:: ── [3/8] garrysmod_common submodules ────────────────────────
echo [3/8] Initialising garrysmod_common submodules...
pushd "%GMCOMMON_DIR%"
git submodule update --init --recursive
if errorlevel 1 ( echo ERROR: garrysmod_common submodules failed & popd & exit /b 1 )
popd

:: ── [4/8] Build whisper.cpp with Vulkan ──────────────────────
echo [4/8] Building whisper.cpp with Vulkan (this takes a while)...
pushd "%WHISPER_DIR%"
cmake -B build -DGGML_VULKAN=1
if errorlevel 1 ( echo ERROR: cmake configure failed & popd & exit /b 1 )
cmake --build build -j --config Release
if errorlevel 1 ( echo ERROR: cmake build failed & popd & exit /b 1 )
pushd build
cmake -DGGML_VULKAN=ON ..
cmake --build . -j
popd
popd

:: ── [5/8] Build vulkan-shaders-gen ───────────────────────────
echo [5/8] Building vulkan-shaders-gen...
cl.exe /nologo /EHsc /O2 /std:c++17 /permissive- /Fe:"%GEN_EXE%" "%GEN_SRC%"
if errorlevel 1 ( echo ERROR: Failed to build vulkan-shaders-gen.exe & exit /b 1 )

:: ── [6/8] Find glslc ─────────────────────────────────────────
set GLSLC=glslc
where glslc >nul 2>&1
if errorlevel 1 (
    if defined VULKAN_SDK (
        set GLSLC=!VULKAN_SDK!\Bin\glslc.exe
    ) else (
        for /d %%D in ("C:\VulkanSDK\*") do set GLSLC=%%D\Bin\glslc.exe
    )
)
echo [glslc] Using: %GLSLC%

:: ── [7/8] Compile shaders ────────────────────────────────────
if exist "%SPV_DIR%" rmdir /s /q "%SPV_DIR%"
if exist "%CPP_DIR%" rmdir /s /q "%CPP_DIR%"
mkdir "%SPV_DIR%"
mkdir "%CPP_DIR%"

"%GEN_EXE%" --output-dir "%SPV_DIR%" --target-hpp "%OUT_HPP%"

echo [7/8] Compiling 151 shaders...
set COUNT=0
for %%F in ("%SHADERS_DIR%\*.comp") do (
    set /a COUNT+=1
    set "NAME=%%~nF"
    "%GEN_EXE%" --glslc "%GLSLC%" --source "%%F" --output-dir "%SPV_DIR%" --target-hpp "%OUT_HPP%" --target-cpp "%CPP_DIR%\!NAME!.cpp"
    if errorlevel 1 ( echo ERROR: Failed on %%F & goto cleanup_fail )
)
echo Compiled %COUNT% shaders.

:: ── [8/8] Merge + patch ───────────────────────────────────────
echo [8/8] Merging shaders and patching vendor files...
if exist "%OUT_CPP%" del "%OUT_CPP%"
for %%F in ("%CPP_DIR%\*.cpp") do type "%%F" >> "%OUT_CPP%"

rmdir /s /q "%SPV_DIR%"
rmdir /s /q "%CPP_DIR%"
del "%GEN_EXE%" 2>nul

findstr /c:"operator<<(std::ostream" "%VULKAN_CPP%" >nul 2>&1
if errorlevel 1 (
    echo [patch] Patching ggml-vulkan.cpp...
    powershell -NoProfile -ExecutionPolicy Bypass -Command ^
        "$f = [System.IO.Path]::GetFullPath('%VULKAN_CPP:\=\\%');" ^
        "$content = [System.IO.File]::ReadAllText($f);" ^
        "$old = '#include <vulkan/vulkan.hpp>';" ^
        "$inject = $old + [System.Environment]::NewLine + [System.Environment]::NewLine + '#ifdef _MSC_VER' + [System.Environment]::NewLine + '#include <ostream>' + [System.Environment]::NewLine + 'inline std::ostream& operator<<(std::ostream& os, vk::Buffer const^&) { return os; }' + [System.Environment]::NewLine + '#endif';" ^
        "$content = $content.Replace($old, $inject);" ^
        "[System.IO.File]::WriteAllText($f, $content);"
    echo [patch] Done.
) else (
    echo [patch] Already patched, skipping.
)

echo.
echo Setup complete. Now run premake to generate the VS solution.
exit /b 0

:cleanup_fail
rmdir /s /q "%SPV_DIR%" 2>nul
rmdir /s /q "%CPP_DIR%" 2>nul
del "%GEN_EXE%" 2>nul
exit /b 1
