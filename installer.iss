; Inno Setup script for DocBoss.
;
; Build with release.ps1, which finds ISCC from a candidate list
; (C:\bin\InnoSetup6 first) and passes /DMDBossDir.  By hand, from the repo
; root, after a Release build:
;
;   C:\bin\InnoSetup6\ISCC.exe installer.iss      (-> installer\DocBoss-Setup.exe)
;
; The render assets are MD Boss's, shipped from the pinned sibling tree; see
; cmake/Siblings.cmake.

#define AppName "DocBoss"
#define AppVersion "1.0.0"
#define AppExe "DocBoss.exe"
#define BuildDir "build\app\Release"
#ifndef MDBossDir
  #define MDBossDir "..\MDBoss"
#endif

[Setup]
; DocBoss's own, never shared with MD Boss or PDFBoss: the three install side
; by side, each with its own uninstall entry.
AppId={{241554E5-0447-414E-BEA1-F6FCD461051D}
AppName={#AppName}
AppVersion={#AppVersion}
AppPublisher=RabidFox
DefaultDirName={autopf}\{#AppName}
DefaultGroupName={#AppName}
UninstallDisplayIcon={app}\{#AppExe}
OutputDir=installer
OutputBaseFilename=DocBoss-Setup
Compression=lzma2
SolidCompression=yes
; MuPDF is linked in, so the whole program is AGPL-3.0-or-later.
LicenseFile=LICENSE
; Ask per-user or per-machine; per-machine (Program Files) is the default, as
; for every installer in this workspace.  A silent run takes the per-machine
; default, so the in-app updater passes /CURRENTUSER or /ALLUSERS to keep an
; update in the scope it is already installed in (install_scope_flag in MD
; Boss's Updater.cpp, which DocBoss compiles).
PrivilegesRequired=admin
PrivilegesRequiredOverridesAllowed=dialog
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
WizardStyle=modern

[Files]
Source: "{#BuildDir}\{#AppExe}"; DestDir: "{app}"; Flags: ignoreversion
; MD Boss's render assets, less the two that belong to its deprecated Python
; app (see MD Boss's installer-cpp.iss for why they are not shipped).
Source: "{#MDBossDir}\assets\*"; DestDir: "{app}\assets"; \
    Excludes: "template.html,pygments-github.css"; \
    Flags: ignoreversion recursesubdirs createallsubdirs
Source: "HELP.md"; DestDir: "{app}"; Flags: ignoreversion
Source: "LICENSE"; DestDir: "{app}"; Flags: ignoreversion
Source: "README.md"; DestDir: "{app}"; Flags: ignoreversion

[Icons]
Name: "{group}\{#AppName}"; Filename: "{app}\{#AppExe}"
Name: "{group}\Uninstall {#AppName}"; Filename: "{uninstallexe}"
Name: "{autodesktop}\{#AppName}"; Filename: "{app}\{#AppExe}"; \
    Tasks: desktopicon

[Tasks]
Name: "desktopicon"; Description: "Create a &desktop shortcut"; \
    GroupDescription: "Additional icons:"
; Unchecked: DocBoss registers its own ProgID (DocBoss.Markdown) and adds
; itself to "Open with", but someone installing it beside MD Boss may well want
; MD Boss to stay what a double-click opens.  PDFs are never claimed.
Name: "associate"; Description: "&Register DocBoss as a Markdown (.md) handler"; \
    GroupDescription: "File types:"; Flags: unchecked

; The registry layout is FileAssoc.cpp's (compiled from MD Boss, under
; DocBoss's identity), applied by the exe itself.  runasoriginaluser: the keys
; are per-user, and an elevated install would otherwise write the admin's hive.
[Run]
Filename: "{app}\{#AppExe}"; Parameters: "--register-file-types"; \
    StatusMsg: "Registering Markdown file types..."; \
    Flags: runhidden waituntilterminated runasoriginaluser; Tasks: associate
Filename: "{app}\{#AppExe}"; Description: "Launch {#AppName}"; \
    Flags: nowait postinstall skipifsilent runasoriginaluser

[UninstallRun]
Filename: "{app}\{#AppExe}"; Parameters: "--unregister-file-types"; \
    Flags: runhidden waituntilterminated; RunOnceId: "UnregisterFileTypes"
