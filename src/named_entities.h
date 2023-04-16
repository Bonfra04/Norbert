#pragma once

#include <stdbool.h>
#include <wchar.h>

typedef struct named_entity
{
    const wchar_t* name;
    wchar_t value[2];
    bool flag;
} named_entity_t;

typedef struct control_character
{
    wchar_t value;
    wchar_t replacement;
} control_character_t;

#define NAMED_ENTITIES_COUNT 2231
extern named_entity_t entities[NAMED_ENTITIES_COUNT];

#define CONTROL_CHARACTERS_COUNT 27
extern control_character_t control_characters[CONTROL_CHARACTERS_COUNT];
