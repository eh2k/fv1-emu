/* FV1 Emulator
 * Copyright (C) 2018 - E.Heidt
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 * 
 * You should have received a copy of the GNU Affero General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

// http://www.spinsemi.com/knowledge_base/arch.html
// http://www.spinsemi.com/Products/appnotes/spn1001/AN-0001.pdf
// http://www.spinsemi.com/Products/datasheets/spn1001-dev/SPINAsmUserManual.pdf
// http://www.spinsemi.com/programs.php
// http://www.spinsemi.com/knowledge_base/cheat.html
// http://www.spinsemi.com/knowledge_base/inst_syntax.html
// http://www.spinsemi.com/forum/viewtopic.php?f=3&t=110&p=2227&hilit=CHO#p2227

#pragma once

#include <inttypes.h>
#include <cmath>
#include <cstring>
#include <array>
#include <vector>

constexpr int SIN0_RATE = 0x00;
constexpr int SIN0_RANGE = 0x01;
constexpr int SIN1_RATE = 0x02;
constexpr int SIN1_RANGE = 0x03;
constexpr int RMP0_RATE = 0x04;
constexpr int RMP0_RANGE = 0x05;
constexpr int RMP1_RATE = 0x06;
constexpr int RMP1_RANGE = 0x07;
constexpr int POT0 = 0x10;
constexpr int POT1 = 0x11;
constexpr int POT2 = 0x12;
constexpr int ADCL = 0x14;
constexpr int ADCR = 0x15;
constexpr int DACL = 0x16;
constexpr int DACR = 0x17;
constexpr int ADDR_PTR = 0x18;
constexpr int REG0 = 0x20;
constexpr int REG31 = 0x3f;
constexpr int MAX_DELAY_ADDR = 32767;

constexpr int SKP_NEG = 0x01;
constexpr int SKP_GEZ = 0x02;
constexpr int SKP_ZRO = 0x04;
constexpr int SKP_ZRC = 0x08;
constexpr int SKP_RUN = 0x10;

constexpr int CHO_LFO_SIN0 = 0;
constexpr int CHO_LFO_SIN1 = 1;
constexpr int CHO_LFO_RMP0 = 2;
constexpr int CHO_LFO_RMP1 = 3;
constexpr int CHO_LFO_4 = 4;
constexpr int CHO_LFO_5 = 5;
constexpr int CHO_LFO_6 = 6;
constexpr int CHO_LFO_7 = 7;
constexpr int CHO_LFO_COS0 = 8;
constexpr int CHO_LFO_COS1 = 9;
constexpr int CHO_SIN = 0x00;
constexpr int CHO_COS = 0x01;
constexpr int CHO_REG = 0x02;
constexpr int CHO_COMPC = 0x04;
constexpr int CHO_COMPA = 0x08;
constexpr int CHO_RPTR2 = 0x10;
constexpr int CHO_NA = 0x20;

#ifndef INLINE
#define INLINE inline
#endif

#if defined(__ARM_ARCH_7EM__)
inline int32_t SSAT24(int32_t value)
{
    int32_t result;
    __asm volatile("ssat %0, %1, %2"
                   : "=r"(result)
                   : "I"(24), "r"(value));
    return result;
}

inline int32_t MUL(int32_t x, int32_t y)
{
    static constexpr int z = 8;
    int32_t t, t2;
    asm("smull    %[t], %[t2], %[a], %[b]\n\t"
        "mov      %[t2], %[t2], asl %[c]\n\t"
        "orr      %[t], %[t2], %[t], lsr %[d]\n\t"
        : [t] "=&r"(t), [t2] "=&r"(t2)
        : [a] "r"(x), [b] "r"(y),
          [c] "Mr"((z) + 1), [d] "Mr"(31 - (z)));
    return t;
}

#else
template <typename T>
inline T clamp(T value, T min, T max)
{
    return value > max ? max : value < min ? min
                                           : value;
}

#define SSAT24(newValue) clamp(newValue, -(F), (F)-1)

// The 24x24 product is truncated towards minus infinity, which is what dropping
// the low bits in hardware does and what the __ARM_ARCH_7EM__ smull path above
// has always done. Dividing instead rounds towards zero, which differs by one
// LSB on negative products and costs a sign-correction sequence on every
// multiply. Define FV1_ROUND_TO_ZERO_MUL to get the old C behaviour back.
#ifdef FV1_ROUND_TO_ZERO_MUL
#define MUL(a, b) ((int32_t)(((int64_t)(a) * (b)) / FixedPoint::F))
#else
#define MUL(a, b) ((int32_t)(((int64_t)(a) * (b)) >> 23))
#endif

#endif

class FixedPoint
{
protected:
    int32_t value; // : 24;
public:
    static constexpr int F = (1 << 23);
    static constexpr int MAX = (F - 1);

    FixedPoint() : value(0)
    {
    }

    FixedPoint(int value) : value(value)
    {
    }

    FixedPoint(const FixedPoint &reg)
    {
        value = reg.value; //(reg.getValue());
    }

    INLINE static FixedPoint fromFloat(float value)
    {
        return FixedPoint(value * F);
    }

    INLINE float toFloat() const
    {
        return (float)this->value / F;
    }

    INLINE void set(int newValue)
    {
        value = SSAT24(newValue);
    }

    INLINE void set(const FixedPoint &newValue)
    {
        value = newValue.value;
    }

    INLINE int getValue() const
    {
        return value;
    }

    INLINE static int sat(int newValue)
    {
        return SSAT24(newValue);
    }

    INLINE static int fixSign(int newValue)
    {
        if (newValue & 0xFF800000)
            newValue |= 0xFF000000;
        else
            newValue &= 0x00FFFFFF;

        return newValue;
    }

    INLINE static int mul(const FixedPoint &a, const FixedPoint &b)
    {
        return MUL(a.getValue(), b.getValue());
    }

    INLINE static int mul_add(const FixedPoint &a, const FixedPoint &b, const FixedPoint &offset)
    {
        return MUL(a.getValue(), b.getValue()) + offset.getValue();
    }
};

#ifdef FV1_DUMP_STATE
#include <iomanip>
std::ostream &operator<<(std::ostream &os, const FixedPoint &m)
{
    return os << std::hex << "(" << std::setfill('0') << std::setw(8) << m.getValue() << "|" << m.toFloat() << ")";
}
#endif

class Reg : public FixedPoint
{
public:
    Reg() : FixedPoint() {}
    Reg(const FixedPoint &value) : FixedPoint(value) {}

    INLINE void clear()
    {
        value = 0;
    }

    INLINE void add(const FixedPoint &b)
    {
        set(getValue() + b.getValue());
    }

    INLINE void sub(const FixedPoint &b)
    {
        set(getValue() - b.getValue());
    }

    INLINE void mul(const FixedPoint &b)
    {
        set(FixedPoint::mul(*this, b));
    }

    INLINE void abs()
    {
        if (value < 0)
            value = 0 - value;
    }

    INLINE void not_()
    {
        set(FixedPoint::fixSign(~getValue()));
    }

    INLINE void and_(int mask)
    {
        set(FixedPoint::fixSign(getValue() & mask));
    }

    INLINE void or_(int mask)
    {
        set(FixedPoint::fixSign(getValue() | mask));
    }

    INLINE void xor_(int mask)
    {
        set(FixedPoint::fixSign(getValue() ^ mask));
    }
};

class SinLFO
{
    Reg sin_ = FixedPoint(0);
    Reg cos_ = FixedPoint(-FixedPoint::MAX);

    const Reg *regRange;
    const Reg *regRate;

public:
    INLINE void init(const Reg &rate, const Reg &range)
    {
        regRange = &range;
        regRate = &rate;
    }

    INLINE void increment()
    {
        int rate = regRate->getValue() >> 8;

        // A zero rate leaves both state words untouched (both updates add zero and
        // neither value can be out of range), so skip the two 24x24 multiplies.
        if (rate == 0)
            return;

        FixedPoint coeff(rate);
        cos_.add(FixedPoint::mul(sin_, coeff));
        sin_.sub(FixedPoint::mul(cos_, coeff));
    }

    INLINE void jam()
    {
        sin_.set(FixedPoint(0));
        cos_.set(FixedPoint(-FixedPoint::MAX));
    }

    INLINE int cos()
    {
        return FixedPoint::mul(cos_, *regRange);
    }

    INLINE int sin()
    {
        return FixedPoint::mul(sin_, *regRange);
    }

    INLINE int getValue(bool cosVal = false)
    {
        return cosVal ? cos() : sin();
    }
};

class RampLFO
{
    static constexpr int AMP_4096 = 0x3fffff;
    //	const int AMP_2048 = 0x1fffff;
    //	const int AMP_1024 = 0x0fffff;
    //	const int AMP_512 =  0x07ffff;

    int pos = 0;

    const Reg *regRange;
    const Reg *regRate;

public:
    static constexpr int AMP_OFFSET = 21;

    INLINE void init(const Reg &rate, const Reg &range)
    {
        regRange = &range;
        regRate = &rate;
    }

    INLINE void increment()
    {
        int freq = regRate->getValue() >> 12;
        pos = (pos - freq) & this->getRange();
    }

    INLINE void jam()
    {
        pos = 0;
    }

    INLINE int getValue(bool ptr2value = false)
    {
        int range = this->getRange();

        if (ptr2value)
            return (pos + (range / 2)) & range;
        else
            return pos;
    }

    INLINE int getRange()
    {
        return AMP_4096 >> (regRange->getValue() >> (AMP_OFFSET));
    }

    INLINE int getXFade()
    {
        int range = this->getRange();

        int halfAmp = range >> 1;
        int xfade = pos > halfAmp ? range - pos : pos;

        return xfade << (regRange->getValue() >> AMP_OFFSET) << 0; // push Amplitude to MAX ?
    }
};

class DelayMemory
{
    int16_t mem[MAX_DELAY_ADDR + 1] = {};
    unsigned int p = 0;

public:
    // int16_t << 8 always lands inside [-(1<<23), (1<<23)-1], so the caller can
    // use the result as an already saturated 24 bit value.
    INLINE int read(int offset) const
    {
        return mem[(offset + p) & MAX_DELAY_ADDR] << 8;
    }

    INLINE int get(int offset, Reg &lr)
    {
        int val = read(offset);
        lr.set(val);
        return val;
    }

    INLINE void set(int offset, int value)
    {
        mem[(offset + p) & MAX_DELAY_ADDR] = value >> 8;
    }

    INLINE void decrPtr()
    {
        --p;
    }
};

struct OP;

// 2^(16 * i / 2^23) in 24 bit fixed point - the kernel of the EXP instruction,
// which only ever calls it with a negative ACC, so the result lies in (0, 1].
//
// Split i = n * 2^19 + f, where n = floor(x) is a whole number of octaves in
// [-16, -1] and f is the fraction of an octave. 2^f comes from a 32 entry table
// times a cubic Taylor expansion of the remaining 1/32nd of an octave (tail
// below 1e-8, two orders finer than the 24th bit), and 2^n is the closing shift.
//
// This replaces a call to exp2f: about twice as fast, and because it never
// rounds through a 24 bit float mantissa it is also closer to the exact value -
// 0.17% of the 2^23 possible results are one LSB off the correctly truncated
// value, where exp2f was 3.1% off. Neither is ever off by more than one LSB.
inline int _exp2i(int i)
{
    static constexpr uint32_t OCTAVE[32] = { // 2^(k/32) in Q30
        1073741824u, 1097253708u, 1121280436u, 1145833280u,
        1170923762u, 1196563654u, 1222764986u, 1249540052u,
        1276901417u, 1304861917u, 1333434672u, 1362633090u,
        1392470869u, 1422962010u, 1454120821u, 1485961921u,
        1518500250u, 1551751076u, 1585730000u, 1620452965u,
        1655936265u, 1692196547u, 1729250827u, 1767116489u,
        1805811301u, 1845353420u, 1885761398u, 1927054196u,
        1969251188u, 2012372174u, 2056437387u, 2101467502u};

    // ln2, ln(2)^2/2 and ln(2)^3/6 in Q30
    static constexpr int64_t C1 = 744261118;
    static constexpr int64_t C2 = 257941248;
    static constexpr int64_t C3 = 59597083;

    if (i >= 0)
        return FixedPoint::F; // 2^0; anything above saturates to MAX anyway

    int n = i >> 19;               // whole octaves, -16..-1
    int f = i & 0x7FFFF;           // fraction of an octave
    int64_t u = f & 0x3FFF;        // residue within 1/32 octave, scaled by 2^19

    int64_t t = C2 + ((u * C3) >> 19);
    t = C1 + ((u * t) >> 19);
    t = (u * t) >> 19;             // 2^u - 1 in Q30

    uint32_t g = OCTAVE[f >> 14];
    g += (uint32_t)((g * t) >> 30); // 2^(f / 2^19) in Q30, stays below 2^31

    return (int)(g >> (7 - n));    // times 2^(23 + n), so a shift by 8..23
}

// Index of the highest set bit; v must not be zero.
inline int _msb(uint32_t v)
{
#if defined(__GNUC__) || defined(__clang__)
    return 31 - __builtin_clz(v);
#else
    int r = 0;
    if (v & 0xFFFF0000u) { v >>= 16; r += 16; }
    if (v & 0x0000FF00u) { v >>= 8;  r += 8;  }
    if (v & 0x000000F0u) { v >>= 4;  r += 4;  }
    if (v & 0x0000000Cu) { v >>= 2;  r += 2;  }
    if (v & 0x00000002u) { r += 1; }
    return r;
#endif
}

inline float fast_log2f(float val)
{
    // Kept for callers outside this header; _log2i no longer needs it. Reading
    // the float back through an int lvalue breaks strict aliasing, so the bits
    // go through memcpy, which every compiler folds away.
    int x;
    memcpy(&x, &val, sizeof(x));
    const int log_2 = ((x >> 23) & 255) - 128;
    x &= ~(255 << 23);
    x += 127 << 23;
    memcpy(&val, &x, sizeof(val));

    val = ((-1.0f / 3) * val + 2) * val - 2.0f / 3; // (1)

    return (val + log_2);
}

// log2(i / 2^23) / 16 in 24 bit fixed point - the kernel of the LOG instruction,
// which passes |ACC| and so keeps i within [1, 2^23].
//
// Write i = 2^e * m with m in [1,2); the result is then 2^19 * (e - 23 + log2(m)).
// The mantissa is split once more, at 1/32 of an octave: the table holds log2 of
// each of the 32 breakpoints m0 together with the cubic coefficients of log2
// about that point, so no division and no normalisation multiply is needed. The
// quartic tail is 3.4e-7, a fifth of one LSB of the result.
//
// What this replaces approximated log2 across a whole octave with a single
// quadratic. That is off by up to 0.0097 - some 5090 LSB here, or 0.06 dB of
// gain error once an EXP undoes the LOG. This version stays within 1 LSB for
// every input, at the same cost.
inline int _log2i(int i)
{
    struct Coeff
    {
        uint32_t log2m0;    // log2(m0), Q30
        int32_t a1, a2, a3; // 1/(m0 ln2), -1/(2 m0^2 ln2), 1/(3 m0^3 ln2), Q30
    };

    static constexpr Coeff TAB[32] = {
        {         0u, 1549082005, -774541002, 516360668},
        {  47667823u, 1502140126, -728310364, 470826902},
        {  93912511u, 1457959534, -686098604, 430493242},
        { 138816582u, 1416303547, -647453050, 394638050},
        { 182455581u, 1376961782, -611983014, 362656601},
        { 224898839u, 1339746599, -579349880, 334039571},
        { 266210141u, 1304490109, -549258993, 308355926},
        { 306448299u, 1271041645, -521452983, 285239238},
        { 345667660u, 1239265604, -495706241, 264376662},
        { 383918542u, 1209039613, -471820337, 245500013},
        { 421247625u, 1180252956, -449620174, 228378501},
        { 457698295u, 1152805213, -428950777, 212812789},
        { 493310944u, 1126605094, -409674580, 198630099},
        { 528123241u, 1101569426, -391669129, 185680180},
        { 562170370u, 1077622264, -374825135, 173831947},
        { 595485245u, 1054694131, -359044811, 162970694},
        { 628098702u, 1032721336, -344240445, 152995754},
        { 660039669u, 1011645391, -330333189, 143818531},
        { 691335320u,  991412483, -317251995, 135360851},
        { 722011213u,  971973023, -304932713, 127553553},
        { 752091421u,  953281234, -293317303, 120335304},
        { 781598637u,  935294795, -282353146, 113651581},
        { 810554283u,  917974521, -271992451, 107453808},
        { 838978604u,  901284075, -262191731, 101698611},
        { 866890747u,  885189717, -252911348,  96347180},
        { 894308843u,  869660073, -244115108,  91364719},
        { 921250079u,  854665934, -235769913,  86719968},
        { 947730758u,  840180070, -227845443,  82384793},
        { 973766362u,  826177069, -220313885,  78333826},
        { 999371606u,  812633183, -213149687,  74544153},
        {1024560487u,  799526196, -206329341,  70995042},
        {1049346328u,  786835304, -199831188,  67667704}};

    if (i <= 0)
        return -23 * (1 << 19); // log2(0); LOG never asks for it

    int e = _msb((uint32_t)i);
    uint32_t frac = ((uint32_t)i << (31 - e)) & 0x7FFFFFFFu; // m - 1, Q31
    const Coeff &c = TAB[frac >> 26];
    int64_t v = frac & 0x3FFFFFF; // m - m0, Q31

    int64_t t = c.a2 + ((v * c.a3) >> 31);
    t = c.a1 + ((v * t) >> 31);
    t = (v * t) >> 31; // log2(m) - log2(m0), Q30

    return (e - 23) * (1 << 19) + (int)((c.log2m0 + t) >> 11);
}

#ifdef MAKE_S

#include "../msfa/exp2.h"

inline int exp2i(int i)
{
    auto b = Exp2::lookup(i * 32) / 2;
    // auto a = _exp2i(i);
    // printf("%d -> %d %d %f\n", i, a, b, (float)a / b);

    return b;
}

inline int log2i(int i)
{
    auto b = _log2i(i);
    // auto a = log2_fixedpoint(i / 16.f);
    // printf("%d -> %d %d %f\n", i, a, b, (float)a / b);

    return b;
}

#else

#define exp2i _exp2i
#define log2i _log2i

#endif

class FV1
{
private:
public:
    Reg acc;
    Reg pacc;
    Reg lr;
    Reg regs[REG31 + 1] = {};
    DelayMemory delay;
    SinLFO sinLFO[2] = {};
    RampLFO rampLFO[2] = {};
    bool firstRun = true;

    void (*program)(FV1 *fv1) = nullptr;
public:
    void *rom;

    INLINE int ACC()
    {
        return acc.getValue();
    }

    INLINE void ACC2PACC()
    {
        pacc.set(acc);
    }

    INLINE const FixedPoint &REG(int reg)
    {
        return regs[reg];
    }

    INLINE void REG_SET(int reg, int value)
    {
        regs[reg].set(value);
    }

    INLINE void SOF(const FixedPoint &scale, const FixedPoint &offset)
    {
        ACC2PACC();
        this->acc.set(FixedPoint::mul_add(this->acc, scale, offset));
    }

    INLINE void AND(int mask)
    {
        ACC2PACC();
        this->acc.and_(mask);
    }

    INLINE void OR(int mask)
    {
        ACC2PACC();
        this->acc.or_(mask);
    }

    INLINE void XOR(int mask)
    {
        ACC2PACC();
        this->acc.xor_(mask);
    }

    INLINE void LOG(const FixedPoint &scale, const FixedPoint &offset)
    {
        ACC2PACC();
        int l;

        if (this->acc.getValue() > 0)
            l = log2i(this->acc.getValue());
        else if (this->acc.getValue() < 0)
            l = log2i(-this->acc.getValue());
        else
            l = -FixedPoint::MAX;

        this->acc.set(FixedPoint::mul_add(l, scale, offset));
        // this->acc.set(l);
        // this->acc.mul(scale);
        // this->acc.add(offset);
    }

    INLINE void EXP(const FixedPoint &scale, const FixedPoint &offset)
    {
        _EXP();
        this->acc.set(FixedPoint::mul_add(this->acc, scale, offset));
        // this->acc.mul(scale);
        // this->acc.add(offset);
    }

    INLINE void _EXP()
    {
        ACC2PACC();
        if (this->ACC() >= 0)
        {
            this->acc.set(FixedPoint(FixedPoint::MAX));
        }
        else
        {
            this->acc.set(exp2i(this->acc.getValue()));
        }
    }

    INLINE int SKP(int flags, int nskip)
    {
        bool skip = false;

        if (flags & SKP_RUN)
            skip |= firstRun == false;
        if (flags & SKP_ZRO)
            skip |= this->ACC() == 0;
        if (flags & SKP_GEZ)
            skip |= this->ACC() > 0;
        if (flags & SKP_NEG)
            skip |= this->ACC() < 0;
        if (flags & SKP_ZRC)
            skip |= (this->ACC() & FixedPoint::F) != (this->pacc.getValue() & FixedPoint::F);

        if (skip)
            return nskip;

        return 0;
    }

    INLINE void RDAX(int addr, const FixedPoint &scale)
    {
        _RDAX(&regs[addr], scale);
    }

    INLINE void _RDAX(Reg *addr, const FixedPoint &scale)
    {
        ACC2PACC();
        // Reg reg(*addr);
        // reg.mul(scale);
        // this->acc.add(reg);
        this->acc.set(FixedPoint::mul_add(*addr, scale, acc));
    }

    INLINE void _RDAX(Reg *addr)
    {
        ACC2PACC();
        this->acc.add(*addr);
    }

    INLINE void WRAX(int addr, const FixedPoint &scale)
    {
        _WRAX(&regs[addr], scale);
    }

    INLINE void _WRAX(Reg *addr, const FixedPoint &scale)
    {
        ACC2PACC();
        addr->set(this->acc);
        this->acc.mul(scale);
    }

    INLINE void _WRAX(Reg *addr)
    {
        ACC2PACC();
        addr->set(this->acc);
    }

    INLINE void MAXX(int addr, const FixedPoint &scale)
    {
        _MAXX(&regs[addr], scale);
    }

    INLINE void _MAXX(Reg *addr, const FixedPoint &scale)
    {
        ACC2PACC();
        Reg temp(*addr);
        temp.abs();
        temp.mul(scale);

        auto a = this->ACC();
        if (a < 0)
            a = -a;
        auto b = temp.getValue();
        if (b < 0)
            b = -b;
        this->acc.set(a > b ? a : b);
    }

    INLINE void MULX(int addr)
    {
        _MULX(&regs[addr]);
    }

    INLINE void _MULX(Reg *addr)
    {
        ACC2PACC();
        this->acc.mul(*addr);
    }

    INLINE void RDFX(int addr, const FixedPoint &scale)
    {
        _RDFX(&regs[addr], scale);
    }

    INLINE void _RDFX(Reg *addr, const FixedPoint &scale)
    {
        ACC2PACC();
        // this->acc.sub(*addr);
        // this->acc.mul(scale);
        // this->acc.add(*addr);
        this->acc.set(FixedPoint::mul_add(this->acc.getValue() - addr->getValue(), scale, *addr));
    }

    INLINE void WRLX(int addr, const FixedPoint &scale)
    {
        _WRLX(&regs[addr], scale);
    }

    INLINE void _WRLX(Reg *addr, const FixedPoint &scale) // ACC­>REG[ADDR], (PACC-­ACC) * C + PACC
    {
        int _pacc = this->pacc.getValue();
        ACC2PACC();
        addr->set(this->acc);
        // Reg tmp(this->pacc);
        // tmp.sub(this->acc);
        // tmp.mul(scale);
        // tmp.add(this->acc);
        // this->acc.set(tmp);
        this->acc.set(FixedPoint::mul_add(_pacc - this->acc.getValue(), scale, _pacc));
    }

    INLINE void WRHX(int addr, const FixedPoint &scale)
    {
        _WRHX(&regs[addr], scale);
    }

    INLINE void _WRHX(Reg *addr, const FixedPoint &scale) // ACC­>REG[ADDR], (ACC*C) + PACC
    {
        int _pacc = this->pacc.getValue();
        ACC2PACC();
        addr->set(this->acc);
        // this->acc.mul(scale);
        // this->acc.add(this->pacc);
        this->acc.set(FixedPoint::mul_add(acc, scale, _pacc));
    }

    INLINE void RDA(int addr, const FixedPoint &scale)
    {
        ACC2PACC();
        // Reg tmp(this->delay.get(addr, this->lr));
        // tmp.mul(scale);
        // this->acc.add(tmp);
        this->acc.set(FixedPoint::mul_add(this->delay.get(addr, this->lr), scale, this->acc));
    }

    INLINE void RMPA(const FixedPoint &scale)
    {
        ACC2PACC();
        Reg tmp(this->delay.get(this->REG(ADDR_PTR).getValue() >> 8, this->lr));
        tmp.mul(scale);
        this->acc.add(tmp);
    }

    INLINE void WRA(int addr, const FixedPoint &scale)
    {
        ACC2PACC();
        this->delay.set(addr, this->ACC());
        this->acc.mul(scale);
    }

    INLINE void WRAP(int addr, const FixedPoint &scale)
    {
        ACC2PACC();
        this->delay.set(addr, this->ACC());
        this->acc.mul(scale);
        this->acc.add(this->lr);
    }

    INLINE void WLDS(int lfo, int freq, int amp)
    {
        if (lfo == 0)
        {
            this->REG_SET(SIN0_RATE, freq << 14);
            this->REG_SET(SIN0_RANGE, amp << 8);
            this->sinLFO[0].jam();
        }
        else
        {
            this->REG_SET(SIN1_RATE, freq << 14);
            this->REG_SET(SIN1_RANGE, amp << 8);
            this->sinLFO[1].jam();
        }
    }

    INLINE void WLDR(int lfo, int freq, int amp)
    {
        amp <<= RampLFO::AMP_OFFSET;

        if (lfo == 0)
        {
            this->REG_SET(RMP0_RATE, freq << 8);
            this->REG_SET(RMP0_RANGE, amp);
            this->rampLFO[0].jam();
        }
        else
        {
            this->REG_SET(RMP1_RATE, freq << 8);
            this->REG_SET(RMP1_RANGE, amp);
            this->rampLFO[1].jam();
        }
    }

    INLINE void JAM(int lfo)
    {
        this->rampLFO[lfo].jam();
    }

    INLINE void CHO(int lfo, int flags, int *lfoval, int *scale)
    {
        static constexpr auto ONE = FixedPoint::MAX / 2;

        if (lfo == CHO_LFO_SIN0 || lfo == CHO_LFO_SIN1)
        {
            *lfoval = this->sinLFO[lfo].getValue(flags & CHO_COS);

            *scale = flags & CHO_COMPC ? ONE - *lfoval : *lfoval;

            if (flags & CHO_COMPA)
                *lfoval = -*lfoval;
        }
        else if (lfo == CHO_LFO_RMP0 || lfo == CHO_LFO_RMP1)
        {
            auto range = this->rampLFO[lfo - CHO_LFO_RMP0].getRange();
            *lfoval = this->rampLFO[lfo - CHO_LFO_RMP0].getValue(flags & CHO_RPTR2);

            if (flags & CHO_COMPA)
                *lfoval = range - *lfoval;

            if (flags & CHO_NA)
                *lfoval = this->rampLFO[lfo - CHO_LFO_RMP0].getXFade();

            *scale = flags & CHO_COMPC ? ONE - *lfoval : *lfoval;
        }

        // scale.set(scale.getValue() / 2);
    }

    INLINE void CHO_RDA(int lfo, int flags, int addr)
    {
        int lfoval = 0;
        int scale = 0;

        CHO(lfo, flags, &lfoval, &scale);

        if ((flags & CHO_NA) == 0)
            addr += (lfoval >> 10);

        RDA(addr, scale);
    }

    INLINE void CHO_SOF(int lfo, int flags, const FixedPoint &offset)
    {
        int lfoval = 0;
        int scale = 0;

        CHO(lfo, flags, &lfoval, &scale);
        SOF(scale, offset);
    }

    INLINE void CHO_RDAL(int lfo)
    {
        ACC2PACC();
        this->acc.set(CHO_RDAL_VALUE(lfo));
    }

    INLINE int CHO_RDAL_VALUE(int lfo)
    {
        int lfoval = 0;

        switch (lfo)
        {
        case CHO_LFO_SIN0:
            lfoval = this->sinLFO[0].sin();
            break;
        case CHO_LFO_SIN1:
            lfoval = this->sinLFO[1].sin();
            break;
        case CHO_LFO_RMP0:
            lfoval = this->rampLFO[0].getValue();
            break;
        case CHO_LFO_RMP1:
            lfoval = this->rampLFO[1].getValue();
            break;
        case CHO_LFO_COS0:
        case CHO_LFO_4:
            lfoval = this->sinLFO[0].cos();
            break;
        case CHO_LFO_COS1:
        case CHO_LFO_5:
            lfoval = this->sinLFO[1].cos();
            break;
        default:
            break;
        }

        return lfoval;
    }

    INLINE void CLR()
    {
        ACC2PACC();
        this->acc.clear();
    }

    INLINE void NOT()
    {
        ACC2PACC();
        this->acc.not_();
    }

    INLINE void ABSA()
    {
        ACC2PACC();
        this->acc.abs();
    }

    INLINE void LDAX(int addr)
    {
        _LDAX(&regs[addr]);
    }

    INLINE void _LDAX(Reg *addr)
    {
        ACC2PACC();
        this->acc.set(*addr);
    }

public:
    FV1() : rom(nullptr)
    {
        this->sinLFO[0].init(this->regs[SIN0_RATE], this->regs[SIN0_RANGE]);
        this->sinLFO[1].init(this->regs[SIN1_RATE], this->regs[SIN1_RANGE]);
        this->rampLFO[0].init(this->regs[RMP0_RATE], this->regs[RMP0_RANGE]);
        this->rampLFO[1].init(this->regs[RMP1_RATE], this->regs[RMP1_RANGE]);
    }

#ifdef FV1_DUMP_STATE
    void dump(std::string &str, const std::string &endl = "")
    {
        std::ostringstream out;

        out << " ACC: " << std::hex << this->acc.getValue() << endl
            << " POT0: " << this->REG(POT0) << endl
            << " POT1: " << this->REG(POT1) << endl
            << " POT2: " << this->REG(POT2) << endl
            << " RMP0_RATE: " << this->REG(RMP0_RATE) << endl
            << " RMP0_RANGE: " << this->REG(RMP0_RANGE) << endl
            << " RMP0_AMP: " << FixedPoint(this->rampLFO[0].getRange()) << endl
            << " RMP0_VALUE: " << FixedPoint(this->rampLFO[0].getValue()) << endl
            << " RMP0_XFADE " << FixedPoint(this->rampLFO[0].getXFade()) << endl
            << " RMP1_RATE: " << FixedPoint(this->REG(RMP1_RATE)) << endl
            << " RMP1_RANGE: " << FixedPoint(this->REG(RMP1_RANGE)) << endl
            << " RMP1_VALUE: " << FixedPoint(this->rampLFO[1].getValue()) << endl
            << " RMP1_XFADE " << FixedPoint(this->rampLFO[1].getXFade()) << endl
            << " SIN0_RATE: " << this->REG(SIN0_RATE) << endl
            << " SIN0_RANGE: " << this->REG(SIN0_RANGE) << endl
            << " SIN0_SIN: " << FixedPoint(this->sinLFO[0].getValue()) << endl
            << " SIN1_RATE: " << this->REG(SIN1_RATE) << endl
            << " SIN1_RANGE: " << this->REG(SIN1_RANGE) << endl
            << " SIN1_SIN: " << FixedPoint(this->sinLFO[1].getValue()) << endl
            << " DACL: " << this->REG(DACL) << endl
            << " DACR: " << this->REG(DACR) << endl;

        for (int i = REG0; i < REG31; i++)
            out << "REG" << i << ": " << this->regs[i] << endl;

        out << endl;

        str += out.str();
    }
#endif

    void execute(float inL, float inR, float pot0, float pot1, float pot2, float &outL, float &outR)
    {
        execute(&inL, &inR, pot0, pot1, pot2, &outL, &outR, 1);
    }

    void execute(const float *inL, const float *inR, float pot0, float pot1, float pot2, float *outL, float *outR, size_t size)
    {
        for (size_t i = 0; i < size; ++i)
        {
            this->REG_SET(ADCL, FixedPoint::fromFloat(inL[i]).getValue());
            this->REG_SET(ADCR, FixedPoint::fromFloat(inR[i]).getValue());
            this->REG_SET(POT0, FixedPoint::fromFloat(pot0).getValue());
            this->REG_SET(POT1, FixedPoint::fromFloat(pot1).getValue());
            this->REG_SET(POT2, FixedPoint::fromFloat(pot2).getValue());

            program(this);

            firstRun = false;

            this->delay.decrPtr();

            this->sinLFO[0].increment();
            this->sinLFO[1].increment();
            this->rampLFO[0].increment();
            this->rampLFO[1].increment();

            outL[i] = this->REG(DACL).toFloat();
            outR[i] = this->REG(DACR).toFloat();
        }
    }

    void setFx(void (*program)(FV1 *fv1))
    {

        this->firstRun = true;
        this->acc.clear();
        this->pacc.clear();

        for (int i = REG0; i < REG31; i++)
            this->regs[i].clear();

        this->program = program;
    }
};
