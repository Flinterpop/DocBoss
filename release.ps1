<#
Release DocBoss: bump the version everywhere it appears, build and test,
build the installer and portable zip, commit and push the bump, and publish a
GitHub release with both assets.

The version lives in four files and is bumped in lockstep: CMakeLists.txt
(project VERSION), app/Version.h, installer.iss, and BOTH the string and the
numeric forms in app/DocBoss.rc.  tests/test_version.cpp fails the build when
they disagree, which is why ctest runs AFTER the bump.

DocBoss compiles MD Boss and PDFBoss by SOURCE PULL (cmake/Siblings.cmake), so
what ships is those two trees as they stand on disk.  A release is refused
unless each sibling is exactly at the commit this repo pins and has no
uncommitted change to a tracked file: otherwise the published exe would
contain code no commit anywhere records.

The asset names are load-bearing -- the in-app updater matches them exactly
(Identity.cpp) -- so they are DocBoss-Setup.exe and DocBoss-Portable.zip, and
never either sibling's.

Usage:
  .\release.ps1 1.0.0
  .\release.ps1 1.0.0 -NotesFile notes.md
  .\release.ps1 1.0.0 -Notes "- fixed X"
  .\release.ps1 1.0.0 -Install            # also install the new build here

Requires: CMake + VS 18 + vcpkg (classic, x64-windows-static), the built MuPDF
tree at C:\source\mupdf, Inno Setup 6, gh (authenticated), git.  Windows
PowerShell 5.1 compatible.  Nothing is written under %TEMP%: staging happens
under installer\, which is git-ignored.
#>
param(
    [Parameter(Mandatory = $true, Position = 0)]
    [ValidatePattern('^\d+\.\d+\.\d+$')]
    [string]$Version,

    [string]$Notes = "",
    [string]$NotesFile = "",
    [switch]$Install
)

$ErrorActionPreference = "Stop"
Set-Location $PSScriptRoot
# Both, and the second is not redundant: Set-Location moves PowerShell's own
# location, not .NET's working directory, and [IO.File] below resolves
# relative paths against the latter (MD Boss's release script learned this).
[Environment]::CurrentDirectory = $PSScriptRoot

function Fail($msg) { Write-Host "ERROR: $msg" -ForegroundColor Red; exit 1 }
function CheckExit($what) {
    if ($LASTEXITCODE -ne 0) { Fail "$what failed (exit $LASTEXITCODE)" }
}

# --- Preflight ---------------------------------------------------------------
# ISCC from a candidate list, never from %LOCALAPPDATA% (the workspace forbids
# AppData), and a failure names every path tried.
$isccCandidates = @(
    "C:\bin\InnoSetup6\ISCC.exe",
    "$env:ProgramFiles\Inno Setup 6\ISCC.exe",
    "${env:ProgramFiles(x86)}\Inno Setup 6\ISCC.exe"
)
$iscc = $isccCandidates | Where-Object { Test-Path $_ } | Select-Object -First 1
if (-not $iscc) { Fail ("ISCC.exe (Inno Setup 6) not found. Tried:`n  " + ($isccCandidates -join "`n  ")) }
foreach ($tool in "git", "gh", "cmake", "ctest") {
    if (-not (Get-Command $tool -ErrorAction SilentlyContinue)) { Fail "$tool not on PATH" }
}
if ($NotesFile -and -not (Test-Path $NotesFile)) { Fail "notes file not found: $NotesFile" }
$origin = git remote get-url origin 2>$null
if (-not $origin) { Fail "no 'origin' remote -- create github.com/Flinterpop/DocBoss and add it first" }

$dirty = git status --porcelain
if ($dirty) { Fail "working tree not clean -- commit or stash first:`n$dirty" }

