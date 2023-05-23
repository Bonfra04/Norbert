#pragma once

#include <oop/object.h>

#include <utils/stream/consumable.h>
#include <utils/vector.h>

typedef struct Stream
{
    ObjectExtends(Object);

    void* (*Consume)();
    void* (*Current)();
    size_t (*ConsumeN)(size_t n, void** out);
    void* (*Peek)(size_t n);
    void* (*Next)();
    void (*Reconsume)();
    size_t (*ReconsumeN)(size_t n);

    Consumable* source;
    Vector* data;
    size_t pos;
} Stream;

Stream* Stream_new(Consumable* source);
