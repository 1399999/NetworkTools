/***************************************************************************
 * NetworkToolsHelperTest.c -- A program used to test NetworkToolsHelper.exe             *
 * Note: this code is now integrated into packet.dll, this file is only    *
 * used for test. This is for "Admin-only mode", as packet.dll runs on     *
 * non-Admin level and NetworkToolsHelper.exe runs on Admin level. If user denies *
 * the UAC prompt, NetworkToolsHelper.exe will not start.                         *
 *                                                                         *
 * This program is based on Microsoft example:                             *
 * https://msdn.microsoft.com/en-us/library/windows/desktop/aa365592%28v=vs.85%29.aspx?f=255&MSPPError=-2147217396
 * https://msdn.microsoft.com/en-us/library/windows/desktop/aa365588%28v=vs.85%29.aspx?f=255&MSPPError=-2147217396
 ***************************************************************************/

#include <stdio.h>
#include <windows.h>
//#define _DBG
#include "../debug.h"

#define BUFSIZE 512
#define MAX_SEM_COUNT 10
#define MAX_TRY_TIME 50
#define SLEEP_TIME 50
// Handle for NetworkToolsHelper named pipe.
HANDLE g_NetworkToolsHelperPipe = INVALID_HANDLE_VALUE;
// Whether this process is running in Administrator mode.
BOOL g_IsAdminMode = FALSE;
// Whether we have already tried NetworkToolsHelper.
BOOL g_NetworkToolsHelperTried = FALSE;
// The handle to this DLL.
HANDLE g_DllHandle = NULL;

BOOL NetworkToolsCreatePipe(char *pipeName, HANDLE moduleName)
{
	int pid = GetCurrentProcessId();
	char params[BUFSIZE];
	SHELLEXECUTEINFOA shExInfo = {0};
	DWORD nResult;
	char lpFilename[BUFSIZE];
	char szDrive[BUFSIZE];
	char szDir[BUFSIZE];

	TRACE_ENTER("NetworkToolsCreatePipe");

	// Get Path to This Module
	nResult = GetModuleFileNameA((HMODULE) moduleName, lpFilename, BUFSIZE);
	if (nResult == 0)
	{
		TRACE_PRINT1("GetModuleFileNameA failed. GLE=%d\n", GetLastError() ); 
		TRACE_EXIT("NetworkToolsCreatePipe");
		return FALSE;
	}
	_splitpath_s(lpFilename, szDrive, BUFSIZE, szDir, BUFSIZE, NULL, 0, NULL, 0);
	_makepath_s(lpFilename, BUFSIZE, szDrive, szDir, "NetworkToolsHelper", ".exe");

	sprintf_s(params, BUFSIZE, "%s %d", pipeName, pid);

	shExInfo.cbSize = sizeof(shExInfo);
	shExInfo.fMask = SEE_MASK_NOCLOSEPROCESS;
	shExInfo.hwnd = 0;
	shExInfo.lpVerb = "runas";				// Operation to perform
	shExInfo.lpFile = lpFilename;			// Application to start    
	shExInfo.lpParameters = params;			// Additional parameters
	shExInfo.lpDirectory = 0;
	shExInfo.nShow = SW_SHOW;
	shExInfo.hInstApp = 0;  

	if (!ShellExecuteExA(&shExInfo))
	{
		DWORD dwError = GetLastError();
		if (dwError == ERROR_CANCELLED)
		{
			// The user refused to allow privileges elevation.
			// Do nothing ...
		}
		TRACE_EXIT("NetworkToolsCreatePipe");
		return FALSE;
	}
	else
	{
		TRACE_EXIT("NetworkToolsCreatePipe");
		return TRUE;
		//		hChildProcess = shExInfo.hProcess;
		// 		WaitForSingleObject(shExInfo.hProcess, INFINITE);
		// 		CloseHandle(shExInfo.hProcess);
	}
}

