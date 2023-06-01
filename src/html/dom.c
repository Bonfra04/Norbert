#include <html/dom.h>

static void __DOM_Node_appendChild(const __DOM_Node* node, __DOM_Node* self)
{
    self->_childNodes->_nodes->append(node);
    self->_childNodes->_length++;
}

static void __DOM_Node_destructor(__DOM_Node* self)
{
    self->_childNodes->delete();
    self->super.destruct();
}

static __DOM_Node* __DOM_Node_new(unsigned short nodeType, const __DOM_Node_Document* document)
{
    __DOM_Node* self = ObjectBase(__DOM_Node, 4);
    
    self->_nodeType = nodeType;
    self->_ownerDocument = document;
    self->_childNodes = DOM.NodeList.new();

    ObjectGetter(__DOM_Node, nodeType);
    ObjectGetter(__DOM_Node, ownerDocument);
    ObjectGetter(__DOM_Node, childNodes);

    ObjectFunction(__DOM_Node, appendChild, 1);

    Object_prepare((Object*)&self->super);
    return self;
}

static void __DOM_NodeList_destructor(__DOM_NodeList* self)
{
    self->_nodes->delete();
    self->super.destruct();
}

static const __DOM_Node* __DOM_NodeList_item(unsigned long index, __DOM_NodeList* self)
{
    if(index >= self->_length)
    {
        return NULL;
    }
    return (__DOM_Node*)self->_nodes->at(index);
}

static __DOM_NodeList* __DOM_NodeList_new()
{
    __DOM_NodeList* self = ObjectBase(__DOM_NodeList, 2);
    
    self->_length = 0;
    self->_nodes = Vector_new();

    ObjectGetter(__DOM_NodeList, length);
    ObjectFunction(__DOM_NodeList, item, 1);

    Object_prepare((Object*)&self->super);
    return self;
}

static const __DOM_Node_Attr*  __DOM_NamedNodeMap_item(unsigned long index, __DOM_NamedNodeMap* self)
{
    if(index >= self->_length)
    {
        return NULL;
    }
    return (__DOM_Node_Attr*)self->_attributes->at(index);
}

static const __DOM_Node_Attr* __DOM_NamedNodeMap_getNamedItem(const DOMString qualifiedName, __DOM_NamedNodeMap* self)
{
    for(unsigned long i = 0; i < self->_length; i++)
    {
        __DOM_Node_Attr* attr = (__DOM_Node_Attr*)self->_attributes->at(i);
        if(attr->_name->equals(qualifiedName))
        {
            return attr;
        }
    }
    return NULL;
}

static const __DOM_Node_Attr* __DOM_NamedNodeMap_setNamedItem(const __DOM_Node_Attr* attr, __DOM_NamedNodeMap* self)
{
    for(unsigned long i = 0; i < self->_length; i++)
    {
        __DOM_Node_Attr* existing = (__DOM_Node_Attr*)self->_attributes->at(i);
        if(existing->_name->equals(attr->_name))
        {
            self->_attributes->set(i, attr);
            return existing;
        }
    }
    self->_attributes->append(attr);
    self->_length++;
    return NULL;
}

static const __DOM_Node_Attr* __DOM_NamedNodeMap_removeNamedItem(DOMString qualifiedName, __DOM_NamedNodeMap* self)
{
    for(unsigned long i = 0; i < self->_length; i++)
    {
        __DOM_Node_Attr* attr = (__DOM_Node_Attr*)self->_attributes->at(i);
        if(attr->_name->equals(qualifiedName))
        {
            self->_attributes->remove(i);
            self->_length--;
            return attr;
        }
    }
    return NULL;
}

static void __DOM_NamedNodeMap_destructor(__DOM_NamedNodeMap* self)
{
    self->_attributes->delete();
    self->super.destruct();
}

static __DOM_NamedNodeMap* __DOM_NamedNodeMap_new()
{
    __DOM_NamedNodeMap* self = ObjectBase(__DOM_NamedNodeMap, 5);
    
    self->_length = 0;
    self->_attributes = Vector_new();

    ObjectGetter(__DOM_NamedNodeMap, length);

    ObjectFunction(__DOM_NamedNodeMap, item, 1);
    ObjectFunction(__DOM_NamedNodeMap, getNamedItem, 1);
    ObjectFunction(__DOM_NamedNodeMap, setNamedItem, 1);
    ObjectFunction(__DOM_NamedNodeMap, removeNamedItem, 1);

    Object_prepare((Object*)&self->super);
    return self;
} 

static void __DOM_Node_Document_destructor(__DOM_Node_Document* self)
{
    self->super.destruct();
}

static __DOM_Node_Document* __DOM_Node_Document_new()
{
    __DOM_Node_Document* self = ObjectFromSuper(__DOM_Node, __DOM_Node_Document, 1, DOM.Node.DOCUMENT_NODE, NULL);

    self->_mode = no_quirks;

    ObjectGetter(__DOM_Node_Document, mode);

    Object_prepare((Object*)&self->super);
    return self;
}

