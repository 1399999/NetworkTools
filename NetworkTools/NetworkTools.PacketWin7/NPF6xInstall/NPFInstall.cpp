#pragma warning(disable: 4311 4312)

#include <tchar.h>
#include <windows.h>

#include "..\..\..\version.h"

#include "ProtInstall.h"
#include "LoopbackInstall.h"
#include "CalloutInstall.h"
#include "DriverStoreClear.h"
#include "WlanRecord.h"
#include "RegUtil.h"
#include "ProcessUtil.h"

#include "debug.h"

extern BOOLEAN bWiFiService;

BOOL PacketInstallDriver60();
BOOL PacketStopDriver60();
BOOL PacketInstallDriver40();
BOOL PacketStopDriver40();

#define STR_COMMAND_USAGE \
_T("NPFInstall for NetworkTools ") _T(NETWORK_TOOLS_VER_STRING) _T(" ( http://NetworkTools.org )\n")\
_T("Usage: NPFInstall [Options]\n")\
_T("\n")\
_T("OPTIONS:\n")\
_T("  -i\t\t\t: Install the LWF driver\n")\
_T("  -i2\t\t\t: Install the LWF driver (with Wi-Fi support)\n")\
_T("  -u\t\t\t: Uninstall the LWF driver\n")\
_T("  -u2\t\t\t: Uninstall the LWF driver (with Wi-Fi support)\n")\
_T("  -iw\t\t\t: Install the WFP callout driver\n")\
_T("  -uw\t\t\t: Uninstall the WFP callout driver\n")\
_T("  -il\t\t\t: Install \"NetworkTools loopback adapter\"\n")\
_T("  -ul\t\t\t: Uninstall \"NetworkTools loopback adapter\"\n")\
_T("  -ii\t\t\t: Install the legacy driver (for XP)\n")\
_T("  -uu\t\t\t: Uninstall the legacy driver (for XP)\n")\
_T("  -r\t\t\t: Restart all bindings\n")\
_T("  -r2\t\t\t: Restart all bindings (with Wi-Fi support)\n")\
_T("  -d\t\t\t: Detect whether the driver service is pending to stop\n")\
_T("  -check_dll\t\t\t: Detect whether the NetworkTools DLLs are still used by any processes, will list them if yes\n")\
_T("  -c\t\t\t: Clear all the driverstore cache for the driver\n")\
_T("  -wlan_check\t\t: Check whether this machine owns a wireless adapter\n")\
_T("  -wlan_write_reg\t: Write the names of all wireless adapters to registry\n")\
_T("  -add_path\t\t: Add NetworkTools folder to the PATH environment variable\n")\
_T("  -remove_path\t\t: Remove NetworkTools folder from the PATH environment variable\n")\
_T("  -n\t\t\t: Hide this window when executing the command\n")\
_T("  -h\t\t\t: Print this help summary page\n")\
_T("\n")\
_T("EXAMPLES:\n")\
_T("  NPFInstall -i\n")\
_T("  NPFInstall -iw\n")\
_T("\n")\
_T("SEE THE MAN PAGE (https://github.com/1399999/NetworkTools) FOR MORE OPTIONS AND EXAMPLES\n")

#define STR_INVALID_PARAMETER _T("Error: invalid parameter, type in \"NPFInstall -h\" for help.\n")

BOOL PacketIsServiceStopPending()
{
	TRACE_ENTER();

	BOOL bResult = FALSE;
	SERVICE_STATUS_PROCESS ssp;
	DWORD dwStartTime = GetTickCount();
	DWORD dwBytesNeeded;
	DWORD dwTimeout = 30000; // 30-second time-out
	// DWORD dwWaitTime;

	// Get a handle to the SCM database.

	SC_HANDLE schSCManager = OpenSCManager(
		NULL,                    // local computer
		NULL,                    // ServicesActive database
		SC_MANAGER_ALL_ACCESS);  // full access rights

	if (NULL == schSCManager)
	{
		TRACE_PRINT1("OpenSCManager failed (0x%08x)", GetLastError());
		TRACE_EXIT();
		return FALSE;
	}

	// Get a handle to the service.

	SC_HANDLE schService = OpenService(
		schSCManager,         // SCM database
		_T(NPF_DRIVER_NAME_SMALL),            // name of service
		SERVICE_STOP |
		SERVICE_QUERY_STATUS |
		SERVICE_ENUMERATE_DEPENDENTS);

	if (schService == NULL)
	{
		TRACE_PRINT1("OpenService failed (0x%08x)", GetLastError());
		CloseServiceHandle(schSCManager);
		TRACE_EXIT();
		return FALSE;
	}

	// Make sure the service is not already stopped.

	if ( !QueryServiceStatusEx(
		schService,
		SC_STATUS_PROCESS_INFO,
		(LPBYTE)&ssp,
		sizeof(SERVICE_STATUS_PROCESS),
		&dwBytesNeeded ) )
	{
		TRACE_PRINT1("QueryServiceStatusEx failed (0x%08x)", GetLastError());
		goto stop_cleanup;
	}

	if ( ssp.dwCurrentState == SERVICE_STOPPED )
	{
		TRACE_PRINT("Service is already stopped.");
		goto stop_cleanup;
	}

	if (ssp.dwCurrentState == SERVICE_STOP_PENDING)
	{
		bResult = TRUE;
	}

stop_cleanup:
	CloseServiceHandle(schService);
	CloseServiceHandle(schSCManager);
	TRACE_EXIT();
	return bResult;
}

