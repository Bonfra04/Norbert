#pragma once

#include <oop/object.h>

#include <utils/stream/wc_stream.h>
#include <utils/vector.h>
#include <utils/wstring.h>

typedef enum HTMLTokenType
{
    CSSTokenType_ident,
    CSSTokenType_function,
    CSSTokenType_atKeyword,
    CSSTokenType_hash,
    CSSTokenType_string,
    CSSTokenType_badString,
    CSSTokenType_url,
    CSSTokenType_badUrl,
    CSSTokenType_delim,
    CSSTokenType_number,
    CSSTokenType_percentage,
    CSSTokenType_dimension,
    CSSTokenType_whitespace,
    CSSTokenType_CDO,
    CSSTokenType_CDC,
    CSSTokenType_colon,
    CSSTokenType_semicolon,
    CSSTokenType_comma,
    CSSTokenType_leftSquareBracket,
    CSSTokenType_rightSquareBracket,
    CSSTokenType_leftParenthesis,
    CSSTokenType_rightParenthesis,
    CSSTokenType_leftCurlyBracket,
    CSSTokenType_rightCurlyBracket,
    CSSTokenType_EOF,
} HTMLTokenType;

typedef struct CSSToken
{
    HTMLTokenType type;
    union
    {
        struct
        {
            WString* value;
        } atKeyword, url, ident, string, function;
        struct
        {
            WString* value;
            enum { CSSToken_hash_id, CSSToken_hash_unrestriced } type;
        } hash;
        struct
        {
            wchar_t value;
        } delim;
        struct
        {
            float value;
        } percentage;
        struct
        {
            float value;
            enum { CSSToken_number_integer, CSSToken_number_number } type;
        } number;
        struct
        {
            float value;
            enum { CSSToken_dimension_integer, CSSToken_dimension_number } type;
            WString* unit;
        } dimension;
        struct
        {
        } EOF, rightSquareBracket, leftSquareBracket, leftCurlyBracket, rightCurlyBracket, badUrl, whitespace, badString, leftParenthesis, rightParenthesis, comma, colon, semicolon, CDC, CDO;
    } as;
} CSSToken;

typedef struct CSSTokenizer
{
    ObjectExtends(Object);

    CSSToken* (*ConsumeToken)();
    void (*DisposeToken)(CSSToken* token);

    WCStream* stream;
} CSSTokenizer;

CSSTokenizer* CSSTokenizer_new(WCStream* stream);
