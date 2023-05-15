#pragma once

#include <oop/object.h>

#include <stream/consumable.h>

#include <stdbool.h>

typedef struct StreamRec
{
    Object object;

    wchar_t (*consume)();
    wchar_t (*current)();
    void (*reconsume)();
    size_t (*consume_n)(size_t n, wchar_t* out);
    size_t (*reconsume_n)(size_t n);
    bool (*match)(wchar_t* str, bool consume, bool case_sensitive);

    consumable_t source;
    wchar_t* data;
    size_t pos;
} Stream;

Stream* Stream_new(consumable_t source);
void Stream_delete(Stream* self);
