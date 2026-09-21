// Benchmark + golden-output harness for fv1-emu
#include <cstdio>
#include <cstdint>
#include <cmath>
#include <cstring>
#include <chrono>
#include <vector>
#include <string>
#include "FV1emu.hpp"

static const int BLOCK = 64;
static const int SAMPLES = 400000;  // per program

static std::vector<float> g_inL, g_inR, g_p0, g_p1, g_p2;

static void makeSignal()
{
    g_inL.resize(SAMPLES); g_inR.resize(SAMPLES);
    g_p0.resize(SAMPLES / BLOCK); g_p1.resize(SAMPLES / BLOCK); g_p2.resize(SAMPLES / BLOCK);
    double phase = 0;
    for (int i = 0; i < SAMPLES; ++i) {
        g_inL[i] = 0.6f * sinf((float)phase) + 0.25f * sinf((float)phase * 7.3f);
        g_inR[i] = 0.5f * sinf((float)phase * 1.7f) - 0.3f * sinf((float)phase * 0.13f);
        phase += 0.031415926;
    }
    for (int b = 0; b < SAMPLES / BLOCK; ++b) {
        g_p0[b] = 0.5f + 0.49f * sinf(b * 6.4e-3f);
        g_p1[b] = 0.5f + 0.49f * sinf(b * 1.9e-2f);
        g_p2[b] = 0.5f + 0.49f * sinf(b * 4.5e-3f);
    }
}

struct Res { uint64_t hash; double ms; };

static Res runProgram(const char *path, int n)
{
    FV1emu fx;
    if (!fx.load(path)) { printf("LOAD FAIL %s\n", path); return {0, 0}; }

    float outL[BLOCK], outR[BLOCK];
    uint64_t h = 1469598103934665603ull;

    auto t0 = std::chrono::high_resolution_clock::now();
    for (int i = 0, b = 0; i + BLOCK <= n; i += BLOCK, ++b)
    {
        float p0 = g_p0[b], p1 = g_p1[b], p2 = g_p2[b];
        for (int k = 0; k < BLOCK; ++k)
            fx.run(g_inL[i + k], g_inR[i + k], p0, p1, p2, outL[k], outR[k]);
        for (int k = 0; k < BLOCK; ++k) {
            uint32_t a, c;
            memcpy(&a, &outL[k], 4); memcpy(&c, &outR[k], 4);
            h = (h ^ a) * 1099511628211ull;
            h = (h ^ c) * 1099511628211ull;
        }
    }
    auto t1 = std::chrono::high_resolution_clock::now();
    return { h, std::chrono::duration<double, std::milli>(t1 - t0).count() };
}

int main(int argc, char **argv)
{
    makeSignal();
    double total = 0;
    for (int i = 1; i < argc; ++i)
    {
        runProgram(argv[i], SAMPLES / 20);   // warmup
        Res best{0, 1e300};
        for (int r = 0; r < 5; ++r) {
            Res x = runProgram(argv[i], SAMPLES);
            if (x.ms < best.ms) best.ms = x.ms;
            best.hash = x.hash;
        }
        total += best.ms;
        const char *nm = strrchr(argv[i], '/'); nm = nm ? nm + 1 : argv[i];
        fprintf(stderr, "%-26s %8.2f ms  %6.1f ns/smp  hash=%016llx\n",
                nm, best.ms, best.ms * 1e6 / SAMPLES, (unsigned long long)best.hash);
    }
    fprintf(stderr, "%-26s %8.2f ms\n", "TOTAL", total);
    return 0;
}
