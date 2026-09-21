#!/bin/sh
# Builds the Cortex-M7 benchmark twice: once against the working tree and once
# against the headers at git HEAD, so the two can be compared directly.
set -e
cd "$(dirname "$0")/.."
ROOT=$(cd .. && pwd)
GCC=$(echo tools/xpack-arm-none-eabi-gcc-*/bin)
CXX=$GCC/arm-none-eabi-g++
SIZE=$GCC/arm-none-eabi-size

[ -x "$CXX" ] || { echo "toolchain missing - run ./fetch-tools.sh" >&2; exit 1; }
[ -d spn ] || { echo "programs missing - run ./fetch-programs.sh" >&2; exit 1; }

mkdir -p build/baseline
[ -x host/spn2rom.exe ] || g++ -O2 -std=c++17 -I"$ROOT" host/spn2rom.cpp -o host/spn2rom.exe
./host/spn2rom.exe build/programs.h spn/*.spn

# headers as of HEAD, for the before/after comparison
for f in FV1.hpp FV1_DASM.hpp; do (cd "$ROOT" && git show HEAD:$f) > build/baseline/$f; done

CPU="-mcpu=cortex-m7 -mthumb -mfloat-abi=hard -mfpu=fpv5-sp-d16"
OPT="-O2 -std=c++17 -fno-exceptions -fno-rtti -fno-threadsafe-statics"
OPT="$OPT -ffunction-sections -fdata-sections -Wall"
LNK="-specs=nosys.specs -nostartfiles -T m7/mps2-an500.ld -Wl,--gc-sections"

build() { # name include-dir
    $CXX $CPU $OPT -Ibuild -I"$2" $LNK -Wl,-Map,"build/$1.map" \
         m7/bench_m7.cpp m7/startup.c -o "build/$1.elf"
    printf '%-10s ' "$1"
    $SIZE "build/$1.elf" | tail -1
}

build current  "$ROOT"
build baseline build/baseline
