# Microsoft Developer Studio Project File - Name="cdr2raw" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Console Application" 0x0103

CFG=cdr2raw - Win32 Release
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "cdr2raw.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "cdr2raw.mak" CFG="cdr2raw - Win32 Release"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "cdr2raw - Win32 Release" (based on "Win32 (x86) Console Application")
!MESSAGE "cdr2raw - Win32 Debug" (based on "Win32 (x86) Console Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "cdr2raw - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
MTL=midl.exe
# ADD BASE MTL /nologo /win32
# ADD MTL /nologo /win32
# ADD BASE CPP /nologo /MT /W3 /GX /Zi /I "..\..\inc" /I "librevenge-0.9" /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /c
# ADD CPP /nologo /MT /W3 /GX /I "..\..\inc" /I "librevenge-0.9" /D "NDEBUG" /D "WIN32" /D "_CONSOLE" /c
# ADD BASE RSC /l 0x409
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 libcdr-0.0.lib librevenge-stream-0.9.lib librevenge-0.9.lib /nologo /subsystem:console /machine:IX86 /out:"Release\bin\cdr2raw.exe" /libpath:"Release\lib"
# ADD LINK32 libcdr-0.0.lib librevenge-stream-0.9.lib librevenge-0.9.lib /nologo /subsystem:console /machine:IX86 /out:"Release\bin\cdr2raw.exe" /libpath:"Release\lib"
# SUBTRACT LINK32 /nodefaultlib

!ELSEIF  "$(CFG)" == "cdr2raw - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
MTL=midl.exe
# ADD BASE MTL /nologo /win32
# ADD MTL /nologo /win32
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /I "\..\..\inc" /I "librevenge-0.9" /D "WIN32" /D "_DEBUG" /D "DEBUG" /D "_CONSOLE" /GZ /c
# ADD CPP /nologo /MTd /W3 /GX /ZI /Od /I "..\..\inc" /I "librevenge-0.9" /D "_DEBUG" /D "DEBUG" /D "WIN32" /D "_CONSOLE" /GZ /c
# ADD BASE RSC /l 0x409
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 libcdr-0.0.lib librevenge-stream-0.9.lib librevenge-0.9.lib /nologo /subsystem:console /debug /machine:IX86 /out:"Debug\bin\cdr2raw.exe" /libpath:"Debug\lib"
# ADD LINK32 libcdr-0.0.lib librevenge-stream-0.9.lib librevenge-0.9.lib kernel32.lib /nologo /subsystem:console /debug /machine:IX86 /out:"Debug\bin\cdr2raw.exe" /libpath:"Debug\lib"
# SUBTRACT LINK32 /nodefaultlib

!ENDIF 

# Begin Target

# Name "cdr2raw - Win32 Release"
# Name "cdr2raw - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cc;cxx;def;odl;idl;hpj;bat;asm;asmx"
# Begin Source File

SOURCE=..\..\src\conv\raw\cdr2raw.cpp
# End Source File
# End Group
# End Target
# End Project
