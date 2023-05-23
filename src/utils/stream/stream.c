#include <utils/stream/stream.h>

static void* Stream_Consume(Stream* self)
{
    while(self->data->length() <= self->pos)
    {
        void* e = self->source->Consume();
        self->data->append(e);
    }

    return self->data->at(self->pos++);
}

static void* Stream_Current(Stream* self)
{
    return self->data->at(self->pos - 1);
}

static size_t Stream_ConsumeN(size_t n, void** out, Stream* self)
{
    for(size_t i = 0; i < n; i++)
    {
        void* e = self->Consume();
        if(e == NULL)
        {
            return i;
        }
        if(out != NULL)
        {
            *out++ = e;
        }
    }
    return n;
}

static void* Stream_Peek(size_t n, Stream* self)
{
    for(size_t i = 0; i < n; i++)
    {
        self->Consume();
    }
    void* e = self->Current();
    for(size_t i = 0; i < n; i++)
    {
        self->Reconsume();
    }
    return e;
}

static void* Stream_Next(Stream* self)
{
    return self->Peek(1);
}

static void Stream_Reconsume(Stream* self)
{
    if(self->pos > 0)
        self->pos--;
}

static size_t Stream_ReconsumeN(size_t n, Stream* self)
{
    size_t i;
    for(i = 0; self->pos > 0 && i < n; i++)
    {
        self->Reconsume();
    }
    return i;
}

static void Stream_delete(Stream* self)
{
    self->data->delete();
    self->source->delete();
    self->super.delete();
}

Stream* Stream_new(Consumable* source)
{
    Stream* self = ObjectBase(Stream, 8);

    self->source = source;
    self->data = Vector_new();
    self->pos = 0;

    ObjectFunction(Stream, Consume, 0);
    ObjectFunction(Stream, Current, 0);
    ObjectFunction(Stream, ConsumeN, 2);
    ObjectFunction(Stream, Peek, 1);
    ObjectFunction(Stream, Next, 0);
    ObjectFunction(Stream, Reconsume, 0);
    ObjectFunction(Stream, ReconsumeN, 1);

    Object_prepare((Object*)&self->super);
    return self;
}
