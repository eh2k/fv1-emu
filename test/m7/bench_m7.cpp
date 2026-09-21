// FV-1 emulator throughput on a Cortex-M7, measured under QEMU (mps2-an500).
//
// QEMU does not model the M7 pipeline, caches or flash wait states, so this is
// not a cycle accurate figure. Run with -icount shift=0: QEMU then advances the
// virtual clock by exactly 1 ns per executed instruction, and since SysTick runs
// off that virtual clock the tick count is directly proportional to the number
// of instructions retired. That makes the before/after ratio meaningful and the
// absolute value a usable estimate (an M7 out of ITCM retires roughly one
// instruction per cycle on code like this).
#include <stdint.h>
#include <string.h>
#include "FV1.hpp"
#include "FV1_DASM.hpp"
#include "programs.h"

extern "C" void sh_write0(const char *s);
extern "C" void systick_init(void);
extern "C" uint64_t systick_now(void);

#ifndef SAMPLES
#define SAMPLES 20000
#endif
#define BLOCK 64

static char buf[256];
static char *put(char *p, const char *s) { while (*s) *p++ = *s++; return p; }
static char *putu(char *p, uint64_t v, int width)
{
    char t[24]; int n = 0;
    do { t[n++] = '0' + (int)(v % 10); v /= 10; } while (v);
    for (int i = n; i < width; ++i) *p++ = ' ';
    while (n) *p++ = t[--n];
    return p;
}
static char *putpad(char *p, const char *s, int width)
{
    int n = 0; while (s[n]) n++;
    p = put(p, s);
    for (int i = n; i < width; ++i) *p++ = ' ';
    return p;
}
// v is a fixed point value with `frac` decimal digits
static char *putfix(char *p, uint64_t v, int frac, int width)
{
    uint64_t scale = 1; for (int i = 0; i < frac; ++i) scale *= 10;
    char t[32]; int n = 0;
    uint64_t f = v % scale, w = v / scale;
    for (int i = 0; i < frac; ++i) { t[n++] = '0' + (int)(f % 10); f /= 10; }
    t[n++] = '.';
    do { t[n++] = '0' + (int)(w % 10); w /= 10; } while (w);
    for (int i = n; i < width; ++i) *p++ = ' ';
    while (n) *p++ = t[--n];
    return p;
}

static FV1 fv1;

int main(void)
{
    systick_init();

    // Calibration: a loop of known length tells us how many instructions one
    // SysTick tick stands for, so the numbers below do not depend on guessing
    // the board clock.
    uint64_t c0 = systick_now();
    volatile int sink = 0;
    for (int i = 0; i < 1000000; ++i) sink += i; // 1e6 iterations
    uint64_t c1 = systick_now();

    char *p = buf;
    p = put(p, "calibration: 1e6 loop iterations = ");
    p = putu(p, c1 - c0, 0);
    p = put(p, " ticks\r\n\r\n");
    p = put(p, "program                  ticks/sample   instr/sample (est)\r\n");
    *p = 0; sh_write0(buf);

    uint64_t totalTicks = 0;

    for (int a = 0; a < PROGRAM_COUNT; ++a)
    {
        fv1 = FV1();
        fv1_load(&fv1, PROGRAMS[a].rom, 0);

        float oL[BLOCK], oR[BLOCK];
        uint32_t h = 2166136261u;
        int si = 0;

        uint64_t t0 = systick_now();
        for (int i = 0; i + BLOCK <= SAMPLES; i += BLOCK)
        {
            for (int k = 0; k < BLOCK; ++k)
            {
                fv1.execute(SIGNAL_L[si], SIGNAL_R[si], 0.3f, 0.7f, 0.5f, oL[k], oR[k]);
                si = (si + 1) & (SIGNAL_LEN - 1);
            }
            for (int k = 0; k < BLOCK; ++k)
            {
                uint32_t v; memcpy(&v, &oL[k], 4); h = (h ^ v) * 16777619u;
                memcpy(&v, &oR[k], 4); h = (h ^ v) * 16777619u;
            }
        }
        uint64_t t1 = systick_now();
        uint64_t ticks = t1 - t0;
        totalTicks += ticks;

        int n = (SAMPLES / BLOCK) * BLOCK;
        p = buf;
        p = putpad(p, PROGRAMS[a].name, 24);
        p = putfix(p, ticks * 1000 / n, 3, 12);       // ticks per sample
        p = putu(p, ticks * 40 / n, 15);              // 40 instr per tick at 25 MHz
        p = put(p, "   hash=");
        p = putu(p, h, 0);
        p = put(p, "\r\n");
        *p = 0; sh_write0(buf);
    }

    p = buf;
    p = put(p, "\r\ntotal ticks: ");
    p = putu(p, totalTicks, 0);
    p = put(p, "\r\n");
    *p = 0; sh_write0(buf);
    return 0;
}
