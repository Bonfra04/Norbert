#include "vector.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#define VECTOR_CAPACITY 16

typedef struct vector_data
{
    size_t length;
    size_t capacity;
    size_t strie;
} vector_data_t;

void* vector_new(size_t stride)
{
    vector_data_t* vector = malloc(sizeof(vector_data_t) + sizeof(stride) * VECTOR_CAPACITY);
    vector->length = 0;
    vector->capacity = VECTOR_CAPACITY;
    vector->strie = stride;
    return (uint8_t*)vector + sizeof(vector_data_t);
}

size_t vector_length(const void *vector)
{
    vector_data_t* data = (vector_data_t*)((uint8_t*)vector - sizeof(vector_data_t));
    return data->length;
}

void vector_append(void* vector, void* value)
{
    vector_data_t* data = (vector_data_t*)((uint8_t*)vector - sizeof(vector_data_t));
    if (data->length + 1 >= data->capacity)
    {
        data = (vector_data_t*)realloc(data, sizeof(vector_data_t) + sizeof(data->strie) * data->capacity * 2);
        data->capacity *= 2;
    }
    memcpy((uint8_t*)vector + data->length * data->strie, value, data->strie);
    data->length++;
}

void* vector_pop(void* vector)
{
    vector_data_t* data = (vector_data_t*)((uint8_t*)vector - sizeof(vector_data_t));
    assert(data->length > 0);
    data->length--;
    return (uint8_t*)vector + data->length * data->strie;
}

void vector_free(void* vector)
{
    vector_data_t* data = (vector_data_t*)((uint8_t*)vector - sizeof(vector_data_t));
    free(data);
}

void vector_insert(void* vector, size_t index, void* value)
{
    vector_data_t* data = (vector_data_t*)((uint8_t*)vector - sizeof(vector_data_t));
    assert(index <= data->length);
    if (data->length + 1 >= data->capacity)
    {
        data = (vector_data_t*)realloc(data, sizeof(vector_data_t) + sizeof(data->strie) * data->capacity * 2);
        data->capacity *= 2;
    }
    memmove((uint8_t*)vector + (index + 1) * data->strie, (uint8_t*)vector + index * data->strie, (data->length - index) * data->strie);
    memcpy((uint8_t*)vector + index * data->strie, value, data->strie);
    data->length++;
}

void vector_remove(void* vector, size_t index)
{
    vector_data_t* data = (vector_data_t*)((uint8_t*)vector - sizeof(vector_data_t));
    assert(index < data->length);
    memmove((uint8_t*)vector + index * data->strie, (uint8_t*)vector + (index + 1) * data->strie, (data->length - index - 1) * data->strie);
    data->length--;
}

void vector_remove_first(void* vector, void* value)
{
    vector_data_t* data = (vector_data_t*)((uint8_t*)vector - sizeof(vector_data_t));
    for (size_t i = 0; i < data->length; i++)
    {
        if (memcmp((uint8_t*)vector + i * data->strie, value, data->strie) == 0)
        {
            memmove((uint8_t*)vector + i * data->strie, (uint8_t*)vector + (i + 1) * data->strie, (data->length - i - 1) * data->strie);
            data->length--;
            return;
        }
    }
}

void* vector_find_first(void *vector, bool (*matcher)(void* value))
{
    vector_data_t* data = (vector_data_t*)((uint8_t*)vector - sizeof(vector_data_t));
    for (size_t i = 0; i < data->length; i++)
        if (matcher((uint8_t*)vector + i * data->strie))
            return (uint8_t*)vector + i * data->strie;
    return NULL;
}
