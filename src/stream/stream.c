#include "stream.h"

#include <stdlib.h>
#include <assert.h>

stream_t stream_new(consumable_t source)
{
    stream_t stream;
    stream.source = source;
    stream.data = calloc(sizeof(wchar_t), 1024);
    stream.pos = 0;
    return stream;
}

void stream_free(stream_t* stream)
{
    stream->source.free(stream->source.data);
    free(stream->data);
}

wchar_t stream_consume(stream_t* stream)
{
    if(stream->data[stream->pos] == L'\0')
        return stream->data[stream->pos++] = stream->source.consume(stream->source.data);
    else
        return stream->data[stream->pos++];
}

wchar_t stream_current(stream_t* stream)
{
    return stream->data[stream->pos - 1];
}

void stream_reconsume(stream_t* stream)
{
    assert(stream->pos > 0);
    stream->pos--;
}

size_t stream_consume_n(stream_t* stream, size_t n, wchar_t* out)
{
    for(size_t i = 0; i < n; i++)
    {
        wchar_t c = stream_consume(stream);
        if(c == L'\0')
            return i;
        *out++ = c;
    }
    return n;
}

size_t stream_reconsume_n(stream_t* stream, size_t n)
{
    size_t i;
    for(i = 0; stream->pos > 0 && i < n; i++)
        stream_reconsume(stream);
    return i;
}

bool stream_match(stream_t* stream, wchar_t* str, bool consume)
{
    size_t i;
    for(i = 0; str[i] != L'\0'; i++)
    {
        wchar_t c = stream_consume(stream);
        if(c != str[i])
        {
            stream_reconsume_n(stream, i);
            return false;
        }
    }
    if(!consume)
        stream_reconsume_n(stream, i);
    return true;
}
