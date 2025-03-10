/*
    This is used for enumerating our "NetworkTools Loopback Adapter" using netsh.exe tool, if found, we changed its name from "Ethernet X" to "NetworkTools Loopback Adapter".

    This code is based on the Windows built-in netsh.exe tool.
*/

#include "..\..\Common\WpcapNames.h"

#include <windows.h>
#include <vector>
using namespace std;

typedef std::basic_string<TCHAR> tstring;
#include <tchar.h>

tstring executeCommand(TCHAR* strCmd);

void PrepareRenameLoopbackNetwork2();

BOOL DoRenameLoopbackNetwork2();

BOOL IsWindowsWin10();
