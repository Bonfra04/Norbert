#include <css/tokenizer.h>

#include <css/definitions.h>

#include "errors.h"

#include <stdlib.h>

#define consumeComments() consume_comments(self)
#define consumeStringToken() consume_string_token(self, currentCodePoint())
#define consumeStringTokenWith(ending) consume_string_token(self, ending)
#define consumeEscapedCodePoint() consume_escaped_code_point(self)
#define consumeIdentSequence() consume_ident_sequence(self)
#define consumeNumericToken() consume_numeric_token(self)
#define consumeNumber() consume_number(self)
#define consumeIdentLikeToken() consume_ident_like_token(self)
#define consumeURLToken() consume_url_token(self)
#define consumeBadURLRemnants() consume_bad_url_remnants(self)

#define consumeAsMuchWhitespaceAsPossible() do { while(isWhitespace(nextCodePoint())) consumeCodePoint(); } while(0)

#define consumeCodePoint() self->stream->Consume()
#define consumeCodePoints(n) self->stream->ConsumeN(n, NULL)
#define reconsumeCodePoint() self->stream->Reconsume()

#define currentCodePoint() self->stream->Current()
#define nextCodePoint() self->stream->Next()
#define nextCodePoints(s) self->stream->Match(s, false, false)
#define nextCodePointsC(s) self->stream->Match(s, true, false)
#define nextTwoCodePoints() self->stream->Peek(1), self->stream->Peek(2)
#define nextThreeCodePoints() self->stream->Peek(1), self->stream->Peek(2), self->stream->Peek(3)
#define peekCodePoint(n) self->stream->Peek(n)

#define on_codepoint(condition) (currentCodePoint() == (CodePoint)(uint64_t)(condition))
#define on_function(condition) (((bool(*)(CodePoint))(uint64_t)(condition))(currentCodePoint()))
#define checker(condition) (_Generic((condition), CodePoint: on_codepoint(condition), default: on_function(condition)))
#define on(condition) if(checker(condition))
#define anythingElse() if(true)

#define Token(ttype, ...) ({ CSSToken* _ = malloc(sizeof(CSSToken)); *_ = (CSSToken){ .type = CSSTokenType_##ttype, .as = { .ttype = { __VA_ARGS__ } } }; _; })
#define whitespaceToken() Token(whitespace)
#define badStringToken() Token(badString)
#define leftParenthesisToken() Token(leftParenthesis)
#define rightParenthesisToken() Token(rightParenthesis)
#define commaToken() Token(comma)
#define colonToken() Token(colon)
#define semicolonToken() Token(semicolon)
#define CDCToken() Token(CDC)
#define CDOToken() Token(CDO)
#define badUrlToken() Token(badUrl)
#define leftSquareBracketToken() Token(leftSquareBracket)
#define rightSquareBracketToken() Token(rightSquareBracket)
#define leftCurlyBracketToken() Token(leftCurlyBracket)
#define rightCurlyBracketToken() Token(rightCurlyBracket)
#define EOFToken() Token(EOF)
#define stringToken() Token(string, .value = WString_new())
#define urlToken() Token(url, .value = WString_new())
#define functionToken(str) Token(function, .value = str)
#define atKeywordToken(str) Token(atKeyword, .value = str)
#define identToken(str) Token(ident, .value = str)
#define delimToken() Token(delim, .value = currentCodePoint())
#define hashToken() Token(hash, .value = WString_new(), .type = CSSToken_hash_unrestriced)
#define dimensionToken(number) Token(dimension, .value = (number).value, .type = (number).is_integer ? CSSToken_dimension_integer : CSSToken_dimension_number, .unit = WString_new())
#define percentageToken(number) Token(percentage, .value = (number).value)
#define numberToken(number) Token(number, .value = (number).value, .type = (number).is_integer ? CSSToken_number_integer : CSSToken_number_number)

#define areValidEscape(...) are_valid_escape_equence(self, __VA_ARGS__)
#define wouldStartIdentSequence(...) would_start_ident_sequence(self, __VA_ARGS__)
#define wouldStartNumber(...) would_start_number(self, ...)

#define startsValidEscape() are_valid_escape_equence(self, currentCodePoint(), nextCodePoint())
#define startsIdentSequence() would_start_ident_sequence(self, currentCodePoint(), nextTwoCodePoints())
#define startsNumber() would_start_number(self, currentCodePoint(), nextTwoCodePoints())

