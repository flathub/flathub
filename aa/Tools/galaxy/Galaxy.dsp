# Microsoft Developer Studio Project File - Name="Galaxy" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Application" 0x0101

CFG=Galaxy - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "Galaxy.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "Galaxy.mak" CFG="Galaxy - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "Galaxy - Win32 Release" (based on "Win32 (x86) Application")
!MESSAGE "Galaxy - Win32 Debug" (based on "Win32 (x86) Application")
!MESSAGE "Galaxy - Win32 InHouse" (based on "Win32 (x86) Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "Galaxy - Win32 Release"

# PROP BASE Use_MFC 5
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 5
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /Yu"stdafx.h" /FD /c
# ADD CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /FR /Yu"stdafx.h" /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 /nologo /subsystem:windows /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib wsock32.lib comctl32.lib Winmm.lib fce32.lib /nologo /subsystem:windows /machine:I386 /out:"../../Galaxy.exe"
# SUBTRACT LINK32 /incremental:yes

!ELSEIF  "$(CFG)" == "Galaxy - Win32 Debug"

# PROP BASE Use_MFC 5
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 5
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /Yu"stdafx.h" /FD /GZ /c
# ADD CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /Yu"stdafx.h" /FD /GZ /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 /nologo /subsystem:windows /debug /machine:I386 /pdbtype:sept
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib wsock32.lib comctl32.lib Winmm.lib fce32.lib /nologo /subsystem:windows /debug /machine:I386 /pdbtype:sept

!ELSEIF  "$(CFG)" == "Galaxy - Win32 InHouse"

# PROP BASE Use_MFC 5
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Galaxy___Win32_InHouse"
# PROP BASE Intermediate_Dir "Galaxy___Win32_InHouse"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 5
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Galaxy___Win32_InHouse"
# PROP Intermediate_Dir "Galaxy___Win32_InHouse"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /Yu"stdafx.h" /FD /c
# ADD CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "INHOUSE" /Yu"stdafx.h" /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib wsock32.lib comctl32.lib Winmm.lib /nologo /subsystem:windows /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib wsock32.lib comctl32.lib Winmm.lib fce32.lib /nologo /subsystem:windows /machine:I386 /out:"c:/Alien Arena Files/galaxy.exe"

!ENDIF 

# Begin Target

# Name "Galaxy - Win32 Release"
# Name "Galaxy - Win32 Debug"
# Name "Galaxy - Win32 InHouse"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\BuddyName.cpp
# End Source File
# Begin Source File

SOURCE=.\Galaxy.cpp
# End Source File
# Begin Source File

SOURCE=.\Galaxy.rc
# End Source File
# Begin Source File

SOURCE=.\GalaxyDlg.cpp
# End Source File
# Begin Source File

SOURCE=.\Help.cpp
# End Source File
# Begin Source File

SOURCE=.\hyperlink.cpp
# End Source File
# Begin Source File

SOURCE=.\mfcpp.cpp
# End Source File
# Begin Source File

SOURCE=.\minmaxlogic.cpp
# End Source File
# Begin Source File

SOURCE=.\PlayerProfile.cpp
# End Source File
# Begin Source File

SOURCE=.\PollServer.cpp
# End Source File
# Begin Source File

SOURCE=.\SkinHeaderCtrl.cpp
# End Source File
# Begin Source File

SOURCE=.\SkinHorizontalScrollbar.cpp
# End Source File
# Begin Source File

SOURCE=.\SkinListCtrl.cpp
# End Source File
# Begin Source File

SOURCE=.\SkinVerticleScrollbar.cpp
# End Source File
# Begin Source File

SOURCE=.\StdAfx.cpp
# ADD CPP /Yc"stdafx.h"
# End Source File
# Begin Source File

SOURCE=.\UpdateDlg.cpp
# End Source File
# Begin Source File

SOURCE=.\UpdateGame.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\BuddyName.h
# End Source File
# Begin Source File

SOURCE=.\Functions.h
# End Source File
# Begin Source File

SOURCE=.\Galaxy.h
# End Source File
# Begin Source File

SOURCE=.\GalaxyDlg.h
# End Source File
# Begin Source File

SOURCE=.\Help.h
# End Source File
# Begin Source File

SOURCE=.\hyperlink.h
# End Source File
# Begin Source File

SOURCE=.\PlayerProfile.h
# End Source File
# Begin Source File

SOURCE=.\PollServer.h
# End Source File
# Begin Source File

SOURCE=.\Resource.h
# End Source File
# Begin Source File

SOURCE=.\SkinHeaderCtrl.h
# End Source File
# Begin Source File

SOURCE=.\SkinHorizontalScrollbar.h
# End Source File
# Begin Source File

SOURCE=.\SkinListCtrl.h
# End Source File
# Begin Source File

SOURCE=.\SkinVerticleScrollbar.h
# End Source File
# Begin Source File

SOURCE=.\Socket.h
# End Source File
# Begin Source File

SOURCE=.\StdAfx.h
# End Source File
# Begin Source File

SOURCE=.\UpdateDlg.h
# End Source File
# Begin Source File

SOURCE=.\userinfo.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# Begin Source File

SOURCE=.\res\bitmap1.bmp
# End Source File
# Begin Source File

SOURCE=.\res\bitmap10.bmp
# End Source File
# Begin Source File

SOURCE=.\res\bitmap11.bmp
# End Source File
# Begin Source File

SOURCE=.\res\bitmap12.bmp
# End Source File
# Begin Source File

SOURCE=.\res\bitmap13.bmp
# End Source File
# Begin Source File

SOURCE=.\res\bitmap14.bmp
# End Source File
# Begin Source File

SOURCE=.\res\bitmap15.bmp
# End Source File
# Begin Source File

SOURCE=.\res\bitmap16.bmp
# End Source File
# Begin Source File

SOURCE=.\res\bitmap17.bmp
# End Source File
# Begin Source File

SOURCE=.\res\bitmap18.bmp
# End Source File
# Begin Source File

SOURCE=.\res\bitmap19.bmp
# End Source File
# Begin Source File

SOURCE=.\res\bitmap2.bmp
# End Source File
# Begin Source File

SOURCE=.\res\bitmap20.bmp
# End Source File
# Begin Source File

SOURCE=.\res\bitmap21.bmp
# End Source File
# Begin Source File

SOURCE=.\res\bitmap22.bmp
# End Source File
# Begin Source File

SOURCE=.\res\bitmap23.bmp
# End Source File
# Begin Source File

SOURCE=.\res\bitmap24.bmp
# End Source File
# Begin Source File

SOURCE=.\res\bitmap25.bmp
# End Source File
# Begin Source File

SOURCE=.\res\bitmap26.bmp
# End Source File
# Begin Source File

SOURCE=.\res\bitmap27.bmp
# End Source File
# Begin Source File

SOURCE=.\res\bitmap28.bmp
# End Source File
# Begin Source File

SOURCE=.\res\bitmap29.bmp
# End Source File
# Begin Source File

SOURCE=.\res\bitmap3.bmp
# End Source File
# Begin Source File

SOURCE=.\res\bitmap4.bmp
# End Source File
# Begin Source File

SOURCE=.\res\bitmap5.bmp
# End Source File
# Begin Source File

SOURCE=.\res\bitmap6.bmp
# End Source File
# Begin Source File

SOURCE=.\res\bitmap7.bmp
# End Source File
# Begin Source File

SOURCE=.\res\bitmap8.bmp
# End Source File
# Begin Source File

SOURCE=.\res\bitmap9.bmp
# End Source File
# Begin Source File

SOURCE=.\res\ColumnHeaderEnd.bmp
# End Source File
# Begin Source File

SOURCE=.\res\ColumnHeaderSpan.bmp
# End Source File
# Begin Source File

SOURCE=.\res\ColumnHeaderStart.bmp
# End Source File
# Begin Source File

SOURCE=.\res\computer.ico
# End Source File
# Begin Source File

SOURCE=.\res\Galaxy.ico
# End Source File
# Begin Source File

SOURCE=.\res\Galaxy.rc2
# End Source File
# Begin Source File

SOURCE=.\res\HorizontalScrollBarLeftArrow.bmp
# End Source File
# Begin Source File

SOURCE=.\res\HorizontalScrollBarRightArrow.bmp
# End Source File
# Begin Source File

SOURCE=.\res\HorizontalScrollBarSpan.bmp
# End Source File
# Begin Source File

SOURCE=.\res\icon1.ico
# End Source File
# Begin Source File

SOURCE=.\res\icon10.ico
# End Source File
# Begin Source File

SOURCE=.\res\icon11.ico
# End Source File
# Begin Source File

SOURCE=.\res\icon12.ico
# End Source File
# Begin Source File

SOURCE=.\res\icon13.ico
# End Source File
# Begin Source File

SOURCE=.\res\icon2.ico
# End Source File
# Begin Source File

SOURCE=.\res\icon3.ico
# End Source File
# Begin Source File

SOURCE=.\res\icon5.ico
# End Source File
# Begin Source File

SOURCE=.\res\icon6.ico
# End Source File
# Begin Source File

SOURCE=.\res\icon7.ico
# End Source File
# Begin Source File

SOURCE=.\res\logo_big_02.bmp
# End Source File
# Begin Source File

SOURCE=.\res\VerticleScrollbarBottom.bmp
# End Source File
# Begin Source File

SOURCE=.\res\VerticleScrollBarDownArrow.bmp
# End Source File
# Begin Source File

SOURCE=.\res\VerticleScrollBarSpan.bmp
# End Source File
# Begin Source File

SOURCE=.\res\VerticleScrollBarThumb.bmp
# End Source File
# Begin Source File

SOURCE=.\res\VerticleScrollbarTop.bmp
# End Source File
# Begin Source File

SOURCE=.\res\VerticleScrollBarUpArrow.bmp
# End Source File
# End Group
# Begin Source File

SOURCE=.\fce32.dll
# End Source File
# Begin Source File

SOURCE=.\ReadMe.txt
# End Source File
# End Target
# End Project
