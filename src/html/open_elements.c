#include <html/open_elements.h>

#include <html/html.h>

#define basicScopeList HTML.TagNames.applet, HTML.TagNames.caption, HTML.TagNames.html, HTML.TagNames.table, HTML.TagNames.td, HTML.TagNames.th, HTML.TagNames.marquee, HTML.TagNames.object, HTML.TagNames.template

static __DOM_Node* StackOfOpenElements_at(int64_t index, StackOfOpenElements* self)
{
    return self->data->at(index);
}

static size_t StackOfOpenElements_length(StackOfOpenElements* self)
{
    return self->data->length();
}

static void StackOfOpenElements_push(__DOM_Node* node, StackOfOpenElements* self)
{
    self->data->append(node);
}

static void StackOfOpenElements_pop(StackOfOpenElements* self)
{
    self->data->pop();
}

static void StackOfOpenElements_remove(__DOM_Node* node, StackOfOpenElements* self)
{
    self->data->remove_first(node);
}

static bool StackOfOpenElements_contains(DOMString tagName, StackOfOpenElements* self)
{
    for (size_t i = 0; i < self->data->length(); i++)
    {
        __DOM_Node_Element* element = (__DOM_Node_Element*)self->at(i);
        if (element->localName()->equals(tagName))
        {
            return true;
        }
    }

    return false;
}

static bool has_in_scope_specific_str(StackOfOpenElements* self, DOMString tagName, DOMString scope[])
{
    for(size_t i = self->data->length(); i > 0; i--)
    {
        __DOM_Node_Element* element = (__DOM_Node_Element*)self->at(i - 1);
        if(element->localName()->equals(tagName))
        {
            return true;
        }

        for(DOMString* list = scope; *list; list++)
        {
            if(element->localName()->equals(*list))
            {
                return false;
            }
        }
    }

    return false;
}

static bool StackOfOpenElements_hasInScopeS(DOMString tagName, StackOfOpenElements* self)
{
    DOMString base_list[] = { basicScopeList, NULL };
    return has_in_scope_specific_str(self, tagName, base_list);
}

static bool StackOfOpenElements_hasInButtonScopeS(DOMString tagName, StackOfOpenElements* self)
{
    DOMString base_list[] = { HTML.TagNames.button, basicScopeList, NULL };
    return has_in_scope_specific_str(self, tagName, base_list);
}

static void StackOfOpenElements_popUntilOneOffPopped(DOMString tagNames[], StackOfOpenElements* self)
{
    __DOM_Node_Element* element = (__DOM_Node_Element*)self->at(-1);
    while(!element->localName()->equalsOneOff(tagNames))
    {
        self->pop();
    }
    self->pop();
}

static void StackOfOpenElements_popUntilPopped(DOMString tagName, StackOfOpenElements* self)
{
    self->popUntilOneOffPopped((DOMString[]){ tagName, NULL });
}

static void StackOfOpenElements_destructor(StackOfOpenElements* self)
{
    self->data->delete();
    self->super.destruct();
}

StackOfOpenElements* StackOfOpenElements_new()
{
    StackOfOpenElements* self = ObjectBase(StackOfOpenElements, 10);

    self->data = Vector_new();

    ObjectFunction(StackOfOpenElements, at, 1);
    ObjectFunction(StackOfOpenElements, length, 0);
    ObjectFunction(StackOfOpenElements, push, 1);
    ObjectFunction(StackOfOpenElements, pop, 0);
    ObjectFunction(StackOfOpenElements, remove, 1);
    ObjectFunction(StackOfOpenElements, contains, 1);
    ObjectFunction(StackOfOpenElements, hasInScopeS, 1);
    ObjectFunction(StackOfOpenElements, hasInButtonScopeS, 1);
    ObjectFunction(StackOfOpenElements, popUntilOneOffPopped, 1);
    ObjectFunction(StackOfOpenElements, popUntilPopped, 1);

    Object_prepare((Object*)&self->super);
    return self;
}
