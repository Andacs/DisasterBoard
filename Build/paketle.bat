@echo off
rem Editor KAPALI olmali. Teslim konfigurasyonu: Shipping. Cikti: Build\Package\Windows\<Proje>.exe
rem Sallanti'da 111-134 sn (sicak). Ilk soguk paket icin 30 dk butcele. Build\ altina koy.
set "ROOT=%~dp0.."
for %%F in ("%ROOT%\*.uproject") do set "PROJ=%%~fF"
"C:\Program Files\Epic Games\UE_5.8\Engine\Build\BatchFiles\RunUAT.bat" BuildCookRun -project="%PROJ%" -noP4 -platform=Win64 -clientconfig=Shipping -build -cook -stage -pak -archive -archivedirectory="%ROOT%\Build\Package" -utf8output -unattended -ubtargs="-NoXGE -MaxParallelActions=2"
