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

  CreateDirectory "$SMPROGRAMS\N++"
  CreateShortcut "$SMPROGRAMS\N++\N++ Compiler.lnk" "$INSTDIR\bin\npp.exe"
  CreateShortcut "$SMPROGRAMS\N++\Uninstall.lnk" "$INSTDIR\Uninstall.exe"
SectionEnd

Section "Uninstall"
  Delete "$INSTDIR\Uninstall.exe"
  RMDir /r "$INSTDIR"
  Delete "$SMPROGRAMS\N++\N++ Compiler.lnk"
  Delete "$SMPROGRAMS\N++\Uninstall.lnk"
  RMDir "$SMPROGRAMS\N++"
  DeleteRegKey HKCU "Software\NPP"
SectionEnd
