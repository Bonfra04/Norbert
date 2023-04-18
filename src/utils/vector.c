#include "vector.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

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

void vector_free(void* vector)
{
    vector_data_t* data = (vector_data_t*)((uint8_t*)vector - sizeof(vector_data_t));
    free(data);
}
