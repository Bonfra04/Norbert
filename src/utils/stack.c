#include "stack.h"

#include <stdlib.h>

#define STACK_CAPACITY 16

size_t Stack_size(Stack* self)
{
    return self->siz;
}

void* Stack_peek(Stack* self)
{
    return self->data[self->siz - 1];
}

void Stack_push(void* value, Stack* self)
{
    if(self->siz == self->capacity)
    {
        self->capacity *= 2;
        self->data = realloc(self->data, sizeof(void*) * self->capacity);
    }

    self->data[self->siz++] = value;
}

void* Stack_pop(Stack* self)
{
    return self->data[--self->siz];
}

Stack* Stack_new()
{
    Stack* self = Object_create(sizeof(Stack), 4);
    self->data = malloc(sizeof(void*) * STACK_CAPACITY);
    self->capacity = STACK_CAPACITY;
    self->siz = 0;

    ObjectFunction(Stack, size, 0);
    ObjectFunction(Stack, peek, 0);

    ObjectFunction(Stack, pop, 0);
    ObjectFunction(Stack, push, 1);

    Object_prepare(&self->object);
    return self;
}
