#include "stack.h"

#include <stdlib.h>

#define STACK_CAPACITY 16

static size_t Stack_size(Stack* self)
{
    return self->siz;
}

static void* Stack_peek(Stack* self)
{
    return self->data[self->siz - 1];
}

static void Stack_push(void* value, Stack* self)
{
    if(self->siz == self->capacity)
    {
        self->capacity *= 2;
        self->data = realloc(self->data, sizeof(void*) * self->capacity);
    }

    self->data[self->siz++] = value;
}

static void* Stack_pop(Stack* self)
{
    return self->data[--self->siz];
}

static void Stack_destructor(Stack* self)
{
    free(self->data);
    self->super.destruct();
}

Stack* Stack_new()
{
    Stack* self = ObjectBase(Stack, 4);

    self->data = malloc(sizeof(void*) * STACK_CAPACITY);
    self->capacity = STACK_CAPACITY;
    self->siz = 0;

    ObjectFunction(Stack, size, 0);
    ObjectFunction(Stack, peek, 0);

    ObjectFunction(Stack, pop, 0);
    ObjectFunction(Stack, push, 1);

    Object_prepare((Object*)&self->super);
    return self;
}
