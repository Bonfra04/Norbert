#pragma once

#include <stddef.h>

void* vector_new(size_t stride);
size_t vector_length(const void* vector);
void vector_append(void* vector, void* value);
