@echo off
rem Editor KAPALI olmali (Live Coding kilidi). -NoXGE -MaxParallelActions=2 bu makinede sart.
rem Proje adi repo kokundeki .uproject'ten bulunur; repo nereye klonlanirsa calisir. Build\ altina koy.
set "ROOT=%~dp0.."
for %%F in ("%ROOT%\*.uproject") do set "PROJ=%%~fF" & set "AD=%%~nF"
"C:\Program Files\Epic Games\UE_5.8\Engine\Build\BatchFiles\Build.bat" %AD%Editor Win64 Development -project="%PROJ%" -waitmutex -NoXGE -MaxParallelActions=2
