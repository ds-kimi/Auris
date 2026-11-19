# Fetches the pinned sherpa-onnx static libs (/MT, static ONNX Runtime) for the
# 64-bit Windows build into vendor/sherpa-onnx. Idempotent; checks SHA256.
$ErrorActionPreference = "Stop"

$Version = "v2.0.1"
$LibSha = "bcd6125c90cfa5454387b983913e2f058bc52cfac342c021bd2c42d2d88b9488"
$HdrSha = "5698f38ba09ad2bdf085c56a6cbbd7741acf15e25f4ce5c7097b9702cadf437f"
$Root = (Resolve-Path "$PSScriptRoot\..\..").Path
$Dest = Join-Path $Root "vendor\sherpa-onnx"
$Tar = "sherpa-onnx-$Version-win-x64-static-MT-Release-no-tts-lib.tar.bz2"
$Hdr = Join-Path $Dest "include\sherpa-onnx\c-api\c-api.h"

if ((Test-Path "$Dest\win-x64\lib\sherpa-onnx-c-api.lib") -and (Test-Path $Hdr)) {
    Write-Output "[sherpa-onnx] already present, skipping"
    exit 0
}
New-Item -ItemType Directory -Force "$Dest\dl", "$Dest\win-x64", (Split-Path $Hdr) | Out-Null
$TarPath = Join-Path (Join-Path $Dest "dl") $Tar
Write-Output "[sherpa-onnx] downloading $Tar"
curl.exe -fsSL -o $TarPath --max-time 600 "https://github.com/k2-fsa/sherpa-onnx/releases/download/$Version/$Tar"
if ((Get-FileHash $TarPath -Algorithm SHA256).Hash -ne $LibSha.ToUpper()) { throw "sherpa-onnx archive SHA256 mismatch" }
Write-Output "[sherpa-onnx] extracting"
# Python tarfile: Git's GNU tar reads "C:\..." as a remote host, and the
# Server 2022 bsdtar hands bzip2 to an external program; both hang on CI
$Py = "import sys, tarfile`nwith tarfile.open(sys.argv[1], 'r:bz2') as t:`n    for m in t.getmembers():`n        p = m.name.split('/', 1)`n        if len(p) == 2 and p[1]:`n            m.name = p[1]`n            t.extract(m, sys.argv[2], filter='data')"
python -I -c $Py $TarPath "$Dest\win-x64"
if ($LASTEXITCODE -ne 0) { throw "sherpa-onnx extraction failed" }
Write-Output "[sherpa-onnx] fetching c-api.h"
curl.exe -fsSL -o $Hdr --max-time 120 "https://raw.githubusercontent.com/k2-fsa/sherpa-onnx/$Version/sherpa-onnx/c-api/c-api.h"
if ((Get-FileHash $Hdr -Algorithm SHA256).Hash -ne $HdrSha.ToUpper()) { throw "c-api.h SHA256 mismatch" }
Write-Output "[sherpa-onnx] $Version ready"