#define hexDigitToInt(digit) ({ int d = (digit); int _; if(d >= L'0' && d <= L'9') _ = d - L'0'; else if(d >= L'A' && d <= L'F') _ = d - L'A' + 10; else if(d >= L'a' && d <= L'f') _ = d - L'a' + 10; else _ = -1; _; })

typedef struct numeric_value
{
    float value;
    bool is_integer;
} numeric_value_t;

static bool would_start_number(CSSTokenizer* self, CodePoint a, CodePoint b, CodePoint c)
{
    if(a == L'+' || a == L'-')
    {
        if(isDigit(b))
        {
            return true;
        }
        else if(b == L'.' && isDigit(c))
        {
            return true;
        }
        else
        {
            return false;
        }
    }
    else if(a == '.')
    {
        if(isDigit(b))
        {
            return true;
        }
        else
        {
            return false;
        }
    }
    else if(isDigit(a))
    {
        return true;
    }
    else
    {
        return false;
    }
}

static bool are_valid_escape_equence(CSSTokenizer* self, CodePoint a, CodePoint b)
{
    if(a != L'\\')
    {
        return false;
    }
    else if(isNewline(b))
    {
        return false;
    }
    else
    {
        return true;
    }
}

static bool would_start_ident_sequence(CSSTokenizer* self, CodePoint a, CodePoint b, CodePoint c)
{
    if(a == L'-')
    {
        return isIdentStart(b) || b == L'-' || areValidEscape(b, c);
    }
    else if(isIdentStart(a))
    {
        return true;
    }
    else if(a == L'\\')
    {
        return areValidEscape(a, b);
    }
    else
    {
        return false;
    }
}

static void consume_comments(CSSTokenizer* self)
{
    if(nextCodePointsC(L"/*"))
    {
        while(currentCodePoint() != -1 && !nextCodePointsC(L"*/"))
        {
            consumeCodePoint();
        }
        if(currentCodePoint() == -1)
        {
            parser_error("generic");
        }
    }
}

static CodePoint consume_escaped_code_point(CSSTokenizer* self)
{
    consumeCodePoint();
    on(HexDigit)
    {
        uint32_t value = hexDigitToInt(currentCodePoint());
        int count = 0;
        while(isHexDigit(nextCodePoint()) && count < 5)
        {
            consumeCodePoint();
            value = value * 16 + hexDigitToInt(currentCodePoint());
            count++;
        }

        if(isWhitespace(nextCodePoint()))
        {
            consumeCodePoint();
        }

        if(value == 0x00 || isSurrogate(value) || value > 0x10FFFF)
        {
            return 0xFFFD;
        }

        return value;
    }
    on(-1)
    {
        parser_error("generic");
        return 0xFFFD;
    }
    anythingElse()
    {
        return currentCodePoint();
    }
}

static CSSToken* consume_string_token(CSSTokenizer* self, CodePoint ending)
{
    CSSToken* token = stringToken();

    while(true)
    {
        consumeCodePoint();
        on(ending)
        {
            return token;
        }
        on(-1)
        {
            parser_error("generic");
            return token;
        }
        on(Newline)
        {
            parser_error("generic");
            reconsumeCodePoint();
            self->DisposeToken(token);
            return badStringToken();
        }
        on(L'\\')
        {
            if(nextCodePoint() == -1)
            {
                continue;
            }
            else if(nextCodePoint() == '\n')
            {
                consumeCodePoint();
            }
            else
            {
                CodePoint codepoint = consumeEscapedCodePoint();
                token->as.string.value->appendwc(codepoint);
            }
            continue;
        }
        anythingElse()
        {
            token->as.string.value->appendwc(currentCodePoint());
        }
    }
}

static void consume_bad_url_remnants(CSSTokenizer* self)
{
    while(true)
    {
        consumeCodePoint();
        if(currentCodePoint() == L')')
        {
            return;
        }
        else if(currentCodePoint() == -1)
        {
            return;
        }
        else if(startsValidEscape())
        {
            consumeEscapedCodePoint();
            continue;
        }
        else
        {
            continue;
        }
    }
}

