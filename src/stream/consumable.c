#include "consumable.h"

#include <stdlib.h>

typedef struct wsconsumable_data
{
    wchar_t* str;
    size_t pos;
    size_t len;
} wsconsumable_data_t;

static void wsconsumable_free(void* data)
{
    free(((wsconsumable_data_t*)data)->str);
    free(data);
}

static wchar_t wsconsumable_consume(void* data)
{
    wsconsumable_data_t* d = (wsconsumable_data_t*)data;
    if(d->pos >= d->len)
        return -1;
    wchar_t c = d->str[d->pos];
    d->pos++;
    return c;
}

consumable_t wsconsumable_new(wchar_t* wstr)
{
    consumable_t c;
    wsconsumable_data_t* data = malloc(sizeof(wsconsumable_data_t));
    data->pos = 0;
    data->str = wcsdup(wstr);
    data->len = wcslen(wstr);
    c.data = data;
    c.free = wsconsumable_free;
    c.consume = wsconsumable_consume;
    return c;
}
