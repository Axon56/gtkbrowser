#!/bin/bash
# Run GTKBrowser in headless mode with Xvfb
set -e

SOCKET_PATH="${1:-/tmp/gtkbrowser.sock}"

# Start Xvfb if not already running
if ! pgrep -x Xvfb > /dev/null 2>&1; then
    echo "Starting Xvfb..."
    Xvfb :99 -screen 0 1280x800x24 &
    export DISPLAY=:99
    sleep 1
else
    export DISPLAY="${DISPLAY:-:99}"
fi

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
BINARY="${SCRIPT_DIR}/../build/gtkbrowser"

if [ ! -f "$BINARY" ]; then
    echo "Binary not found. Building first..."
    cd "${SCRIPT_DIR}/.."
    mkdir -p build && cd build
    cmake ..
    make -j$(nproc)
    BINARY="./gtkbrowser"
fi

echo "Starting GTKBrowser on $SOCKET_PATH..."
exec "$BINARY" --headless --socket "$SOCKET_PATH" "$@"
