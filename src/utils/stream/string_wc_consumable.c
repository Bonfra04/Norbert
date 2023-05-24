#include <utils/stream/string_wc_consumable.h>

#include <wchar.h>

static wchar_t StringWCConsumable_Consume(StringWCConsumable* self)
{
    if (self->pos >= self->length)
        return -1;
    return self->buffer[self->pos++];
}

static void StringWCConsumable_destructor(StringWCConsumable* self)
{
    self->super.destruct();
}

StringWCConsumable* StringWCConsumable_new(wchar_t* buffer)
{
    StringWCConsumable* self = ObjectFromSuper(WCConsumable, StringWCConsumable, 0);

    self->buffer = buffer;
    self->length = wcslen(buffer);
    self->pos = 0;

    ObjectOverride(StringWCConsumable, Consume);

    Object_prepare((Object*)&self->super);
    return self;
}