static CSSToken* consume_url_token(CSSTokenizer* self)
{
    CSSToken* token = urlToken();

    consumeAsMuchWhitespaceAsPossible();

    while(true)
    {
        consumeCodePoint();
        on(L')')
        {
            return token;
        }
        on(-1)
        {
            parser_error("generic");
            return token;
        }
        on(Whitespace)
        {
            consumeAsMuchWhitespaceAsPossible();

            if(nextCodePoint() == L')')
            {
                consumeCodePoint();
                return token;
            }
            else if(nextCodePoint() == -1)
            {
                consumeCodePoint();
                parser_error("generic");
                return token;
            }
            else
            {
                consumeBadURLRemnants();
                self->DisposeToken(token);
                return badUrlToken();
            }
        }
        on(L'"')
        {
            parser_error("generic");
            consumeBadURLRemnants();
            self->DisposeToken(token);
        }
        on(L'\'')
        {
            parser_error("generic");
            consumeBadURLRemnants();
            self->DisposeToken(token);
        }
        on(L'(')
        {
            parser_error("generic");
            consumeBadURLRemnants();
            self->DisposeToken(token);
        }
        on(NonPrintable)
        {
            parser_error("generic");
            consumeBadURLRemnants();
            self->DisposeToken(token);
        }
        on(L'\\')
        {
            if(startsValidEscape())
            {
                token->as.url.value->appendwc(consumeEscapedCodePoint());
            }
            else
            {
                parser_error("generic");
                consumeBadURLRemnants();
                self->DisposeToken(token);
            }
        }
        anythingElse()
        {
            token->as.url.value->appendwc(currentCodePoint());
        }
    }
}

static WString* consume_ident_sequence(CSSTokenizer* self)
{
    WString* result = WString_new();

    while(true)
    {
        consumeCodePoint();
        on(Ident)
        {
            result->appendwc(currentCodePoint());
            continue;
        }
        if(startsValidEscape())
        {
            CodePoint codepoint = consumeEscapedCodePoint();
            result->appendwc(codepoint);
            continue;
        }
        anythingElse()
        {
            reconsumeCodePoint();
            return result;
        }
    }
}

static CSSToken* consume_ident_like_token(CSSTokenizer* self)
{
    WString* string = consumeIdentSequence();

    if(string->equalssi("url") && nextCodePoint() == L'(')
    {
        consumeCodePoint();

        while(isWhitespace(nextCodePoint()) && isWhitespace(peekCodePoint(2)))
        {
            consumeCodePoint();
        }

        if(nextCodePoint() == L'"' || nextCodePoint() == L'\'' || (nextCodePoint() == L' ' && (peekCodePoint(2) == L'"' || peekCodePoint(2) == L'\'')))
        {
            return functionToken(string);
        }
        else
        {
            return consumeURLToken();
        }
    }
    else if(nextCodePoint() == L'(')
    {
        consumeCodePoint();
        return functionToken(string);
    }
    else
    {
        return identToken(string);
    }
}

static numeric_value_t consume_number(CSSTokenizer* self)
{
    bool is_integer = true;
    WString* repr = WString_new();

    if(nextCodePoint() == L'+' || nextCodePoint() == L'-')
    {
        repr->appendwc(consumeCodePoint());
    }

    while(isDigit(nextCodePoint()))
    {
        repr->appendwc(consumeCodePoint());
    }

    if(nextCodePoint() == L'.' && isDigit(peekCodePoint(2)))
    {
        repr->appendwc(consumeCodePoint());
        repr->appendwc(consumeCodePoint());
        is_integer = false;

        while(isDigit(nextCodePoint()))
        {
            repr->appendwc(consumeCodePoint());
        }
    }

    if(nextCodePoint() == L'e' || nextCodePoint() == L'E')
    {
        bool sign = peekCodePoint(2) == L'+' || peekCodePoint(2) == L'-';

        if((sign == false && isDigit(peekCodePoint(2))) || (sign == true && isDigit(peekCodePoint(3))))
        {
            repr->appendwc(consumeCodePoint());
            if(sign)
            {
                repr->appendwc(consumeCodePoint());
            }
            repr->appendwc(consumeCodePoint());
            is_integer = false;

            while(isDigit(nextCodePoint()))
            {
                repr->appendwc(consumeCodePoint());
            }
        }
    }

    numeric_value_t value = { .value = wcstof(repr->data, NULL), .is_integer = is_integer };
    repr->delete();
    return value;
}

