#pragma once

#include "stream/stream.h"

#include <wchar.h>
#include <stdbool.h>

typedef enum token_type
{
    token_character,
    token_eof,
    token_start_tag,
    token_end_tag,
    token_comment,
    token_doctype
} token_type_t;

typedef struct token_attribute
{
    wchar_t* name;
    wchar_t* value;
} token_attribute_t;

typedef struct token
{
    token_type_t type;
    wchar_t value; // character
    union
    {
        wchar_t* name; // character / doctype
        wchar_t* data; // comment
    };
    union
    {
        bool selfClosing; // tag_token
        bool forceQuirks; // doctype
    };
    token_attribute_t* attributes; // tag_token
    wchar_t* public_identifier; // doctype
    wchar_t* system_identfier; // doctype
} token_t;

void tokenizer_init(stream_t* html);

token_t* tokenizer_emit_token();

void free_token(token_t* token);
