#include "utils/stream/stream.h"
#include "utils/stream/string_wc_consumable.h"
#include "html/tokenizer.h"
#include "html/parser.h"

#include "utils/vector.h"
#include "utils/wstring.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>

// #include "tests/perform.h"

void pretty_print(__DOM_Node* node, int64_t indentation, bool is_last)
{
    if(indentation > 0)
    {
        if(indentation > 1)
        {
            fputs("    ", stdout);
            for(size_t i = 2; i < indentation; i++)
                fputs("│   ", stdout);
        }
        fputs(is_last ? "└── " : "├── ", stdout);
    }

    if(node->nodeType() == DOM.Node.COMMENT_NODE)
    {
        auto comment = ((__DOM_Node_Comment*)node);
        printf("Comment: '%ls'\n", comment->data()->data);
    }

    else if(node->nodeType() == DOM.Node.DOCUMENT_NODE)
    {
        printf("Document\n");
    }

    else if(node->nodeType() == DOM.Node.DOCUMENT_TYPE_NODE)
    {
        auto doctype = ((__DOM_Node_DocumentType*)node);
        printf("DocumentType: '%ls'\n", doctype->name()->data);
    }

    else if(node->nodeType() == DOM.Node.ELEMENT_NODE)
    {
        auto element = ((__DOM_Node_Element*)node);
        printf("Element: '%ls' ", element->localName()->data);
        for(size_t i = 0; i < element->attributes()->length(); i++)
        {
            auto attr = element->attributes()->item(i);
            printf("[%ls='%ls'] ", attr->name()->data, attr->value()->data);
        }
        printf("\n");
    }

    else if(node->nodeType() == DOM.Node.TEXT_NODE)
    {
        auto text = ((__DOM_Node_Text*)node);
        printf("Text: '%ls'\n", text->wholeText()->data);
    }

    size_t len = node->childNodes()->length();
    for (size_t i = 0; i < len; i++)
    {
        auto child = node->childNodes()->item(i);
        pretty_print(child, indentation+1, i == len - 1);
    }
}

int main()
{
    // freopen("/dev/null", "w", stderr);
    setvbuf(stdout, NULL, _IONBF, 0);

    WCStream* input_stream = WCStream_new((WCConsumable*)StringWCConsumable_new(
L"<!DOCTYPE html>"
L"<!--anothercomment-->"
L"<html lang=eng>"
L"<!--comment-->"
L"<head> <title>titlest</title></head>"
L"<body>"
L"<p>inp</p>afp"
L"</body>"
L"</html>"
    ));
    
    HTMLParser* parser = HTMLParser_new(input_stream);
    __DOM_Node_Document* document = parser->parse();
    pretty_print((__DOM_Node*)document, 0, false);

    parser->delete();
    input_stream->delete();
}
