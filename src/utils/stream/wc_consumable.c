#include <utils/stream/wc_consumable.h>

static void WCConsumable_delete(WCConsumable* self)
{
    self->super.delete();
}

WCConsumable* WCConsumable_new()
{
    WCConsumable* self = ObjectFromSuper(Consumable, WCConsumable, 0);
 
    ObjectInherit(Consume);

    Object_prepare((Object*)&self->super);
    return self;
}
