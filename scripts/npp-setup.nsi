; N++ compiler installer (NSIS).
; Packages the staged `dist/` prefix produced by `cmake --install`.
;
;   makensis /DNPP_VERSION=1.0.0 /DDIST_DIR=D:\path\to\dist scripts/npp-setup.nsi
;
; DIST_DIR may be relative to the repository root or absolute; CI passes
; an absolute path because NSIS resolves File paths relative to the script.

!include "MUI2.nsh"
!include "LogicLib.nsh"

!ifndef NPP_VERSION
  !define NPP_VERSION "0.0.0"
!endif
!ifndef DIST_DIR
  !define DIST_DIR "dist"
!endif

Name "N++ ${NPP_VERSION}"
; NSIS resolves OutFile relative to this script's directory (scripts/).
; The CI step moves the produced installer to the workspace root.
OutFile "npp-setup-${NPP_VERSION}-windows-x86_64.exe"
InstallDir "$LOCALAPPDATA\Programs\N++"
InstallDirRegKey HKCU "Software\NPP" "InstallDir"
RequestExecutionLevel user
SetCompressor /SOLID lzma

!define MUI_ABORTWARNING
!insertmacro MUI_PAGE_WELCOME
!insertmacro MUI_PAGE_DIRECTORY
!insertmacro MUI_PAGE_INSTFILES
!insertmacro MUI_PAGE_FINISH
!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES
!insertmacro MUI_LANGUAGE "English"

Section "Install"
  SetOutPath "$INSTDIR"
  File /r "${DIST_DIR}\*.*"

  ; Prepend <install>\bin to the user PATH, once.
  ; $R4 tracks whether the registry value changed.
  StrCpy $R4 "0"
  ReadRegStr $R1 HKCU "Environment" "Path"
  ${If} $R1 == ""
    WriteRegStr HKCU "Environment" "Path" "$INSTDIR\bin"
    StrCpy $R4 "1"
  ${Else}
    ; Compare the PATH prefix with our entry to stay idempotent.
    StrLen $R2 "$INSTDIR\bin;"
    StrCpy $R3 $R1 $R2
    ${If} $R3 != "$INSTDIR\bin;"
      WriteRegStr HKCU "Environment" "Path" "$INSTDIR\bin;$R1"
      StrCpy $R4 "1"
    ${EndIf}
  ${EndIf}
  ${If} $R4 == "1"
    SendMessage ${HWND_BROADCAST} ${WM_SETTINGCHANGE} 0 "STR:Environment" /TIMEOUT=5000
  ${EndIf}

  WriteRegStr HKCU "Software\NPP" "InstallDir" "$INSTDIR"
  WriteRegStr HKCU "Software\NPP" "Version" "${NPP_VERSION}"

  ; Associate .npp files: double-click compiles and runs the program via
  ; `npp <file>` (direct file invocation). `run` is registered as an
  ; explicit verb as well for clarity in the shell menu.
  WriteRegStr HKCU "Software\Classes\.npp" "" "NPP.SourceFile"
  WriteRegStr HKCU "Software\Classes\.npp" "Content Type" "text/plain"
  WriteRegStr HKCU "Software\Classes\NPP.SourceFile" "" "N++ source file"
  WriteRegStr HKCU "Software\Classes\NPP.SourceFile" "DefaultIcon" '"$INSTDIR\bin\npp.exe",0'
  WriteRegStr HKCU "Software\Classes\NPP.SourceFile\shell\open\command" "" '"$INSTDIR\bin\npp.exe" "%1"'
  WriteRegStr HKCU "Software\Classes\NPP.SourceFile\shell\run\command" "" '"$INSTDIR\bin\npp.exe" run "%1"'
  WriteRegStr HKCU "Software\Classes\NPP.SourceFile\shell\run" "" "Run with N++"
  WriteRegStr HKCU "Software\Classes\NPP.SourceFile\shell\run" "Icon" '"$INSTDIR\bin\npp.exe",0'

  CreateDirectory "$SMPROGRAMS\N++"
  CreateShortcut "$SMPROGRAMS\N++\N++ Compiler.lnk" "$INSTDIR\bin\npp.exe" "" "$INSTDIR\bin\npp.exe",0
  CreateShortcut "$SMPROGRAMS\N++\Uninstall.lnk" "$INSTDIR\Uninstall.exe"
SectionEnd

Section "Uninstall"
  Delete "$INSTDIR\Uninstall.exe"
  RMDir /r "$INSTDIR"

  ; Remove <install>\bin from the user PATH (prefix, suffix, or exact entry).
  ReadRegStr $R1 HKCU "Environment" "Path"
  StrLen $R2 "$INSTDIR\bin;"
  StrCpy $R3 $R1 $R2
  ${If} $R3 == "$INSTDIR\bin;"
    StrCpy $R4 $R1 "" $R2
    WriteRegStr HKCU "Environment" "Path" "$R4"
    StrCpy $R1 $R4
  ${Else}
    StrLen $R5 ";$INSTDIR\bin"
    ${If} $R1 != ""
      IntOp $R6 $R1 - $R5
      StrCpy $R7 $R1 $R5 $R6
      ${If} $R7 == ";$INSTDIR\bin"
        StrCpy $R4 $R1 $R6
        WriteRegStr HKCU "Environment" "Path" "$R4"
        StrCpy $R1 $R4
      ${EndIf}
    ${EndIf}
  ${EndIf}
  ${If} $R1 == "$INSTDIR\bin"
    DeleteRegValue HKCU "Environment" "Path"
  ${EndIf}
  SendMessage ${HWND_BROADCAST} ${WM_SETTINGCHANGE} 0 "STR:Environment" /TIMEOUT=5000
  Delete "$SMPROGRAMS\N++\N++ Compiler.lnk"
  Delete "$SMPROGRAMS\N++\Uninstall.lnk"
  RMDir "$SMPROGRAMS\N++"
  DeleteRegKey HKCU "Software\NPP"
  DeleteRegKey HKCU "Software\Classes\.npp"
  DeleteRegKey HKCU "Software\Classes\NPP.SourceFile"
SectionEnd
