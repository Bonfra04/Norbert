#include "stream/stream.h"
#include "tokenizer.h"
#include "parser.h"

#include "utils/vector.h"
#include "utils/wstring.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>

#include "tests/perform.h"

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
        printf("Comment: '%ls'\n", node->as.Comment.data);
    }

    else if(node->nodeType == DOM.Node.DOCUMENT_NODE)
    {
        printf("Document\n");
    }

    else if(node->nodeType == DOM.Node.DOCUMENT_TYPE_NODE)
    {
        printf("DocumentType: '%ls'\n", node->as.DocumentType.name);
    }

    else if(node->nodeType == DOM.Node.ELEMENT_NODE)
    {
        printf("Element: '%ls' ", node->as.Element.localName);
        for(size_t i = 0; i < node->as.Element.attributes->length; i++)
        {
            type(DOM.Node)* attr = node->as.Element.attributes->attributes[i];
            printf("[%ls='%ls'] ", attr->as.Attr.name, attr->as.Attr.value);
        }
        printf("\n");
    }

    else if(node->nodeType == DOM.Node.TEXT_NODE)
    {
        printf("Text: '%ls'\n", node->as.Text.wholeText);
    }

    size_t len = vector_length(node->childNodes);
    for (size_t i = 0; i < len; i++)
    {
        type(DOM.Node)* child = node->childNodes[i];
        pretty_print(child, indentation+1, i == len - 1);
    }
}

int main()
{
    // freopen("/dev/null", "w", stderr);
    setvbuf(stdout, NULL, _IONBF, 0);

    stream_t input_stream = stream_new(wsconsumable_new(
L"<!--test-->"
L"<!DOCTYPE html>"
L"<!--anothercomment-->"
L"<html lang=eng>"
L"<!--comment-->"
L"<head> <title>titlest</title></head>"
L"bodycontent text"
L"</html>"
    ));
    tokenizer_init(&input_stream);
    
    parser_init();
    type(DOM.Node)* document = parser_parse();
    pretty_print(document, 0, false);
}
