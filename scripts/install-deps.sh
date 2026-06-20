#!/bin/bash
# Install dependencies for AxonSurf
set -e

echo "Installing AxonSurf dependencies..."

if command -v apt-get &>/dev/null; then
    # Debian/Ubuntu
    sudo apt-get update
    sudo apt-get install -y \
        build-essential \
        cmake \
        pkg-config \
        libgtk-3-dev \
        libwebkit2gtk-4.0-dev \
        libjson-glib-dev \
        libcurl4-openssl-dev \
        xvfb \
        dbus-x11

elif command -v dnf &>/dev/null; then
    # Fedora
    sudo dnf install -y \
        gcc \
        cmake \
        pkg-config \
        gtk3-devel \
        webkit2gtk4.0-devel \
        json-glib-devel \
        libcurl-devel \
        xorg-x11-server-Xvfb \
        dbus-x11

elif command -v pacman &>/dev/null; then
    # Arch
    sudo pacman -S --noconfirm \
        gcc \
        cmake \
        pkg-config \
        gtk3 \
        webkit2gtk \
        json-glib \
        curl \
        xorg-server-xvfb \
        dbus

else
    echo "Unsupported package manager. Install manually:"
    echo "  - GTK3 dev headers"
    echo "  - WebKitGTK 4.0 dev headers"
    echo "  - json-glib dev headers"
    echo "  - libcurl dev headers"
    echo "  - cmake, pkg-config, gcc"
    echo "  - Xvfb (for headless)"
    exit 1
fi

echo "Dependencies installed."
echo ""
echo "Build with:"
echo "  mkdir build && cd build"
echo "  cmake .."
echo "  make -j\$(nproc)"
echo ""
echo "Run with:"
echo "  ./axonsurf https://example.com"
echo "  echo 'click 100 200' | ./axonsurf"
