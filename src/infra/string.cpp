#include "infra/string.hpp"

namespace Norbert::Infra
{
    string::string(const char* cstr)
    {
        while (*cstr)
            this->codeUnits.push_back(static_cast<code_unit>(*cstr++));
    }

    string::string(const wchar_t* cwstr)
    {
        while (*cwstr)
            this->codeUnits.push_back(static_cast<code_unit>(*cwstr++));
    }

    string::string(list<code_point> list)
    {
        for (const code_point& point : list)
            if (point > 0xFFFF)
            {
                code_point temp = point - 0x10000;
                code_unit lead = (temp >> 10) + 0xD800;
                code_unit trail = (temp & 0x3FF) + 0xDC00;
                this->codeUnits.emplace_back(lead);
                this->codeUnits.emplace_back(trail);
            }
            else
                this->codeUnits.emplace_back(point);
    }

    list<code_point> string::asCodePoints() const
    {
        list<code_point> result;
        for (size_t i = 0; i < this->codeUnits.size(); i++)
        {
            code_point point = this->codeUnits[i];
            if (point.isLeadingSurrogate())
            {
                if (i + 1 < this->codeUnits.size())
                {
                    code_unit next = this->codeUnits[i + 1];
                    if (code_point(next).isTrailingSurrogate())
                    {
                        code_point full_codepoint = 0x10000 + ((point - 0xD800) << 10) + (next - 0xDC00);
                        result.append(full_codepoint);
                        i++;
                        continue;
                    }
                }
                result.append(point);
            }
            else
                result.append(point);
        }
        return result;
    }

    bool string::isASCIIString() const
    {
        list<code_point> codePoints = this->asCodePoints();
        for (const code_point& code : codePoints)
            if (code.isASCIICodePoint() == false)
                return false;
        return true;
    }

    bool string::isIsomorphicString() const
    {
        for (const code_point& code : this->asCodePoints())
            if (code > 0x00FF)
                return false;
        return true;
    }

    bool string::isScalarValueString() const
    {
        for (const code_point& code : this->asCodePoints())
            if (code.isScalarValue() == false)
                return false;
        return true;
    }

    string string::convertToScalarValueString() const
    {
        list<code_point> result;
        for (const code_point& code : this->asCodePoints())
            if (code.isSurrogate())
                result.append(0xFFFD);
            else
                result.append(code);
        return result;
    }

    bool string::isIdenticalTo(const string& other) const
    {
        if (this->codeUnits.size() != other.codeUnits.size())
            return false;
        for (size_t i = 0; i < this->codeUnits.size(); i++)
            if (this->codeUnits[i] != other.codeUnits[i])
                return false;
        return true;
    }

    bool string::isCodeUnitPrefix(string input) const
    {
        size_t i = 0;
        while (true)
        {
            if (i >= this->length())
                return true;
            if (i >= input.length())
                return false;

            code_unit potentialPrefixCodeUnit = this->codeUnits[i];
            code_unit inputCodeUnit = input.codeUnits[i];

            if (potentialPrefixCodeUnit != inputCodeUnit)
                return false;

            i++;
        }
    }

    bool string::isCodeUnitPostfix(string input) const
    {
        size_t i = 1;
        while (true)
        {
            size_t potentialSuffixIndex = this->length() - i;
            size_t inputIndex = input.length() - i;
            if (potentialSuffixIndex < 0)
                return true;
            if (inputIndex < 0)
                return false;
            code_unit potentialSuffixCodeUnit = this->codeUnits[potentialSuffixIndex];
            code_unit inputCodeUnit = input.codeUnits[inputIndex];
            if(potentialSuffixCodeUnit != inputCodeUnit)
                return false;
            i++;
        }
    }

    bool string::isCodeUnitLessThan(string b) const
    {
        if (b.isCodeUnitPrefix(*this))
            return false;
        if (this->isCodeUnitPrefix(b))
            return true;

        size_t n;
        for (n = 0; n < this->length() && n < b.length(); n++)
            if (this->codeUnits[n] != b.codeUnits[n])
                break;

        if (this->codeUnits[n] < b.codeUnits[n])
            return true;
        return false;       
    }

    string string::codeUnitSubstr(size_t start, size_t length) const
    {
        if (start < 0 || length < 0)
            throw std::out_of_range("start and length must not be negative");

        if (start + length > this->length())
            throw std::out_of_range("start is out of range");

        string result;
        ordered_set<size_t>::exclusiveRange(start, start + length).forEach([&](size_t i) {
            result.codeUnits.push_back(this->codeUnits[i]);
        });

        return result;
    }

    string string::codePointSubstr(size_t start, size_t length) const
    {
        if (start < 0 || length < 0)
            throw std::out_of_range("start and length must not be negative");

        if (start + length > this->codePointLength())
            throw std::out_of_range("start is out of range");

        list<code_point> codes = this->asCodePoints();
        list<code_point> result;
        ordered_set<size_t>::exclusiveRange(start, start + length).forEach([&](size_t i) {
            result.append(codes[i]);
        });

        return result;
    }

    byte_sequence string::isomorphicEncode() const
    {
        if (this->isIsomorphicString() == false)
            throw std::runtime_error("String is not isomorphic");

        list<code_point> codePoints = this->asCodePoints();
        return byte_sequence{codePoints.begin(), codePoints.end()};
    }

