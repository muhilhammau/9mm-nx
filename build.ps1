# build.ps1 -- PowerShell version of runtime/tools/docker_build.sh
# usage: .\build.ps1 | .\build.ps1 clean | .\build.ps1 rt-files | .\build.ps1 DCR_GL_MESA=0
# Docker writes progress to stderr; with 'Stop' + 2>&1 that aborts the build
# mid-way on Windows PowerShell 5.1, so errors are judged by the exit code only.
param([Parameter(ValueFromRemainingArguments=$true)] [string[]]$MakeArgs)
$ErrorActionPreference = 'Continue'

$here = $PSScriptRoot

# libnx32 prefix: $env:DCR_LIBNX32, else next to the port, else the known install.
$cands = @(
  $env:DCR_LIBNX32,
  (Join-Path $here '..\libnx32\prefix'),
  (Join-Path $here '..\..\libnx32\prefix'),
  'C:\Users\muhil\Downloads\GAMES\Android\libnx32\prefix'
) | Where-Object { $_ -and (Test-Path (Join-Path $_ 'lib\libnx.a')) }
if (-not $cands) { Write-Host "libnx32 prefix tidak ditemukan (set DCR_LIBNX32)" -ForegroundColor Red; exit 1 }
$libnx = (Resolve-Path @($cands)[0]).Path
Write-Host "libnx32: $libnx"

$img = if ($env:DCR_TOOLCHAIN_IMAGE) { $env:DCR_TOOLCHAIN_IMAGE } else { 'ghcr.io/vita2hos/devcontainer/vita2hos:latest' }
$nxd = '/opt/devkitpro/libnx32'

if (-not (Test-Path "$here\Makefile") -or -not (Test-Path "$here\runtime\runtime.mk")) {
  Write-Host "$here bukan folder port (Makefile atau runtime\runtime.mk tidak ada)" -ForegroundColor Red; exit 1
}
if (-not (Test-Path "$here\portlibs32\lib\libEGL.a")) { Write-Warning "portlibs32 tidak ada: build pakai null renderer" }

$cmd = 'exec make -j$(nproc) ' + ($MakeArgs -join ' ')

docker run --rm --platform linux/amd64 `
  -v "${here}:/work" -w /work `
  -v "${libnx}\include\switch:${nxd}/include/switch:ro" `
  -v "${libnx}\include\switch.h:${nxd}/include/switch.h:ro" `
  -v "${libnx}\lib\libnx.a:${nxd}/lib/libnx.a:ro" `
  -v "${libnx}\lib\libnxd.a:${nxd}/lib/libnxd.a:ro" `
  $img bash -lc $cmd 2>&1 | ForEach-Object { "$_" }
$rc = $LASTEXITCODE
if ($rc -ne 0) { Write-Host "build gagal (exit $rc)" -ForegroundColor Red; exit $rc }
Write-Host "build OK" -ForegroundColor Green
