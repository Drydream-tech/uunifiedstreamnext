; UnifiedStreamNext Windows Installer & Uninstaller (Inno Setup 6)
; Deploys UnifiedStreamNext UI, core runtime, bundled upstream engines, and third-party notices.

[Setup]
AppId={{8E7C3C2A-9B4D-4F18-9A1F-6F2E9B112001}
AppName=UnifiedStreamNext
AppVersion=0.1.0
DefaultDirName={autopf}\UnifiedStreamNext
DefaultGroupName=UnifiedStreamNext
UninstallDisplayIcon={app}\UnifiedStreamNext.exe
OutputDir=..\..\build\packaging
OutputBaseFilename=UnifiedStreamNext-Setup-0.1.0-x64
Compression=lzma2
SolidCompression=yes
ArchitecturesInstallIn64BitMode=x64compatible
PrivilegesRequired=admin

[Files]
Source: "..\..\build\windows\UnifiedStreamNext.exe"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\..\build\windows\engines\sunshine\*"; DestDir: "{app}\engines\sunshine"; Flags: ignoreversion recursesubdirs createallsubdirs skipifsourcedoesntexist
Source: "..\..\build\windows\engines\moonlight\*"; DestDir: "{app}\engines\moonlight"; Flags: ignoreversion recursesubdirs createallsubdirs skipifsourcedoesntexist
Source: "..\..\build\windows\engines\rustdesk\*"; DestDir: "{app}\engines\rustdesk"; Flags: ignoreversion recursesubdirs createallsubdirs skipifsourcedoesntexist
Source: "..\..\LICENSES.md"; DestDir: "{app}\licenses"; Flags: ignoreversion
Source: "..\..\THIRD_PARTY_NOTICES.txt"; DestDir: "{app}\licenses"; Flags: ignoreversion

[Icons]
Name: "{group}\UnifiedStreamNext"; Filename: "{app}\UnifiedStreamNext.exe"
Name: "{autodesktop}\UnifiedStreamNext"; Filename: "{app}\UnifiedStreamNext.exe"

[UninstallDelete]
Type: filesandordirs; Name: "{app}\engines"
Type: filesandordirs; Name: "{app}\licenses"
