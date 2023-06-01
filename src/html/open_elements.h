#pragma once

#include <oop/object.h>

#include <html/dom.h>

#include <utils/vector.h>

typedef struct StackOfOpenElements
{
    ObjectExtends(Object);

    __DOM_Node_Element* (*at)(int64_t index);
    size_t (*length)();
    void (*push)(__DOM_Node_Element* node);
    void (*pop)();
    void (*remove)(__DOM_Node_Element* node);
    bool (*contains)(DOMString tagName);

    bool (*hasInScopeS)(DOMString tagName);
    bool (*hasInButtonScopeS)(DOMString tagName);

    void (*popUntilOneOffPopped)(DOMString tagNames[]);
    void (*popUntilPopped)(DOMString tagName);

    Vector* data;
} StackOfOpenElements;

StackOfOpenElements* StackOfOpenElements_new();
