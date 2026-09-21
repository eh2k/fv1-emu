# Benchmarks

Reproducible throughput measurements for the FV-1 emulator, on the host and on a
Cortex-M7. Nothing here is required to use the emulator; the whole directory can
be deleted.

## Setup (once)

    ./fetch-programs.sh    # .spn corpus -> test/spn/
    ./fetch-tools.sh       # arm-none-eabi-gcc + qemu -> test/tools/  (~500 MB)

`fetch-tools.sh` installs nothing system wide. Remove `test/tools` to undo it.

## Host

    ./host/run.sh

Throughput over the corpus, plus a randomised differential test that checks the
loader's instruction folding against an unfolded build over 2400 random ROM
images (full machine state hashed every sample).

## Cortex-M7

    ./m7/build.sh          # builds build/current.elf and build/baseline.elf
    ./m7/run.sh            # runs both under QEMU

`build.sh` builds the same benchmark twice: once against the working tree and
once against the headers at `git HEAD`, so the two are directly comparable.
`.spn` files are assembled to ROM images on the host (`host/spn2rom.cpp`) and
linked into the image, so the target needs no filesystem, no `libm` and no SPN
parser - it exercises `fv1_load()` and `FV1::execute()` only.

Target is QEMU's `mps2-an500` (Cortex-M7), built `-mcpu=cortex-m7 -mfloat-abi=hard
-mfpu=fpv5-sp-d16 -O2`, which selects the `__ARM_ARCH_7EM__` code path in
`FV1.hpp` (the `smull`/`ssat` inline assembly).

### How the numbers are obtained

QEMU does **not** model the M7 pipeline, caches or flash wait states, so it
cannot give cycle counts. `run.sh` passes `-icount shift=0`, which makes QEMU
advance the virtual clock by exactly one nanosecond per retired instruction.
SysTick runs off that virtual clock at 25 MHz, so one tick is 40 instructions,
and the guest's own SysTick reading becomes an exact instruction count.

The benchmark prints a calibration line first: a loop that compiles to exactly
six instructions, run a million times. It must report 150000 ticks
(6e6 / 40) - if it does not, the 40 instructions per tick scaling is wrong and
the rest of the output should not be trusted.

So the **ratios are exact** and the absolute instruction counts are exact; only
the translation to cycles is an estimate (an M7 running from ITCM retires
roughly one instruction per cycle on code of this shape).

### Reading the output

`hash` is an FNV-1a over every output sample. It is expected to be identical
between `baseline` and `current` except where a change was intended:

  * `dual_lfo_chorus` - the loader now remaps SKP offsets across folded
    instructions, which the old one did not.
  * `envelope_phaser` - the only corpus program that uses `LOG`.

`demo` uses `EXP` but its hash does not move, because at the fixed pot settings
used here the value reaching `EXP` is constant and old and new `exp2` agree on it.
