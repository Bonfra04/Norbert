#include "stream.h"

#include <stdlib.h>
#include <assert.h>
#include <wctype.h>

wchar_t Stream_consume(Stream* self)
{
    if(self->data[self->pos] == L'\0')
    {
        return self->data[self->pos++] = self->source.consume(self->source.data);
    }
    else
    {
        return self->data[self->pos++];
    }
}

wchar_t Stream_current(Stream* self)
{
    return self->data[self->pos - 1];
}

wchar_t Stream_next(Stream* self)
{
    return self->peek(1);
}

wchar_t Stream_peek(size_t n, Stream* self)
{
    for(size_t i = 0; i < n; i++)
    {
        self->consume();
    }
    wchar_t c = self->current();
    for(size_t i = 0; i < n; i++)
    {
        self->reconsume();
    }
    return c;
}

void Stream_reconsume(Stream* self)
{
    assert(self->pos > 0);
    self->pos--;
}

size_t Stream_consume_n(size_t n, wchar_t* out, Stream* self)
{
    for(size_t i = 0; i < n; i++)
    {
        wchar_t c = self->consume();
        if(c == L'\0')
        {
            return i;
        }
        if(out != NULL)
        {
            *out++ = c;
        }
    }
    return n;
}

size_t Stream_reconsume_n(size_t n, Stream* self)
{
    size_t i;
    for(i = 0; self->pos > 0 && i < n; i++)
    {
        self->reconsume();
    }
    return i;
}

bool Stream_match(wchar_t* str, bool consume, bool case_sensitive, Stream* self)
{
    size_t i;
    for(i = 0; str[i] != L'\0'; i++)
    {
        wchar_t c = self->consume();
        if(case_sensitive ? (c != str[i]) : (towlower(c) != towlower(str[i])))
        {
            self->reconsume_n(i + 1);
            return false;
        }
    }
    if(consume == false)
    {
        self->reconsume_n(i + 1);
    }
    return true;
}

Stream* Stream_new(consumable_t source)
{
    Stream* self = Object_create(sizeof(Stream), 8);
    self->source = source;
    self->data = calloc(sizeof(wchar_t), 1024);
    self->pos = 0;

    ObjectFunction(Stream, consume, 0);
    ObjectFunction(Stream, current, 0);
    ObjectFunction(Stream, next, 0);
    ObjectFunction(Stream, peek, 1);
    ObjectFunction(Stream, reconsume, 0);
    ObjectFunction(Stream, consume_n, 2);
    ObjectFunction(Stream, reconsume_n, 1);
    ObjectFunction(Stream, match, 3);


    Object_prepare(&self->object);
    return self;
}

void Stream_delete(Stream* self)
{
    self->source.free(self->source.data);
    self->object.destroy();
}