HANDLE NetworkToolsConnect(char *pipeName)
{
	HANDLE hPipe = INVALID_HANDLE_VALUE;
	int tryTime = 0;
	char lpszPipename[BUFSIZE];

	TRACE_ENTER("NetworkToolsConnect");

	sprintf_s(lpszPipename, BUFSIZE, "\\\\.\\pipe\\%s", pipeName);

	// Try to open a named pipe; wait for it, if necessary. 
	while (tryTime < MAX_TRY_TIME) 
	{ 
		hPipe = CreateFileA( 
			lpszPipename,   // pipe name 
			GENERIC_READ |  // read and write access 
			GENERIC_WRITE, 
			0,              // no sharing 
			NULL,           // default security attributes
			OPEN_EXISTING,  // opens existing pipe 
			0,              // default attributes 
			NULL);          // no template file 

		// Break if the pipe handle is valid. 

		if (hPipe != INVALID_HANDLE_VALUE)
		{
			break;
		}
		else
		{
			tryTime ++;
			Sleep(SLEEP_TIME);
		}

		// Exit if an error other than ERROR_PIPE_BUSY occurs. 

		// 		if (GetLastError() != ERROR_PIPE_BUSY) 
		// 		{
		// 			printf("Could not open pipe. GLE=%d\n", GetLastError());
		// 			return INVALID_HANDLE_VALUE;
		// 		}

		// 		// All pipe instances are busy, so wait for 20 seconds. 
		// 
		// 		if ( ! WaitNamedPipe(lpszPipename, 2000)) 
		// 		{ 
		// 			printf("Could not open pipe: 2 second wait timed out."); 
		// 			return INVALID_HANDLE_VALUE;
		// 		} 
	}

	TRACE_EXIT("NetworkToolsConnect");
	return hPipe;
}

HANDLE NetworkToolsRequestHandle(char *sMsg)
{
	LPSTR lpvMessage = sMsg; 
	char  chBuf[BUFSIZE]; 
	BOOL   fSuccess = FALSE; 
	DWORD  cbRead, cbToWrite, cbWritten, dwMode; 
	HANDLE hPipe = g_NetworkToolsHelperPipe;

	TRACE_ENTER("NetworkToolsRequestHandle");

	if (hPipe == INVALID_HANDLE_VALUE)
	{
		TRACE_EXIT("NetworkToolsRequestHandle");
		return INVALID_HANDLE_VALUE;
	}

	// The pipe connected; change to message-read mode. 
	dwMode = PIPE_READMODE_MESSAGE; 
	fSuccess = SetNamedPipeHandleState( 
		hPipe,    // pipe handle 
		&dwMode,  // new pipe mode 
		NULL,     // don't set maximum bytes 
		NULL);    // don't set maximum time 
	if ( ! fSuccess) 
	{
		TRACE_PRINT1("SetNamedPipeHandleState failed. GLE=%d\n", GetLastError() ); 
		TRACE_EXIT("NetworkToolsRequestHandle");
		return INVALID_HANDLE_VALUE;
	}

	// Send a message to the pipe server. 

	cbToWrite = (DWORD) (strlen(lpvMessage) + 1) * sizeof(char);
	TRACE_PRINT2("\nSending %d byte message: \"%s\"\n", cbToWrite, lpvMessage); 

	fSuccess = WriteFile( 
		hPipe,                  // pipe handle 
		lpvMessage,             // message 
		cbToWrite,              // message length 
		&cbWritten,             // bytes written 
		NULL);                  // not overlapped 

	if ( ! fSuccess) 
	{
		TRACE_PRINT1("WriteFile to pipe failed. GLE=%d\n", GetLastError() ); 
		TRACE_EXIT("NetworkToolsRequestHandle");
		return INVALID_HANDLE_VALUE;
	}

	TRACE_PRINT("Message sent to server, receiving reply as follows:\n");

	do 
	{ 
		// Read from the pipe. 

		fSuccess = ReadFile( 
			hPipe,    // pipe handle 
			chBuf,    // buffer to receive reply 
			BUFSIZE*sizeof(char),  // size of buffer 
			&cbRead,  // number of bytes read 
			NULL);    // not overlapped 

		if ( ! fSuccess && GetLastError() != ERROR_MORE_DATA )
			break; 

		//printf("\"%s\"\n", chBuf ); 
	} while ( ! fSuccess);  // repeat loop if ERROR_MORE_DATA 

	if ( ! fSuccess)
	{
		TRACE_PRINT1("ReadFile from pipe failed. GLE=%d\n", GetLastError() );
		TRACE_EXIT("NetworkToolsRequestHandle");
		return INVALID_HANDLE_VALUE;
	}

	//printf("\n<End of message, press ENTER to terminate connection and exit\n>");
	if (cbRead != 0)
	{
		//int hd = atoi(chBuf);
		HANDLE hd = (HANDLE) strtoul(chBuf, NULL, 16);
		TRACE_PRINT1("Received Driver Handle: 0x%08x\n", hd);
		TRACE_EXIT("NetworkToolsRequestHandle");
		return hd;
	}
	else
	{
		TRACE_EXIT("NetworkToolsRequestHandle");
		return INVALID_HANDLE_VALUE;
	}
}

