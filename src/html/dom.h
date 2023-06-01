#pragma once

#include <oop/object.h>

#include <utils/wstring.h>
#include <utils/vector.h>

typedef WString* DOMString;

typedef struct __DOM_Node __DOM_Node;
typedef struct __DOM_NodeList __DOM_NodeList;
typedef struct __DOM_NamedNodeMap __DOM_NamedNodeMap;
typedef struct __DOM_Node_Document __DOM_Node_Document;
typedef struct __DOM_Node_DocumentType __DOM_Node_DocumentType;
typedef struct __DOM_Node_Element __DOM_Node_Element;
typedef struct __DOM_Node_Attr __DOM_Node_Attr;
typedef struct __DOM_Node_Text __DOM_Node_Text;
typedef struct __DOM_Node_Comment __DOM_Node_Comment;

struct __DOM
{
    struct
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

        __DOM_Node* (*new)(unsigned short nodeType, const __DOM_Node_Document* document);
    } Node;

    struct
    {
        __DOM_NodeList* (*new)();
    } NodeList;

    struct
    {
        __DOM_NamedNodeMap* (*new)();
    } NamedNodeMap;

    struct
    {
        __DOM_Node_Document* (*new)();
    } Document;

    struct
    {
        __DOM_Node_DocumentType* (*new)(const __DOM_Node_Document* document, const DOMString name, const DOMString publicId, DOMString systemId);
    } DocumentType;

    struct
    {
        __DOM_Node_Element* (*new)(const __DOM_Node_Document* document);
    } Element;

    struct
    {
        __DOM_Node_Attr* (*new)(const __DOM_Node_Document* document, const DOMString name, const DOMString value);
    } Attr;

    struct
    {
        __DOM_Node_Text* (*new)(const __DOM_Node_Document* document, const DOMString wholeText);
    } Text;

    struct
    {
        __DOM_Node_Comment* (*new)(const __DOM_Node_Document* document, const DOMString data);
    } Comment;

    __DOM_Node_Element* (*create_element)(const __DOM_Node_Document* document, const DOMString localName);
};

struct __DOM_Node
{
    ObjectExtends(Object);

    const unsigned short (*nodeType)();
    const __DOM_Node_Document* (*ownerDocument)();
    const __DOM_NodeList* (*childNodes)();

    void (*appendChild)(const __DOM_Node* node);

    unsigned short _nodeType;
    const struct __DOM_Node_Document* _ownerDocument;
    struct __DOM_NodeList* _childNodes;
};

struct __DOM_NodeList
{
    ObjectExtends(Object);

    const __DOM_Node* (*item)(unsigned long index);
    const unsigned long (*length)();

    unsigned long _length;
    Vector* _nodes;
};

struct __DOM_NamedNodeMap
{
    ObjectExtends(Object);

    const unsigned long (*length)();

    const __DOM_Node_Attr* (*item)(unsigned long index);
    const __DOM_Node_Attr* (*getNamedItem)(const DOMString qualifiedName);
    const __DOM_Node_Attr* (*setNamedItem)(const __DOM_Node_Attr* attr);
    const __DOM_Node_Attr* (*removeNamedItem)(const DOMString qualifiedName);

    unsigned long _length;
    Vector* _attributes;
};

enum __DOM_Node_Document_Modes
{
    no_quirks,
    quirks,
    limited_quirks
};

struct __DOM_Node_Document
{
    ObjectExtends(__DOM_Node);
    
    const enum __DOM_Node_Document_Modes (*mode)();

    enum __DOM_Node_Document_Modes _mode;
};

struct __DOM_Node_DocumentType
{
    ObjectExtends(__DOM_Node);

    const DOMString (*name)();
    const DOMString (*publicId)();
    const DOMString (*systemId)();

    DOMString _name;
    DOMString _publicId;
    DOMString _systemId;
};

struct __DOM_Node_Element
{
    ObjectExtends(__DOM_Node);

    const DOMString (*localName)();
    const struct __DOM_NamedNodeMap* (*attributes)();

    DOMString _localName;
    struct __DOM_NamedNodeMap* _attributes;
};

struct __DOM_Node_Attr
{
    ObjectExtends(__DOM_Node);

    DOMString (*name)();
    DOMString (*value)();

    DOMString _name;
    DOMString _value;
};

struct __DOM_Node_Text
{
    ObjectExtends(__DOM_Node);

    DOMString (*wholeText)();

    DOMString _wholeText;
};

struct __DOM_Node_Comment
{
    ObjectExtends(__DOM_Node);

    DOMString (*data)();

    DOMString _data;
};

extern struct __DOM DOM;
