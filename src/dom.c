#include "dom.h"

#include "utils/wstring.h"
#include "utils/vector.h"

#include <stdlib.h>

static type(DOM.Node)* __DOM_Node_new(unsigned short nodeType, type(DOM.Node)* document);
static type(DOM.Node)* __DOM_Node_Document_new();
static type(DOM.Node)* __DOM_Node_DocumentType_new(type(DOM.Node)* document, DOMString name, DOMString publicId, DOMString systemId);
static type(DOM.Node)* __DOM_Node_Element_new(type(DOM.Node)* document);
static type(DOM.Node)* __DOM_Node_Comment_new(type(DOM.Node)* document, DOMString data);
static type(DOM.Node)* __DOM_Node_Text_new(type(DOM.Node)* document, DOMString wholeText);
static type(DOM.Node)* __DOM_Node_Attr_new(type(DOM.Node)* document, DOMString name, DOMString value);

static type(DOM.Node)* __DOM_create_element(type(DOM.Node)* document, wchar_t* localName);

static type(DOM.NamedNodeMap)* __DOM_NamedNodeMap_new();
static type(DOM.Node)* __DOM_NamedNodeMap_item(type(DOM.NamedNodeMap)* self, unsigned long index);
static type(DOM.Node)* __DOM_NamedNodeMap_getNamedItem(type(DOM.NamedNodeMap)* self, DOMString name);
static type(DOM.Node)* __DOM_NamedNodeMap_setNamedItem(type(DOM.NamedNodeMap)* self, type(DOM.Node)* node);
static type(DOM.Node)* __DOM_NamedNodeMap_removeNamedItem(type(DOM.NamedNodeMap)* self, DOMString name);

static void __DOM_Node_delete(type(DOM.Node)* self);
static void __DOM_Node_Attr_delete(type(DOM.Node)* self);

struct __namespace_DOM DOM = {
    .NamedNodeMap = {
        .new = __DOM_NamedNodeMap_new,
        .item = __DOM_NamedNodeMap_item,
        .getNamedItem = __DOM_NamedNodeMap_getNamedItem,
        .setNamedItem = __DOM_NamedNodeMap_setNamedItem,
        .removeNamedItem = __DOM_NamedNodeMap_removeNamedItem,
    },

    .Node = {
        .ELEMENT_NODE = 1,
        .ATTRIBUTE_NODE = 2,
        .TEXT_NODE = 3,
        .CDATA_SECTION_NODE = 4,
        .ENTITY_REFERENCE_NODE = 5,
        .ENTITY_NODE = 6,
        .PROCESSING_INSTRUCTION_NODE = 7,
        .COMMENT_NODE = 8,
        .DOCUMENT_NODE = 9,
        .DOCUMENT_TYPE_NODE = 10,
        .DOCUMENT_FRAGMENT_NODE = 11,
        .NOTATION_NODE = 12,

        .Document = {
            .new = __DOM_Node_Document_new,
        },

        .DocumentType = {
            .new = __DOM_Node_DocumentType_new,
        },
        
        .Element = {
            .new = __DOM_Node_Element_new,
        },

        .Comment = {
            .new = __DOM_Node_Comment_new,
        },

        .Text = {
            .new = __DOM_Node_Text_new,
        },

        .Attr = {
            .new = __DOM_Node_Attr_new,
        },

        .new = __DOM_Node_new,
        .delete = __DOM_Node_delete,
    },

    .create_element = __DOM_create_element,
};

static type(DOM.Node)* __DOM_Node_new(unsigned short nodeType, type(DOM.Node)* document)
{
    type(DOM.Node)* self = calloc(sizeof(type(DOM.Node)), 1);
    
    self->nodeType = nodeType;
    self->childNodes = vector_new(sizeof(type(DOM.Node)*));
    self->ownerDocument = document;

    return self;
}

static void __DOM_Node_delete(type(DOM.Node)* self)
{
    size_t num_childs = vector_length(self->childNodes);
    for (size_t i = 0; i < num_childs; i++)
        self->childNodes[i]->delete(self->childNodes[i]);

    vector_free(self->childNodes);
    free(self);
}

static type(DOM.Node)* __DOM_Node_Document_new()
{
    type(DOM.Node)* self = DOM.Node.new(DOM.Node.DOCUMENT_NODE, NULL);
    self->ownerDocument = self;

    return self;
}

