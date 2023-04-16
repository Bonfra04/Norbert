#pragma once

#include <stdint.h>
#include <stddef.h>

typedef struct queue_node
{
    uint64_t value;
    struct queue_node* next;
    struct queue_node* prev;
} queue_node_t;

typedef struct queue
{
    queue_node_t* front;
    queue_node_t* back;
    size_t size;
} queue_t;

queue_t queue_create();

void queue_push(queue_t* queue, uint64_t value);
uint64_t queue_pop(queue_t* queue);
