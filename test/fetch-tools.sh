#!/bin/sh
# Downloads the cross toolchain and the emulator used by the Cortex-M7
# benchmark into test/tools/. Nothing is installed system wide - everything
# lands under this directory and can be removed with `rm -rf test/tools`.
set -e
cd "$(dirname "$0")"

GCC_VER=14.2.1-1.1
QEMU_VER=8.2.6-1
GCC_ZIP=xpack-arm-none-eabi-gcc-$GCC_VER-win32-x64.zip
QEMU_ZIP=xpack-qemu-arm-$QEMU_VER-win32-x64.zip
GCC_URL=https://github.com/xpack-dev-tools/arm-none-eabi-gcc-xpack/releases/download/v$GCC_VER/$GCC_ZIP
QEMU_URL=https://github.com/xpack-dev-tools/qemu-arm-xpack/releases/download/v$QEMU_VER/$QEMU_ZIP

mkdir -p tools
cd tools

fetch() { # url file dir
    if [ -d "$3" ]; then echo "$3 already present, skipping"; return; fi
    if [ ! -f "$2" ]; then
        echo "downloading $2 ..."
        curl -fL --retry 3 --progress-bar -o "$2" "$1"
    fi
    echo "unpacking $2 ..."
    if command -v unzip >/dev/null 2>&1; then
        unzip -q "$2"
    elif [ -x /c/Windows/System32/tar.exe ]; then
        /c/Windows/System32/tar.exe -xf "$2"   # bsdtar reads zip, GNU tar does not
    else
        powershell.exe -NoProfile -Command "Expand-Archive -Force '$2' ." 
    fi
    rm -f "$2"
}

fetch "$GCC_URL"  "$GCC_ZIP"  "xpack-arm-none-eabi-gcc-$GCC_VER"
fetch "$QEMU_URL" "$QEMU_ZIP" "xpack-qemu-arm-$QEMU_VER"

echo
echo "toolchain: $(pwd)/xpack-arm-none-eabi-gcc-$GCC_VER/bin"
echo "qemu:      $(pwd)/xpack-qemu-arm-$QEMU_VER/bin"