static CSSToken* consume_numeric_token(CSSTokenizer* self)
{
    numeric_value_t number = consumeNumber();

    if(wouldStartIdentSequence(nextThreeCodePoints()))
    {
        CSSToken* token = dimensionToken(number);
        token->as.dimension.unit->append(consumeIdentSequence());
        return token;
    }
    else if(nextCodePoint() == L'%')
    {
        consumeCodePoint();
        return percentageToken(number);
    }
    else
    {
        return numberToken(number);
    }
}

static CSSToken* CSSTokenizer_ConsumeToken(CSSTokenizer* self)
{
    consumeComments();

    consumeCodePoint();
    on(Whitespace)
    {
        consumeAsMuchWhitespaceAsPossible();

        return whitespaceToken();
    }
    on(L'"')
    {
        return consumeStringToken();
    }
    on(L'#')
    {
        if(isIdent(nextCodePoint()) || areValidEscape(nextTwoCodePoints()))
        {
            CSSToken* token = hashToken();

            if(wouldStartIdentSequence(nextThreeCodePoints()))
            {
                token->as.hash.type = CSSToken_hash_id;
            }

            token->as.hash.value->append(consumeIdentSequence());

            return token;
        }

        return delimToken();
    }
    on(L'\'')
    {
        return consumeStringToken(); 
    }
    on(L'(')
    {
        return leftParenthesisToken();
    }
    on(L')')
    {
        return rightParenthesisToken();
    }
    on(L'+')
    {
        if(startsNumber())
        {
            reconsumeCodePoint();
            return consumeNumericToken();
        }

        return delimToken();
    }
    on(L',')
    {
        return commaToken();
    }
    on(L'-')
    {
        if(startsNumber())
        {
            reconsumeCodePoint();
            return consumeNumericToken();
        }
        else if(nextCodePointsC(L"->"))
        {
            return CDCToken();
        }
        else if(startsIdentSequence())
        {
            reconsumeCodePoint();
            return consumeIdentLikeToken();
        }
        else
        {
            return delimToken();
        }
    }
    on(L'.')
    {
        if(startsNumber())
        {
            reconsumeCodePoint();
            return consumeNumericToken();
        }
        else
        {
            return delimToken();
        }
    }
    on(L':')
    {
        return colonToken();
    }
    on(L';')
    {
        return semicolonToken();
    }
    on(L'<')
    {
        if(nextCodePointsC(L"!--"))
        {
            return CDOToken();
        }
        else
        {
            return delimToken();
        }
    }
    on(L'@')
    {
        if(wouldStartIdentSequence(nextThreeCodePoints()))
        {
            return atKeywordToken(consumeIdentSequence());
        }
        else
        {
            return delimToken();
        }
    }
    on(L'[')
    {
        return leftSquareBracketToken();
    }
    on(L'\\')
    {
        if(areValidEscape(nextTwoCodePoints()))
        {
            reconsumeCodePoint();
            return consumeIdentLikeToken();
        }
        else
        {
            parser_error("generic");
            return delimToken();
        }
    }
    on(L']')
    {
        return rightSquareBracketToken();
    }
    on(L'{')
    {
        return leftCurlyBracketToken();
    }
    on(L'}')
    {
        return rightCurlyBracketToken();
    }
    on(Digit)
    {
        reconsumeCodePoint();
        return consumeNumericToken();
    }
    on(IdentStart)
    {
        reconsumeCodePoint();
        return consumeIdentLikeToken();
    }
    on(-1)
    {
        return EOFToken();
    }
    anythingElse()
    {
        return delimToken();
    }
}

static void CSSTokenizer_DisposeToken(CSSToken* token)
{
    switch(token->type)
    {
        case CSSTokenType_string:
            token->as.string.value->delete();
            break;
        default:
            break;
    }

    free(token);
}

static void CSSTokenizer_delete(CSSTokenizer* self)
{
    self->super.delete();
}

CSSTokenizer* CSSTokenizer_new(WCStream* stream)
{
    CSSTokenizer* self = ObjectBase(CSSTokenizer, 2);

    self->stream = stream;

    ObjectFunction(CSSTokenizer, ConsumeToken, 0);
    ObjectFunction(CSSTokenizer, DisposeToken, 1);

    Object_prepare((Object*)&self->super);
    return self;
}
