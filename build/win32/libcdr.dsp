# Microsoft Developer Studio Project File - Name="libcdr" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Static Library" 0x0104

CFG=libcdr - Win32 Release
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "libcdr.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "libcdr.mak" CFG="libcdr - Win32 Release"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "libcdr - Win32 Release" (based on "Win32 (x86) Static Library")
!MESSAGE "libcdr - Win32 Debug" (based on "Win32 (x86) Static Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "libcdr - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /I "..\..\inc" /I "librevenge-0.9" /D "WIN32" /D "NDEBUG" /D "_LIB" /D "_CRT_SECURE_NO_WARNINGS" /YX /FD /c
# ADD CPP /nologo /MT /W3 /GR /GX /O2 /I "..\..\inc" /I "librevenge-0.9" /D "NDEBUG" /D "WIN32" /D "_LIB" /D "_CRT_SECURE_NO_WARNINGS" /FD /c
# SUBTRACT CPP /YX
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo /out:"Release\lib\libcdr-0.0.lib"

!ELSEIF  "$(CFG)" == "libcdr - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /I "..\..\inc" /I "librevenge-0.9" /D "WIN32" /D "_DEBUG" /D "DEBUG" /D "_LIB" /D "_CRT_SECURE_NO_WARNINGS" /YX /FD /GZ /c
# ADD CPP /nologo /MTd /W3 /GR /GX /ZI /Od /I "..\..\inc" /I "librevenge-0.9" /D "_DEBUG" /D "DEBUG" /D "WIN32" /D "_LIB" /D "_CRT_SECURE_NO_WARNINGS" /FD /GZ /c
# SUBTRACT CPP /YX
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo /out:"Debug\lib\libcdr-0.0.lib"

!ENDIF 

# Begin Target

# Name "libcdr - Win32 Release"
# Name "libcdr - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=..\..\src\lib\CDRCollector.cpp
# End Source File
# Begin Source File

SOURCE=..\..\src\lib\CDRContentCollector.cpp
# End Source File
# Begin Source File

SOURCE=..\..\src\lib\CDRDocument.cpp
# End Source File
# Begin Source File

SOURCE=..\..\src\lib\CDRInternalStream.cpp
# End Source File
# Begin Source File

SOURCE=..\..\src\lib\CDROutputElementList.cpp
# End Source File
# Begin Source File

SOURCE=..\..\src\lib\CDRParser.cpp
# End Source File
# Begin Source File

SOURCE=..\..\src\lib\CDRPath.cpp
# End Source File
# Begin Source File

SOURCE=..\..\src\lib\librevenge::RVNGStringVector.cpp
# End Source File
# Begin Source File

SOURCE=..\..\src\lib\CDRStylesCollector.cpp
# End Source File
# Begin Source File

SOURCE=..\..\src\lib\CDRSVGGenerator.cpp
# End Source File
# Begin Source File

SOURCE=..\..\src\lib\CDRTypes.cpp
# End Source File
# Begin Source File

SOURCE=..\..\src\lib\CDRTransforms.cpp
# End Source File
# Begin Source File

SOURCE=..\..\src\lib\CDRZipStream.cpp
# End Source File
# Begin Source File

SOURCE=..\..\src\lib\CMXDocument.cpp
# End Source File
# Begin Source File

SOURCE=..\..\src\lib\CMXParser.cpp
# End Source File
# Begin Source File

SOURCE=..\..\src\lib\CommonParser.cpp
# End Source File
# Begin Source File

SOURCE=..\..\src\lib\libcdr_utils.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=..\..\inc\libcdr\CDRDocument.h
# End Source File
# Begin Source File

SOURCE=..\..\inc\libcdr\librevenge::RVNGStringVector.h
# End Source File
# Begin Source File

SOURCE=..\..\inc\libcdr\CMXDocument.h
# End Source File
# Begin Source File

SOURCE=..\..\inc\libcdr\libcdr.h
# End Source File
# Begin Source File

SOURCE=..\..\src\lib\CDRCollector.h
# End Source File
# Begin Source File

SOURCE=..\..\src\lib\CDRColorPalettes.h
# End Source File
# Begin Source File

SOURCE=..\..\src\lib\CDRColorProfiles.h
# End Source File
# Begin Source File

SOURCE=..\..\src\lib\CDRContentCollector.h
# End Source File
# Begin Source File

SOURCE=..\..\src\lib\CDRDocumentStructure.h
# End Source File
# Begin Source File

SOURCE=..\..\src\lib\CDRInternalStream.h
# End Source File
# Begin Source File

SOURCE=..\..\src\lib\CDROutputElementList.h
# End Source File
# Begin Source File

SOURCE=..\..\src\lib\CDRParser.h
# End Source File
# Begin Source File

SOURCE=..\..\src\lib\CDRPath.h
# End Source File
# Begin Source File

SOURCE=..\..\src\lib\CDRStylesCollector.h
# End Source File
# Begin Source File

SOURCE=..\..\src\lib\CDRSVGGenerator.h
# End Source File
# Begin Source File

SOURCE=..\..\src\lib\CDRTypes.h
# End Source File
# Begin Source File

SOURCE=..\..\src\lib\CDRTransforms.h
# End Source File
# Begin Source File

SOURCE=..\..\src\lib\CDRZipStream.h
# End Source File
# Begin Source File

SOURCE=..\..\src\lib\CMXDocumentStructure.h
# End Source File
# Begin Source File

SOURCE=..\..\src\lib\CMXParser.h
# End Source File
# Begin Source File

SOURCE=..\..\src\lib\CommonParser.h
# End Source File
# Begin Source File

SOURCE=..\..\src\lib\libcdr_utils.h
# End Source File
# End Group
# End Target
# End Project
