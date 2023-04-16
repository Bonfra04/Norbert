#include "stream/stream.h"
#include "tokenizer.h"

#include "utils/vector.h"
#include "utils/wstring.h"

#include <stdlib.h>
#include <stdio.h>

int main()
{
    // freopen("/dev/null", "w", stderr);
    setvbuf(stdout, NULL, _IONBF, 0);

    stream_t input_stream = stream_new(wsconsumable_new(L"o<!--ciao--> <a href=\"&lt;&gt;ciao\">ciao</a>"));
    tokenizer_init(&input_stream);
    
    bool goon = true;
    while(goon)
    {
        token_t* token = tokenizer_emit_token();

        switch (token->type)
        {
        case token_character:
            wprintf(L"\033[1;36m%lc\033[0m", token->value);
            break;

        case token_start_tag:
        {
            size_t len = vector_length(token->attributes);
            wprintf(L"\033[1;32m<%ls", token->name);
            for (size_t i = 0; i < len; i++)
            {
                attribute_t* attr = &token->attributes[i];
                wprintf(L" %ls", attr->name, attr->value);
                if(wstring_length(attr->value) > 0)
                    wprintf(L"=\"%ls\"", attr->value);
            }
            wprintf(L"%lc>\033[0m", token->selfClosing ? L'/' : L'\0');
        }
        break;

        case token_end_tag:
            wprintf(L"\033[1;32m</%ls>\033[0m", token->name);
            break;

        case token_comment:
            wprintf(L"\033[1;33m<!--%ls-->\033[0m", token->data);
            break;

        case token_eof:
            goon = false;
            break;
        }

        free(token);
    }
    wprintf(L"\033[0m\n");
}