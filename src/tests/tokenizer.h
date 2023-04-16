#include "test.h"

#include "../stream/stream.h"
#include "../tokenizer.h"

#include <limits.h>

Test(singleCharacterToken)
{
    for(wchar_t chr = 0; chr < CHAR_MAX; chr++)
    {
        if(chr == L'&' || chr == L'<' || chr == L'\0' || chr == -1)
            continue;
        
        wchar_t str[] = { chr, L'\0' };
        stream_t input_stream = stream_new(wsconsumable_new(str));
        tokenizer_init(&input_stream);

        token_t* character = tokenizer_emit_token();

        Assert(character->type == token_character);
        Assert(character->value == chr);

        token_t* eof = tokenizer_emit_token();
        Assert(eof->type == token_eof);
    }

    SucceedTest();
}
