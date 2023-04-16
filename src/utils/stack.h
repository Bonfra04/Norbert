#pragma once

#include <stdint.h>
#include <stddef.h>

typedef struct stack
{
    size_t size;
    size_t capacity;
    uint64_t* data;
} stack_t;

stack_t stack_create();
void stack_destroy(stack_t* stack);
void stack_push(stack_t* stack, uint64_t value);
uint64_t stack_pop(stack_t* stack);
uint64_t* stack_peek(stack_t* stack);