BOOL PacketInstallDriver60()
{
	TRACE_ENTER();
	BOOL result = FALSE;

	result = (BOOL) InstallDriver();

	TRACE_EXIT();
	return result;
}

BOOL PacketStopDriver60()
{
	TRACE_ENTER();
	BOOL result;

	result = (BOOL) UninstallDriver();

	TRACE_EXIT();
	return result;
}

BOOL PacketInstallDriver40()
{
	TRACE_ENTER();

	PacketStopDriver40();

	SC_HANDLE schSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
	if (schSCManager == NULL)
	{
		TRACE_EXIT();
		return FALSE;
	}

	TCHAR szFileFullPath[_MAX_PATH];
	DWORD nResult = GetServiceSysFilePath(szFileFullPath, MAX_PATH);
	if (nResult == 0)
	{
		TRACE_EXIT();
		return FALSE;
	}

	SC_HANDLE schService = CreateService(schSCManager, _T(NPF_DRIVER_NAME_SMALL), NPF_SERVICE_DESC_TCHAR,
		SERVICE_ALL_ACCESS,
		SERVICE_KERNEL_DRIVER,
		SERVICE_DEMAND_START,
		SERVICE_ERROR_NORMAL,
		szFileFullPath,
		NULL, NULL, NULL, NULL, NULL);
	if (schService == NULL)
	{
		TRACE_EXIT();
		return FALSE;
	}

	CloseServiceHandle(schSCManager);
	CloseServiceHandle(schService);

	TRACE_EXIT();
	return TRUE;
}

BOOL PacketStopDriver40()
{
	TRACE_ENTER();

	SC_HANDLE schSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
	if (schSCManager == NULL)
	{
		TRACE_EXIT();
		return FALSE;
	}

	SC_HANDLE schService = OpenService(schSCManager, _T(NPF_DRIVER_NAME_SMALL), SERVICE_ALL_ACCESS | DELETE);
	if (schService == NULL)
	{
		TRACE_EXIT();
		return FALSE;
	}

	DeleteService(schService);

	CloseServiceHandle(schSCManager);
	CloseServiceHandle(schService); 

	TRACE_EXIT();
	return TRUE;
}

BOOL PacketRenableBindings()
{
	BOOL result;

	TRACE_ENTER();

	result = (BOOL) RenableBindings();

	TRACE_EXIT();
	return result;
}

