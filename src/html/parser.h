#pragma once

#include <oop/object.h>

#include <html/open_elements.h>
#include <html/tokenizer.h>
#include <dom.h>

#include <utils/stack.h>

typedef enum HTMLParser_insertion_modes
{
    HTMLParser_insertion_mode_Initial,

    HTMLParser_insertion_mode_BeforeHTML,
    HTMLParser_insertion_mode_BeforeHead,
    HTMLParser_insertion_mode_InHead,
    HTMLParser_insertion_mode_InHeadNoScript,
    HTMLParser_insertion_mode_AfterHead,
    
    HTMLParser_insertion_mode_InBody,
    HTMLParser_insertion_mode_AfterBody,
    HTMLParser_insertion_mode_AfterAfterBody,
    
    HTMLParser_insertion_mode_InTemplate,

    HTMLParser_insertion_mode_InTable,

    HTMLParser_insertion_mode_Text,

    HTMLParser_insertion_mode_NONE,
} HTMLParser_insertion_modes;

typedef struct ParserRec
{
    ObjectExtends(Object);

    type(DOM.Node)* (*parse)();

    HTMLTokenizer* tokenizer;
    HTMLParser_insertion_modes insertionMode;
    HTMLParser_insertion_modes originalInsertionMode;
    
    HTMLToken* currentToken;
    type(DOM.Node)* document;
    type(DOM.Node)* head;
    StackOfOpenElements* openElements;
    Stack* templateInsertionModes;

    bool reprocess;
    struct
    {
        bool parserCannotChangeTheMode;
        bool framesetOk;
        bool scripting;
    } flags;
} HTMLParser;

HTMLParser* HTMLParser_new(WCStream* stream);
