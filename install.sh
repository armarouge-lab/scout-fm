#!/bin/sh
set -e

echo "-> Compiling Scout File Manager..."
make clean && make

echo "-> Installing binary to /usr/local/bin..."
cp scout /usr/local/bin/scout
chmod +x /usr/local/bin/scout

echo "-> Installing desktop shortcut..."
mkdir -p ~/.local/share/applications
cp scout.desktop ~/.local/share/applications/

echo "-> Scout File Manager installed successfully!"
