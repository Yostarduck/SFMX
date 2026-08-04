#!/bin/bash
#
# sizereport.sh — Executable size breakdown for Game.exe (MSVC / Windows).
#
# Produces two tables:
#   1. Section totals (.text/.rdata/.data/...) via dumpbin — the ground truth.
#   2. Per-library weight inside the exe, parsed from the linker .map.
#
# The per-library number is an approximation: symbols are sorted by address and
# the gap to the next symbol (within the same PE section) is charged to the owning
# library. It includes both public and static symbols, so the mapped total lands
# within ~1% of the real exe size. Treat the ranking as authoritative and the
# absolute per-lib KB as "good enough to decide what to cut".
#
# Usage (run from anywhere; resolves the repo root relative to this script):
#   ./BuildScripts/sizereport.sh [Debug|Release] [x64|x86]     # defaults: Release x64
#
# Windows equivalent: BuildScripts\sizereport.bat (same args).
#
# It will (re)build the Game target with the opt-in SFMX_SIZE_MAP linker map if the
# .map is missing, then report. Normal builds are unaffected (the option is OFF by
# default). See ExeSize.md for the full analysis and the reduction playbook.

set -e

CONFIG="Release"
ARCH="x64"

for arg in "$1" "$2"; do
  case "${arg,,}" in
    release) CONFIG="Release" ;;
    debug)   CONFIG="Debug" ;;
    x64)     ARCH="x64" ;;
    x86)     ARCH="x86" ;;
    "")      ;;
    *) echo "Ignoring unknown arg: $arg" ;;
  esac
done

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"   # BuildScripts/ -> repo root
BUILD="$ROOT/Build"
EXE="$BUILD/$ARCH/$CONFIG/Game.exe"
MAP="$BUILD/$ARCH/$CONFIG/Game.map"

# --- locate dumpbin via vswhere (no hard-coded MSVC version) -----------------
find_dumpbin() {
  local vswhere="/c/Program Files (x86)/Microsoft Visual Studio/Installer/vswhere.exe"
  local vsroot dumpbin
  if [[ -x "$vswhere" ]]; then
    vsroot="$("$vswhere" -latest -property installationPath 2>/dev/null | tr -d '\r')"
    if [[ -n "$vsroot" ]]; then
      dumpbin="$(find "$vsroot/VC/Tools/MSVC" -path "*/bin/Host$ARCH/$ARCH/dumpbin.exe" 2>/dev/null | head -1)"
      [[ -n "$dumpbin" ]] && { echo "$dumpbin"; return 0; }
    fi
  fi
  # fall back to PATH
  command -v dumpbin.exe 2>/dev/null && return 0
  command -v dumpbin    2>/dev/null && return 0
  return 1
}

DUMPBIN="$(find_dumpbin || true)"

# --- ensure the map exists --------------------------------------------------
if [[ ! -f "$MAP" ]]; then
  echo ">> No $MAP found — building Game with SFMX_SIZE_MAP=ON ..."
  cmake -B "$BUILD" -DCMAKE_BUILD_TYPE="$CONFIG" -DSFMX_SIZE_MAP=ON >/dev/null
  cmake --build "$BUILD" --config "$CONFIG" --target Game >/dev/null
fi

[[ -f "$EXE" ]] || { echo "!! $EXE not found. Build first (./build.sh $CONFIG)."; exit 1; }

EXE_BYTES=$(stat -c%s "$EXE" 2>/dev/null || wc -c < "$EXE")
printf '\n=== Game.exe : %.2f MB (%s / %s) ===\n' "$(awk "BEGIN{print $EXE_BYTES/1048576}")" "$ARCH" "$CONFIG"

# --- 1) section totals (ground truth) ---------------------------------------
if [[ -n "$DUMPBIN" ]]; then
  echo
  echo "--- PE sections (raw data, ground truth) ---"
  "$DUMPBIN" -HEADERS "$EXE" 2>/dev/null | awk '
    /SECTION HEADER/            {inh=1; name="(unnamed)"; next}
    inh && $2=="name"          {name=$1; next}
    inh && /size of raw data/  {sz=strtonum("0x"$1);
                                printf "  %-12s %10.1f KB\n", name, sz/1024;
                                tot+=sz; inh=0}
    END{printf "  %-12s %10.1f KB\n","TOTAL", tot/1024}'
else
  echo "  (dumpbin not found — skipping section table)"
fi

# --- 2) per-library breakdown from the map ----------------------------------
echo
echo "--- Per-library weight inside the exe (from Game.map) ---"
awk '
  /Publics by Value/ {s=1; next}
  s && $NF ~ /:/ {
    split($1,sec,":"); if (sec[1] !~ /^[0-9a-fA-F]{4}$/) next;
    rva="";
    for (i=2;i<=NF;i++) if ($i ~ /^[0-9a-fA-F]{16}$/) { rva=$i; break }
    if (rva=="") next;
    print strtonum("0x" rva), sec[1], $NF
  }' "$MAP" | sort -n | awk '
  {
    split($3,a,":"); lib=a[1]; secid=$2;
    if (prevrva>0 && secid==prevsec) { d=$1-prevrva; if (d>0 && d<2000000) size[prevlib]+=d }
    cnt[lib]++; prevrva=$1; prevsec=secid; prevlib=lib;
  }
  END{
    for (l in size) total+=size[l];
    for (l in size) printf "%12.1f %6.1f%% %8d  %s\n", size[l]/1024, 100*size[l]/total, cnt[l], l
    printf "%12.1f %6s  %8s  %s\n", total/1024, "", "", "== TOTAL (mapped)"
  }' | sort -rn | awk '
  BEGIN{ printf "  %10s %7s %8s  %s\n","SIZE_KB","%","#syms","LIBRARY";
         print  "  ------------------------------------------------------------" }
  { printf "  %s\n", $0 }'

echo
