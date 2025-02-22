#pragma once

#include <cstdint>
#include <ostream>
#undef EOF

namespace Norbert::Infra
{
    class byte
    {
    public:
        inline byte(uint8_t value = 0) : value(value) {}
        inline operator uint8_t() const { return this->value; }
        inline byte& operator=(uint8_t value) { this->value = value; return *this; }

        inline bool isASCIIbyte() const { return this->value < 0x7F; }

    public:
        friend std::ostream& operator<<(std::ostream &strm, const byte &arg);

    private:
        uint8_t value;
    };
}
