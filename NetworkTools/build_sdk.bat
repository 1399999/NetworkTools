@echo off

if "%2"== "" ( rd /s/q ./NetworkTools-sdk 2>nul >nul) else ( rd /s /q "%2" 2>nul >nul)

call create_include.bat %1 %2

call create_lib.bat %1 %2

call create_examples.bat %1 %2

call create_docs.bat %1 %2

"C:\Program Files\7-Zip\7z.exe" a installer\NetworkTools-sdk-0.07.zip .\NetworkTools-sdk
PAUSE
