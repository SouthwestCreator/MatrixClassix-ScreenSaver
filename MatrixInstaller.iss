[Setup]
AppId={{Matrix-Saver-Anthony-Hodge}}
AppName=Matrix Classic Screen Saver
AppVersion=1.0
AppPublisher=Anthony Hodge
DefaultDirName={autopf}\MatrixClassic
DefaultGroupName=Matrix Classic Screen Saver
UninstallDisplayIcon={sys}\MatrixClassix.scr
; Displays your documentation at the end of the setup
InfoAfterFile="C:\Users\hodge\source\repos\MatrixSaverTest2\README.md"
Compression=lzma
SolidCompression=yes
WizardStyle=modern
; This creates the Setup.exe on your Desktop
OutputDir={userdesktop}
OutputBaseFilename=MatrixClassic_Setup

[Files]
; Ensure the source path matches your project folder
Source: "C:\Users\hodge\source\repos\MatrixSaverTest2\x64\Release\MatrixClassix.scr"; DestDir: "{sys}"; Flags: restartreplace sharedfile

[Icons]
; Shortcut in the Start Menu to Windows Screen Saver Settings
Name: "{group}\Configure Screen Saver"; Filename: "{sys}\control.exe"; Parameters: "desk.cpl,,@screensaver"

[Run]
; Automatically opens the settings window after installation is done
Filename: "{sys}\control.exe"; Parameters: "desk.cpl,,@screensaver"; Flags: postinstall nowait