static void __DOM_Node_DocumentType_destructor(__DOM_Node_DocumentType* self)
{
    self->_name->delete();
    self->_publicId->delete();
    self->_systemId->delete();
    self->super.destruct();
}

static __DOM_Node_DocumentType* __DOM_Node_DocumentType_new(const __DOM_Node_Document* document, const DOMString name, const DOMString publicId, const DOMString systemId)
{
    __DOM_Node_DocumentType* self = ObjectFromSuper(__DOM_Node, __DOM_Node_DocumentType, 3, DOM.Node.DOCUMENT_TYPE_NODE, document);

    self->_name = name ? name->copy() : WString_new();
    self->_publicId = publicId ? publicId->copy() : WString_new();
    self->_systemId = systemId ? systemId->copy() : WString_new();

    ObjectGetter(__DOM_Node_DocumentType, name);
    ObjectGetter(__DOM_Node_DocumentType, publicId);
    ObjectGetter(__DOM_Node_DocumentType, systemId);

    Object_prepare((Object*)&self->super);
    return self;
}

static void __DOM_Node_Element_destructor(__DOM_Node_Element* self)
{
    self->_localName->delete();
    // self->_attributes->delete();
    self->super.destruct();
}

static __DOM_Node_Element* __DOM_Node_Element_new(const __DOM_Node_Document* document)
{
    __DOM_Node_Element* self = ObjectFromSuper(__DOM_Node, __DOM_Node_Element, 2, DOM.Node.ELEMENT_NODE, document);

    self->_attributes = DOM.NamedNodeMap.new();

    ObjectGetter(__DOM_Node_Element, localName);
    ObjectGetter(__DOM_Node_Element, attributes);

    Object_prepare((Object*)&self->super);
    return self;
}

static void __DOM_Node_Attr_destructor(__DOM_Node_Attr* self)
{
    self->_name->delete();
    self->_value->delete();
    self->super.destruct();
}

static __DOM_Node_Attr* __DOM_Node_Attr_new(const __DOM_Node_Document* document, const DOMString name, const DOMString value)
{
    __DOM_Node_Attr* self = ObjectFromSuper(__DOM_Node, __DOM_Node_Attr, 2, DOM.Node.ATTRIBUTE_NODE, document);

    self->_name = name ? name->copy() : WString_new();
    self->_value = value ? value->copy() : WString_new();

    ObjectGetter(__DOM_Node_Attr, name);
    ObjectGetter(__DOM_Node_Attr, value);

    Object_prepare((Object*)&self->super);
    return self;
}

static void __DOM_Node_Text_destructor(__DOM_Node_Text* self)
{
    self->_wholeText->delete();
    self->super.destruct();
}

static __DOM_Node_Text* __DOM_Node_Text_new(const __DOM_Node_Document* document, const DOMString wholeText)
{
    __DOM_Node_Text* self = ObjectFromSuper(__DOM_Node, __DOM_Node_Text, 1, DOM.Node.TEXT_NODE, document);

    self->_wholeText = wholeText ? wholeText->copy() : WString_new();

    ObjectGetter(__DOM_Node_Text, wholeText);

    Object_prepare((Object*)&self->super);
    return self;
}

static void __DOM_Node_Comment_destructor(__DOM_Node_Comment* self)
{
    self->_data->delete();
    self->super.destruct();
}

static __DOM_Node_Comment* __DOM_Node_Comment_new(const __DOM_Node_Document* document, const DOMString data)
{
    __DOM_Node_Comment* self = ObjectFromSuper(__DOM_Node, __DOM_Node_Comment, 1, DOM.Node.COMMENT_NODE, document);

    self->_data = data ? data->copy() : WString_new();

    ObjectGetter(__DOM_Node_Comment, data);

    Object_prepare((Object*)&self->super);
    return self;
}

static __DOM_Node_Element* __DOM_create_element(const __DOM_Node_Document* document, const DOMString localName)
{
    __DOM_Node_Element* result = DOM.Element.new(document);

    result->_localName = localName ? localName->copy() : WString_new();

    return result;
}

struct __DOM DOM = {
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
    
        .new = __DOM_Node_new,
    },

    .NodeList = {
        .new = __DOM_NodeList_new,
    },

    .NamedNodeMap = {
        .new = __DOM_NamedNodeMap_new,
    },

    .Document = {
        .new = __DOM_Node_Document_new,
    },

    .DocumentType = {
        .new = __DOM_Node_DocumentType_new,
    },

    .Element = {
        .new = __DOM_Node_Element_new,
    },

    .Attr = {
        .new = __DOM_Node_Attr_new,
    },
    
    .Text = {
        .new = __DOM_Node_Text_new,
    },

    .Comment = {
        .new = __DOM_Node_Comment_new,
    },

    .create_element = __DOM_create_element,
};
