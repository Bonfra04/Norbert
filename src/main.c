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

void pretty_print(type(DOM.Node)* node, int64_t indentation, bool is_last)
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

    if(node->nodeType == DOM.Node.COMMENT_NODE)
    {
        printf("Comment: '%ls'\n", node->as.Comment.data->data);
    }

    else if(node->nodeType == DOM.Node.DOCUMENT_NODE)
    {
        printf("Document\n");
    }

    else if(node->nodeType == DOM.Node.DOCUMENT_TYPE_NODE)
    {
        printf("DocumentType: '%ls'\n", node->as.DocumentType.name->data);
    }

    else if(node->nodeType == DOM.Node.ELEMENT_NODE)
    {
        printf("Element: '%ls' ", node->as.Element.localName->data);
        for(size_t i = 0; i < node->as.Element.attributes->length; i++)
        {
            type(DOM.Node)* attr = node->as.Element.attributes->attributes->at(i);
            printf("[%ls='%ls'] ", attr->as.Attr.name->data, attr->as.Attr.value->data);
        }
        printf("\n");
    }

    else if(node->nodeType == DOM.Node.TEXT_NODE)
    {
        printf("Text: '%ls'\n", node->as.Text.wholeText->data);
    }

    size_t len = node->childNodes->length();
    for (size_t i = 0; i < len; i++)
    {
        type(DOM.Node)* child = node->childNodes->at(i);
        pretty_print(child, indentation+1, i == len - 1);
    }
}

int main()
{
    // freopen("/dev/null", "w", stderr);
    setvbuf(stdout, NULL, _IONBF, 0);

    WCStream* input_stream = WCStream_new((WCConsumable*)StringWCConsumable_new(
L"<!--test-->"
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
    type(DOM.Node)* document = parser->parse();
    pretty_print(document, 0, false);

    parser->delete();
    input_stream->delete();
}
