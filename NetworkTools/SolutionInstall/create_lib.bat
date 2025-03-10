@echo off

IF "%2"=="" (set WPDPACKDESTDIR=.\NetworkToolsSDK\) ELSE (set WPDPACKDESTDIR=%2)

IF ""=="%1" (set NETWORK_TOOLSSOURCEDIR=.\) ELSE (set NETWORK_TOOLSSOURCEDIR=%1) 

echo Creating \Lib folder
mkdir %WPDPACKDESTDIR% 		>nul 2>nul
mkdir %WPDPACKDESTDIR%\Lib 	>nul 2>nul
mkdir %WPDPACKDESTDIR%\Lib\x64	>nul 2>nul

xcopy /v /Y "%NETWORK_TOOLSSOURCEDIR%\wpcap\libpcap\Win32\Prj\Release\wpcap.lib"				%WPDPACKDESTDIR%\Lib\ >nul
xcopy /v /Y "%NETWORK_TOOLSSOURCEDIR%\wpcap\libpcap\Win32\Prj\x64\Release\wpcap.lib"				%WPDPACKDESTDIR%\Lib\x64 >nul
xcopy /v /Y "%NETWORK_TOOLSSOURCEDIR%\packetWin7\Dll\Project\Release No NetMon and AirPcap\packet.lib"	 	%WPDPACKDESTDIR%\Lib\ >nul
xcopy /v /Y "%NETWORK_TOOLSSOURCEDIR%\packetWin7\Dll\Project\x64\Release No NetMon and AirPcap\packet.lib"	 	%WPDPACKDESTDIR%\Lib\x64 >nul

echo Folder \Lib created successfully

set WPDPACKDESTDIR=
set NETWORK_TOOLSSOURCEDIR=
