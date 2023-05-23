#pragma once

#include <utils/stream/wc_consumable.h>

typedef struct StringWCConsumable
{
    ObjectExtends(WCConsumable);

    wchar_t (*Consume)();

    wchar_t* buffer;
    size_t length;
    size_t pos;
} StringWCConsumable;

StringWCConsumable* StringWCConsumable_new(wchar_t* buffer);
