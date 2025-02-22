#pragma once

#include <cstdint>
#include <iomanip>
#include <ostream>
#undef EOF

namespace Norbert::Infra
{
    class code_point
    {
    public:
        inline constexpr code_point(uint32_t value = 0)
            : value(value)
        {
            if (value > 0x10FFFF)
                throw std::out_of_range("Value out of range for CodePoint");
        }
        constexpr operator uint32_t() const { return this->value; }
        inline code_point& operator=(code_point value) { this->value = value.value; return *this; }

        inline bool isLeadingSurrogate() const { return this->value >= 0xD800 && this->value <= 0xDBFF; }
        inline bool isTrailingSurrogate() const { return this->value >= 0xDC00 && this->value <= 0xDFFF; }
        inline bool isSurrogate() const { return this->isLeadingSurrogate() || this->isTrailingSurrogate(); }
        inline bool isScalarValue() const { return !this->isSurrogate(); }
        // TODO: inline bool isNoncharacter() const { }
        inline bool isASCIICodePoint() const { return this->value <= 0x7F; }
        inline bool isASCIITabOrNewline() const { return this->value == 0x09 || this->value == 0x0A || this->value == 0x0D; }
        inline bool isASCIIWhitespace() const { return this->value == 0x09 || this->value == 0x0A || this->value == 0x0C || this->value == 0x0D || this->value == 0x20; }
        inline bool isC0Control() const { return this->value <= 0x1F; }
        inline bool isC0ControlOrSpace () const { return this->isC0Control() || this->value == 0x20; }
        inline bool isControl() const { return this->isC0Control() || (this->value >= 0x7F && this->value <= 0x9F); }
        inline bool isASCIIDigit() const { return this->value >= 0x30 && this->value <= 0x39; }
        inline bool isASCIIUpperHexDigit() const { return this->value >= 0x41 && this->value <= 0x46; }
        inline bool isASCIILowerHexDigit() const { return this->value >= 0x61 && this->value <= 0x66; }
        inline bool isASCIIHexDigit() const { return this->isASCIIDigit() || this->isASCIIUpperHexDigit() || this->isASCIILowerHexDigit(); }
        inline bool isASCIIUpperAlpha() const { return this->value >= 0x41 && this->value <= 0x5A; }
        inline bool isASCIILowerAlpha() const { return this->value >= 0x61 && this->value <= 0x7A; }
        inline bool isASCIIAlpha() const { return this->isASCIIUpperAlpha() || this->isASCIILowerAlpha(); }
        inline bool isASCIIAlphaNumeric() const { return this->isASCIIDigit() || this->isASCIIAlpha(); }

    public:
        inline friend std::ostream& operator<<(std::ostream &strm, const code_point &arg)
        {
            uint32_t value = static_cast<uint32_t>(arg);
            strm << "U+" << std::hex << std::uppercase << std::setw(4) << std::setfill('0') << value;
            if(value <= 0x7F)
            {
                strm << " (";
                if (value == 0x28)
                    strm << "left parenthesis";
                else if (value == 0x29)
                    strm << "right parenthesis";
                else
                    strm << static_cast<char>(arg);
                return strm << ")";
            }
            return strm;
        }

        inline friend std::wostream& operator<<(std::wostream &strm, const code_point &arg)
        {
            uint32_t value = static_cast<uint32_t>(arg);
            strm << L"U+" << std::hex << std::uppercase << std::setw(4) << std::setfill(L'0') << value;
            strm << L" (";
            if (value == 0x28)
                strm << L"left parenthesis";
            else if (value == 0x29)
                strm << L"right parenthesis";
            else
                strm << static_cast<wchar_t>(arg);
            return strm << L")";
        }

    private:
        uint32_t value;
    };
}
