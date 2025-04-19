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

#include <vector>
#include <iostream>
#include <string>
#include <fstream>
#include <sstream>
#include <map>
#include <regex>
#include <functional>
#include <algorithm>
#include <assert.h>
#include <memory>
#include <codecvt>

#define FV1_DUMP_STATE

#include "FV1.hpp"
#include "FV1_DASM.hpp"
#include "FV1_SPN.hpp"

class FV1emu
{
private:
    FV1 fv1;
    std::string display;
    std::string fxcode;

public:
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
        std::vector<uint32_t> rom;
        rom.resize(512 / 4);

        std::stringstream buffer;
        std::ifstream t(file, std::ios::binary);
        buffer << t.rdbuf();

        if (SpnParser::asmSPN(buffer.str(), rom, this->display))
        {
            printf("ok\n");
            fv1_load(&this->fv1, (const uint8_t *)&rom[0], nullptr);
            return true;
        }
        else
        {
            return false;
        }
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
    printf("%s", SpnParser::log.str().c_str());
    return r;
}
#endif
