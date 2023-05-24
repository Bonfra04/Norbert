#include <utils/stream/wc_stream.h>

#include <wctype.h>

bool WCStream_Match(wchar_t* str, bool consume, bool case_sensitive, WCStream* self)
{
    size_t i;
    for(i = 0; str[i] != L'\0'; i++)
    {
        wchar_t c = self->Consume();
        if(case_sensitive ? (c != str[i]) : (towlower(c) != towlower(str[i])))
        {
            self->ReconsumeN(i + 1);
            return false;
        }
    }
    if(consume == false)
    {
        self->ReconsumeN(i + 1);
    }
    return true;
}

static void WCStream_destructor(WCStream* self)
{
    self->super.destruct();
}

WCStream* WCStream_new(WCConsumable* source)
{
    WCStream* self = ObjectFromSuper(Stream, WCStream, 1, (Consumable*)source);

    ObjectInherit(Consume);
    ObjectInherit(Current);
    ObjectInherit(ConsumeN);
    ObjectInherit(Peek);
    ObjectInherit(Next);
    ObjectInherit(Reconsume);
    ObjectInherit(ReconsumeN);

    ObjectFunction(WCStream, Match, 3);

    Object_prepare((Object*)&self->super);
    return self;
}
