#pragma once

#include "utils/wstring.h"

#define type(t) typeof(t)

typedef wstring_t DOMString;
struct __namespace_DOM
{
    struct __namespace_DOM_NamedNodeMap
    {
        unsigned long length;

        struct __namespace_DOM_NamedNodeMap* (*new)();
        struct __namespace_DOM_Node* (*item)(struct __namespace_DOM_NamedNodeMap* self, unsigned long index);
        struct __namespace_DOM_Node* (*getNamedItem)(struct __namespace_DOM_NamedNodeMap* self, DOMString qualifiedName);
        struct __namespace_DOM_Node* (*setNamedItem)(struct __namespace_DOM_NamedNodeMap* self, struct __namespace_DOM_Node* attr);
        struct __namespace_DOM_Node* (*removeNamedItem)(struct __namespace_DOM_NamedNodeMap* self, DOMString qualifiedName);
        struct __namespace_DOM_Node** attributes;
    } NamedNodeMap;

    struct __namespace_DOM_Node
    {
        const unsigned short ELEMENT_NODE;
        const unsigned short ATTRIBUTE_NODE;
        const unsigned short TEXT_NODE;
        const unsigned short CDATA_SECTION_NODE;
        const unsigned short ENTITY_REFERENCE_NODE;
        const unsigned short ENTITY_NODE;
        const unsigned short PROCESSING_INSTRUCTION_NODE;
        const unsigned short COMMENT_NODE;
        const unsigned short DOCUMENT_NODE;
        const unsigned short DOCUMENT_TYPE_NODE;
        const unsigned short DOCUMENT_FRAGMENT_NODE;
        const unsigned short NOTATION_NODE;

        unsigned short nodeType;
        struct __namespace_DOM_Node* ownerDocument;
        struct __namespace_DOM_Node* parentNode;
        struct __namespace_DOM_Node** childNodes;

        union
        {
            struct
            {
                enum { no_quirks, quirks, limited_quirks } mode;
            } Document;
            struct
            {
                DOMString data;
            } Comment;
            struct
            {
                DOMString name;
                DOMString publicId;
                DOMString systemId;
            } DocumentType;
            struct
            {
                DOMString localName;
                struct __namespace_DOM_NamedNodeMap* attributes;
            } Element;
            struct
            {
                DOMString wholeText;
            } Text;
            struct
            {
                DOMString name;
                DOMString value;
            } Attr;
        } as;

        struct
        {
            struct
            {
                struct __namespace_DOM_Node* (*new)();
            } Document;
            struct
            {
                struct __namespace_DOM_Node* (*new)(struct __namespace_DOM_Node* document, DOMString name, DOMString publicId, DOMString systemId);
            } DocumentType;
            struct
            {
                struct __namespace_DOM_Node* (*new)(struct __namespace_DOM_Node* document);
            } Element;
            struct
            {
                struct __namespace_DOM_Node* (*new)(struct __namespace_DOM_Node* document, DOMString data);
            } Comment;
            struct
            {
                struct __namespace_DOM_Node* (*new)(struct __namespace_DOM_Node* document, DOMString wholeText);
            } Text;
            struct
            {
                struct __namespace_DOM_Node* (*new)(struct __namespace_DOM_Node* document, DOMString name, DOMString value);
            } Attr;
        };

        struct __namespace_DOM_Node* (*new)(unsigned short nodeType, struct __namespace_DOM_Node* document);
        void (*delete)(struct __namespace_DOM_Node* self);
    } Node;

    struct __namespace_DOM_Node* (*create_element)(struct __namespace_DOM_Node* document, DOMString localName);
};

extern struct __namespace_DOM DOM;
