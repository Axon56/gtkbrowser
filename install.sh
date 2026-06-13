#!/bin/bash
set -e

BINARY_NAME="gtkbrowser"
INSTALL_DIR="${INSTALL_DIR:-/usr/local/bin}"
REPO="https://github.com/Axon56/gtkbrowser.git"
TMPDIR="/tmp/gtkbrowser_install_$$"

echo "🔧 GTKBrowser Installer"
echo "========================"
echo ""

# Detect distro
if command -v apt-get &>/dev/null; then
    PKG_MANAGER="apt"
elif command -v dnf &>/dev/null; then
    PKG_MANAGER="dnf"
elif command -v pacman &>/dev/null; then
    PKG_MANAGER="pacman"
elif command -v zypper &>/dev/null; then
    PKG_MANAGER="zypper"
else
    echo "❌ Could not detect package manager. Install manually:"
    echo "   GTK3, WebKitGTK, cmake, git, ffmpeg"
    exit 1
fi

echo "📦 Installing dependencies..."
case $PKG_MANAGER in
    apt)
        sudo apt-get update -qq
        sudo apt-get install -y -qq \
            build-essential cmake git pkg-config \
            libgtk-3-dev libwebkit2gtk-4.0-dev libjson-glib-dev libcurl4-openssl-dev \
            ffmpeg xvfb xclip
        ;;
    dnf)
        sudo dnf install -y \
            gcc cmake git make \
            gtk3-devel webkit2gtk4.0-devel json-glib-devel libcurl-devel \
            ffmpeg xorg-x11-server-Xvfb xclip
        ;;
    pacman)
        sudo pacman -S --noconfirm \
            base-devel cmake git \
            gtk3 webkit2gtk json-glib curl \
            ffmpeg xorg-server-xvfb xclip
        ;;
    zypper)
        sudo zypper install -y \
            gcc cmake git make \
            gtk3-devel webkit2gtk4.0-devel json-glib-devel libcurl-devel \
            ffmpeg xorg-x11-server-Xvfb xclip
        ;;
esac

echo ""
echo "📥 Cloning GTKBrowser..."
rm -rf "$TMPDIR"
git clone --depth 1 "$REPO" "$TMPDIR"

echo ""
echo "🔨 Building..."
cd "$TMPDIR"
mkdir -p build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX="$INSTALL_DIR"
make -j$(nproc)

echo ""
echo "📦 Installing to $INSTALL_DIR..."
sudo make install

echo ""
echo "🧹 Cleaning up..."
cd /
rm -rf "$TMPDIR"

# Verify installation
if command -v gtkbrowser &>/dev/null; then
    echo ""
    echo "✅ GTKBrowser installed successfully!"
    echo ""
    echo "   Binary: $(which gtkbrowser)"
    echo "   Version: $(gtkbrowser --help 2>&1 | head -1 || echo 'unknown')"
    echo ""
    echo "   Quick start:"
    echo "     gtkbrowser --headless https://example.com"
    echo ""
else
    echo ""
    echo "⚠️  GTKBrowser installed to $INSTALL_DIR"
    echo "   Add to PATH: export PATH=\"$INSTALL_DIR:\$PATH\""
    echo ""
    echo "   Quick start:"
    echo "     $INSTALL_DIR/$BINARY_NAME --headless https://example.com"
fi
