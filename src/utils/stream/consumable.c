#include <utils/stream/consumable.h>

#include <assert.h>

static void* Consumable_Consume(Consumable* self)
{
    assert(0);
}

static void Consumable_delete(Consumable* self)
{
    self->super.delete();
}

Consumable* Consumable_new()
{
    Consumable* self = ObjectBase(Consumable, 3);

    ObjectFunction(Consumable, Consume, 0);

    Object_prepare((Object*)&self->super);
    return self;
}
