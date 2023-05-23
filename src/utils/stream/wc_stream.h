#pragma once

#include <utils/stream/stream.h>
#include <utils/stream/wc_consumable.h>

typedef struct WCStream
{
    ObjectExtends(Stream);

    wchar_t (*Consume)();
    wchar_t (*Current)();
    size_t (*ConsumeN)(size_t n, wchar_t** out);
    wchar_t (*Peek)(size_t n);
    wchar_t (*Next)();
    void (*Reconsume)();
    size_t (*ReconsumeN)(size_t n);

    bool (*Match)(wchar_t* str, bool consume, bool case_sensitive);
} WCStream;

WCStream* WCStream_new(WCConsumable* source);