    string string::ASCIILowercase() const
    {
        list<code_point> result;
        for (const code_point& code : this->asCodePoints())
            if (code.isASCIIUpperAlpha())
                result.append(code + 0x20);
            else
                result.append(code);
        return result;
    }

    string string::ASCIIUppercase() const
    {
        list<code_point> result;
        for (const code_point& code : this->asCodePoints())
            if (code.isASCIILowerAlpha())
                result.append(code - 0x20);
            else
                result.append(code);
        return result;
    }

    byte_sequence string::ASCIIEncode() const
    {
        if (!this->isASCIIString())
            throw std::runtime_error("String contains non-ASCII characters");

        return this->isomorphicEncode();
    }

    string string::ASCIIDecode(const byte_sequence& input)
    {
        for (const byte& b : input)
            if (!b.isASCIIbyte())
                throw std::runtime_error("Byte sequence contains non-ASCII characters");
        return input.isomorphicDecode();
    }

    string string::stripNewlines() const
    {
        list<code_point> result;
        for (const code_point& code : this->asCodePoints())
            if (code != 0x000D && code != 0x000A)
                result.append(code);
        return result;
    }

    string string::normalizeNewlines() const
    {
        list<code_point> result;
        list<code_point> codePoints = this->asCodePoints();
        for (size_t i = 0; i < codePoints.size(); i++)
            if (codePoints[i] == 0x000D)
            {
                if (i + 1 < codePoints.size() && codePoints[i + 1] == 0x000A)
                    i++;
                result.append(0x000A);
            }
            else
                result.append(codePoints[i]);
        return result;
    }

    string string::stripLeadingAndTrailingASCIIWhitespace() const
    {
        list<code_point> result;
        list<code_point> codePoints = this->asCodePoints();
        size_t start = 0;
        size_t end = this->codePointLength();
        while (start < end && codePoints[start].isASCIIWhitespace())
            start++;
        while (end > 0 && codePoints[end - 1].isASCIIWhitespace())
            end--;
        return this->codePointSubstring(start, end);
    }

    string string::stripAndCollapseASCIIWhitespace() const
    {
        list<code_point> result;
        bool lastWasWhitespace = false;
        for (const code_point& code : this->asCodePoints())
        {
            if (code.isASCIIWhitespace())
            {
                if (!lastWasWhitespace)
                    result.append(0x0020);
                lastWasWhitespace = true;
            }
            else
            {
                result.append(code);
                lastWasWhitespace = false;
            }
        }
        return string(result).stripLeadingAndTrailingASCIIWhitespace();
    }

    string string::collectSequenceOfCodePoints(std::function<bool(const code_point&)> condition, size_t& position) const
    {
        list<code_point> result;
        list<code_point> codePoints = this->asCodePoints();
        while (position < codePoints.size() && condition(codePoints[position]))
        {
            result.append(codePoints[position++]);
            position++;
        }
        return result;
    }

    list<string> string::strictlySplit(code_point delimiter) const
    {
        size_t position = 0;
        list<string> tokens;

        string token = this->collectSequenceOfCodePoints([&](const code_point& code) { return code != delimiter; }, position);
        tokens.append(token);

        list<code_point> codePoints = this->asCodePoints();
        while(position < codePoints.size())
        {
            if(codePoints[position] != delimiter)
                throw std::runtime_error("Delimiter not found");
            position++;
            string token = this->collectSequenceOfCodePoints([&](const code_point& code) { return code != delimiter; }, position);
            tokens.append(token);
        }

        return tokens;
    }

    list<string> string::splitOnASCIIWhitespace() const
    {
        size_t position = 0;
        list<string> tokens = list<string>();
        this->skipAsciiWhitespace(position);
        
        size_t length = this->codePointLength();
        while (position < length)
        {
            string token = this->collectSequenceOfCodePoints([](const code_point& code) { return code.isASCIIWhitespace(); }, position);
            tokens.append(token);
            this->skipAsciiWhitespace(position);
        }

        return tokens;
    }

    list<string> string::splitOnCommas() const
    {
        size_t position = 0;
        list<string> tokens = list<string>();
       
        size_t length = this->codePointLength();
        while (position < length)
        {
            string token = this->collectSequenceOfCodePoints([](const code_point& code) { return code != 0x002C; }, position);
            token = token.stripLeadingAndTrailingASCIIWhitespace();
            tokens.append(token);
            if(position < length)
            {
                if(this->asCodePoints()[position] != 0x002C)
                    throw std::runtime_error("Comma not found");
                position++;
            }
        }

        return tokens;        
    }

    string string::concatenate(const list<string>& strings, const string& separator)
    {
        if (strings.isEmpty())
            return "";

        string result = strings[0];
        for (size_t i = 1; i < strings.size(); i++)
        {
            result.codeUnits.insert(result.codeUnits.end(), separator.codeUnits.begin(), separator.codeUnits.end());
            result.codeUnits.insert(result.codeUnits.end(), strings[i].codeUnits.begin(), strings[i].codeUnits.end());
        }

        return result;
    }

    code_point string::toCodePointAsHex() const
    {
        code_point value = 0;
        for(const code_point& c : this->asCodePoints())
            value = value * 16 + (c >= '0' && c <= '9' ? c - '0' : c - 'A' + 10);
        return value;
    }
}
