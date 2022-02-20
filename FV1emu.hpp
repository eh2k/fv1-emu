/* FV1 Emulator
 * Copyright (C)2018 - Eduard Heidt
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

#include <vector>
#include <iostream>
#include <string>
#include <sstream>
#include <map>
#include <regex>
#include <functional>
#include <algorithm>
#include <assert.h>
#include <memory>
#include <codecvt>

#include "FV1.hpp"

class FV1emu
{
private:
    int ParseFloat(const std::string &param, std::map<std::string, int> &vars)
    {
        if (vars.find(param) == vars.end())
        {
            if (param.find('/') != std::string::npos)
            {
                std::stringstream ss(param);
                std::string tmp;

                std::getline(ss, tmp, '/');

                auto a = (float)ParseFloat(tmp, vars) / FixedPoint::F;

                while (std::getline(ss, tmp, '/'))
                    a = a / ((float)ParseFloat(tmp, vars) / FixedPoint::F);

                return a * FixedPoint::F;
            }
            else
            {
                //assert(std::regex_match(param, std::regex("[+-]?([0-9]*[.])?[0-9]+")));

                float f = 0;
                std::istringstream istr(param);

                istr.imbue(std::locale("C"));
                istr >> f;

                return f * FixedPoint::F;
            }
        }
        else
            return vars[param];
    }

    static std::vector<std::string> SplitString(std::string s, std::string delimiter)
    {
        size_t pos_start = 0, pos_end, delim_len = delimiter.length();
        std::string token;
        std::vector<std::string> res;

        while ((pos_end = s.find(delimiter, pos_start)) != std::string::npos)
        {
            token = s.substr(pos_start, pos_end - pos_start);
            pos_start = pos_end + delim_len;
            res.push_back(token);
        }

        res.push_back(s.substr(pos_start));
        return res;
    }

    int ParseInt(const std::string &param, std::map<std::string, int> &vars)
    {
        if (vars.find(param) == vars.end())
        {
            std::string tmp;
            if (param.find('/') != std::string::npos)
            {
                auto a = 0;

                for (auto tmp : SplitString(param, "/"))
                {
                    if (a == 0)
                        a = ParseInt(tmp, vars);
                    else
                        a /= ParseInt(tmp, vars);
                }
                return a;
            }
            else if (param.find('+') != std::string::npos)
            {
                auto a = 0;

                for (auto tmp : SplitString(param, "+"))
                    a += ParseInt(tmp, vars);

                return a;
            }
            else if (param.find('-') != std::string::npos)
            {
                auto a = 0;

                for (auto tmp : SplitString(param, "-"))
                    a -= ParseInt(tmp, vars);

                return a;
            }
            else if (param.find('|') != std::string::npos)
            {
                auto a = 0;

                for (auto tmp : SplitString(param, "|"))
                    a |= ParseInt(tmp, vars);

                return a;
            }
            else if (param.find('X') != std::string::npos)
            {
                return std::stoul(param, nullptr, 16);
            }
            else if (param.find('$') != std::string::npos)
            {
                tmp = param;
                replaceAll(tmp, "$", "0x");
                return std::stoul(tmp, nullptr, 16);
            }
            else if (param.find('%') != std::string::npos)
            {
                tmp = param;
                replaceAll(tmp, "%", "");
                replaceAll(tmp, "_", "");
                return std::stoul(tmp, nullptr, 2);
            }
            else
            {
                int i = 0;
                std::istringstream istr(param);

                istr.imbue(std::locale("C"));
                istr >> i;

                return i;
            }
        }
        else
            return vars[param];
    }

    void replaceAll(std::string &str, const std::string &from, const std::string &to)
    {
        if (from.empty())
            return;
        size_t start_pos = 0;
        while ((start_pos = str.find(from, start_pos)) != std::string::npos)
        {
            while ((start_pos = str.find(from, start_pos)) != std::string::npos)
            {
                str.replace(start_pos, from.length(), to);
                start_pos += to.length(); // In case 'to' contains 'from', like replacing 'x' with 'yx'
            }
            start_pos = 0;
        }
    }

    void trim(std::string &str, const std::string &chars = "\t\n\v\f\r ")
    {
        str.erase(0, str.find_first_not_of(chars));
        str.erase(str.find_last_not_of(chars) + 1);
    }

    void toupper(std::string &str)
    {
        for (auto &c : str)
            c = ::toupper(c);
    }

    void ReadLine(std::string line, std::string &op, std::string &paramA, std::string &paramB, std::string &paramC, std::string &paramD)
    {
        std::stringstream a(line);
        std::getline(a, line, ';');

        trim(line);
        replaceAll(line, " ", "\t");
        replaceAll(line, ",,", ",0,");
        replaceAll(line, ",", "\t");
        replaceAll(line, "\t\t", "\t");
        replaceAll(line, "\t|", "|");
        replaceAll(line, "|\t", "|");

        std::stringstream ss(line);

        std::getline(ss, op, '\t');
        trim(op);

        toupper(op);
        std::getline(ss, paramA, '\t');
        std::getline(ss, paramB, '\t');
        std::getline(ss, paramC, '\t');
        std::getline(ss, paramD, '\t');
        trim(paramA);
        trim(paramB);
        trim(paramC);
        trim(paramD);
        toupper(paramA);
        toupper(paramB);
        toupper(paramC);
        toupper(paramD);
    }

    template <typename A, typename B, typename C>
    void _DEBUG(int i, const std::string &op, const A &p1, const B &p2, const C &p3, const std::string &line)
    {
        std::ostringstream tmp;

        //out << std::string("\nP2 ") << std::hex << this->getRegVal(POT2) << " DACL " << std::hex << this->getRegVal(DACL);

        tmp << i << ":";

        while (tmp.tellp() < 4)
            tmp << " ";

        tmp << op << " " << p1 << " " << p2 << " " << p3;

        while (tmp.tellp() < 40)
            tmp << " ";

        tmp << line;

        if (!line.empty())
        {
            log << tmp.str();
        }
    }

    template <typename A, typename B>
    void _DEBUG(int i, const std::string &op, const A &p1, const B &p2, const std::string &line)
    {
        _DEBUG(i, op, p1, p2, "", line);
    }

    template <typename A>
    void _DEBUG(int i, const std::string &op, const A &p1, const std::string &line)
    {
        _DEBUG(i, op, p1, "", "", line);
    }

    std::string basename(const std::string &pathname)
    {
        return {std::find_if(pathname.rbegin(), pathname.rend(), [](char c) { return c == '/' || c == '\\'; }).base(), pathname.end()};
    }

    inline int S114(const std::string &param, std::map<std::string, int> &vars) //16BIT => -2 to 2
    {
        auto fp = ParseFloat(param, vars);
        return (fp / FixedPoint::S) & 0xffffff00;
    }

    inline int S19(const std::string &param, std::map<std::string, int> &vars) //11BIT => -2 to 2
    {
        auto fp = ParseFloat(param, vars);
        return (fp / FixedPoint::S) & 0xffffe000;
    }

    inline int S10(const std::string &param, std::map<std::string, int> &vars) //11BIT => -1 to 1
    {
        auto fp = ParseFloat(param, vars);
        return fp & 0xffffff00;
    }

    inline int S15(const std::string &param, std::map<std::string, int> &vars) //16 BIT => -1 to 1
    {
        auto fp = ParseFloat(param, vars);
        return fp & 0xffffe000;
    }

    inline int LFO1(const std::string &param, std::map<std::string, int> &vars)
    {
        auto fp = ParseInt(param, vars);
        return fp & 0x1;
    }

    inline int LFO2(const std::string &param, std::map<std::string, int> &vars)
    {
        auto fp = ParseInt(param, vars);
        return fp & 0xF;
    }

    inline int REGADDR(const std::string &param, std::map<std::string, int> &vars)
    {
        auto fp = ParseInt(param, vars);
        assert(fp <= 0x3F);
        return fp & 0x3F;
    }

    inline int DELADDR(const std::string &param, std::map<std::string, int> &vars)
    {
        auto fp = ParseInt(param, vars);
        assert(fp <= 0x7FFF);
        return fp & 0x7FFF;
    }

    inline int MASK(const std::string &param, std::map<std::string, int> &vars)
    {
        auto fp = ParseInt(param, vars);
        assert(fp <= 0x00FFFFFF);
        return fp & 0x00FFFFFF;
    }

    inline int INT(const std::string &param, std::map<std::string, int> &vars)
    {
        auto fp = ParseInt(param, vars);
        return fp;
    }

    FV1 fv1;
    std::string display;
    std::string fxcode;

public:
    std::stringstream log;

    std::string getDisplay()
    {
        return this->display;
    }

    std::string getCode()
    {
        return this->fxcode;
    }

    std::string dumpState(const std::string endl = "")
    {
        std::string tmp;
        this->fv1.dump(tmp, endl);
        return tmp;
    }

    bool load(const std::string &file)
    {
        std::stringstream stream;

        FILE *pFile = fopen(file.c_str(), "r");
        if (pFile)
        {
            std::string line;
            char buf[1024];
            while (fgets(buf, 1024, pFile))
            {
                line.assign(buf);
                bool isComment = false;

                for (auto &c : line)
                {
                    if (c != 0 && isascii(c))
                    {
                        stream << (char)c;

                        if (c == ';')
                            isComment = true;

                        if (isComment == false && c == ':')
                            stream << std::endl;
                    }
                }

                stream << std::endl;
            }

            auto fxname = basename(file);
            toupper(fxname);
            fclose(pFile);
            return loadFromSPN(fxname, stream.str());
        }
        else
        {
            return false;
        }
    }

    bool loadFromSPN(std::string fxname, const std::string &spnCode)
    {
        log = std::stringstream();

        std::vector<std::vector<int>> fx;
        std::string fxcode = spnCode;

        this->display = fxname;
        replaceAll(fxcode, "&#58;", ":");

        std::stringstream stream;
        stream << fxcode;

        stream.seekg(0, std::ios::beg);
        std::map<std::string, int> VAR;

        VAR[""] = 0;
        VAR["RUN"] = SKP_RUN;
        VAR["GEZ"] = SKP_GEZ;
        VAR["NEG"] = SKP_NEG;
        VAR["ZRC"] = SKP_ZRC;
        VAR["ZRO"] = SKP_ZRO;

        for (int i = 0; i < (REG31 - REG0); i++)
            VAR[std::string("REG") + std::to_string(i)] = REG0 + i;

        VAR["ADDR_PTR"] = ADDR_PTR;

        VAR["RMP0_RATE"] = RMP0_RATE;
        VAR["RMP0_RANGE"] = RMP0_RANGE;
        VAR["RMP1_RATE"] = RMP1_RATE;
        VAR["RMP1_RANGE"] = RMP1_RANGE;
        VAR["SIN0_RATE"] = SIN0_RATE;
        VAR["SIN0_RANGE"] = SIN0_RANGE;
        VAR["SIN1_RATE"] = SIN1_RATE;
        VAR["SIN1_RANGE"] = SIN1_RANGE;

        VAR["POT0"] = POT0;
        VAR["POT1"] = POT1;
        VAR["POT2"] = POT2;

        VAR["ADCL"] = ADCL;
        VAR["ADCR"] = ADCR;

        VAR["DACL"] = DACL;
        VAR["DACR"] = DACR;

        VAR["RMP0"] = CHO_LFO_RMP0;
        VAR["RMP1"] = CHO_LFO_RMP1;
        VAR["SIN0"] = CHO_LFO_SIN0;
        VAR["SIN1"] = CHO_LFO_SIN1;
        VAR["COS0"] = CHO_LFO_COS0;
        VAR["COS1"] = CHO_LFO_COS1;
        VAR["REG"] = CHO_REG;
        VAR["SIN"] = CHO_SIN;
        VAR["COS"] = CHO_COS;
        VAR["COMPC"] = CHO_COMPC;
        VAR["COMPA"] = CHO_COMPA;
        VAR["RPTR2"] = CHO_RPTR2;
        VAR["NA"] = CHO_NA;

        int nextDelayAddress = 0;

        std::string line;
        int i = 1;
        int d = 0;

        while (std::getline(stream, line))
        {
            std::string op, paramA, paramB, paramC, paramD;
            ReadLine(line, op, paramA, paramB, paramC, paramD);

            //_DEBUG(i, op, paramA, paramB, paramC, line);
            auto tmp = line;
            toupper(tmp);
            auto pos = tmp.find("POT");
            if (pos != std::string::npos && d < 3)
            {
                this->display += "\n";
                this->display += line.substr(pos, std::min(line.size() - pos - 1, (size_t)24));
                d++;
            }

            if (op.length() > 0 && *op.rbegin() == ':') //GOTO
            {
                replaceAll(op, ":", "");
                VAR[op] = i;
            }
            else if (op == "EQU" || paramA == "EQU" || op == "MEM" || paramA == "MEM" || op.length() == 0 || op[0] == ';' || op[0] == 13 || op[0] == 0)
            {
                //Comment
            }
            else
            {
                i++;
            }
        }

        stream.clear();
        stream.seekg(0, std::ios::beg);
        i = 1;

        while (std::getline(stream, line))
        {
            std::string op, paramA, paramB, paramC, paramD;
            ReadLine(line, op, paramA, paramB, paramC, paramD);

            //_DEBUG(i, op, paramA, paramB, paramC, line);

            if (op.length() > 0 && *op.rbegin() == ':')
            {
                //GOTO REF
                _DEBUG(0, op, paramA, paramB, paramC, line);
            }
            else if (op == "SKP")
            {
                auto a = INT(paramA, VAR);
                auto b = INT(paramB, VAR);

                if (VAR.find(paramB) != VAR.end())
                    b -= (int)fx.size() + 2;

                fx.push_back({OP_SKP, a, b});
                _DEBUG(i++, op, a, b, line);
            }
            else if (op.length() == 0 || op[0] == ';' || op[0] == 13 || op[0] == 0)
            {
                //Comment
                _DEBUG(0, op, paramA, paramB, paramC, line);
            }
            else if (op == "EQU" || paramA == "EQU")
            {
                auto a = op == "EQU" ? paramA : op;

                if (paramB.find('.') != std::string::npos)
                {
                    auto b = ParseFloat(paramB, VAR);

                    _DEBUG(0, op, paramA, b, line);
                    VAR[a] = b;
                    VAR[std::string("-") + a] = -b;
                }
                else
                {
                    auto b = INT(paramB, VAR);

                    _DEBUG(0, op, paramA, b, line);
                    VAR[a] = b;
                    VAR[std::string("-") + a] = -b;
                }
            }
            else if (op == "CLR")
            {
                fx.push_back({OP_AND, 0});
                _DEBUG(i++, op, paramA, paramB, paramC, line);
            }
            else if (op == "NOT")
            {
                fx.push_back({OP_XOR, 0x00FFFFFF});
                _DEBUG(i++, op, paramA, paramB, paramC, line);
            }
            else if (op == "ABSA")
            {
                fx.push_back({OP_MAXX, 0, 0});
                _DEBUG(i++, op, paramA, paramB, paramC, line);
            }
            else if (op == "SOF")
            {
                auto a = S114(paramA, VAR);
                auto b = S10(paramB, VAR);

                fx.push_back({OP_SOF, a, b});
                _DEBUG(i++, op, a, b, line);
            }
            else if (op == "AND")
            {
                auto a = MASK(paramA, VAR);

                fx.push_back({OP_AND, a});
                _DEBUG(i++, op, a, line);
            }
            else if (op == "OR")
            {
                auto a = MASK(paramA, VAR);

                fx.push_back({OP_OR, a});
                _DEBUG(i++, op, a, line);
            }
            else if (op == "XOR")
            {
                auto a = MASK(paramA, VAR);

                fx.push_back({OP_XOR, a});
                _DEBUG(i++, op, a, line);
            }
            else if (op == "LOG")
            {
                auto a = S114(paramA, VAR);
                auto b = S10(paramB, VAR);

                fx.push_back({OP_LOG, a, b});
                _DEBUG(i++, op, a, b, line);
            }
            else if (op == "EXP")
            {
                auto a = S114(paramA, VAR);
                auto b = S10(paramB, VAR);

                fx.push_back({OP_EXP, a, b});
                _DEBUG(i++, op, a, b, line);
            }
            else if (op == "WRAX")
            {
                auto a = REGADDR(paramA, VAR);
                auto b = S114(paramB, VAR);

                fx.push_back({OP_WRAX, a, b});
                _DEBUG(i++, op, a, b, line);
            }
            else if (op == "MAXX")
            {
                auto a = REGADDR(paramA, VAR);
                auto b = S114(paramB, VAR);

                fx.push_back({OP_MAXX, a, b});
                _DEBUG(i++, op, a, b, line);
            }
            else if (op == "RDAX")
            {
                auto a = REGADDR(paramA, VAR);
                auto b = S114(paramB, VAR);

                fx.push_back({OP_RDAX, a, b});
                _DEBUG(i++, op, a, b, line);
            }
            else if (op == "LDAX")
            {
                auto a = REGADDR(paramA, VAR);

                fx.push_back({OP_RDFX, a, 0});
                _DEBUG(i++, op, a, line);
            }
            else if (op == "RDFX")
            {
                auto a = REGADDR(paramA, VAR);
                auto b = S114(paramB, VAR);

                fx.push_back({OP_RDFX, a, b});
                _DEBUG(i++, op, a, b, line);
            }
            else if (op == "WRLX")
            {
                auto a = REGADDR(paramA, VAR);
                auto b = S114(paramB, VAR);

                fx.push_back({OP_WRLX, a, b});
                _DEBUG(i++, op, a, b, line);
            }
            else if (op == "WRHX")
            {
                auto a = REGADDR(paramA, VAR);
                auto b = S114(paramB, VAR);

                fx.push_back({OP_WRHX, a, b});
                _DEBUG(i++, op, a, b, line);
            }
            else if (op == "MULX")
            {
                auto a = REGADDR(paramA, VAR);

                fx.push_back({OP_MULX, a});
                _DEBUG(i++, op, a, line);
            }
            else if (op == "WLDS")
            {
                auto a = LFO1(paramA, VAR);
                auto b = INT(paramB, VAR) & 0x1ff;
                auto c = INT(paramC, VAR) & 0x7fff;

                fx.push_back({OP_WLDS, a, b, c});
                _DEBUG(i++, op, a, b, c, line);
            }
            else if (op == "WLDR")
            {
                auto a = LFO1(paramA, VAR);
                auto b = INT(paramB, VAR);
                auto c = INT(paramC, VAR);

                if (c == 512)
                    c = 0x03 << RampLFO::AMP_OFFSET;
                else if (c == 1024)
                    c = 0x02 << RampLFO::AMP_OFFSET;
                else if (c == 2048)
                    c = 0x01 << RampLFO::AMP_OFFSET;
                else // 4096
                    c = 0;

                fx.push_back({OP_WLDR, a, b, c});
                _DEBUG(i++, op, a, b, c, line);
            }
            else if (op == "JAM")
            {
                auto a = LFO1(paramA, VAR);

                fx.push_back({OP_JAM, a});
                _DEBUG(i++, op, a, line);
            }
            else if (op == "MEM" || paramA == "MEM")
            {
                if (paramA == "MEM")
                    paramA = op;

                auto address = nextDelayAddress;
                auto len = DELADDR(paramB, VAR);

                VAR[paramA] = address;
                VAR[paramA + "#"] = address + len;
                VAR[paramA + "^"] = address + (len / 2);

                nextDelayAddress += len + 1;

                _DEBUG(0, op, address, len, line);
            }
            else if (op == "RDA")
            {
                auto a = DELADDR(paramA, VAR);
                auto b = S19(paramB, VAR);

                fx.push_back({OP_RDA, a, b});
                _DEBUG(i++, op, a, b, line);
            }
            else if (op == "RMPA")
            {
                auto a = S19(paramA, VAR);

                fx.push_back({OP_RMPA, a});
                _DEBUG(i++, op, a, line);
            }
            else if (op == "WRA")
            {
                auto a = DELADDR(paramA, VAR);
                auto b = S19(paramB, VAR);

                fx.push_back({OP_WRA, a, b});
                _DEBUG(i++, op, a, b, line);
            }
            else if (op == "WRAP")
            {
                auto a = DELADDR(paramA, VAR);
                auto b = S19(paramB, VAR);

                fx.push_back({OP_WRAP, a, b});
                _DEBUG(i++, op, a, b, line);
            }
            else if (op == "CHO" && paramA == "RDA")
            {
                auto a = LFO2(paramB, VAR);
                auto b = INT(paramC, VAR);
                auto c = DELADDR(paramD, VAR);

                fx.push_back({OP_CHO_RDA, a, b, c});
                _DEBUG(i++, op + "-" + paramA, a, b, c, line);
            }
            else if (op == "CHO" && paramA == "SOF")
            {
                auto a = LFO2(paramB, VAR);
                auto b = INT(paramC, VAR);
                auto c = S15(paramD, VAR);

                fx.push_back({OP_CHO_SOF, a, b, c});
                _DEBUG(i++, op + "-" + paramA, a, b, c, line);
            }
            else if (op == "CHO" && paramA == "RDAL")
            {
                auto a = LFO2(paramB, VAR);

                fx.push_back({OP_CHO_RDAL, a});
                _DEBUG(i++, op + "-" + paramA, a, line);
            }
            else
            {
                //_DEBUG(i++, op, paramA, paramB, paramC, line);
                std::stringstream hValStr;
                hValStr << "[";
                for (std::size_t i = 0; i < op.length(); i++)
                {
                    int hValInt = (char)op[i];
                    hValStr << "0x" << std::hex << hValInt << " ";
                }
                hValStr << "]";
                _DEBUG(i, "#####FAIL ", hValStr.str(), line);
                return false;
            }
        }

        fv1.loadFx(fx);
        return true;
    }

    void run(float inL, float inR, float pot0, float pot1, float pot2, float &outL, float &outR)
    {
        fv1.execute(inL, inR, pot0, pot1, pot2, outL, outR);
    }
};

#ifdef TEST
int main(int argc, char *argv[])
{
    printf("FV1emu\n");
    FV1emu fx;
    int r = !fx.load(argv[1]);
    printf(fx.log.str().c_str());
    return r;
}
#endif