int _tmain(int argc, _TCHAR* argv[])
{
	TRACE_ENTER();

	BOOL bSuccess = FALSE;
	BOOL bNoWindow = FALSE;
	int nStatus = 0;

	SetConsoleTitle(_T("NPFInstall for NetworkTools ") _T(NETWORK_TOOLS_VER_STRING) _T(" (http://NetworkTools.org)"));
	vector<tstring> strArgs;
	tstring strTmp;
	for (int i = 0; i < argc; i++)
	{
		strTmp = argv[i];
		TRACE_PRINT2("_tmain: executing, argv[%d] = %s.", i, argv[i]);
		if (strTmp == _T("-n"))
		{
			bNoWindow = TRUE;
		}
		else
		{
			strArgs.push_back(strTmp);
		}
	}

	if (bNoWindow)
	{
		ShowWindow(GetConsoleWindow(), SW_HIDE);
	}

	if (strArgs.size() == 1)
	{
		_tprintf(STR_COMMAND_USAGE);
		nStatus = 0;
		goto _EXIT;

	}
	else if (strArgs.size() >= 3)
	{
		_tprintf(STR_INVALID_PARAMETER);
		nStatus = -1;
		goto _EXIT;
	}
	else //strArgs.size() == 2
	{
		if (strArgs[1] == _T("-i"))
		{
			bWiFiService = FALSE;
			bSuccess = PacketInstallDriver60();
			if (bSuccess)
			{
				_tprintf(_T("NetworkTools LWF driver has been successfully installed!\n"));
				nStatus = 0;
				goto _EXIT;
			}
			else
			{
				_tprintf(_T("NetworkTools LWF driver has failed to be installed.\n"));
				nStatus = -1;
				goto _EXIT;
			}
		}
		if (strArgs[1] == _T("-i2"))
		{
			bWiFiService = TRUE;
			bSuccess = PacketInstallDriver60();
			if (bSuccess)
			{
				_tprintf(_T("NetworkTools LWF driver (with Wi-Fi support) has been successfully installed!\n"));
				nStatus = 0;
				goto _EXIT;
			}
			else
			{
				_tprintf(_T("NetworkTools LWF driver (with Wi-Fi support) has failed to be installed.\n"));
				nStatus = -1;
				goto _EXIT;
			}
		}
		else if (strArgs[1] == _T("-u"))
		{
			bWiFiService = FALSE;
			bSuccess = PacketStopDriver60();
			if (bSuccess)
			{
				_tprintf(_T("NetworkTools LWF driver has been successfully uninstalled!\n"));
				nStatus = 0;
				goto _EXIT;
			}
			else
			{
				_tprintf(_T("NetworkTools LWF driver has failed to be uninstalled.\n"));
				nStatus = -1;
				goto _EXIT;
			}
		}
		else if (strArgs[1] == _T("-u2"))
		{
			bWiFiService = TRUE;
			bSuccess = PacketStopDriver60();
			if (bSuccess)
			{
				_tprintf(_T("NetworkTools LWF driver (with Wi-Fi support) has been successfully uninstalled!\n"));
				nStatus = 0;
				goto _EXIT;
			}
			else
			{
				_tprintf(_T("NetworkTools LWF driver (with Wi-Fi support) has failed to be uninstalled.\n"));
				nStatus = -1;
				goto _EXIT;
			}
		}
		else if (strArgs[1] == _T("-r"))
		{
			bWiFiService = FALSE;
			bSuccess = PacketRenableBindings();
			if (bSuccess)
			{
				_tprintf(_T("The bindings of NetworkTools driver have been successfully restarted!\n"));
				nStatus = 0;
				goto _EXIT;
			}
			else
			{
				_tprintf(_T("The bindings of NetworkTools driver have failed to be restarted.\n"));
				nStatus = -1;
				goto _EXIT;
			}
		}
		else if (strArgs[1] == _T("-r2"))
		{
			bWiFiService = TRUE;
			bSuccess = PacketRenableBindings();
			if (bSuccess)
			{
				_tprintf(_T("The bindings of NetworkTools driver (with Wi-Fi support) have been successfully restarted!\n"));
				nStatus = 0;
				goto _EXIT;
			}
			else
			{
				_tprintf(_T("The bindings of NetworkTools driver (with Wi-Fi support) have failed to be restarted.\n"));
				nStatus = -1;
				goto _EXIT;
			}
		}
		else if (strArgs[1] == _T("-ii"))
		{
			bSuccess = PacketInstallDriver40();
			if (bSuccess)
			{
				_tprintf(_T("NPF legacy driver has been successfully installed!\n"));
				nStatus = 0;
				goto _EXIT;
			}
			else
			{
				_tprintf(_T("NPF legacy driver has failed to be installed.\n"));
				nStatus = -1;
				goto _EXIT;
			}
		}
		else if (strArgs[1] == _T("-uu"))
		{
			bSuccess = PacketStopDriver40();
			if (bSuccess)
			{
				_tprintf(_T("NPF legacy driver has been successfully uninstalled!\n"));
				nStatus = 0;
				goto _EXIT;
			}
			else
			{
				_tprintf(_T("NPF legacy driver has failed to be uninstalled.\n"));
				nStatus = -1;
				goto _EXIT;
			}
		}
		else if (strArgs[1] == _T("-il"))
		{
			bSuccess = InstallLoopbackAdapter();
			if (bSuccess)
			{
				_tprintf(_T("NetworkTools Loopback adapter has been successfully installed!\n"));
				nStatus = 0;
				goto _EXIT;
			}
			else
			{
				_tprintf(_T("NetworkTools Loopback adapter has failed to be installed.\n"));
				nStatus = -1;
				goto _EXIT;
			}
		}
		else if (strArgs[1] == _T("-ul"))
		{
			bSuccess = UninstallLoopbackAdapter();
			if (bSuccess)
			{
				_tprintf(_T("NetworkTools Loopback adapter has been successfully uninstalled!\n"));
				nStatus = 0;
				goto _EXIT;
			}
			else
			{
				_tprintf(_T("NetworkTools Loopback adapter has failed to be uninstalled.\n"));
				nStatus = -1;
				goto _EXIT;
			}
		}
		else if (strArgs[1] == _T("-iw"))
		{
			bSuccess = InstallWFPCallout();
			if (bSuccess)
			{
				_tprintf(_T("NetworkTools WFP callout driver has been successfully installed!\n"));
				nStatus = 0;
				goto _EXIT;
			}
			else
			{
				_tprintf(_T("NetworkTools WFP callout driver has failed to be installed.\n"));
				nStatus = -1;
				goto _EXIT;
			}
		}
		else if (strArgs[1] == _T("-uw"))
		{
			bSuccess = UninstallWFPCallout();
			if (bSuccess)
			{
				_tprintf(_T("NetworkTools WFP callout driver has been successfully uninstalled!\n"));
				nStatus = 0;
				goto _EXIT;
			}
			else
			{
				_tprintf(_T("NetworkTools WFP callout driver has failed to be uninstalled.\n"));
				nStatus = -1;
				goto _EXIT;
			}
		}
		else if (strArgs[1] == _T("-d"))
		{
			bSuccess = PacketIsServiceStopPending();
			if (bSuccess)
			{
				_tprintf(_T("NetworkTools service is pending to stop!\n"));
				nStatus = 0;
				goto _EXIT;
			}
			else
			{
				_tprintf(_T("NetworkTools service is not pending to stop.\n"));
				nStatus = -1;
				goto _EXIT;
			}
		}
		else if (strArgs[1] == _T("-check_dll"))
		{
			tstring strInUseProcesses = getInUseProcesses();
			if (strInUseProcesses == _T(""))
			{
				_tprintf(_T("<NULL>\n"));
				nStatus = 0;
				goto _EXIT;
			}
			else
			{
				_tprintf(_T("%s\n"), strInUseProcesses.c_str());
				nStatus = -1;
				goto _EXIT;
			}
		}
		else if (strArgs[1] == _T("-c"))
		{
			bSuccess = ClearDriverStore();
			if (bSuccess)
			{
				_tprintf(_T("NetworkTools driver cache in Driver Store has been successfully cleaned up!\n"));
				nStatus = 0;
				goto _EXIT;
			}
			else
			{
				_tprintf(_T("NetworkTools driver cache in Driver Store has failed to be cleaned up.\n"));
				nStatus = -1;
				goto _EXIT;
			}
		}
		else if (strArgs[1] == _T("-wlan_check"))
		{
			vector<tstring> nstrAdapterGuids;
			nstrAdapterGuids = getWlanAdapterGuids();
			if (nstrAdapterGuids.size() != 0)
			{
				_tprintf(_T("Wlan adapters: %s\n"), printArray(nstrAdapterGuids).c_str());
				nStatus = 0;
				goto _EXIT;
			}
			else
			{
				_tprintf(_T("Wlan adapters: NULL\n"));
				nStatus = -1;
				goto _EXIT;
			}
		}
		else if (strArgs[1] == _T("-wlan_write_reg"))
		{
			bSuccess = writeWlanAdapterGuidsToRegistry();
			if (bSuccess)
			{
				_tprintf(_T("Wlan adapters have been successfully written to registry!\n"));
				nStatus = 0;
				goto _EXIT;
			}
			else
			{
				_tprintf(_T("Wlan adapters have failed to be written to registry.\n"));
				nStatus = -1;
				goto _EXIT;
			}
		}
		else if (strArgs[1] == _T("-add_path"))
		{
			bSuccess = addNetworkToolsFolderToPath();
			if (bSuccess)
			{
				_tprintf(_T("NetworkTools folder has been successfully added to PATH!\n"));
				nStatus = 0;
				goto _EXIT;
			}
			else
			{
				_tprintf(_T("NetworkTools folder has failed to be added to PATH.\n"));
				nStatus = -1;
				goto _EXIT;
			}
		}
		else if (strArgs[1] == _T("-remove_path"))
		{
			bSuccess = removeNetworkToolsFolderFromPath();
			if (bSuccess)
			{
				_tprintf(_T("NetworkTools folder has been successfully removed from PATH!\n"));
				nStatus = 0;
				goto _EXIT;
			}
			else
			{
				_tprintf(_T("NetworkTools folder has failed to be removed from PATH.\n"));
				nStatus = -1;
				goto _EXIT;
			}
		}
		else if (strArgs[1] == _T("-h"))
		{
			_tprintf(STR_COMMAND_USAGE);
			nStatus = 0;
			goto _EXIT;
		}
		else
		{
			_tprintf(STR_INVALID_PARAMETER);
			nStatus = -1;
			goto _EXIT;
		}
	}

_EXIT:
	if (nStatus == 0)
	{
		TRACE_PRINT1("_tmain: succeed, nStatus = %d.", nStatus);
	}
	else
	{
		TRACE_PRINT1("_tmain: error, nStatus = %d.", nStatus);
	}
	TRACE_EXIT();
	return nStatus;
}