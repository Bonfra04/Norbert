#pragma once

#include <wchar.h>

typedef struct consumable
{
    void* data;
    void (*free)(void* data);
    wchar_t (*consume)(void* data);
} consumable_t;

consumable_t wsconsumable_new(wchar_t* wstr);
