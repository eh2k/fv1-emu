// Differential harness: random FV-1 ROM images, full machine state hashed every
// sample. Prints one line per program to stderr (unbuffered) so a crash still
// shows how far it got.
#include <cstdio>
#include <cstdint>
#include <cstdlib>
#include <cmath>
#include <cstring>
#include "FV1.hpp"
#include "FV1_DASM.hpp"

static uint64_t rs;
static uint32_t rnd() { rs ^= rs << 13; rs ^= rs >> 7; rs ^= rs << 17; return (uint32_t)(rs >> 32); }

static const uint32_t OPS[] = {
    OP_RDA, OP_RMPA, OP_WRA, OP_WRAP, OP_RDAX, OP_RDFX, OP_WRAX, OP_WRHX,
    OP_WRLX, OP_MAXX, OP_MULX, OP_LOG, OP_EXP, OP_SOF, OP_AND, OP_OR,
    OP_XOR, OP_SKP, OP_WLDS, OP_JAM, OP_CHO_RDA };

static FV1 fv1;

static inline void mix(uint64_t &h, uint32_t v) { h = (h ^ v) * 1099511628211ull; }

int main(int argc, char **argv)
{
    int iters = argc > 1 ? atoi(argv[1]) : 300;
    uint64_t seed = argc > 2 ? strtoull(argv[2], 0, 0) : 1;
    bool safeSkp = getenv("SAFE_SKP") != nullptr;
    rs = seed * 0x9E3779B97F4A7C15ull + 0x243F6A8885A308D3ull;
    void *prev = nullptr;

    for (int t = 0; t < iters; ++t)
    {
        uint8_t rom[512];
        int n = 4 + (rnd() % 124);
        memset(rom, 0, sizeof(rom));
        for (int i = 0; i < n; ++i)
        {
            uint32_t w;
            for (;;)
            {
                uint32_t code = OPS[rnd() % (sizeof(OPS) / sizeof(OPS[0]))];
                w = (rnd() & ~0x1Fu) | code;
                if (code == OP_WLDS && (rnd() & 1)) w |= 0x40000000u;
                if (code == OP_CHO_RDA) {
                    uint32_t v = rnd() % 3;
                    w = (w & 0x3FFFFFFFu) | (v == 1 ? 0x80000000u : v == 2 ? 0xC0000000u : 0u);
                }
                // Keep every branch target inside the 128 word program: the
                // unpatched upstream interpreter has no bounds check and walks
                // off its op buffer otherwise.
                if (safeSkp && code == OP_SKP) {
                    uint32_t nskip = (rnd() % (uint32_t)(128 - i)) & 0x3F;
                    w = (w & ~0x07E00000u) | (nskip << 21);
                }
                // 0x11 terminates the program and 0 is the padding word - both
                // would change the program length under us.
                if (w != 0x11 && w != 0) break;
            }
            rom[i * 4 + 0] = w >> 24; rom[i * 4 + 1] = w >> 16;
            rom[i * 4 + 2] = w >> 8;  rom[i * 4 + 3] = w;
        }

        fv1 = FV1();
        if (prev) free(prev);
        prev = fv1_load(&fv1, rom, nullptr);

        uint64_t h = 1469598103934665603ull;
        for (int i = 0; i < 2000; ++i)
        {
            float l, r;
            float in = sinf(i * 0.07f) * 0.8f;
            fv1.execute(in, -in * 0.6f, 0.3f, 0.8f, 0.55f, l, r);

            // hash the whole observable machine, not just the DACs - random
            // programs rarely write DACL/DACR at all
            uint32_t a, b; memcpy(&a, &l, 4); memcpy(&b, &r, 4);
            mix(h, a); mix(h, b);
            mix(h, (uint32_t)fv1.acc.getValue());
            mix(h, (uint32_t)fv1.pacc.getValue());
            mix(h, (uint32_t)fv1.lr.getValue());
            for (int k = 0; k <= REG31; ++k)
                mix(h, (uint32_t)fv1.regs[k].getValue());
        }
        fprintf(stderr, "%d %d %016llx\n", t, n, (unsigned long long)h);
    }
    return 0;
}
