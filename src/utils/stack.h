#pragma once

#include <stddef.h>

#include "oop/object.h"

typedef struct StackRec
{
    ObjectExtends(Object);

    size_t (*size)();
    
    void* (*peek)();
    void (*push)(void* value);
    void* (*pop)();

    size_t siz;
    size_t capacity;
    void** data;
} Stack;

Stack* Stack_new();
