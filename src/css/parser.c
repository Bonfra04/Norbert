#include <css/parser.h>

static void CSSParser_delete(CSSParser* self)
{
    self->tokenizer->delete();
    self->super.delete();
}

CSSParser* CSSParser_new(WCStream* stream)
{
    CSSParser* self = ObjectBase(CSSParser, 2);

    self->tokenizer = CSSTokenizer_new(stream);

    Object_prepare((Object*)&self->super);
    return self;
}
