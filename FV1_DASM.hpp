/* FV1 Emulator
 * Copyright (C) 2018 - E.Heidt
 *
 *   This program is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#pragma once

#include <stdio.h>
#include <set>
#define SPN_PARSER
#include "FV1.hpp"

constexpr uint32_t OP_RDA = 0x00;
constexpr uint32_t OP_RMPA = 0x01;
constexpr uint32_t OP_WRA = 0x02;
constexpr uint32_t OP_WRAP = 0x03;
constexpr uint32_t OP_RDAX = 0x04;
constexpr uint32_t OP_RDFX = 0x05;
constexpr uint32_t OP_WRAX = 0x06;
constexpr uint32_t OP_WRHX = 0x07;
constexpr uint32_t OP_WRLX = 0x08;
constexpr uint32_t OP_MAXX = 0x09;
constexpr uint32_t OP_MULX = 0x0a;
constexpr uint32_t OP_LOG = 0x0b;
constexpr uint32_t OP_EXP = 0x0c;
constexpr uint32_t OP_SOF = 0x0d;
constexpr uint32_t OP_AND = 0x0e;
constexpr uint32_t OP_OR = 0x0f;
constexpr uint32_t OP_XOR = 0x10;
constexpr uint32_t OP_SKP = 0x11;
constexpr uint32_t OP_WLDS = 0x12;
constexpr uint32_t OP_JAM = 0x13;
constexpr uint32_t OP_CHO_RDA = 0x00000014;

constexpr uint32_t OP_WLDR = 0x40000012;
constexpr uint32_t OP_CHO_SOF = 0x80000014;
constexpr uint32_t OP_CHO_RDAL = 0xc0000014;
// const int OP_CLR = 0x0e;
// const int OP_NOT = 0xffffff10;
// const int OP_ABSA = 0x09;
// const int OP_LDAX = 0x05;

namespace FV1dasm
{
    static void dasmOP(uint32_t op, int32_t *instruction)
    {
        struct _
        {
            static int af(unsigned int op, unsigned int mask)
            {
                int32_t r = op & mask;

                while ((mask & 0b10000000000000000000000000000000) == 0)
                {
                    mask <<= 1;
                    r <<= 1;
                }

                r /= (1 << 8);
                // r = FixedPoint::fixSign(r);
                return r;
            }

            static int af2(unsigned int op, unsigned int mask)
            {
                return af(op, mask) * 2;
            }

            static int ai(unsigned int op, unsigned int mask)
            {
                uint32_t r = op & mask;

                while ((mask & 0x1) == 0)
                {
                    mask >>= 1;
                    r >>= 1;
                }

                return r;
            }
        };
        uint32_t cmd = op & 0b00000000000000000000000000011111;
        switch (cmd)
        {
        case OP_SKP:
            instruction[0] = cmd;
            instruction[1] = _::ai(op, 0b11111000000000000000000000000000);
            instruction[2] = _::ai(op, 0b00000111111000000000000000000000);
            break;
        case OP_SOF:
        case OP_LOG:
        case OP_EXP:
            instruction[0] = cmd;
            instruction[1] = _::af2(op, 0b11111111111111110000000000000000);
            instruction[2] = _::af(op, 0b00000000000000001111111111100000);
            break;
        case OP_AND:
        case OP_OR:
        case OP_XOR:
            instruction[0] = cmd;
            instruction[1] = _::ai(op, 0b11111111111111111111111100000000);
            break;
        case OP_RDAX:
        case OP_WRAX:
        case OP_MAXX:
        case OP_RDFX:
        case OP_WRLX:
        case OP_WRHX:
        case OP_MULX:
            instruction[0] = cmd;
            instruction[1] = _::ai(op, 0b00000000000000000000011111100000);
            instruction[2] = _::af2(op, 0b11111111111111110000000000000000);
            break;
        case OP_RDA:
        case OP_WRA:
        case OP_WRAP:
            instruction[0] = cmd;
            instruction[1] = _::ai(op, 0b00000000000111111111111111100000);
            instruction[2] = _::af2(op, 0b11111111111000000000000000000000);
            break;
        case OP_RMPA:
            instruction[0] = cmd;
            instruction[1] = _::af2(op, 0b11111111111000000000000000000000);
            break;
        case OP_WLDS:
            // case OP_WLDR:
            instruction[0] = cmd;
            if ((op & OP_WLDR) == OP_WLDR)
            {
                instruction[0] |= OP_WLDR;
                instruction[1] = _::ai(op, 0b00100000000000000000000000000000);
                instruction[2] = _::ai(op, 0b00011111111111111110000000000000);
                instruction[3] = _::ai(op, 0b00000000000000000000000001100000);
            }
            else
            {
                instruction[1] = _::ai(op, 0b00100000000000000000000000000000);
                instruction[2] = _::ai(op, 0b00011111111100000000000000000000);
                instruction[3] = _::ai(op, 0b00000000000011111111111111100000);
            }
            break;
        case OP_JAM:
            instruction[0] = cmd;
            instruction[1] = _::ai(op, 0b00000000000000000000000001000000);
            break;
        case OP_CHO_RDA:
        case OP_CHO_SOF:
        case OP_CHO_RDAL:
            instruction[0] = ((int)(op & 0xC0000000) | cmd);
            instruction[1] = _::ai(op, 0b00000000011000000000000000000000);
            instruction[2] = _::ai(op, 0b00111111000000000000000000000000);
            instruction[3] = _::ai(op, 0b00000000000111111111111111100000);
            break;
        default:
            break;
        }
    }

#ifndef MAKE_S

    char *replace_char(char *str, char in, char out)
    {
        char *p = str;

        while (*p != '\0')
        {
            if (*p == in)
                *p = out;
            ++p;
        }

        return str;
    }

    static uint32_t asmOP(uint32_t op, int a, int b = 0, int c = 0)
    {
        uint32_t args[4];
        args[0] = op;
        args[1] = (uint32_t)a;
        args[2] = (uint32_t)b;
        args[3] = (uint32_t)c;

        struct _
        {
            static int af2(uint32_t arg, uint32_t mask)
            {
                uint32_t m = 0b10000000000000000000000000000000;
                uint32_t r = arg << 7;

                while ((m & mask) == 0)
                {
                    m >>= 1;
                    r >>= 1;
                }

                return r & mask;
            }

            static int af(uint32_t arg, uint32_t mask)
            {
                return af2(arg << 1, mask);
            }

            static int ai(uint32_t arg, uint32_t mask)
            {
                uint32_t m = 0x1;
                uint32_t r = arg;

                while ((m & mask) == 0)
                {
                    m <<= 1;
                    r <<= 1;
                }

                return r & mask;
            }
        };

        uint32_t cmd = args[0] & 0b00000000000000000000000000011111;
        switch (cmd)
        {
        case OP_SKP:
            cmd |= _::ai(args[1], 0b11111000000000000000000000000000);
            cmd |= _::ai(args[2], 0b00000111111000000000000000000000);
            break;
        case OP_SOF:
        case OP_LOG:
        case OP_EXP:
            cmd |= _::af2(args[1], 0b11111111111111110000000000000000);
            cmd |= _::af(args[2], 0b00000000000000001111111111100000);
            break;
        case OP_AND:
        case OP_OR:
        case OP_XOR:
            cmd |= _::ai(args[1], 0b11111111111111111111111100000000);
            break;
        case OP_RDAX:
        case OP_WRAX:
        case OP_MAXX:
        case OP_RDFX:
        case OP_WRLX:
        case OP_WRHX:
        case OP_MULX:
            cmd |= _::ai(args[1], 0b00000000000000000000011111100000);
            cmd |= _::af2(args[2], 0b11111111111111110000000000000000);
            break;
        case OP_RDA:
        case OP_WRA:
        case OP_WRAP:
            cmd |= _::ai(args[1], 0b00000000000111111111111111100000);
            cmd |= _::af2(args[2], 0b11111111111000000000000000000000);
            break;
        case OP_RMPA:
            cmd |= _::af2(args[1], 0b11111111111000000000000000000000);
            break;
        case OP_WLDS:
            // case OP_WLDR:
            if ((args[0] & OP_WLDR) == OP_WLDR)
            {
                cmd |= OP_WLDR;
                cmd |= _::ai(args[1], 0b00100000000000000000000000000000);
                cmd |= _::ai(args[2], 0b00011111111111111110000000000000);
                cmd |= _::ai(args[3], 0b00000000000000000000000001100000);
            }
            else
            {
                cmd |= _::ai(args[1], 0b00100000000000000000000000000000);
                cmd |= _::ai(args[2], 0b00011111111100000000000000000000);
                cmd |= _::ai(args[3], 0b00000000000011111111111111100000);
            }
            break;
        case OP_JAM:
            cmd |= _::ai(args[1], 0b00000000000000000000000001000000);
            cmd |= 0b00000000000000000000000010000000;
            break;
        case OP_CHO_RDA:
        case OP_CHO_SOF:
        case OP_CHO_RDAL:
            cmd |= ((int)(args[0] & 0xC0000000) | cmd);
            cmd |= _::ai(args[1], 0b00000000011000000000000000000000);
            cmd |= _::ai(args[2], 0b00111111000000000000000000000000);
            cmd |= _::ai(args[3], 0b00000000000111111111111111100000);
            break;
        default:
            break;
        }

        return __builtin_bswap32(cmd);
    }

    void printASM(const int32_t *a, char *p)
    {
        switch ((uint32_t)a[0])
        {
        case OP_SKP:
            sprintf(p, "SKP %d, %d", a[1], a[2]);
            break;
        case OP_WLDS:
            sprintf(p, "WLDS %d, %d, %d", a[1], a[2], a[3]);
            break;
        case OP_RDAX:
            sprintf(p, "RDAX %d, %f", a[1], FixedPoint(a[2]).toFloat());
            break;
        case OP_SOF:
            sprintf(p, "SOF %f, %f", FixedPoint(a[1]).toFloat(), FixedPoint(a[2]).toFloat());
            break;
        case OP_LOG:
            sprintf(p, "LOG %f, %f", FixedPoint(a[1]).toFloat(), FixedPoint(a[2]).toFloat());
            break;
        case OP_EXP:
            sprintf(p, "EXP %f, %f", FixedPoint(a[1]).toFloat(), FixedPoint(a[2]).toFloat());
            break;
        case OP_AND:
            sprintf(p, "AND %x", a[1]);
            break;
        case OP_OR:
            sprintf(p, "OR %x", a[1]);
            break;
        case OP_XOR:
            sprintf(p, "XOR %x", a[1]);
            break;
        case OP_WRAX:
            sprintf(p, "WRAX %d, %f", a[1], FixedPoint(a[2]).toFloat());
            break;
        case OP_RDFX:
            sprintf(p, "RDFX %d, %f", a[1], FixedPoint(a[2]).toFloat());
            break;
        case OP_MULX:
            sprintf(p, "MULX %d", a[1]);
            break;
        case OP_RMPA:
            sprintf(p, "RMPA %f", FixedPoint(a[1]).toFloat());
            break;
        case OP_WRA:
            sprintf(p, "WRA %d, %f", a[1], FixedPoint(a[2]).toFloat());
            break;
        case OP_CHO_RDA:
            sprintf(p, "CHO RDA %d, %d, %d", a[1], a[2], a[3]);
            break;
        case OP_WRLX:
            sprintf(p, "WRLX %d, %f", a[1], FixedPoint(a[2]).toFloat());
            break;
        case OP_WRHX:
            sprintf(p, "WRHX %d, %f", a[1], FixedPoint(a[2]).toFloat());
            break;
        case OP_RDA:
            sprintf(p, "RDA %d, %f ;(%d)", a[1], FixedPoint(a[2]).toFloat(), a[2]);
            break;
        case OP_WRAP:
            sprintf(p, "WRAP %d, %f", a[1], FixedPoint(a[2]).toFloat());
            break;
        case OP_CHO_SOF:
            sprintf(p, "CHO SOF %d, %d, %d", a[1], a[2], a[3]);
            break;
        case OP_CHO_RDAL:
            sprintf(p, "CHO RDAL %d, %d, %d", a[1], a[2], a[3]);
            break;
        case OP_MAXX:
            sprintf(p, "MAXX %d, %f", a[1], FixedPoint(a[2]).toFloat());
            break;
        case OP_WLDR:
            sprintf(p, "WLDR %d, %d, %d", a[1], a[2], a[3]);
            break;
        case OP_JAM:
            sprintf(p, "JAM %d", a[1]);
            break;
        default:
            sprintf(p, ";??? %x", (unsigned)a[0]);
            break;
        }
    }

    int printCode(const int32_t *a, char *p, int line = 0)
    {
        switch ((uint32_t)a[0])
        {
        case OP_SKP:
            sprintf(p, "if(fv1->SKP(%d, %d)) goto R%03d;", a[1], a[2], line + a[2]);
            return line + a[2];
            break;
        case OP_WLDS:
            sprintf(p, "fv1->WLDS(%d, %d, %d);", a[1], a[2], a[3]);
            break;
        case OP_RDAX:
            sprintf(p, "fv1->RDAX(%d, %d);", a[1], a[2]);
            break;
        case OP_SOF:
            sprintf(p, "fv1->SOF(%d, %d);", a[1], a[2]);
            break;
        case OP_LOG:
            sprintf(p, "fv1->LOG(%d, %d);", a[1], a[2]);
            break;
        case OP_EXP:
            sprintf(p, "fv1->EXP(%d, %d);", a[1], a[2]);
            break;
        case OP_AND:
            sprintf(p, "fv1->AND(0x%x);", a[1]);
            break;
        case OP_OR:
            sprintf(p, "fv1->OR(0x%x);", a[1]);
            break;
        case OP_XOR:
            sprintf(p, "fv1->XOR(0x%x);", a[1]);
            break;
        case OP_WRAX:
            sprintf(p, "fv1->WRAX(%d, %d);", a[1], a[2]);
            break;
        case OP_RDFX:
            sprintf(p, "fv1->RDFX(%d, %d);", a[1], a[2]);
            break;
        case OP_MULX:
            sprintf(p, "fv1->MULX(%d);", a[1]);
            break;
        case OP_RMPA:
            sprintf(p, "fv1->RMPA(%d);", a[1]);
            break;
        case OP_WRA:
            sprintf(p, "fv1->WRA(%d, %d);", a[1], a[2]);
            break;
        case OP_CHO_RDA:
            sprintf(p, "fv1->CHO_RDA(%d, %d, %d);", a[1], a[2], a[3]);
            break;
        case OP_WRLX:
            sprintf(p, "fv1->WRLX(%d, %d);", a[1], a[2]);
            break;
        case OP_WRHX:
            sprintf(p, "fv1->WRHX(%d, %d);", a[1], a[2]);
            break;
        case OP_RDA:
            sprintf(p, "fv1->RDA(%d, %d);", a[1], a[2]);
            break;
        case OP_WRAP:
            sprintf(p, "fv1->WRAP(%d, %d);", a[1], a[2]);
            break;
        case OP_CHO_SOF:
            sprintf(p, "fv1->CHO_SOF(%d, %d, %d);", a[1], a[2], a[3]);
            break;
        case OP_CHO_RDAL:
            sprintf(p, "fv1->CHO_RDAL(%d);", a[1]);
            break;
        case OP_MAXX:
            sprintf(p, "fv1->MAXX(%d, %d);", a[1], a[2]);
            break;
        case OP_WLDR:
            sprintf(p, "fv1->WLDR(%d, %d, %d);", a[1], a[2], a[3]);
            break;
        case OP_JAM:
            sprintf(p, "fv1->JAM(%d);", a[1]);
            break;
        default:
            sprintf(p, ";??? %x", (unsigned)a[0]);
            break;
        }
        return 0;
    }

#endif
}; // end namespace

struct OP
{
    void *label;

    union
    {
        int32_t arg0;
        Reg *reg0;
    };

    union
    {
        int32_t arg1;
        Reg *reg1;
    };

    int32_t arg2;
};

static void **_dispatch_table;

constexpr uint32_t D_OP_WLDR = OP_CHO_RDA + 1;
constexpr uint32_t D_OP_CHO_SOF = OP_CHO_RDA + 2;
constexpr uint32_t D_OP_CHO_RDAL = OP_CHO_RDA + 3;
constexpr uint32_t D_OP_END = OP_CHO_RDA + 4;
constexpr uint32_t D_OP_NOP = OP_CHO_RDA + 5;
constexpr uint32_t D_OP_SOF_SET = OP_CHO_RDA + 6;
constexpr uint32_t D_OP_SOF_ADD = OP_CHO_RDA + 7;
constexpr uint32_t D_OP_SOF_MUL = OP_CHO_RDA + 8;
constexpr uint32_t D_OP_EXP_10 = OP_CHO_RDA + 9;
constexpr uint32_t D_OP_RDAX_1 = OP_CHO_RDA + 10;
constexpr uint32_t D_OP_WRAX_1 = OP_CHO_RDA + 11;
constexpr uint32_t D_OP_WRAX_RDAX = OP_CHO_RDA + 12;
constexpr uint32_t D_OP_LDAX = OP_CHO_RDA + 13;

void execute_program(FV1 *fv1)
{
    // https:// eli.thegreenplace.net/2012/07/12/computed-goto-for-efficient-dispatch-tables

    static void *dispatch_table[] = {
        &&_OP_RDA,
        &&_OP_RMPA,
        &&_OP_WRA,
        &&_OP_WRAP,
        &&_OP_RDAX,
        &&_OP_RDFX,
        &&_OP_WRAX,
        &&_OP_WRHX,
        &&_OP_WRLX,
        &&_OP_MAXX,
        &&_OP_MULX,
        &&_OP_LOG,
        &&_OP_EXP,
        &&_OP_SOF,
        &&_OP_AND,
        &&_OP_OR,
        &&_OP_XOR,
        &&_OP_SKP,
        &&_OP_WLDS,
        &&_OP_JAM,
        &&_OP_CHO_RDA, // 0x14

        &&_OP_WLDR,     // 0x40000012;
        &&_OP_CHO_SOF,  // 0x80000014;
        &&_OP_CHO_RDAL, // 0xc0000014;
        &&_OP_END,
        &&_OP_NOP,
        &&_OP_SOF_SET,
        &&_OP_SOF_ADD,
        &&_OP_SOF_MUL,
        &&_OP_EXP_10,
        &&_OP_RDAX_1,
        &&_OP_WRAX_1,
        &&_OP_WRAX_RDAX,
        &&_OP_LDAX};

    if (fv1 == nullptr)
    {
        _dispatch_table = dispatch_table;
        return;
    }

    auto op = (OP *)fv1->rom;

    // ACC, PACC and LR are held in locals for the duration of one program pass.
    // As members they were reloaded after every single instruction: a store
    // through an `Reg *` operand may alias `fv1->acc` as far as the compiler can
    // tell, so each opcode paid a store-to-load forward on the critical path.
    // Nothing reachable from here reads them through `fv1`, so they only have to
    // be written back at _OP_END.
    int32_t acc = fv1->acc.getValue();
    int32_t pacc = fv1->pacc.getValue();
    int32_t lr = fv1->lr.getValue();

    // ACC is saturated on every write, so a register receiving it needs no
    // further clamping - FixedPoint::set(const FixedPoint &) stores it as is.
#define SAT(v) FixedPoint::sat(v)
#define ACC_TO(reg) (reg)->set(FixedPoint(acc))

#define DISPATCH() \
    goto * (++op)->label;

    goto * op->label;

_OP_RDA:
{
    pacc = acc;
    lr = fv1->delay.read(op->arg0);
    acc = SAT(MUL(lr, op->arg1) + acc);
}
    DISPATCH();
_OP_RMPA:
{
    pacc = acc;
    lr = fv1->delay.read(fv1->regs[ADDR_PTR].getValue() >> 8);
    acc = SAT(acc + SAT(MUL(lr, op->arg0)));
}
    DISPATCH();
_OP_WRA:
{
    pacc = acc;
    fv1->delay.set(op->arg0, acc);
    acc = SAT(MUL(acc, op->arg1));
}
    DISPATCH();
_OP_WRAP:
{
    pacc = acc;
    fv1->delay.set(op->arg0, acc);
    acc = SAT(SAT(MUL(acc, op->arg1)) + lr);
}
    DISPATCH();
_OP_RDAX:
{
    pacc = acc;
    acc = SAT(MUL(op->reg0->getValue(), op->arg1) + acc);
}
    DISPATCH();
_OP_RDFX:
{
    pacc = acc;
    int reg = op->reg0->getValue();
    acc = SAT(MUL(acc - reg, op->arg1) + reg);
}
    DISPATCH();
_OP_WRAX:
{
    pacc = acc;
    ACC_TO(op->reg0);
    acc = SAT(MUL(acc, op->arg1));
}
    DISPATCH();
_OP_WRHX: // ACC->REG[ADDR], (ACC*C) + PACC
{
    int prev = pacc;
    pacc = acc;
    ACC_TO(op->reg0);
    acc = SAT(MUL(acc, op->arg1) + prev);
}
    DISPATCH();
_OP_WRLX: // ACC->REG[ADDR], (PACC-ACC) * C + PACC
{
    int prev = pacc;
    pacc = acc;
    ACC_TO(op->reg0);
    acc = SAT(MUL(prev - acc, op->arg1) + prev);
}
    DISPATCH();
_OP_MAXX:
{
    pacc = acc;
    int reg = op->reg0->getValue();
    if (reg < 0)
        reg = -reg;
    reg = SAT(MUL(reg, op->arg1));
    if (reg < 0)
        reg = -reg;
    int a = acc < 0 ? -acc : acc;
    acc = SAT(a > reg ? a : reg);
}
    DISPATCH();
_OP_MULX:
{
    pacc = acc;
    acc = SAT(MUL(acc, op->reg0->getValue()));
}
    DISPATCH();
_OP_LOG:
{
    pacc = acc;
    int l;
    if (acc > 0)
        l = log2i(acc);
    else if (acc < 0)
        l = log2i(-acc);
    else
        l = -FixedPoint::MAX;
    acc = SAT(MUL(l, op->arg0) + op->arg1);
}
    DISPATCH();
_OP_EXP:
{
    pacc = acc;
    acc = acc >= 0 ? FixedPoint::MAX : SAT(exp2i(acc));
    acc = SAT(MUL(acc, op->arg0) + op->arg1);
}
    DISPATCH();
_OP_SOF:
{
    pacc = acc;
    acc = SAT(MUL(acc, op->arg0) + op->arg1);
}
    DISPATCH();
_OP_AND:
{
    pacc = acc;
    acc = SAT(FixedPoint::fixSign(acc & op->arg0));
}
    DISPATCH();
_OP_OR:
{
    pacc = acc;
    acc = SAT(FixedPoint::fixSign(acc | op->arg0));
}
    DISPATCH();
_OP_XOR:
{
    pacc = acc;
    acc = SAT(FixedPoint::fixSign(acc ^ op->arg0));
}
    DISPATCH();
_OP_SKP:
{
    int flags = op->arg0;
    bool skip = false;

    if (flags & SKP_RUN)
        skip |= fv1->firstRun == false;
    if (flags & SKP_ZRO)
        skip |= acc == 0;
    if (flags & SKP_GEZ)
        skip |= acc > 0;
    if (flags & SKP_NEG)
        skip |= acc < 0;
    if (flags & SKP_ZRC)
        skip |= (acc & FixedPoint::F) != (pacc & FixedPoint::F);

    if (skip)
        op += op->arg1;
}
    DISPATCH();
_OP_WLDS:
    fv1->WLDS(op->arg0, op->arg1, op->arg2);
    DISPATCH();
_OP_JAM:
    fv1->JAM(op->arg0);
    DISPATCH();
_OP_CHO_RDA:
{
    int lfoval = 0;
    int scale = 0;
    fv1->CHO(op->arg0, op->arg1, &lfoval, &scale);

    int addr = op->arg2;
    if ((op->arg1 & CHO_NA) == 0)
        addr += (lfoval >> 10);

    pacc = acc;
    lr = fv1->delay.read(addr);
    acc = SAT(MUL(lr, scale) + acc);
}
    DISPATCH();
_OP_WLDR:
    fv1->WLDR(op->arg0, op->arg1, op->arg2);
    DISPATCH();
_OP_CHO_RDAL:
{
    pacc = acc;
    acc = SAT(fv1->CHO_RDAL_VALUE(op->arg0));
}
    DISPATCH();
_OP_CHO_SOF:
{
    int lfoval = 0;
    int scale = 0;
    fv1->CHO(op->arg0, op->arg1, &lfoval, &scale);

    pacc = acc;
    acc = SAT(MUL(acc, scale) + op->arg2);
}
    DISPATCH();
_OP_NOP:
    DISPATCH();
_OP_END:
    fv1->acc.set(FixedPoint(acc));
    fv1->pacc.set(FixedPoint(pacc));
    fv1->lr.set(FixedPoint(lr));
    return;
_OP_SOF_SET:
    acc = SAT(op->arg1);
    DISPATCH();
_OP_SOF_ADD:
    acc = SAT(acc + op->arg1);
    DISPATCH();
_OP_SOF_MUL:
    acc = SAT(MUL(acc, op->arg0));
    DISPATCH();
_OP_EXP_10:
{
    pacc = acc;
    acc = acc >= 0 ? FixedPoint::MAX : SAT(exp2i(acc));
}
    DISPATCH();
_OP_RDAX_1:
{
    pacc = acc;
    acc = SAT(acc + op->reg0->getValue());
}
    DISPATCH();
_OP_WRAX_1:
{
    pacc = acc;
    ACC_TO(op->reg0);
}
    DISPATCH();
_OP_WRAX_RDAX:
{
    ACC_TO(op->reg0);
    acc = SAT(MUL(op->reg1->getValue(), op->arg2));
}
    DISPATCH();
_OP_LDAX:
{
    pacc = acc;
    acc = op->reg0->getValue();
}
    DISPATCH();

#undef SAT
#undef ACC_TO
#undef DISPATCH
}

// Translates the 128-word FV-1 ROM image into the threaded-code form executed by
// execute_program(). Besides the per-opcode specializations (D_OP_*) the loader
// folds instruction sequences that provably cannot be observed:
//
//  * a run of all-zero words (`RDA 0,0`) - what SPINAsm pads unused program
//    space with - is idempotent: each one re-reads the same delay cell into LR,
//    copies ACC to PACC and leaves ACC untouched. Executing it once is
//    indistinguishable from executing it 117 times, so a run collapses to one op.
//  * `WRAX reg,0` followed by `RDAX reg2,c` becomes a single fused op.
//
// Both change instruction indices, so SKP offsets are remapped afterwards.
extern "C" void *fv1_load(FV1 *fv1, const uint8_t *prog, void *(*malloc)(size_t size))
{
    if (malloc == nullptr)
        malloc = ::malloc;

    auto fx = (OP *)malloc(sizeof(OP) * 129);
    fv1->rom = fx;

    execute_program(nullptr);

    if (prog == nullptr)
    {
        int j = 0;
        while (j < 128)
            fx[j++] = OP{_dispatch_table[D_OP_NOP], 0, 0, 0}; //_OP_NOP
        fx[j] = OP{_dispatch_table[D_OP_END], 0, 0, 0};       //_OP_END
        fv1->setFx(execute_program);
        return fv1->rom;
    }

    uint8_t kind[128];     // dispatch index per source instruction
    uint8_t emitKind[128]; // dispatch index per emitted instruction
    bool pad[128];         // source word was all zero (`RDA 0,0`)
    bool target[129];      // source index is reachable by a SKP
    int16_t src[128];      // emitted index -> source index
    int16_t newIdx[129];   // source index -> emitted index

    int n = 0;
    for (int a = 0; a < 512; a += 4)
    {
        uint32_t i = 0;
        i |= prog[a] << 24;
        i |= prog[a + 1] << 16;
        i |= prog[a + 2] << 8;
        i |= prog[a + 3] << 0;

        if (i == 0x11)
            break;

        int32_t instr[4] = {};
        FV1dasm::dasmOP(i, instr);

        uint32_t k = (uint32_t)instr[0];
        if (k == OP_WLDR)
            k = D_OP_WLDR;
        else if (k == OP_CHO_SOF)
            k = D_OP_CHO_SOF;
        else if (k == OP_CHO_RDAL)
            k = D_OP_CHO_RDAL;
        else if (k == OP_SOF)
        {
            if (instr[1] == 0)
                k = D_OP_SOF_SET;
            else if (instr[1] == FixedPoint::F)
                k = D_OP_SOF_ADD;
            else if (instr[2] == 0)
                k = D_OP_SOF_MUL;
        }
        else if (k == OP_EXP)
        {
            if (instr[1] == FixedPoint::F && instr[2] == 0)
                k = D_OP_EXP_10;
        }
        else if (k == OP_RDAX && instr[2] == FixedPoint::F)
            k = D_OP_RDAX_1;
        else if (k == OP_WRAX && instr[2] == FixedPoint::F)
            k = D_OP_WRAX_1;
        else if (k == OP_RDFX && instr[2] == 0)
            k = D_OP_LDAX;

        kind[n] = (uint8_t)k;
        pad[n] = (i == 0);
        target[n] = false;
        fx[n] = OP{nullptr, instr[1], instr[2], instr[3]};
        ++n;
    }
    target[n] = false;

    for (int i = 0; i < n; ++i)
    {
        if (kind[i] != OP_SKP)
            continue;
        int t = i + fx[i].arg1 + 1;
        target[t > n ? n : t] = true;
    }

    int j = 0;
    for (int i = 0; i < n; ++i)
    {
        // continuation of a run of `RDA 0,0` - already covered by the run's head.
        // Entering the run halfway is fine too: one execution equals any number.
        if (pad[i] && i > 0 && pad[i - 1])
        {
            newIdx[i] = (int16_t)(j - 1);
            continue;
        }

        // WRAX reg,0 + RDAX reg2,c. Only foldable while nothing branches onto the
        // RDAX - otherwise the jump would start running the WRAX as well.
        if (kind[i] == OP_RDAX && !target[i] && j > 0 &&
            emitKind[j - 1] == OP_WRAX && fx[j - 1].arg1 == 0)
        {
            emitKind[j - 1] = D_OP_WRAX_RDAX;
            fx[j - 1].reg1 = &fv1->regs[fx[i].arg0];
            fx[j - 1].arg2 = fx[i].arg1;
            newIdx[i] = (int16_t)(j - 1);
            continue;
        }

        newIdx[i] = (int16_t)j;
        src[j] = (int16_t)i;
        emitKind[j] = kind[i];
        if (j != i)
            fx[j] = fx[i];
        ++j;
    }
    newIdx[n] = (int16_t)j; // falling off the end lands on _OP_END

    for (int d = 0; d < j; ++d)
    {
        if (emitKind[d] != OP_SKP)
            continue;
        int t = src[d] + fx[d].arg1 + 1;
        fx[d].arg1 = newIdx[t > n ? n : t] - d - 1;
    }

    for (int d = 0; d < j; ++d)
    {
        switch (emitKind[d])
        {
        case D_OP_LDAX:
        case OP_RDAX:
        case D_OP_RDAX_1:
        case OP_WRAX:
        case D_OP_WRAX_1:
        case D_OP_WRAX_RDAX:
        case OP_MULX:
        case OP_RDFX:
        case OP_WRHX:
        case OP_WRLX:
        case OP_MAXX:
        {
            int reg = fx[d].arg0;
            fx[d].reg0 = &fv1->regs[reg];
            break;
        }
        }

        fx[d].label = _dispatch_table[emitKind[d]];
    }

    fx[j] = OP{_dispatch_table[D_OP_END], 0, 0, 0}; //_OP_END

    fv1->setFx(execute_program);
    return fv1->rom;
}