#include "wstring.h"

#include <stddef.h>
#include <stdlib.h>

#define WSTRING_CAPACITY 16

typedef struct wsdata
{
    size_t length;
    size_t capacity;
    wchar_t data[];
} wsdata_t;

wstring_t wstring_new()
{
    wsdata_t* data = (wsdata_t*)malloc(sizeof(wsdata_t) + sizeof(wchar_t) * WSTRING_CAPACITY);
    data->capacity = WSTRING_CAPACITY;
    data->length = 0;
    data->data[0] = L'\x0000';
    return data->data;
}

size_t wstring_length(const wstring_t str)
{
    if(!str)
        return 0;
    wsdata_t* data = (wsdata_t*)((char*)str - offsetof(wsdata_t, data));
    return data->length;
}

void wstring_free(wstring_t str)
{
    wsdata_t* data = (wsdata_t*)((char*)str - offsetof(wsdata_t, data));
    free(data);
}

void wstring_append(wstring_t str, wchar_t append)
{
    wsdata_t* data = (wsdata_t*)((char*)str - offsetof(wsdata_t, data));
    if (data->length + 1 >= data->capacity)
    {
        data = (wsdata_t*)realloc(data, sizeof(wsdata_t) + sizeof(wchar_t) * data->capacity * 2);
        data->capacity *= 2;
    }
    data->data[data->length] = append;
    data->data[data->length + 1] = L'\x0000';
    data->length++;
}

void wstring_appends(wstring_t str, wchar_t* append)
{
    while(*append)
    {
        wstring_append(str, *append);
        append++;
    }
}

void wstring_clear(wstring_t str)
{
    wsdata_t* data = (wsdata_t*)((char*)str - offsetof(wsdata_t, data));
    data->length = 0;
    data->data[0] = L'\x0000';
}

void wstring_popback(wstring_t str)
{
    wsdata_t* data = (wsdata_t*)((char*)str - offsetof(wsdata_t, data));
    data->length--;
    data->data[data->length] = L'\x0000';
}

void wstring_popfront(wstring_t str)
{
    wsdata_t* data = (wsdata_t*)((char*)str - offsetof(wsdata_t, data));
    for (size_t i = 0; i < data->length; i++)
        data->data[i] = data->data[i + 1];
    data->length--;
}