# The siblings must be exactly what this repo pins.
$siblings = [IO.File]::ReadAllText("cmake\Siblings.cmake")
foreach ($name in "MDBOSS", "PDFBOSS") {
    $dir = [regex]::Match($siblings, "set\(${name}_DIR `"([^`"]+)`"").Groups[1].Value
    $pin = [regex]::Match($siblings, "set\(${name}_EXPECTED_COMMIT `"([0-9a-f]{40})`"\)").Groups[1].Value
    if (-not $dir -or -not $pin) { Fail "could not read ${name}_DIR / ${name}_EXPECTED_COMMIT from cmake\Siblings.cmake" }
    $head = git -C $dir rev-parse HEAD
    CheckExit "git -C $dir rev-parse"
    if ($head -ne $pin) { Fail "$dir is at $head but DocBoss pins $pin -- bump the pin (and diff the pulled files) or check out the pinned commit" }
    $changed = git -C $dir status --porcelain --untracked-files=no
    if ($changed) { Fail "$dir has uncommitted changes to tracked files; the release would ship code no commit records:`n$changed" }
    Write-Host "==> $dir at pinned $($pin.Substring(0, 8)), clean" -ForegroundColor Cyan
}
$mdbossDir = [regex]::Match($siblings, 'set\(MDBOSS_DIR "([^"]+)"').Groups[1].Value

# --- Bump versions -----------------------------------------------------------
Write-Host "==> Bumping version to $Version" -ForegroundColor Cyan
$parts = $Version.Split(".")
$tuple = "$($parts[0]),$($parts[1]),$($parts[2]),0"
# Each entry must already match, so a renamed constant fails here instead of
# silently leaving one file behind.
$bumps = @(
    @{ File = "CMakeLists.txt";   Match = 'project\(DocBoss VERSION [0-9.]+';       New = "project(DocBoss VERSION $Version" },
    @{ File = "app\Version.h";    Match = 'kAppVersion = "[^"]+"';                  New = "kAppVersion = `"$Version`"" },
    @{ File = "installer.iss";    Match = '#define AppVersion "[^"]+"';             New = "#define AppVersion `"$Version`"" },
    @{ File = "app\DocBoss.rc";   Match = 'FILEVERSION\s+\d+,\d+,\d+,\d+';         New = "FILEVERSION    $tuple" },
    @{ File = "app\DocBoss.rc";   Match = 'PRODUCTVERSION\s+\d+,\d+,\d+,\d+';      New = "PRODUCTVERSION $tuple" },
    @{ File = "app\DocBoss.rc";   Match = 'VALUE "FileVersion",\s+"[^"]+"';        New = "VALUE `"FileVersion`",      `"$Version`"" },
    @{ File = "app\DocBoss.rc";   Match = 'VALUE "ProductVersion",\s+"[^"]+"';     New = "VALUE `"ProductVersion`",   `"$Version`"" }
)
# UTF-8 without a BOM, both directions: ReadAllText/WriteAllText with an
# explicit encoding, never Get-Content|Set-Content.
$utf8 = New-Object System.Text.UTF8Encoding($false)
foreach ($bump in $bumps) {
    $path = Join-Path $PSScriptRoot $bump.File
    $text = [IO.File]::ReadAllText($path, $utf8)
    if ($text -notmatch $bump.Match) { Fail "pattern not found in $($bump.File): $($bump.Match)" }
    [IO.File]::WriteAllText($path, ($text -replace $bump.Match, $bump.New), $utf8)
}

# --- Build and test ----------------------------------------------------------
try { Stop-Process -Name DocBoss -Force -Confirm:$false -ErrorAction Stop } catch {}

Write-Host "==> Configuring" -ForegroundColor Cyan
cmake --preset windows-static
CheckExit "cmake configure"
Write-Host "==> Building" -ForegroundColor Cyan
cmake --build build --config Release
CheckExit "cmake build"
# The tests write throwaway files under the temp folder; point it inside the
# ignored build tree for the run rather than at %TEMP%.
$scratch = Join-Path $PSScriptRoot "build\claude-scratch\tmp"
$null = New-Item -ItemType Directory -Force $scratch
$savedTmp = $env:TMP; $savedTemp = $env:TEMP
$env:TMP = $scratch; $env:TEMP = $scratch
try {
    Write-Host "==> Tests (ctest)" -ForegroundColor Cyan
    ctest --test-dir build -C Release --output-on-failure
    CheckExit "ctest"
} finally { $env:TMP = $savedTmp; $env:TEMP = $savedTemp }

# --- Package -----------------------------------------------------------------
Write-Host "==> Installer (ISCC)" -ForegroundColor Cyan
& $iscc "/DMDBossDir=$mdbossDir" installer.iss
CheckExit "ISCC"

# The portable zip mirrors installer.iss's [Files]: the exe, MD Boss's render
# assets (less the two that belong to its deprecated Python app), HELP.md,
# LICENSE and README.md, under a "DocBoss\" root folder -- the portable
# updater accepts the exe at the zip's root or one folder down.
Write-Host "==> Portable zip" -ForegroundColor Cyan
$stage = "installer\portable-stage"
if (Test-Path $stage) { Remove-Item -Recurse -Force $stage }
$null = New-Item -ItemType Directory -Force "$stage\DocBoss"
Copy-Item build\app\Release\DocBoss.exe "$stage\DocBoss\"
Copy-Item -Recurse (Join-Path $mdbossDir "assets") "$stage\DocBoss\assets"
Remove-Item -Force -ErrorAction SilentlyContinue `
    "$stage\DocBoss\assets\template.html", "$stage\DocBoss\assets\pygments-github.css"
Copy-Item HELP.md, README.md, LICENSE "$stage\DocBoss\"
Compress-Archive -Force -Path "$stage\DocBoss" -DestinationPath installer\DocBoss-Portable.zip
Remove-Item -Recurse -Force $stage

$assets = @("installer\DocBoss-Setup.exe", "installer\DocBoss-Portable.zip")
foreach ($asset in $assets) {
    if (-not (Test-Path $asset)) { Fail "expected artifact missing: $asset" }
}

# --- Commit + push -----------------------------------------------------------
git add CMakeLists.txt app\Version.h installer.iss app\DocBoss.rc
$staged = git diff --cached --name-only
if ($staged) {
    git commit -m "Bump version to $Version"
    CheckExit "git commit"
} else {
    Write-Host "==> Versions already at $Version, nothing to commit" -ForegroundColor Yellow
}
Write-Host "==> Syncing with origin" -ForegroundColor Cyan
git pull --rebase origin main
CheckExit "git pull --rebase"
git push origin main
CheckExit "git push"

# --- Publish -----------------------------------------------------------------
Write-Host "==> Publishing GitHub release v$Version" -ForegroundColor Cyan
$ghArgs = @("release", "create", "v$Version") + $assets + @("--title", "v$Version")
if ($NotesFile)  { $ghArgs += @("--notes-file", $NotesFile) }
elseif ($Notes)  { $ghArgs += @("--notes", $Notes) }
else             { $ghArgs += "--generate-notes" }
& gh @ghArgs
CheckExit "gh release create"

# --- Optional local install --------------------------------------------------
# Opt-in.  A silent run keeps an existing install's scope; a first install
# takes the per-machine default and needs one UAC prompt.
if ($Install) {
    $setup = Start-Process (Join-Path $PSScriptRoot "installer\DocBoss-Setup.exe") `
        -ArgumentList "/VERYSILENT", "/NORESTART", "/SUPPRESSMSGBOXES" -Wait -PassThru
    if ($setup.ExitCode -ne 0) {
        Write-Host "    installer exited with $($setup.ExitCode) (declined UAC?); the release is published regardless" -ForegroundColor Yellow
    } elseif (Test-Path "$env:ProgramFiles\DocBoss\DocBoss.exe") {
        Start-Process "$env:ProgramFiles\DocBoss\DocBoss.exe"
    }
}

Write-Host "==> Done: https://github.com/Flinterpop/DocBoss/releases/tag/v$Version" -ForegroundColor Green
