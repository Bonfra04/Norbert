#pragma once

#include "consumable.h"

#include <stdbool.h>

typedef struct stream
{
    consumable_t source;
    wchar_t* data;
    size_t pos;
} stream_t;

stream_t stream_new(consumable_t source);
void stream_free(stream_t* stream);

wchar_t stream_consume(stream_t* stream);
wchar_t stream_current(stream_t* stream);
void stream_reconsume(stream_t* stream);
size_t stream_consume_n(stream_t* stream, size_t n, wchar_t* out);
size_t stream_reconsume_n(stream_t* stream, size_t n);
bool stream_match(stream_t* stream, wchar_t* str, bool consume); 