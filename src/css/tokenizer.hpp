#pragma once

#include "encoding/encoding.hpp"
#include "infra/infra.hpp"

namespace Norbert::CSS
{
    enum class TokenType {
        Ident, Function, AtKeyword, Hash, String, BadString, Url, BadUrl,
        Delim, Number, Percentage, Dimension, UnicodeRange, Whitespace, CDO, CDC, Colon,
        Semicolon, Comma, LeftBracket, RightBracket, LeftParen, RightParen,
        LeftBrace, RightBrace, EOF
    };

    enum class HashType { Id, Unrestricted };
    enum class NumberType { Integer, Number };

    struct Token {
        TokenType type;
        std::variant<std::monostate, Infra::string, Infra::code_point, double> value = {};
        HashType hashType = HashType::Unrestricted;
        std::optional<Infra::code_point> sign = std::nullopt;
        NumberType numberType = NumberType::Integer; 
        std::optional<Infra::string> unit = std::nullopt;
        Infra::code_point start, end = 0;

        inline Token(TokenType type)
            : type(type) {}

        inline Token(TokenType type, const Infra::string& value, HashType hashType = HashType::Unrestricted) 
            : type(type), value(value), hashType(hashType) {}

        inline Token(TokenType type, Infra::code_point value) 
            : type(type), value(value) {}
            
        inline Token(TokenType type, double value, std::optional<Infra::code_point> sign = std::nullopt, NumberType numberType = NumberType::Integer, std::optional<Infra::string> unit = std::nullopt)
            : type(type), value(value), sign(sign), numberType(numberType), unit(unit) {}

        inline Token(TokenType type, Infra::code_point start, Infra::code_point end)
            : type(type), start(start), end(end) {}
    };

    class Tokenizer
    {
    public:
        inline Tokenizer(Encoding::io_queue<Infra::code_point>& stream)
            : stream(stream) {}

    public:
        Infra::list<Token> tokenize();
        Token consumeToken(bool unicodeRangesAllowed = false);

    private:
        void parseError();

    private:
        Infra::string nextInputCodePoint(size_t n);
        inline Infra::code_point nextInputCodePoint() {  return this->nextInputCodePoint(1)[0]; }
        Infra::code_point currentInputCodePoint = 0;
        Infra::string consume(size_t n);
        inline Infra::code_point consume() { return this->consume(1)[0]; }
        void reconsumeCurrentInputCodePoint();

    private:
        void consumeComments();
        Token consumeNumericToken();
        Token consumeIdentLikeToken();
        Token consumeStringToken(std::optional<Infra::code_point> optionalEndingCodePoint = std::nullopt);
        Token consumeUrlToken();
        Infra::code_point consumeEscapedCodePoint();
        Infra::string consumeIdentSequence();
        std::tuple<double, NumberType, std::optional<Infra::code_point>> consumeNumber();
        Token consumeUnicodeRangeToken();
        void consumeRemnantsOfBadUrl();

        bool areValidEscape(Infra::code_point c1, Infra::code_point c2);
        inline bool isValidEscape(const Infra::string& s) { if(s.length() != 2) throw std::runtime_error("Invalid escape sequence"); return this->areValidEscape(s[0], s[1]); }
        inline bool streamStartsWithValidEscape() { return this->areValidEscape(this->currentInputCodePoint, this->nextInputCodePoint()); }

        bool wouldStartIdentSequence(Infra::code_point c1, Infra::code_point c2, Infra::code_point c3);
        inline bool wouldStartIdentSequence(const Infra::string& s) { if(s.length() != 3) throw std::runtime_error("Invalid ident sequence"); return this->wouldStartIdentSequence(s[0], s[1], s[2]); }
        
        bool wouldStartNumber(Infra::code_point c1, Infra::code_point c2, Infra::code_point c3);
        inline bool wouldStartNumber(const Infra::string& s) { if(s.length() != 3) throw std::runtime_error("Invalid number sequence"); return this->wouldStartNumber(s[0], s[1], s[2]); }
        inline bool streamStartsWithNumber() { return this->wouldStartNumber(Infra::string{this->currentInputCodePoint} + this->nextInputCodePoint(2)); }

        bool wouldStartUnicodeRange(Infra::code_point c1, Infra::code_point c2, Infra::code_point c3);
        inline bool wouldStartUnicodeRange(const Infra::string& s) { if(s.length() != 3) throw std::runtime_error("Invalid unicode range sequence"); return this->wouldStartUnicodeRange(s[0], s[1], s[2]); }
        inline bool streamStartsWithUnicodeRange() { return this->wouldStartUnicodeRange(Infra::string{this->currentInputCodePoint} + this->nextInputCodePoint(2)); }

        double convertStringToNumber(const Infra::string& repr);

    private:
        Encoding::io_queue<Infra::code_point>& stream;
    };
}
