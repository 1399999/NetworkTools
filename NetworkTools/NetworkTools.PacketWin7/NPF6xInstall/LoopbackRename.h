/*
    This is used for enumerating our "NetworkTools Loopback Adapter" using NetCfg API, if found, we changed its name from "Ethernet X" to "NetworkTools Loopback Adapter".
    Also, we need to make a flag in registry to let the NetworkTools driver know that "this adapter is ours", so send the loopback traffic to it.

    This code is modified based on example: https://msdn.microsoft.com/en-us/library/windows/desktop/aa364686.aspx
*/

#include "..\..\Common\WpcapNames.h"

BOOL RenameLoopbackNetwork(wchar_t strDeviceName[]);