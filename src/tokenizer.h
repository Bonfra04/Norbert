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
    union
    {
        struct
        {
            wchar_t value;
        } character;
        struct
        {
            wchar_t* data;
        } comment;
        struct
        {
            wchar_t* name;
            bool selfClosing;
            token_attribute_t* attributes;
        } tag, start_tag, end_tag;
        struct
        {
            wchar_t* name;
            bool forceQuirks;
            wchar_t* public_identifier;
            wchar_t* system_identifier;
        } doctype;
        struct
        {
        } eof;
    } as;
} token_t;

void tokenizer_init(stream_t* html);

token_t* tokenizer_emit_token();

void free_token(token_t* token);
