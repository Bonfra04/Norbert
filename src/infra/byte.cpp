#include "infra/byte.hpp"

#include <iomanip>

namespace Norbert::Infra
{
    std::ostream& operator<<(std::ostream &strm, const byte &arg)
    {
        strm << "0x" << std::hex << std::uppercase << std::setw(2) << std::setfill('0') << static_cast<int>(arg);
        if (arg.isASCIIbyte())
        {
            strm << " (";
            if (arg == 0x28)
                strm << "left parenthesis";
            else if (arg == 0x29)
                strm << "right parenthesis";
            else
                strm << static_cast<char>(arg);
            strm << ")";
        }
        return strm;
    }
};
