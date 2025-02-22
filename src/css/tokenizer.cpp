#include "css/tokenizer.hpp"
#include "css/definitions.hpp"

#include <cmath>
#include <iostream> // TODO: better handle parse errors

using namespace Norbert::Infra;
using namespace Norbert::Encoding;
using namespace Norbert::CSS::Definitions;

namespace Norbert::CSS
{
    list<Token> Tokenizer::tokenize()
    {
        list<Token> tokens;
        while(true)
        {
            Token token = this->consumeToken(true);
            tokens.append(token);
            if(token.type == TokenType::EOF)
                break;
        }
        return tokens;
    }

    Token Tokenizer::consumeToken(bool unicodeRangesAllowed)
    {
        this->consumeComments();
       
        switch(this->consume())
        {
            case whitespace:
            {
                while(isWhitespace(this->nextInputCodePoint()))
                    this->consume();
                return TokenType::Whitespace;
            }
            case '"':
            {
                return this->consumeStringToken();
            }
            case '#':
            {
                if(isIdentCodePoint(this->nextInputCodePoint()) || this->isValidEscape(this->nextInputCodePoint(2)))
                {
                    Token hashToken = TokenType::Hash;
                    if(this->wouldStartIdentSequence(this->nextInputCodePoint(3)))
                        hashToken.hashType = HashType::Id;
                    hashToken.value = this->consumeIdentSequence();
                    return hashToken;
                }
                else
                    return Token(TokenType::Delim, this->currentInputCodePoint);
            }
            case '\'':
            {
                return this->consumeStringToken();
            }
            case '(':
            {
                return TokenType::LeftParen;
            }
            case ')':
            {
                return TokenType::RightParen;
            }
            case '+':
            {
                if(this->streamStartsWithNumber())
                {
                    this->reconsumeCurrentInputCodePoint();
                    return this->consumeNumericToken();
                }
                else
                    return Token(TokenType::Delim, this->currentInputCodePoint);
            }
            case ',':
            {
                return TokenType::Comma;
            }
            case '-':
            {
                if(this->streamStartsWithNumber())
                {
                    this->reconsumeCurrentInputCodePoint();
                    return this->consumeNumericToken();
                }
                else if (this->nextInputCodePoint(2) == "->")
                {
                    this->consume(2);
                    return TokenType::CDC;
                }
                else
                    return Token(TokenType::Delim, this->currentInputCodePoint);
            }
            case '.':
            {
                if(this->streamStartsWithNumber())
                {
                    this->reconsumeCurrentInputCodePoint();
                    return this->consumeNumericToken();
                }
                else
                    return Token(TokenType::Delim, this->currentInputCodePoint);
            }
            case ':':
            {
                return TokenType::Colon;
            }
            case ';':
            {
                return TokenType::Semicolon;
            }
            case '<':
            {
                if(this->nextInputCodePoint(3) == "!--")
                {
                    this->consume(3);
                    return TokenType::CDO;
                }
                else
                    return Token(TokenType::Delim, this->currentInputCodePoint);
            }
            case '@':
            {
                if(this->wouldStartIdentSequence(this->nextInputCodePoint(3)))
                    return Token(TokenType::AtKeyword, this->consumeIdentSequence());
                else
                    return Token(TokenType::Delim, this->currentInputCodePoint);
            }
            case '[':
            {
                return TokenType::LeftBracket;
            }
            case '\\':
            {
                if(this->streamStartsWithValidEscape())
                {
                    this->reconsumeCurrentInputCodePoint();
                    return this->consumeIdentLikeToken();
                }
                else
                {
                    this->parseError();
                    return Token(TokenType::Delim, this->currentInputCodePoint);
                }
            }
            case ']':
            {
                return TokenType::RightBracket;
            }
            case '{':
            {
                return TokenType::LeftBrace;
            }
            case '}':
            {
                return TokenType::RightBrace;
            }
            case digit:
            {
                this->reconsumeCurrentInputCodePoint();
                return this->consumeNumericToken();
            }
            case 'U':
            case 'u':
            {
                if(unicodeRangesAllowed && this->streamStartsWithUnicodeRange())
                {
                    this->reconsumeCurrentInputCodePoint();
                    return this->consumeUnicodeRangeToken();
                }
                else
                {
                    this->reconsumeCurrentInputCodePoint();
                    return this->consumeIdentLikeToken();
                }
                break;
            }
            case EOF:
            {
                return TokenType::EOF;
            }
            default:
            {
                if(isIdentStartCodePoint(this->currentInputCodePoint))
                {
                    this->reconsumeCurrentInputCodePoint();
                    return this->consumeIdentLikeToken();
                }

                return Token(TokenType::Delim, this->currentInputCodePoint);
            }
        }
    }

