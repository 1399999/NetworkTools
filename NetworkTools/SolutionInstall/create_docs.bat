@echo off

IF "%2"=="" (set WPDPACKDESTDIR=.\WpdPack\) ELSE (set WPDPACKDESTDIR=%2)

IF ""=="%1" (set NETWORK_TOOLSSOURCEDIR=.\) ELSE (set NETWORK_TOOLSSOURCEDIR=%1) 

echo Creating \docs folder
mkdir %WPDPACKDESTDIR% >nul 2>nul
mkdir %WPDPACKDESTDIR%\docs >nul 2>nul
mkdir %WPDPACKDESTDIR%\docs\html >nul 2>nul

pushd %NETWORK_TOOLSSOURCEDIR%\dox\prj

echo - Deleting existing NetworkTools documentation
del /q docs\*.* 2> nul > nul
echo - Creating new documentation
doxygen NetworkTools_noc.dox >nul
echo - Copying all gif files
xcopy ..\pics\*.gif docs\. /v /y /q >nul
xcopy ..\*.gif docs\. /v /y /q >nul

popd

xcopy /v /Y %NETWORK_TOOLSSOURCEDIR%\dox\NetworkTools_docs.html	%WPDPACKDESTDIR%\docs\ 		>nul
xcopy /v /Y %NETWORK_TOOLSSOURCEDIR%\dox\prj\docs\*.*		%WPDPACKDESTDIR%\docs\html\	>nul
xcopy /v /Y %NETWORK_TOOLSSOURCEDIR%\dox\*.gif		%WPDPACKDESTDIR%\docs\html\	>nul
xcopy /v /Y %NETWORK_TOOLSSOURCEDIR%\dox\pics\*.gif		%WPDPACKDESTDIR%\docs\html\	>nul
echo Folder \docs created successfully
set WPDPACKDESTDIR=
set NETWORK_TOOLSSOURCEDIR=
