; N++ compiler installer (NSIS).
; Packages the staged `dist/` prefix produced by `cmake --install`.
;
;   makensis /DNPP_VERSION=1.0.0 scripts/npp-setup.nsi
;
; Run from the repository root so the relative `dist\` path resolves.

!include "MUI2.nsh"
!include "FileFunc.nsh"

!ifndef NPP_VERSION
  !define NPP_VERSION "0.0.0"
!endif
!ifndef DIST_DIR
  !define DIST_DIR "dist"
!endif

Name "N++ ${NPP_VERSION}"
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

  ; Add <install>\bin to the user PATH (idempotent).
  ClearErrors
  ReadRegStr $R1 HKCU "Environment" "Path"
  ${IfNot} ${Errors}
    ${If} $R1 != ""
      StrStr $R2 $R1 "$INSTDIR\bin"
      ${If} $R2 == ""
        StrCpy $R1 "$INSTDIR\bin;$R1"
        WriteRegStr HKCU "Environment" "Path" $R1
        SendMessage ${HWND_BROADCAST} ${WM_SETTINGCHANGE} 0 "STR:Environment" /TIMEOUT=5000
      ${EndIf}
    ${Else}
      WriteRegStr HKCU "Environment" "Path" "$INSTDIR\bin"
      SendMessage ${HWND_BROADCAST} ${WM_SETTINGCHANGE} 0 "STR:Environment" /TIMEOUT=5000
    ${EndIf}
  ${EndIf}

  WriteRegStr HKCU "Software\NPP" "InstallDir" "$INSTDIR"
  WriteRegStr HKCU "Software\NPP" "Version" "${NPP_VERSION}"

  CreateDirectory "$SMPROGRAMS\N++"
  CreateShortcut "$SMPROGRAMS\N++\N++ Compiler.lnk" "$INSTDIR\bin\npp.exe"
  CreateShortcut "$SMPROGRAMS\N++\Uninstall.lnk" "$INSTDIR\uninstall.exe"
SectionEnd

Section "Uninstall"
  Delete "$INSTDIR\uninstall.exe"
  RMDir /r "$INSTDIR"
  Delete "$SMPROGRAMS\N++\N++ Compiler.lnk"
  Delete "$SMPROGRAMS\N++\Uninstall.lnk"
  RMDir "$SMPROGRAMS\N++"
  DeleteRegKey HKCU "Software\NPP"
SectionEnd
