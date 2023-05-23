#pragma once

#include <oop/object.h>

typedef struct Consumable
{
    ObjectExtends(Object);

    void* (*Consume)();
} Consumable;

Consumable* Consumable_new();
