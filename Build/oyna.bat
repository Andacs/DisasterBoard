@echo off
rem Oyunu editorsuz pencerede acar (derle.bat yeterli, paket gerekmez). HARITA'yi projeye gore degistir.
rem "oyna.bat 2" -> haritaya ?Bolum=2 ekler (oyun bu secenegi okuyorsa).
set "ROOT=%~dp0.."
for %%F in ("%ROOT%\*.uproject") do set "PROJ=%%~fF"
set "HARITA=/Game/DisasterBoard/Maps/L_Ana"
set "BOLUM=%~1"
if "%BOLUM%"=="" set "BOLUM=0"
start "" "C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe" "%PROJ%" "%HARITA%?Bolum=%BOLUM%" -game -windowed -ResX=1600 -ResY=900 -log
