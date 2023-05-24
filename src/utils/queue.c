#include "queue.h"

#include <stdlib.h>

static size_t Queue_size(Queue* self)
{
    return self->siz;
}

static void Queue_enqueue(void* value, Queue* self)
{
    queue_node_t* node = (queue_node_t*)malloc(sizeof(queue_node_t));
    node->value = value;
    node->next = self->back;
    node->prev = NULL;
    if(self->back != NULL)
        self->back->prev = node;
    self->back = node;
    if(self->front == NULL)
        self->front = node;
    self->siz++;
}

static void* Queue_dequeue(Queue* self)
{
    if(self->siz == 0)
        return NULL;

    queue_node_t* node = self->front;
    void* value = node->value;
    self->front = node->prev;
    if(self->front)
        self->front->next = NULL;
    else
        self->back = NULL;
    free(node);
    self->siz--;
    return value;
}

static void Queue_destructor(Queue* self)
{
    while(self->size() > 0)
        self->dequeue();

    self->super.destruct();
}

Queue* Queue_new()
{
    Queue* self = ObjectBase(Queue, 3);

    self->front = NULL;
    self->back = NULL;
    self->siz = 0;

    ObjectFunction(Queue, size, 0);

    ObjectFunction(Queue, enqueue, 1);
    ObjectFunction(Queue, dequeue, 0);

    Object_prepare((Object*)&self->super);
    return self;
}
