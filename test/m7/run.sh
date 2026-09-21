#!/bin/sh
# Runs the Cortex-M7 benchmark under QEMU. -icount shift=0 makes the virtual
# clock advance one nanosecond per retired instruction, which is what turns the
# guest's SysTick reading into an instruction count.
set -e
cd "$(dirname "$0")/.."
QEMU=$(echo tools/xpack-qemu-arm-*/bin)/qemu-system-arm.exe
[ -x "$QEMU" ] || { echo "qemu missing - run ./fetch-tools.sh" >&2; exit 1; }

run() {
    echo "=== $1 ==="
    "$QEMU" -M mps2-an500 -cpu cortex-m7 -nographic -monitor none -serial none \
            -semihosting-config enable=on,target=native \
            -icount shift=0,sleep=off \
            -kernel "build/$1.elf"
    echo
}

if [ $# -gt 0 ]; then run "$1"; else run baseline; run current; fi
