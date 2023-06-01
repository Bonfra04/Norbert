#include "vector.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#define VECTOR_CAPACITY 16

static size_t Vector_length(Vector* self)
{
    return self->len;
}

static void* Vector_at(int64_t index, Vector* self)
{
    if(index < 0)
    {
        index = self->len + index;
    }
    
    assert(index < self->len);
    return self->data[index];
}

static void* Vector_set(size_t index, void* value, Vector* self)
{
    assert(index < self->len);
    self->data[index] = value;
    return value;
}

static void Vector_append(void* value, Vector* self)
{
    if (self->len + 1 >= self->capacity)
    {
        self->capacity *= 2;
        self->data = (void**)realloc(self->data, sizeof(void*) * self->capacity);
    }

    self->data[self->len++] = value;
}

static void* Vector_pop(Vector* vector)
{
    if(vector->len == 0)
    {
        return NULL;
    }

    return vector->data[--vector->len];
}

static void Vector_insert(size_t index, void* value, Vector* self)
{
    assert(index <= self->len);

    if (self->len + 1 >= self->capacity)
    {
        self->capacity *= 2;
        self->data = (void**)realloc(self->data, sizeof(void*) * self->capacity);
    }

    memmove((char*)self->data + index + 1, (char*)self->data + index, (self->len - index) * sizeof(void*));
    self->data[index] = value;
    self->len++;
}

static void Vector_remove(size_t index, Vector* self)
{
    assert(index < self->len);

    memmove((char*)self->data + index, (char*)self->data + index + 1, (self->len - index - 1) * sizeof(void*));
    self->len--;
}

static void Vector_remove_first(void* value, Vector* self)
{
    for(size_t i = 0; i < self->len; i++)
    {
        if(self->data[i] == value)
        {
            memmove((char*)self->data + i, (char*)self->data + i + 1, (self->len - i - 1) * sizeof(void*));
            self->len--;
            return;
        }
    }
}

static void* Vector_find_first(bool (*matcher)(void* value), Vector* self)
{
    for(size_t i = 0; i < self->len; i++)
    {
        if(matcher(self->data[i]))
        {
            return self->data[i];
        }
    }
    return NULL;
}

static void** Vector_condense(Vector* self)
{
    void** data = malloc(sizeof(void*) * self->len);
    memcpy(data, self->data, sizeof(void*) * self->len);
    return data;
}

static void Vector_destructor(Vector* self)
{
    free(self->data);
    self->super.destruct();
}

Vector* Vector_new()
{
    Vector* self = ObjectBase(Vector, 9);

    self->data = malloc(sizeof(void*) * VECTOR_CAPACITY);
    self->capacity = VECTOR_CAPACITY;
    self->len = 0;

    ObjectFunction(Vector, length, 0);
    ObjectFunction(Vector, at, 1);
    ObjectFunction(Vector, set, 2);

    ObjectFunction(Vector, append, 1);
    ObjectFunction(Vector, pop, 0);
    ObjectFunction(Vector, insert, 2);
    ObjectFunction(Vector, remove, 1);
    ObjectFunction(Vector, remove_first, 1);

    ObjectFunction(Vector, find_first, 1);

    ObjectFunction(Vector, condense, 0);

    Object_prepare((Object*)&self->super);
    return self;
}
