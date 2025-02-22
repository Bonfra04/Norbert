#pragma once

#include <cstdint>
#include <vector>
#include <ostream>
#undef EOF

#include "infra/code_point.hpp"
#include "infra/ordered_set.hpp"
#include "infra/byte_sequence.hpp"

namespace Norbert::Infra
{
    using code_unit = uint16_t;
    
    class string
    {
    public:
        string() = default;
        
        inline string(std::initializer_list<code_unit> init) : codeUnits(init) {}
        inline string(list<code_unit> list) : codeUnits(list.begin(), list.end()) {}
        string(list<code_point> list);
        string(std::initializer_list<code_point> init) : string(list<code_point>(init)) {}
        string(const char* cstr);
        string(const wchar_t* cwstr);

        list<code_point> asCodePoints() const;

        inline size_t length() const { return this->codeUnits.size(); }
        inline size_t codePointLength() const { return this->asCodePoints().size(); }

        bool isASCIIString() const;
        bool isIsomorphicString() const;
        bool isScalarValueString() const;
        string convertToScalarValueString() const;

        bool isIdenticalTo(const string& other) const;
        bool isCodeUnitPrefix(string input) const;
        bool isCodeUnitPostfix(string input) const;
        bool isCodeUnitLessThan(string b) const;
        
        string codeUnitSubstr(size_t start, size_t length) const;
        inline string codeUnitSubstring(size_t start, size_t end) const { return this->codeUnitSubstr(start, end - start); }
        inline string codeUnitSubstring(size_t start) const { return this->codeUnitSubstring(start, this->length()); }
        string codePointSubstr(size_t start, size_t length) const;
        inline string codePointSubstring(size_t start, size_t end) const { return this->codePointSubstr(start, end - start); }
        inline string codePointSubstring(size_t start) const { return this->codePointSubstring(start, this->length()); }
        
        byte_sequence isomorphicEncode() const;
        string ASCIILowercase() const;
        string ASCIIUppercase() const;
        inline bool ASCIICaseInsentivieMatch(const string& B) const { return this->ASCIILowercase().isIdenticalTo(B.ASCIILowercase()); }
        byte_sequence ASCIIEncode() const;
        static string ASCIIDecode(const byte_sequence& input);
        
        string stripNewlines() const;
        string normalizeNewlines() const;
        string stripLeadingAndTrailingASCIIWhitespace() const;
        string stripAndCollapseASCIIWhitespace() const;

        string collectSequenceOfCodePoints(std::function<bool(const code_point&)> condition, size_t& position) const;
        inline void skipAsciiWhitespace(size_t& position) const { this->collectSequenceOfCodePoints([](const code_point& code) { return code.isASCIIWhitespace(); }, position);}
        
        list<string> strictlySplit(code_point delimiter) const;
        list<string> splitOnASCIIWhitespace() const;
        list<string> splitOnCommas() const;

        static string concatenate(const list<string>& strings, const string& separator = "");

        code_point toCodePointAsHex() const;

    public:
        inline code_unit operator[](size_t index) const { return this->codeUnits[index]; }
        inline code_unit& operator[](size_t index) { return this->codeUnits[index]; }
        inline code_point operator()(size_t index) const { return this->asCodePoints()[index]; }

        inline bool operator==(const string& other) const { return this->isIdenticalTo(other); }
        inline bool operator!=(const string& other) const { return !this->isIdenticalTo(other); }
        inline bool operator<(const string& other) const { return this->isCodeUnitLessThan(other); }
        inline bool operator<=(const string& other) const { return this->isCodeUnitLessThan(other) || this->isIdenticalTo(other); }
        inline bool operator>(const string& other) const { return !this->isCodeUnitLessThan(other) && !this->isIdenticalTo(other); }
        inline bool operator>=(const string& other) const { return !this->isCodeUnitLessThan(other); }

        inline string operator+(const string& other) const { return string::concatenate({*this, other}); }
        inline string& operator+=(const string& other) { return *this = *this + other; }
        inline string& operator+=(const code_point& codePoint) { return *this += string(list<code_point>{ codePoint }); }

        inline std::vector<code_point>::iterator begin() { codePointsCache = this->asCodePoints(); return codePointsCache.begin(); }
        inline std::vector<code_point>::const_iterator begin() const { codePointsCache = this->asCodePoints(); return codePointsCache.begin(); }
        inline std::vector<code_point>::iterator end() { return codePointsCache.end(); }
        inline std::vector<code_point>::const_iterator end() const { return codePointsCache.end(); }

    public:
        friend std::ostream& operator<<(std::ostream& strm, const string& str)
        {
            strm << "\"";
            for (const code_unit& codeUnit : str.codeUnits)
                strm << static_cast<char>(codeUnit);
            return strm << "\"";
        }

        friend std::wostream& operator<<(std::wostream& strm, const string& str)
        {
            strm << L"\"";
            for (const code_point& codePoint : str.asCodePoints())
                strm << static_cast<wchar_t>(codePoint);
            return strm << L"\"";
        }

    private:
        std::vector<code_unit> codeUnits;
    private:
        mutable list<code_point> codePointsCache;
    };
}