static type(DOM.Node)* __DOM_Node_DocumentType_new(type(DOM.Node)* document, DOMString name, DOMString publicId, DOMString systemId)
{
    type(DOM.Node)* self = DOM.Node.new(DOM.Node.DOCUMENT_TYPE_NODE, document);

    self->as.DocumentType.name = wstring_copy(name) ?: wstring_new();
    self->as.DocumentType.publicId = wstring_copy(publicId) ?: wstring_new();
    self->as.DocumentType.systemId = wstring_copy(systemId) ?: wstring_new();

    return self;
}

static type(DOM.Node)* __DOM_Node_Element_new(type(DOM.Node)* document)
{
    type(DOM.Node)* self = DOM.Node.new(DOM.Node.ELEMENT_NODE, document);

    self->as.Element.attributes = vector_new(sizeof(type(DOM.NamedNodeMap)*));

    return self;
}

static type(DOM.Node)* __DOM_Node_Comment_new(type(DOM.Node)* document, DOMString data)
{
    type(DOM.Node)* self = DOM.Node.new(DOM.Node.COMMENT_NODE, document);

    self->as.Comment.data = wstring_copy(data);

    return self;
}

static type(DOM.Node)* __DOM_create_element(type(DOM.Node)* document, DOMString localName)
{
    type(DOM.Node)* result = DOM.Node.Element.new(document);

    result->as.Element.localName = wstring_copy(localName);
    result->as.Element.attributes = DOM.NamedNodeMap.new();

    return result;
}

static type(DOM.Node)* __DOM_Node_Text_new(type(DOM.Node)* document, DOMString wholeText)
{
    type(DOM.Node)* self = DOM.Node.new(DOM.Node.TEXT_NODE, document);

    self->as.Text.wholeText = wstring_copy(wholeText);

    return self;
}

static type(DOM.Node)* __DOM_Node_Attr_new(type(DOM.Node)* document, DOMString name, DOMString value)
{
    type(DOM.Node)* self = DOM.Node.new(DOM.Node.ATTRIBUTE_NODE, document);

    self->as.Attr.name = wstring_copy(name);
    self->as.Attr.value = wstring_copy(value);

    self->delete = __DOM_Node_Attr_delete;

    return self;
}

static void __DOM_Node_Attr_delete(type(DOM.Node)* self)
{
    wstring_free(self->as.Attr.name);
    wstring_free(self->as.Attr.value);
    __DOM_Node_delete(self);
}

static type(DOM.NamedNodeMap)* __DOM_NamedNodeMap_new()
{
    type(DOM.NamedNodeMap)* self = calloc(sizeof(type(DOM.NamedNodeMap)), 1);

    self->attributes = vector_new(sizeof(type(DOM.Node)*));

    return self;
}

static type(DOM.Node)* __DOM_NamedNodeMap_item(type(DOM.NamedNodeMap)* self, unsigned long index)
{
    if(index >= self->length)
        return NULL;
    return self->attributes[index];
}

static type(DOM.Node)* __DOM_NamedNodeMap_getNamedItem(type(DOM.NamedNodeMap)* self, DOMString name)
{
    for(unsigned long i = 0; i < self->length; i++)
    {
        type(DOM.Node)* node = self->attributes[i];
        if(wcscmp(node->as.Attr.name, name) == 0)
            return node;
    }
    return NULL;
}

static type(DOM.Node)* __DOM_NamedNodeMap_setNamedItem(type(DOM.NamedNodeMap)* self, type(DOM.Node)* node)
{
    for(unsigned long i = 0; i < self->length; i++)
    {
        type(DOM.Node)* existing = self->attributes[i];
        if(wcscmp(existing->as.Attr.name, node->as.Attr.name) == 0)
        {
            self->attributes[i] = node;
            return existing;
        }
    }
    vector_append(self->attributes, &node);
    self->length++;
    return NULL;
}

static type(DOM.Node)* __DOM_NamedNodeMap_removeNamedItem(type(DOM.NamedNodeMap)* self, DOMString name)
{
    for(unsigned long i = 0; i < self->length; i++)
    {
        type(DOM.Node)* node = self->attributes[i];
        if(wcscmp(node->as.Attr.name, name) == 0)
        {
            vector_remove(self->attributes, i);
            self->length--;
            return node;
        }
    }
    return NULL;
}
