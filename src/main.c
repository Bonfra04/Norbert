#include "stream/stream.h"
#include "tokenizer.h"

#include "utils/vector.h"
#include "utils/wstring.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "tests/perform.h"

int main()
{
    // freopen("/dev/null", "w", stderr);
    setvbuf(stdout, NULL, _IONBF, 0);

    stream_t input_stream = stream_new(wsconsumable_new(L"<!DOCTYPE html PUBLIC 'mario' 'massimo'>o<!--ciao--> <a href=\"&lt;&gt;ciao\">ciao</a>"));
    tokenizer_init(&input_stream);
    
    bool goon = true;
    while(goon)
    {
        token_t* token = tokenizer_emit_token();

        switch (token->type)
        {
        case token_character:
            wprintf(L"\033[1;36m%lc\033[0m", token->as.character.value);
            break;

        case token_start_tag:
        {
            size_t len = vector_length(token->as.tag.attributes);
            wprintf(L"\033[1;32m<%ls", token->as.tag.name);
            for (size_t i = 0; i < len; i++)
            {
                token_attribute_t* attr = &token->as.tag.attributes[i];
                wprintf(L" %ls", attr->name, attr->value);
                if(wstring_length(attr->value) > 0)
                    wprintf(L"=\"%ls\"", attr->value);
            }
            wprintf(L"%lc>\033[0m", token->as.tag.selfClosing ? L'/' : L'\0');
        }
        break;

        case token_end_tag:
            wprintf(L"\033[1;32m</%ls>\033[0m", token->as.tag.name);
            break;

        case token_comment:
            wprintf(L"\033[1;33m<!--%ls-->\033[0m", token->as.comment.data);
            break;

        case token_doctype:
            wprintf(L"\033[1;34m<!DOCTYPE %ls", token->as.doctype.name);
            if(wstring_length(token->as.doctype.public_identifier) > 0)
                wprintf(L" PUBLIC \"%ls\"", token->as.doctype.public_identifier);
            if(wstring_length(token->as.doctype.system_identifier) > 0)
                if(wstring_length(token->as.doctype.public_identifier) > 0)
                    wprintf(L" \"%ls\"", token->as.doctype.system_identifier);
                else
                    wprintf(L" SYSTEM \"%ls\"", token->as.doctype.system_identifier);
            wprintf(L">\033[0m");
            break;

        case token_eof:
            goon = false;
            break;
        }

        free_token(token);
    }
    wprintf(L"\033[0m\n");
}