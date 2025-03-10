@echo off

IF "%2"=="" (set WPDPACKDESTDIR=.\NetworkToolsSDK\) ELSE (set WPDPACKDESTDIR=%2)

IF ""=="%1" (set NETWORK_TOOLSSOURCEDIR=.\) ELSE (set NETWORK_TOOLSSOURCEDIR=%1) 

echo Creating \Examples folder
mkdir %WPDPACKDESTDIR% >nul 2>nul
mkdir %WPDPACKDESTDIR%\Examples-pcap >nul 2>nul
mkdir %WPDPACKDESTDIR%\Examples-remote >nul 2>nul

xcopy /s/e/v /Y %NETWORK_TOOLSSOURCEDIR%\Examples		%WPDPACKDESTDIR%\Examples-remote >nul
 
rem *** Delete Netmeter since it's no more part of the Developer's pack *** 
rd /S /Q %WPDPACKDESTDIR%\Examples-remote\NetMeter\

rem *** Delete kdump since the feature is not available at the moment *** 
rd /S /Q %WPDPACKDESTDIR%\Examples-remote\kdump\

xcopy /s/e/v /Y %NETWORK_TOOLSSOURCEDIR%\Examples-pcap		%WPDPACKDESTDIR%\Examples-pcap >nul

rem *** Delete NetworkToolsStress, since it's not a real example ***
rd /S /Q %WPDPACKDESTDIR%\Examples-pcap\NetworkTools_stress

rem *** Delete stats, since it's not a real example ***
rd /S /Q %WPDPACKDESTDIR%\Examples-pcap\stats


echo Folder \Examples created successfully
set WPDPACKDESTDIR=
set NETWORK_TOOLSSOURCEDIR=

echo ********************************************************************
echo *                                                                  *
echo * Now you can build the examples from the developers' pack folder! *
echo *                                                                  *
echo ********************************************************************
