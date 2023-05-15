#pragma once

#include "oop/object.h"

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

typedef struct VectorRec
{
    Object object;

    size_t (*length)();
    void* (*at)(size_t index);
    void* (*set)(int64_t index, void* value);

    void (*append)(void* value);
    void* (*pop)();
    void (*insert)(size_t index, void* value);
    void (*remove)(size_t index);
    void (*remove_first)(void* value);

    void* (*find_first)(bool (*matcher)(void* value));

    void** data;
    size_t capacity;
    size_t len;
    size_t stride;
} Vector;

Vector* Vector_new();
void Vector_delete(Vector* self);
