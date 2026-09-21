#!/bin/sh
# Downloads the .spn corpus the benchmarks run on into test/spn/.
set -e
cd "$(dirname "$0")"
mkdir -p spn
BASE=https://raw.githubusercontent.com/mstratman/fv1-programs/master/docs/files
for f in dattorro.spn chorus-dual-rate.spn crusher.spn RevRev.spn aliaser.spn \
         8_seconds_delay.spn bm-pi.spn drolo/flanger.spn drolo/envelope-phaser.spn \
         dervish/dual-lfo-chorus.spn; do
    n=$(basename "$f")
    [ -f "spn/$n" ] || curl -fL --retry 3 -o "spn/$n" "$BASE/$f"
done
[ -f spn/demo.spn ] || curl -fL --retry 3 -o spn/demo.spn \
    https://raw.githubusercontent.com/eh2k/vcvrack-fv1-emu/master/fx/demo.spn
ls -l spn
