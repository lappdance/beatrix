!include "mui.nsh"

Name "Beatrix"
InstallDir $PROGRAMFILES\Beatrix
XpStyle on


# $OUTFILE param is set by makefile


# toolbars need to be written into HKLM, so we need to be an admin
RequestExecutionLevel admin

!define MUI_ABORTWARNING

!insertmacro MUI_PAGE_WELCOME
#!insertmacro MUI_PAGE_COMPONENTS
!insertmacro MUI_PAGE_DIRECTORY
!insertmacro MUI_PAGE_INSTFILES
!insertmacro MUI_PAGE_FINISH

!insertmacro MUI_UNPAGE_WELCOME
!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES
!insertmacro MUI_UNPAGE_FINISH


!insertmacro MUI_LANGUAGE "English"


Section "Beatrix Toolbar(Required)" SecToolbar

  # Set output path to the installation directory.
  SetOutPath "$INSTDIR"
  
  # places files into out directory
  # file paths here are given relative to this .nsi file
  File "..\..\bin\Beatrix.dll"
  
  # register toolbar
  # I have no interest in duplicating the com registration logic in the installer,
  # so have regsvr32 do all the work.
  RegDll "$INSTDIR\Beatrix.dll"
  
  # add keys to registry so user can uninstall from control panel
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Beatrix" "DisplayName" "Beatrix Toolbar"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Beatrix" "UninstallString" '"$INSTDIR\Uninstall.exe"'
  WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Beatrix" "NoModify" 1
  WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Beatrix" "NoRepair" 1
  WriteUninstaller "Uninstall.exe"
  
SectionEnd


Section "Uninstall"
  
  # Remove registry keys
  DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Beatrix"
  DeleteRegKey HKLM "SOFTWARE\Somnia\Beatrix"
  
  # unregister toolbar
  UnRegDll "$INSTDIR\Beatrix.dll"

  # Remove files and uninstaller
  Delete "$INSTDIR\Beatrix.dll"
  Delete "$INSTDIR\Uninstall.exe"

  # TODO: only remove directory if empty
  RMDir "$INSTDIR"

SectionEnd

LangString DESC_SecToolbar ${LANG_ENGLISH} "The breadcrumb toolbar."

;Assign language strings to sections
!insertmacro MUI_FUNCTION_DESCRIPTION_BEGIN
 !insertmacro MUI_DESCRIPTION_TEXT ${SecToolbar} $(DESC_SecToolbar)
!insertmacro MUI_FUNCTION_DESCRIPTION_END
