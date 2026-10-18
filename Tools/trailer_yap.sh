#!/usr/bin/env bash
# Tanitim videosunu oyunun icinden ceker (Git Bash). Editor KAPALI, once Build\derle.bat.
#
#   bash Tools/trailer_yap.sh          (ham cekim + Tools/trailer_kurgu.py; KEEP=1 kareleri birakir)
#   ESKI=1 bash Tools/trailer_yap.sh   (eski: oyun ici altyazili tek cekim)
#
# Oyundaki DBSinema konsol komutu (ADBPlayerController::DBSinema) akisi DBTest gibi GameMode cagrilariyla surer,
# baslik / altyazi cizer, her kareyi REC_f_#####.png olarak kaydeder ve oyunun koddan uretilen muzigini
# Saved/DB_muzik_trailer.wav'a yazar (64 sn, yogunluk 0->1). Oynanisa dokunmaz.
#
# 1) Oyunu "-benchmark -fps=30" ile acar: her kare oyunda tam 1/30 sn; makine yavas da olsa video akici.
# 2) ffmpeg: kareler + muzik -> Build/Teslim/${PROJE}_Trailer.mp4 (<= 60 sn, kural 5.2 ve A.6).
# Tetkik acilista video oynatmiyor: Sallanti'daki Intro.mp4 bolumu yok (Content/Movies pakete girerdi).
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
UE="/c/Program Files/Epic Games/UE_5.8/Engine/Binaries/Win64/UnrealEditor.exe"
UPROJ=$(ls "$ROOT"/*.uproject | head -1)
PROJE=$(basename "$UPROJ" .uproject)
HARITA="/Game/DisasterBoard/Maps/L_Ana"       # cekilecek bolum
SINEMA="DBSinema"                              # oyundaki sinema konsol komutu
MUZIK="$ROOT/Saved/DB_muzik_trailer.wav"       # DBSinema yazar (oyunun kendi muzigi)
SHOTS="$ROOT/Saved/Screenshots/WindowsEditor"
OUT="$ROOT/Build/Teslim"
FPS=30
W=1600   # 1920x1080 pencere 1080p ekrana sigmiyor; ffmpeg 1920x1080'e buyutur
H=900
mkdir -p "$OUT"
rm -f "$SHOTS"/REC_f_*.png "$MUZIK"

# Varsayilan: ham cekim + kurgu (logo, buyuk yazilar, gecisler). ESKI=1: oyunun icinde altyazili tek cekim (asagisi).
if [ "${ESKI:-0}" != 1 ]; then
  LOG="$ROOT/Saved/Logs/ham_run.log"
  # Pencere acmadan 1920x1080 (ekrana sigmasi gerekmiyor); yazi / baslik cizilmez, sahne baslari log'a yazilir
  MSYS_NO_PATHCONV=1 "$UE" "$(cygpath -w "$UPROJ")" "$HARITA"     -game -RenderOffscreen -ResX=1920 -ResY=1080 -benchmark -fps=$FPS -ExecCmds="$SINEMA" -DBSinemaHam -unattended -log     -abslog="$(cygpath -w "$LOG")" > /dev/null 2>&1 || true
  N=$(ls "$SHOTS"/REC_f_*.png 2>/dev/null | wc -l)
  echo "ham kare: $N"
  grep -a "DB_SINEMA_DONE" "$LOG" || { echo "cekim bitmedi, $LOG'a bak"; exit 1; }
  python "$ROOT/Tools/trailer_kurgu.py" --log "$LOG" --kareler "$SHOTS" --muzik "$MUZIK" --out "$OUT/${PROJE}_Trailer.mp4"
  ffprobe -v error -show_entries format=duration,size -of default=nw=1 "$OUT/${PROJE}_Trailer.mp4"
  [ "${KEEP:-0}" = 1 ] || rm -f "$SHOTS"/REC_f_*.png
  exit 0
fi

# MSYS_NO_PATHCONV yalnizca bu komutta (cmd //c ile ayni komutta kullanma: CLAUDE.md).
MSYS_NO_PATHCONV=1 "$UE" "$(cygpath -w "$UPROJ")" "$HARITA" \
  -game -windowed -ResX=$W -ResY=$H -benchmark -fps=$FPS -ExecCmds="$SINEMA" -unattended -log \
  -abslog="$(cygpath -w "$ROOT/Saved/Logs/trailer_run.log")" > /dev/null 2>&1 || true
N=$(ls "$SHOTS"/REC_f_*.png 2>/dev/null | wc -l)
echo "kare: $N"
grep -a "DB_SINEMA_DONE" "$ROOT/Saved/Logs/trailer_run.log" || true
[ "$N" -gt 100 ] || { echo "kare yok, Saved/Logs/trailer_run.log'a bak"; exit 1; }
[ -f "$MUZIK" ] || { echo "muzik yok: $MUZIK"; exit 1; }

DUR=$(python -c "print(round($N/$FPS,2))")
python -c "import sys; sys.exit(0 if $DUR <= 60 else 1)" || { echo "sure $DUR sn: 60'i asiyor, DBSinema'yi kisalt"; exit 1; }
FADE_OUT=$(python -c "print(max(0,$DUR-2.5))")

ffmpeg -y -loglevel error -framerate $FPS -i "$SHOTS/REC_f_%05d.png" -i "$MUZIK" \
  -vf "scale=1920:1080:flags=lanczos,fade=t=in:st=0:d=0.6,fade=t=out:st=$(python -c "print(max(0,$DUR-0.8))"):d=0.8,format=yuv420p" \
  -af "afade=t=in:st=0:d=1.5,afade=t=out:st=$FADE_OUT:d=2.5" -t $DUR \
  -c:v libx264 -preset slow -crf 18 -r $FPS -c:a aac -b:a 192k -movflags +faststart "$OUT/${PROJE}_Trailer.mp4"

ffprobe -v error -show_entries format=duration,size -of default=nw=1 "$OUT/${PROJE}_Trailer.mp4"
[ "${KEEP:-0}" = 1 ] || rm -f "$SHOTS"/REC_f_*.png
echo "tamam: $OUT/${PROJE}_Trailer.mp4"
