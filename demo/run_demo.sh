#!/usr/bin/env bash
# ===================================================================
# QuartzDB Demo Runner (Bash / Linux / macOS)
# ===================================================================

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_DIR="$(dirname "$SCRIPT_DIR")"

CLI_PATH=""
for p in "$REPO_DIR/build/quartz" "$REPO_DIR/out/build/x64-Debug/quartz" "$REPO_DIR/build/Debug/quartz"; do
    if [ -x "$p" ]; then
        CLI_PATH="$p"
        break
    fi
done

if [ -z "$CLI_PATH" ]; then
    echo "[!] quartz executable not found. Build it first with:"
    echo "    cmake -B build && cmake --build build --target quartz quartz_c_demo"
    exit 1
fi

echo "==================================================================="
echo "                QuartzDB Automated Demonstration"
echo "==================================================================="
echo "CLI Binary: $CLI_PATH"
echo ""

DEMO_DB="$SCRIPT_DIR/demo_quickstart.db"
rm -f "$DEMO_DB"

echo "[1/2] Running SQL Quickstart Demo (demo/quickstart.sql)..."
"$CLI_PATH" "$DEMO_DB" < "$SCRIPT_DIR/quickstart.sql"

echo ""
echo "[2/2] Checking for compiled C API Demo (quartz_c_demo)..."
CDEMO_PATH=""
for p in "$REPO_DIR/build/quartz_c_demo" "$REPO_DIR/out/build/x64-Debug/quartz_c_demo"; do
    if [ -x "$p" ]; then
        CDEMO_PATH="$p"
        break
    fi
done

if [ -n "$CDEMO_PATH" ]; then
    echo "Running $CDEMO_PATH..."
    "$CDEMO_PATH"
else
    echo "Notice: quartz_c_demo not built yet. Build via: cmake --build build --target quartz_c_demo"
fi

echo ""
echo "Demonstrations finished!"
