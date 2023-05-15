#pragma once

#include <oop/object.h>

#include <dom.h>

#include <utils/vector.h>

typedef struct StackOfOpenElements
{
    Object object;

    type(DOM.Node)* (*at)(int64_t index);
    size_t (*length)();
    void (*push)(type(DOM.Node)* node);
    void (*pop)();
    void (*remove)(type(DOM.Node)* node);
    bool (*contains)(DOMString tagName);

    bool (*hasInScopeS)(DOMString tagName);
    bool (*hasInButtonScopeS)(DOMString tagName);

    void (*popUntilOneOffPopped)(DOMString tagNames[]);
    void (*popUntilPopped)(DOMString tagName);

    Vector* data;
} StackOfOpenElements;

StackOfOpenElements* StackOfOpenElements_new();
void StackOfOpenElements_delete(StackOfOpenElements* self);
