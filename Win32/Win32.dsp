# Microsoft Developer Studio Project File - Name="Win32" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Console Application" 0x0103

CFG=Win32 - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "Win32.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "Win32.mak" CFG="Win32 - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "Win32 - Win32 Release" (based on "Win32 (x86) Console Application")
!MESSAGE "Win32 - Win32 Debug" (based on "Win32 (x86) Console Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "Win32 - Win32 Release"

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
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_MBCS" /YX /FD /c
# ADD CPP /nologo /MT /W3 /GR /GX /O2 /I ".." /I "../../Include" /I "../../stl/inc" /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_MBCS" /FD /c
# SUBTRACT CPP /YX
# ADD BASE RSC /l 0x419 /d "NDEBUG"
# ADD RSC /l 0x419 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib /nologo /entry:"main" /subsystem:console /machine:I386 /out:"Release/nirvana.exe"
# SUBTRACT LINK32 /nodefaultlib

!ELSEIF  "$(CFG)" == "Win32 - Win32 Debug"

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
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "_MBCS" /YX /FD /GZ /c
# ADD CPP /nologo /MTd /W3 /Gm /GR /GX /ZI /Od /I ".." /I "../../Include" /I "../../stl/inc" /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "_MBCS" /FD /GZ /c
# SUBTRACT CPP /YX
# ADD BASE RSC /l 0x419 /d "_DEBUG"
# ADD RSC /l 0x419 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /debug /machine:I386 /pdbtype:sept
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib /nologo /entry:"main" /subsystem:console /map /debug /machine:I386 /out:"Debug/nirvana.exe" /pdbtype:sept
# SUBTRACT LINK32 /nodefaultlib

!ENDIF 

# Begin Target

# Name "Win32 - Win32 Release"
# Name "Win32 - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=..\config.h
# End Source File
# Begin Source File

SOURCE=..\core.h
# End Source File
# Begin Source File

SOURCE=..\ExecDomain.h
# End Source File
# Begin Source File

SOURCE=..\Heap.cpp
# End Source File
# Begin Source File

SOURCE=..\Heap.h
# End Source File
# Begin Source File

SOURCE=.\List.cpp
# End Source File
# Begin Source File

SOURCE=..\List.h
# End Source File
# Begin Source File

SOURCE=..\messages.cpp
# End Source File
# Begin Source File

SOURCE=..\messages.h
# End Source File
# Begin Source File

SOURCE=..\PriorityQueue.cpp
# End Source File
# Begin Source File

SOURCE=..\PriorityQueue.h
# End Source File
# Begin Source File

SOURCE=..\ProtDomain.cpp
# End Source File
# Begin Source File

SOURCE=..\ProtDomain.h
# End Source File
# Begin Source File

SOURCE=..\SyncDomain.cpp
# End Source File
# Begin Source File

SOURCE=..\SyncDomain.h
# End Source File
# Begin Source File

SOURCE=..\SyncDomainMemory.cpp
# End Source File
# Begin Source File

SOURCE=..\SyncDomainMemory.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# End Group
# Begin Group "Win32"

# PROP Default_Filter ""
# Begin Group "Memory"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\allocate.cpp
# End Source File
# Begin Source File

SOURCE=.\check_allocated.cpp
# End Source File
# Begin Source File

SOURCE=.\commit.cpp
# End Source File
# Begin Source File

SOURCE=.\copy.cpp
# End Source File
# Begin Source File

SOURCE=.\CostOfOperation.cpp
# End Source File
# Begin Source File

SOURCE=.\CostOfOperation.h
# End Source File
# Begin Source File

SOURCE=.\decommit.cpp
# End Source File
# Begin Source File

SOURCE=.\get_line_state.cpp
# End Source File
# Begin Source File

SOURCE=.\LineRegions.cpp
# End Source File
# Begin Source File

SOURCE=.\LineRegions.h
# End Source File
# Begin Source File

SOURCE=.\map.cpp
# End Source File
# Begin Source File

SOURCE=.\MemInfo.h
# End Source File
# Begin Source File

SOURCE=.\release.cpp
# End Source File
# Begin Source File

SOURCE=.\reserve.cpp
# End Source File
# Begin Source File

SOURCE=.\stack.cpp
# End Source File
# Begin Source File

SOURCE=.\WinMemory.cpp
# End Source File
# Begin Source File

SOURCE=.\WinMemory.h
# End Source File
# End Group
# Begin Source File

SOURCE=.\ExecDomainBase.h
# End Source File
# Begin Source File

SOURCE=.\ProtDomainBase.h
# End Source File
# Begin Source File

SOURCE=.\SyncDomainBase.h
# End Source File
# Begin Source File

SOURCE=.\win32.h
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=..\..\Include\giop.h
# End Source File
# Begin Source File

SOURCE=..\..\Include\iop.h
# End Source File
# End Group
# Begin Group "test"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\test\coltest.cpp
# End Source File
# Begin Source File

SOURCE=..\test\coltest.h
# End Source File
# Begin Source File

SOURCE=..\test\memtest.cpp
# End Source File
# Begin Source File

SOURCE=..\test\memtest.h
# End Source File
# End Group
# Begin Source File

SOURCE=..\plan.txt
# PROP Exclude_From_Build 1
# End Source File
# End Target
# End Project
