#!/usr/bin/env python3
"""Show the size of every dependency the Game executable pulls in."""

import os
import subprocess
from pathlib import Path

ROOT = Path(__file__).resolve().parent
BUILD = ROOT / "Build"
LIBS = {"Debug": BUILD / "x64/Debug", "Release": BUILD / "x64/Release"}
THIRD = ROOT / "ThirdParty"

# Categories — each entry is (display name, glob pattern, build_output?)
#   build_output=True  → look in Build/x64/{config}/
#   build_output=False → look in ThirdParty/ (vendored source)
CATEGORIES = [
    ("Engine",   [("libFrameworkGame.a", True), ("libImageWebP.a", True)]),
    ("SFML",     [("libsfml-graphics-s-d.a", True), ("libsfml-audio-s-d.a", True),
                   ("libsfml-window-s-d.a", True), ("libsfml-system-s-d.a", True)]),
    ("LuaJIT",   [("libluajit.a", True)]),
    ("WebP",     [("libwebpdecoder.a", True)]),
    ("LZ4",      [("liblz4_vendored.a", True)]),
    ("Executables", [("Game", True), ("UnitTests", True)]),
]

# Vendored source directories (always consumed header-only or compiled)
VENDORED = [
    ("doctest",   THIRD / "doctest"),
    ("nanobench", THIRD / "nanobench"),
    ("mINI",      THIRD / "mINI"),
    ("stdUUID",   THIRD / "stdUUID"),
    ("LZ4 (src)", THIRD / "lz4"),
]


def human(size: int) -> str:
    if size >= 1024 * 1024:
        return f"{size / (1024*1024):,.1f} MB"
    return f"{size / 1024:,.0f} KB"


def file_size(path: str | os.PathLike) -> int:
    try:
        return os.path.getsize(path)
    except OSError:
        return 0


def report(config: str, lib_dir: Path) -> None:
    print(f"\n{'=' * 48}")
    print(f"  Build configuration: {config}")
    print(f"{'=' * 48}")
    print(f"  Library directory:   {lib_dir}\n")

    grand_total = 0
    all_rows = []

    for group, entries in CATEGORIES:
        rows = []
        group_total = 0
        for name, is_build in entries:
            d = lib_dir if is_build else THIRD
            sz = file_size(d / name)
            group_total += sz
            rows.append((sz, name))
        grand_total += group_total
        rows.sort(reverse=True)
        all_rows.append((group, group_total, rows))

    # Print grouped table
    print(f"  {'Dependency':<30} {'Size':>10}")
    print(f"  {'-'*30}   {'-'*10}")
    for group, group_total, rows in all_rows:
        for sz, name in rows:
            label = f"  {group} :: {name}"
            print(f"  {label:<33} {human(sz):>10}")
        if group_total:
            print(f"  {' ':<33} {human(group_total):>10}")
            print()

    # Grand total (libraries only)
    print(f"  {'──' * 20}")
    print(f"  {'Libraries total':<33} {human(grand_total):>10}")

    # Also note: SFML shared libs (not always present; if they exist, mention)
    sfml_shlibs = list(lib_dir.glob("libsfml*.so*"))
    if sfml_shlibs:
        shlib_total = sum(file_size(p) for p in sfml_shlibs)
        print(f"  {'SFML shared libs':<33} {human(shlib_total):>10}  "
              "(staged next to exe, not linked statically)")


def report_vendored() -> None:
    print(f"\n{'=' * 48}")
    print(f"  Vendored ThirdParty source (on disk)")
    print(f"{'=' * 48}\n")
    total = 0
    rows = []
    for name, path in VENDORED:
        if path.is_dir():
            sz = sum(f.stat().st_size for f in path.rglob("*") if f.is_file())
        elif path.is_file():
            sz = path.stat().st_size
        else:
            sz = 0
        total += sz
        rows.append((sz, name))
    rows.sort(reverse=True)
    for sz, name in rows:
        print(f"  {name:<20} {human(sz):>10}")
    print(f"  {'─' * 30}")
    print(f"  {'Total':<20} {human(total):>10}")


def report_fetch_content(build_dir: Path) -> None:
    deps_dir = build_dir / "_deps"
    if not deps_dir.is_dir():
        return
    print(f"\n{'=' * 48}")
    print(f"  Fetched dependencies (FetchContent cache)")
    print(f"{'=' * 48}\n")
    total = 0
    rows = []
    for dep in sorted(deps_dir.iterdir()):
        if dep.is_dir():
            sz = sum(f.stat().st_size for f in dep.rglob("*") if f.is_file())
            total += sz
            rows.append((sz, dep.name))
    rows.sort(reverse=True)
    for sz, name in rows:
        print(f"  {name:<30} {human(sz):>10}")
    print(f"  {'─' * 40}")
    print(f"  {'Total fetched':<30} {human(total):>10}")


if __name__ == "__main__":
    import sys

    configs = [c for c in ["Debug", "Release"] if LIBS[c].is_dir()]
    if not configs:
        print("No build directories found under Build/x64/.")
        print("Run `cmake --build Build` first.")
        sys.exit(1)

    for cfg in configs:
        report(cfg, LIBS[cfg])

    report_vendored()
    report_fetch_content(BUILD)
