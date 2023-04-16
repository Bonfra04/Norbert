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
    token_comment
} token_type_t;

typedef struct attribute
{
    wchar_t* name;
    wchar_t* value;
} attribute_t;

typedef struct token
{
    token_type_t type;
    // { character token
        wchar_t value;
    // } character token
    // { tag_token
        wchar_t* name;
        bool selfClosing;
        attribute_t* attributes;
    // } tag_token
    // { comment_token
        wchar_t* data;
    // } comment_token
} token_t;

void tokenizer_init(stream_t* html);

token_t* tokenizer_emit_token();