    void Tokenizer::parseError()
    {
        // TODO: better handle parse errors
        std::cerr << "IDK Error thrown\nAdd proper error handling (maybe borrow from minsk)" << std::endl;
    }

    string Tokenizer::nextInputCodePoint(size_t n)
    {
        list<code_point> codePoints = this->stream.peek(n);
        while(codePoints.size() < n)
            codePoints.append(EOF);
        return codePoints;
    }

    string Tokenizer::consume(size_t n)
    {
        list<code_point> codePoints = this->stream.read(n);
        while(codePoints.size() < n)
            codePoints.append(EOF);
        this->currentInputCodePoint = codePoints[codePoints.size() - 1];
        return codePoints;
    }

    void Tokenizer::reconsumeCurrentInputCodePoint()
    {
        this->stream.restore(this->currentInputCodePoint);
    }

    void Tokenizer::consumeComments()
    {
        while(this->nextInputCodePoint(2) == "/*")
        {
            this->consume(2);
            while(this->nextInputCodePoint(2) != "*/")
            {
                if(this->consume() == EOF)
                {
                    this->parseError();
                    return;
                }
            }
            this->consume(2);
        }
    }

    Token Tokenizer::consumeNumericToken()
    {
        auto[value, numberType, sign] = this->consumeNumber();

        if(this->wouldStartIdentSequence(this->nextInputCodePoint(3)))
        {
            Token dimensionToken = Token(TokenType::Dimension, value, sign, NumberType::Number, "");
            dimensionToken.unit = this->consumeIdentSequence();
            return dimensionToken;
        }
        else if(this->nextInputCodePoint() == '%')
        {
            this->consume();
            return Token(TokenType::Percentage, value, sign);
        }
        else
            return Token(TokenType::Number, value, sign, numberType);
    }

    Token Tokenizer::consumeIdentLikeToken()
    {
        string s = this->consumeIdentSequence();
        if(s.ASCIICaseInsentivieMatch("url") && this->nextInputCodePoint() == '(')
        {
            this->consume();
            while(isWhitespace(this->nextInputCodePoint(2)))
                this->consume();

            string nextTwoCodePoints = this->nextInputCodePoint(2);
            if(nextTwoCodePoints(0) == '"' || nextTwoCodePoints(0) == '\'' || (isWhitespace(nextTwoCodePoints(0)) && (nextTwoCodePoints(1) == '"' || nextTwoCodePoints(1) == '\'')))
                return Token(TokenType::Function, s);
            else
                return this->consumeUrlToken();
        }
        else if (this->nextInputCodePoint() == '(')
        {
            this->consume();
            return Token(TokenType::Function, s);
        }
        else
            return Token(TokenType::Ident, s);
    }

    Token Tokenizer::consumeStringToken(std::optional<code_point> optionalEndingCodePoint)
    {
        code_point endingCodePoint = optionalEndingCodePoint.value_or(this->currentInputCodePoint);

        Token stringToken(TokenType::String, "");

        while(true)
        {
            this->consume();
            if(this->currentInputCodePoint == endingCodePoint)
                return stringToken;
            switch(this->currentInputCodePoint)
            {
                case EOF:
                {
                    this->parseError();
                    return stringToken;
                }
                case newline:
                {
                    this->parseError();
                    this->reconsumeCurrentInputCodePoint();
                    return TokenType::BadString;
                }
                case '\\':
                {
                    if(this->nextInputCodePoint() == EOF)
                        ;
                    else if(this->nextInputCodePoint() == newline)
                        this->consume();
                    else if (this->streamStartsWithValidEscape())
                    {
                        code_point escapedCodePoint = this->consumeEscapedCodePoint();
                        std::get<string>(stringToken.value) += escapedCodePoint;
                    }
                    break;
                }
                default:
                {
                    std::get<string>(stringToken.value) += this->currentInputCodePoint;
                }
            }
        }
    }

