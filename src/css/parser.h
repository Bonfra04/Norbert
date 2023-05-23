#pragma once

#include <oop/object.h>

#include <css/tokenizer.h>

typedef struct CSSParser
{
    ObjectExtends(Object);

    CSSTokenizer* tokenizer;
} CSSParser;

CSSParser* CSSParser_new(WCStream* stream);