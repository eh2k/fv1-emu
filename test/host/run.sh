#!/bin/sh
# Host (x86) throughput benchmark and the randomised loader/interpreter
# differential test. Same corpus as the Cortex-M7 benchmark.
set -e
cd "$(dirname "$0")/.."
ROOT=$(cd .. && pwd)
mkdir -p build
[ -d spn ] || { echo "programs missing - run ./fetch-programs.sh" >&2; exit 1; }

g++ -O2 -std=c++17 -I"$ROOT" host/bench.cpp -o build/bench.exe
echo "=== throughput ==="
./build/bench.exe spn/*.spn 2>&1 >/dev/null

# folding on vs off must be bit identical
mkdir -p build/nofold
cp "$ROOT"/FV1.hpp "$ROOT"/FV1_DASM.hpp build/nofold/
sed -i -e 's|if (pad\[i\] && i > 0 && pad\[i - 1\])|if (false)|' \
       -e 's|if (kind\[i\] == OP_RDAX && !target\[i\] && j > 0 &&|if (false \&\& j > 0 \&\&|' \
       build/nofold/FV1_DASM.hpp
g++ -O2 -std=c++17 -I"$ROOT"        host/difftest.cpp -o build/dt_fold.exe
g++ -O2 -std=c++17 -Ibuild/nofold   host/difftest.cpp -o build/dt_nofold.exe
echo
echo "=== loader folding on vs off, 8 x 300 random programs ==="
for s in 1 2 3 4 5 6 7 8; do
    ./build/dt_fold.exe   300 $s 2> build/a.txt > /dev/null
    ./build/dt_nofold.exe 300 $s 2> build/b.txt > /dev/null
    if cmp -s build/a.txt build/b.txt; then printf "seed %s: ok  " $s; else printf "seed %s: DIFFER  " $s; fi
done
echo
