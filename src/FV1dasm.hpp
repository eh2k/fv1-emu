#include "FV1.hpp"

static int af(unsigned int op, unsigned int mask)
{
    unsigned r = op & mask;

    while ((mask & 0b10000000000000000000000000000000) == 0)
    {
        mask <<= 1;
        r <<= 1;
    }

    r >>= 8;
    return FixedPoint::fixSign(r) << 1;
}

static int ai(unsigned int op, unsigned int mask)
{
    unsigned r = op & mask;

    while ((mask & 0x1) == 0)
    {
        mask >>= 1;
        r >>= 1;
    }

    return (int)r;
}

static const OP dasmOP(unsigned int op)
{
    int cmd = op & 0b00000000000000000000000000011111;
    switch (cmd)
    {
    case OP_SKP:
        return {cmd,
                ai(op, 0b11111000000000000000000000000000),
                ai(op, 0b00000111111000000000000000000000)};
    case OP_SOF:
    case OP_LOG:
    case OP_EXP:
        return {cmd,
                af(op, 0b11111111111111110000000000000000),
                af(op, 0b00000000000000001111111111100000) >> 1};
    case OP_AND:
    case OP_OR:
    case OP_XOR:
        return {cmd,
                af(op, 0b11111111111111111111111100000000) >> 1};
    case OP_RDAX:
    case OP_WRAX:
    case OP_MAXX:
    case OP_RDFX:
    case OP_WRLX:
    case OP_WRHX:
    case OP_MULX:
        return {cmd,
                ai(op, 0b00000000000000000000011111100000),
                af(op, 0b11111111111111110000000000000000)};
    case OP_RDA:
    case OP_WRA:
    case OP_WRAP:
        return {cmd,
                ai(op, 0b00000000000111111111111111100000),
                af(op, 0b11111111111000000000000000000000)};
    case OP_RMPA:
        return {cmd, af(op, 0b11111111111000000000000000000000)};
    case OP_WLDS:
        //case OP_WLDR:
        if ((op & OP_WLDR) == OP_WLDR)
        {
            return {cmd,
                    ai(op, 0b00100000000000000000000000000000),
                    ai(op, 0b00011111111111111110000000000000),
                    ai(op, 0b00000000000000000000000001100000)};
        }
        else
        {
            return {cmd,
                    ai(op, 0b00100000000000000000000000000000),
                    ai(op, 0b00011111111100000000000000000000),
                    ai(op, 0b00000000000011111111111111100000)};
        }
    case OP_JAM:
        return {cmd, ai(op, 0b00000000000000000000000001000000)};
    case OP_CHO_RDA:
    case OP_CHO_SOF:
    case OP_CHO_RDAL:
        return {((int)(op & 0xC0000000) | cmd),
                ai(op, 0b00000000011000000000000000000000),
                ai(op, 0b00111111000000000000000000000000),
                ai(op, 0b00000000000111111111111111100000)};
    default:
        return {0, 0, 0};
    }
}

void printOP(const OP& a, char *p)
{
    switch (a[0])
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
        sprintf(p, "RDA %d, %f", a[1], FixedPoint(a[2]).toFloat());
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
        sprintf(p, "WLDR %d, %f, %d", a[1], FixedPoint(a[2]).toFloat(), a[3]);
        break;
    case OP_JAM:
        sprintf(p, "JAM %d", a[1]);
        break;
    default:
        sprintf(p, ";??? %x", a[0]);
        break;
    }
}