    Token Tokenizer::consumeUrlToken()
    {
        Token urlToken = Token(TokenType::Url, "");

        while(isWhitespace(this->nextInputCodePoint()))
            this->consume();

        while(true)
        {
            switch(this->consume())
            {
                case ')':
                {
                    return urlToken;
                }
                case EOF:
                {
                    this->parseError();
                    return urlToken;
                }
                case whitespace:
                {
                    while(isWhitespace(this->nextInputCodePoint()))
                        this->consume();
                    if(this->nextInputCodePoint() == ')' || this->nextInputCodePoint() == EOF)
                    {
                        this->consume();
                        if(this->currentInputCodePoint == EOF)
                            this->parseError();
                        return urlToken;
                    }
                    else
                    {
                        this->consumeRemnantsOfBadUrl();
                        return TokenType::BadUrl;
                    }
                }
                case '"': case '\'': case '(': case nonPrintableCodePoint_exceptEOF:
                {
                    this->parseError();
                    this->consumeRemnantsOfBadUrl();
                    return TokenType::BadUrl;
                }
                case '\\':
                {
                    if(this->streamStartsWithValidEscape())
                        std::get<string>(urlToken.value) += this->consumeEscapedCodePoint();
                    else
                    {
                        this->parseError();
                        this->consumeRemnantsOfBadUrl();
                        return TokenType::BadUrl;
                    }
                }
                default:
                {
                    std::get<string>(urlToken.value) += this->currentInputCodePoint;
                }
            }
        }
    }

    code_point Tokenizer::consumeEscapedCodePoint()
    {
        switch(this->consume())
        {
            case hexDigit:
            {
                uint32_t codePoint = 0;
                for(size_t i = 0; i < 6; i++)
                {
                    uint8_t unitValue = 0;
                    if(this->currentInputCodePoint.isASCIIDigit())
                        unitValue = this->currentInputCodePoint - '0';
                    else if(this->currentInputCodePoint.isASCIIUpperHexDigit())
                        unitValue = this->currentInputCodePoint - 'A' + 10;
                    else if(this->currentInputCodePoint.isASCIILowerHexDigit())
                        unitValue = this->currentInputCodePoint - 'a' + 10;

                    codePoint = codePoint * 16 + unitValue;

                    if(this->nextInputCodePoint().isASCIIHexDigit() && i < 5)
                        this->consume();
                }

                if(isWhitespace(this->nextInputCodePoint()))
                    this->consume();

                if (codePoint == 0 || codePoint > maximumAllowedCodePoint || code_point(codePoint).isSurrogate() )
                    return 0xFFFD;
                else
                    return codePoint;
            }
            case EOF:
            {
                this->parseError();
                return 0xFFFD;
            }
            default:
            {
                return this->currentInputCodePoint;
            }
        }
    }

    string Tokenizer::consumeIdentSequence()
    {
        string result = "";
        while(true)
        {
            switch(this->consume())
            {
                case identCodePoint:
                {
                    result += this->currentInputCodePoint;
                    break;
                }
                default:
                {
                    if(this->streamStartsWithValidEscape())
                    {
                        code_point escapedCodePoint = this->consumeEscapedCodePoint();
                        result += escapedCodePoint;
                    }
                    else
                    {
                        this->reconsumeCurrentInputCodePoint();
                        return result;
                    }
                }
            }
        }
    }

    std::tuple<double, NumberType, std::optional<code_point>> Tokenizer::consumeNumber()
    {
        NumberType numberType = NumberType::Integer;
        std::optional<code_point> sign = std::nullopt;
        string numberPart = "", exponentPart = "";

        if(this->nextInputCodePoint() == '+' || this->nextInputCodePoint() == '-')
            sign = this->consume();

        while(isDigit(this->nextInputCodePoint()))
            numberPart += this->consume();

        string nextTwoCodePoints = this->nextInputCodePoint(2);
        if(nextTwoCodePoints(0) == '.' && isDigit(nextTwoCodePoints(1)))
        {
            numberPart += this->consume();
            while(isDigit(this->nextInputCodePoint()))
                numberPart += this->consume();
            numberType = NumberType::Number;
        }

        string nextThreeCodePoints = this->nextInputCodePoint(3);
        if(nextThreeCodePoints(0) == 'E' || nextThreeCodePoints(0) == 'e')
        {
            bool hasSign = nextThreeCodePoints(1) == '+' || nextThreeCodePoints[1] == '-';
            if(isDigit(nextThreeCodePoints(hasSign ? 2 : 1)))
            {
                this->consume();
                if(hasSign)
                    exponentPart += this->consume();
                while(isDigit(this->nextInputCodePoint()))
                    exponentPart += this->consume();
                numberType = NumberType::Number;
            }            
        }

        double value = convertStringToNumber(numberPart + (exponentPart.length() == 0 ? "" : string("E") + exponentPart));
        return { value, numberType, sign };
    }

