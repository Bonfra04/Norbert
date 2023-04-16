#include "queue.h"

#include <stdlib.h>

queue_t queue_create()
{
    queue_t queue;
    queue.front = NULL;
    queue.back = NULL;
    queue.size = 0;
    return queue;
}

void queue_push(queue_t* queue, uint64_t value)
{
    queue_node_t* node = (queue_node_t*)malloc(sizeof(queue_node_t));
    node->value = value;
    node->next = queue->back;
    node->prev = NULL;
    if(queue->back != NULL)
        queue->back->prev = node;
    queue->back = node;
    if(queue->front == NULL)
        queue->front = node;
    queue->size++;
}

uint64_t queue_pop(queue_t* queue)
{
    if(queue->size == 0)
        return -1;

    queue_node_t* node = queue->front;
    uint64_t value = node->value;
    queue->front = node->prev;
    if(queue->front)
        queue->front->next = NULL;
    else
        queue->back = NULL;
    free(node);
    queue->size--;
    return value;
}
