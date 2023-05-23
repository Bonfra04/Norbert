#pragma once

#include <utils/stream/consumable.h>

typedef struct WCConsumable
{
    ObjectExtends(Consumable);

    wchar_t (*Consume)();
} WCConsumable;

WCConsumable* WCConsumable_new();
