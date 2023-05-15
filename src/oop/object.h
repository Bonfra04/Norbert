#pragma once

#include <stddef.h>

typedef struct ObjectRec
{
    void (*destroy)(void);

    size_t codePageSize;
    unsigned char* codePage;
    unsigned char* codePagePtr;
} Object;

void Object_destroy(Object* self);
void* Object_create(size_t size, int functionCount);
void Object_prepare(Object* self);
void* Object_trampoline(Object* self, void* target, int argCount);

#define ObjectFunction(type, name, count) do {                              \
self->name = Object_trampoline(&self->object, type ## _ ## name, count);    \
} while (0);
