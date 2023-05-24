#include <utils/stream/consumable.h>

#include <assert.h>

static void* Consumable_Consume(Consumable* self)
{
    assert(0);
}

static void Consumable_destructor(Consumable* self)
{
    self->super.destruct();
}

Consumable* Consumable_new()
{
    Consumable* self = ObjectBase(Consumable, 3);

    ObjectFunction(Consumable, Consume, 0);

    Object_prepare((Object*)&self->super);
    return self;
}
