#pragma once

#include <stddef.h>
#include <stdbool.h>

void* vector_new(size_t stride);
size_t vector_length(const void* vector);
void vector_append(void* vector, void* value);
void* vector_pop(void* vector);
void vector_free(void* vector);
void vector_insert(void* vector, size_t index, void* value);
void vector_remove(void* vector, size_t index);
void vector_remove_first(void* vector, void* value);
void* vector_find_first(void* vector, bool (*matcher)(void* value));
