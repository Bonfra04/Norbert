#pragma once

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

typedef struct Object
{
    void (*delete)(void);
    void (*destruct)(void);

    size_t codePageSize;
    unsigned char* codePage;
    unsigned char* codePagePtr;
} Object;

void Object_delete(Object* self);
void* Object_new(size_t size, int functionCount, void (*destructor)(Object* self));
void Object_prepare(Object* self);
void* Object_trampoline(Object* self, void* target, int argCount);

void* Object_fromSuper(Object* super, size_t superSize, size_t size, int functionCount, void (*destructor)(Object* self));
void* Object_override(void* target, void* superTrampoline);

void* Object_getter(Object* self, uint32_t offset);

#define ObjectFromSuper(superType, type, functionCount, ...) ({                                                                                     \
superType* super = superType ## _new(__VA_ARGS__);                                                                                                  \
type* self = Object_fromSuper((Object*)&super->super, sizeof(superType), sizeof(type), functionCount + 1, (void(*)(Object*))type ## _destructor);   \
self->delete = self->super.delete;                                                                                                                  \
self->destruct = Object_trampoline((Object*)&self->super, type ## _destructor, 0);                                                                  \
self;})

#define ObjectBase(type, functionCount) ({                                                          \
type* self = Object_new(sizeof(type), functionCount + 1, (void(*)(Object*))type ## _destructor);    \
self->delete = self->super.delete;                                                                  \
self->destruct = Object_trampoline((Object*)&self->super, type ## _destructor, 0);                  \
self;})

#define ObjectInherit(name) do {                    \
self->name = (typeof(self->name))self->super.name;  \
} while (0)

#define ObjectFunction(type, name, count) do {                                      \
self->name = Object_trampoline((Object*)&self->super, type ## _ ## name, count);    \
} while (0)

#define ObjectOverride(type, name) do {                             \
self->name = Object_override(type ## _ ## name, self->super.name);  \
} while (0)

#define ObjectExtends(type) type super; void (*delete)(void); void(*destruct)(void)

#define ObjectGetter(type, name) do {                                       \
self->name = Object_getter((Object*)&self->super, offsetof(type, _##name));    \
} while (0)
