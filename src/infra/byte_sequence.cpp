#include <iomanip>

#include "infra/byte_sequence.hpp"
#include "infra/string.hpp"

namespace Norbert::Infra
{
    byte_sequence byte_sequence::byteLowercase() const
    {
        byte_sequence result;
        for (const byte& b : bytes)
            if(b >= 0x41 && b <= 0x5A)
                result.bytes.push_back(b + 0x20);
            else
                result.bytes.push_back(b);
        return result;
    }
    
    byte_sequence byte_sequence::byteUppercase() const
    {
        byte_sequence result;
        for (const byte& b : bytes)
            if(b >= 0x61 && b <= 0x7A)
                result.bytes.push_back(b - 0x20);
            else
                result.bytes.push_back(b);
        return result;
    }

    bool byte_sequence::isPrefix(const byte_sequence& input) const
    {
        size_t i = 0;
        while (true)
        {
            if (i >= this->length())
                return true;
            if (i >= input.length())
                return false;
            byte potentialPrefixByte = this->bytes[i];
            byte inputByte = input.bytes[i];
            if (potentialPrefixByte != inputByte)
                return false;
            i++;
        }
    }

    bool byte_sequence::isByteLessThan(const byte_sequence& b) const
    {
        if (b.isPrefix(*this))
            return false;
        if (this->isPrefix(b))
            return true;

        size_t n;
        for (n = 0; n < this->length() && n < b.length(); n++)
            if (this->bytes[n] != b.bytes[n])
                break;

        if (this->bytes[n] < b.bytes[n])
            return true;
        return false;
    }

    string byte_sequence::isomorphicDecode() const
    {
        list<code_point> codePoints(this->bytes.begin(), this->bytes.end());
        return codePoints;
    }


    std::ostream& operator<<(std::ostream &strm, const byte_sequence &arg)
    {
        bool isASCII = true;
        for (const byte& b : arg.bytes)
            if (b < 0x20 || b > 0x7E)
            {
                isASCII = false;
                break;
            }

        if (isASCII)
        {
            strm << "`";
            for (const byte& b : arg.bytes)
                strm << static_cast<char>(b);
            strm << "`";
        }
        else
        {
            for (size_t i = 0; i < arg.bytes.size(); ++i)
            {
                strm << "0x" << std::hex << std::setw(2) << std::setfill('0') << (int)arg.bytes[i];
                if (i != arg.bytes.size() - 1)
                    strm << " ";
            }
        }
        return strm;
    }
}