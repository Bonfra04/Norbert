#pragma once

#include "oop/object.h"

#include <stddef.h>

typedef struct queue_node
{
    void* value;
    struct queue_node* next;
    struct queue_node* prev;
} queue_node_t;

typedef struct QueueRec
{
    Object object;

    size_t (*size)();
    void (*enqueue)(void* value);
    void* (*dequeue)();

    queue_node_t* front;
    queue_node_t* back;
    size_t siz;
} Queue;

Queue* Queue_new();
void Queue_delete(Queue* self);
