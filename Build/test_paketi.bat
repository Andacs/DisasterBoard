@echo off
rem Yigit'in varlik testi icin oyun paketi (Development: konsol ve log acik). Editor KAPALI olmali.
rem Cikti: Build\TestKiti\Windows\DisasterBoard.exe ; ZIP'lenip Yigit'e gonderilir, o da repo icinde Build\TestKiti\ altina acar.
set "ROOT=%~dp0.."
for %%F in ("%ROOT%\*.uproject") do set "PROJ=%%~fF"
"C:\Program Files\Epic Games\UE_5.8\Engine\Build\BatchFiles\RunUAT.bat" BuildCookRun -project="%PROJ%" -noP4 -platform=Win64 -clientconfig=Development -build -cook -stage -pak -archive -archivedirectory="%ROOT%\Build\TestKiti" -utf8output -unattended -ubtargs="-NoXGE -MaxParallelActions=2"