    Token Tokenizer::consumeUnicodeRangeToken()
    {
        this->consume(2);

        code_point startOfRange;
        list<code_point> firstSegment;
        for(size_t i = 0; i < 6; i++)
            if(isHexDigit(this->nextInputCodePoint()))
                firstSegment.append(this->consume());
            else
                break;
        for (size_t i = 0; i < 6 - firstSegment.size(); i++)
            if(this->nextInputCodePoint() == '?')
                firstSegment.append(this->consume());
            else
                break;

        if(firstSegment.contains('?'))
        {
            list<code_point> secondSegment = firstSegment;
            firstSegment.replace([&](const code_point& c) { return c == '?'; }, '0');
            startOfRange = string(firstSegment).toCodePointAsHex();

            secondSegment.replace([&](const code_point& c) { return c == '?'; }, 'F');
            code_point endOfRange = string(secondSegment).toCodePointAsHex();

            return Token(TokenType::UnicodeRange, startOfRange, endOfRange);
        }
        else
            startOfRange = string(firstSegment).toCodePointAsHex();

        string nextTwoCodePoints = this->nextInputCodePoint(2);
        if(nextTwoCodePoints(0) == '-'  && isHexDigit(nextTwoCodePoints(1)))
        {
            this->consume();

            list<code_point> secondSegment;
            for(size_t i = 0; i < 6; i++)
            if(isHexDigit(this->nextInputCodePoint()))
                firstSegment.append(this->consume());
            else
                break;

            double endOfRange = string(secondSegment).toCodePointAsHex();
            return Token(TokenType::UnicodeRange, startOfRange, endOfRange);
        }
        else
            return Token(TokenType::UnicodeRange, startOfRange, startOfRange);
    }

    void Tokenizer::consumeRemnantsOfBadUrl()
    {
        while(true)
        {
            switch(this->consume())
            {
                case ')':
                case EOF:
                {
                    return;
                }
                default:
                {
                    if(this->streamStartsWithValidEscape())
                        this->consumeEscapedCodePoint();
                }
            }
        }
    }

    bool Tokenizer::areValidEscape(code_point c1, code_point c2)
    {
        if (c1 != '\\')
            return false;
        else if (c2 == newline)
            return false;
        else
            return true;
    }

    bool Tokenizer::wouldStartIdentSequence(code_point c1, code_point c2, code_point c3)
    {
        switch(c1)
        {
            case '-':
            {
                if(isIdentStartCodePoint(c2) || c2 == '-' || this->areValidEscape(c2, c3))
                    return true;
                else
                    return false;
            }
            case identStartCodePoint:
            {
                return true;
            }
            case '\\':
            {
                if (this->areValidEscape(c1, c2))
                    return true;
                else
                    return false;
            }
            default:
            {
                return false;
            }
        }
    }

    bool Tokenizer::wouldStartNumber(code_point c1, code_point c2, code_point c3)
    {
        switch(c1)
        {
            case '+':
            case '-':
            {
                if(isDigit(c2))
                    return true;
                else if(c2 == '.' && isDigit(c3))
                    return true;
                else
                    return false;
            }
            case '.':
            {
                if (isDigit(c2))
                    return true;
                else
                    return false;
            }
            case digit:
            {
                return true;
            }
            default:
            {
                return false;
            }
        }
    }

    bool Tokenizer::wouldStartUnicodeRange(Infra::code_point c1, Infra::code_point c2, Infra::code_point c3)
    {
        if ((c1 == 'U' || c1 == 'u') && c2 == '+' && (c3 == '?' || isHexDigit(c3)))
            return true;
        else
            return false;
    }

    double Tokenizer::convertStringToNumber(const Infra::string& repr)
    {
        double s = 1;
        double i = 0;
        double f = 0;
        double d = 0;
        double t = 1;
        double e = 0;

        size_t index = 0;
        if(repr(index) == '+' || repr(index) == '-')
        {
            s = repr(index) == '-' ? -1 : 1;
            index++;
        }

        while(isDigit(repr(index)))
        {
            i = i * 10 + (repr(index) - '0');
            index++;
        }

        if(repr(index) == '.')
        {
            index++;
            while(isDigit(repr(index)))
            {
                f = f * 10 + (repr(index) - '0');
                d++;
                index++;
            }
        }

        if(repr(index) == 'E' || repr(index) == 'e')
        {
            index++;
            if(repr(index) == '+' || repr(index) == '-')
            {
                t = repr(index) == '-' ? -1 : 1;
                index++;
            }

            while(isDigit(repr(index)))
            {
                e = e * 10 + (repr(index) - '0');
                index++;
            }
        }

        return s * (i + f * pow(10, -d)) * pow(10, t * e);
    }
}
