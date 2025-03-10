/*
	This is used for enumerating our "NetworkTools Loopback Adapter" using NetCfg API, if found, we changed its name from "Ethernet X" or "Local Network Area" to "NetworkTools Loopback Adapter".
	Also, we need to make a flag in registry to let the NetworkTools driver know that "this adapter is ours", so send the loopback traffic to it.
*/

#include <vector>
using namespace std;

typedef std::basic_string<TCHAR> tstring;

int getIntDevID(TCHAR strDevID[]);
BOOL AddFlagToRegistry(tstring strDeviceName);
BOOL AddFlagToRegistry_Service(tstring strDeviceName);
BOOL RecordLoopbackDevice(int iNetworkToolsAdapterID);