BOOL NetworkToolsIsAdminMode()
{
	BOOL bIsRunAsAdmin = FALSE;
	DWORD dwError = ERROR_SUCCESS;
	PSID pAdministratorsGroup = NULL;
	// Allocate and initialize a SID of the administrators group.
	SID_IDENTIFIER_AUTHORITY NtAuthority = SECURITY_NT_AUTHORITY;

	TRACE_ENTER("NetworkToolsIsAdminMode");

	if (!AllocateAndInitializeSid(
		&NtAuthority, 
		2, 
		SECURITY_BUILTIN_DOMAIN_RID, 
		DOMAIN_ALIAS_RID_ADMINS, 
		0, 0, 0, 0, 0, 0, 
		&pAdministratorsGroup))
	{
		dwError = GetLastError();
		goto Cleanup;
	}

	// Determine whether the SID of administrators group is enabled in 
	// the primary access token of the process.
	if (!CheckTokenMembership(NULL, pAdministratorsGroup, &bIsRunAsAdmin))
	{
		dwError = GetLastError();
		goto Cleanup;
	}

Cleanup:
	// Centralized cleanup for all allocated resources.
	if (pAdministratorsGroup)
	{
		FreeSid(pAdministratorsGroup);
		pAdministratorsGroup = NULL;
	}

	// Throw the error if something failed in the function.
	if (ERROR_SUCCESS != dwError)
	{
		TRACE_PRINT1("IsProcessRunningAsAdminMode failed. GLE=%d\n", dwError);
	}

	TRACE_PRINT1("IsProcessRunningAsAdminMode result: %s\n", bIsRunAsAdmin? "yes" : "no");
	TRACE_EXIT("NetworkToolsIsAdminMode");
	return bIsRunAsAdmin;
}

void NetworkToolsStartHelper()
{
	TRACE_ENTER("NetworkToolsStartHelper");

	g_NetworkToolsHelperTried = TRUE;

	// Check if this process is running in Administrator mode.
	g_IsAdminMode = NetworkToolsIsAdminMode();

	if (!g_IsAdminMode)
	{
		char pipeName[BUFSIZE];
		int pid = GetCurrentProcessId();
		sprintf_s(pipeName, BUFSIZE, "NetworkTools-%d", pid);
		if (NetworkToolsCreatePipe(pipeName, g_DllHandle))
		{
			g_NetworkToolsHelperPipe = NetworkToolsConnect(pipeName);
			if (g_NetworkToolsHelperPipe == INVALID_HANDLE_VALUE)
			{
				// NetworkToolsHelper failed, let g_IsAdminMode be TRUE to avoid next requestHandleFromNetworkToolsHelper() calls.
				g_IsAdminMode = TRUE;
			}
		}
		else
		{
			// NetworkToolsHelper failed, let g_IsAdminMode be TRUE to avoid next requestHandleFromNetworkToolsHelper() calls.
			g_IsAdminMode = TRUE;
		}
	}

	TRACE_EXIT("NetworkToolsStartHelper");
}

void NetworkToolsStopHelper()
{
	TRACE_ENTER("NetworkToolsStopHelper");

	if (g_NetworkToolsHelperPipe != INVALID_HANDLE_VALUE)
	{
		CloseHandle(g_NetworkToolsHelperPipe);
		g_NetworkToolsHelperPipe = INVALID_HANDLE_VALUE;
	}

	TRACE_EXIT("NetworkToolsStopHelper");
}

int main(int argc, char* argv[])
{
	HANDLE hFile;
	char SymbolicLink1[BUFSIZE] = "\\\\.\\Global\\NetworkTools_{14AFDBFA-FD9E-48D4-8FF5-C7FD0EB924A4}";
	char SymbolicLink2[BUFSIZE] = "\\\\.\\Global\\NetworkTools_{14AFDBFA-FD9E-48D4-1111-C7FD0EB924A5}";

	//g_DllHandle = DllHandle;

	// NetworkToolsHelper Initialization, used for accessing the driver with Administrator privilege.
	if (!g_NetworkToolsHelperTried)
	{
		NetworkToolsStartHelper();
	}

	// do the job
	if (!g_IsAdminMode)
	{
		hFile = NetworkToolsRequestHandle(SymbolicLink1);
	}
	else
	{
		hFile = CreateFileA(SymbolicLink1,GENERIC_WRITE | GENERIC_READ,
			0,NULL,OPEN_EXISTING,0,0);
	}
	if (!g_IsAdminMode)
	{
		hFile = NetworkToolsRequestHandle(SymbolicLink2);
	}
	else
	{
		hFile = CreateFileA(SymbolicLink2,GENERIC_WRITE | GENERIC_READ,
			0,NULL,OPEN_EXISTING,0,0);
	}

	getchar();

	if (!g_IsAdminMode)
	{
		// NetworkToolsHelper De-Initialization.
		NetworkToolsStopHelper();
	}

	return 0;
}

