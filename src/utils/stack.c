#include "stack.h"

#include <stdlib.h>

#define STACK_CAPACITY 1024

stack_t stack_create()
{
    stack_t stack;
    stack.capacity = STACK_CAPACITY;
    stack.size = 0;
    stack.data = malloc(sizeof(uint64_t) * stack.capacity);
    return stack;
}

void stack_destroy(stack_t* stack)
{
    free(stack->data);
}

void stack_push(stack_t* stack, uint64_t value)
{
    if(stack->size == stack->capacity)
    {
        stack->capacity *= 2;
        stack->data = realloc(stack->data, sizeof(uint64_t) * stack->capacity);
    }

    stack->data[stack->size++] = value;
}

uint64_t stack_pop(stack_t* stack)
{
    return stack->data[--stack->size];
}

uint64_t* stack_peek(stack_t* stack)
{
    return &stack->data[stack->size - 1];
}
