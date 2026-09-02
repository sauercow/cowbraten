Name "cowbraten"

!ifndef VERSION
  !define VERSION "git-dev"
!endif

OutFile "..\..\cowbraten_setup_${VERSION}.exe"

InstallDir $PROGRAMFILES\Sauerbraten

InstallDirRegKey HKLM "Software\cowbraten" "Install_Dir"

SetCompressor /SOLID lzma

Page components
DirText "Choose your Sauerbraten installation directory. cowbraten will be installed alongside the original files."
Page directory
Page instfiles

UninstPage uninstConfirm
UninstPage instfiles

Section "cowbraten (required)"

  SectionIn RO

  SetOutPath $INSTDIR

  IfFileExists "$INSTDIR\sauerbraten.bat" VanillaFound VanillaNotFound
  VanillaNotFound:
    Abort "Sauerbraten files not found in specified directory!"
  VanillaFound:

  SetOutPath $INSTDIR\bin64
  File "..\..\bin64\cowbraten_x86_64_client.exe"
  SetOutPath $INSTDIR
  File "..\..\cowbraten.bat"

  WriteRegStr HKLM SOFTWARE\cowbraten "Install_Dir" "$INSTDIR"

  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\cowbraten" "DisplayName" "cowbraten"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\cowbraten" "UninstallString" '"$INSTDIR\uninstall_cowbraten.exe"'
  WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\cowbraten" "NoModify" 1
  WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\cowbraten" "NoRepair" 1
  WriteUninstaller "uninstall_cowbraten.exe"

SectionEnd

Section "Start Menu Shortcuts"

  CreateDirectory "$SMPROGRAMS\cowbraten"

  SetOutPath "$INSTDIR"

  CreateShortCut "$INSTDIR\cowbraten.lnk"                "$INSTDIR\cowbraten.bat" "" "$INSTDIR\bin64\cowbraten_x86_64_client.exe" 0 SW_SHOWMINIMIZED
  CreateShortCut "$SMPROGRAMS\cowbraten\cowbraten.lnk"   "$INSTDIR\cowbraten.bat" "" "$INSTDIR\bin64\cowbraten_x86_64_client.exe" 0 SW_SHOWMINIMIZED
  CreateShortCut "$SMPROGRAMS\cowbraten\Uninstall.lnk"   "$INSTDIR\uninstall_cowbraten.exe" "" "$INSTDIR\uninstall_cowbraten.exe" 0

SectionEnd

Section "Uninstall"

  DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\cowbraten"
  DeleteRegKey HKLM SOFTWARE\cowbraten

  RMDir /r "$SMPROGRAMS\cowbraten"
  Delete "$INSTDIR\cowbraten.lnk"
  Delete "$INSTDIR\cowbraten.bat"
  Delete "$INSTDIR\bin64\cowbraten_x86_64_client.exe"
  Delete "$INSTDIR\uninstall_cowbraten.exe"

SectionEnd
