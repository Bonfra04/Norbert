#pragma once

#include "oop/object.h"

#include <wchar.h>
#include <stdbool.h>

typedef struct WStringRec
{
    ObjectExtends(Object);

    size_t (*length)();
    bool (*empty)();
    void (*clear)();
    wchar_t (*at)(size_t index);
    struct WStringRec* (*substr)(size_t start, size_t length);
    struct WStringRec* (*copy)();

    void (*appendwc)(wchar_t c);
    void (*appendc)(char c);
    void (*appendws)(wchar_t* ws);
    void (*appends)(char* s);
    void (*append)(struct WStringRec* other);
    wchar_t (*popback)();
    wchar_t (*popfront)();

    bool (*equals)(struct WStringRec* other);
    bool (*equalss)(char* other);
    bool (*equalsws)(wchar_t* other);
    bool (*equalsi)(struct WStringRec* other);
    bool (*equalssi)(char* other);
    bool (*equalswsi)(wchar_t* other);

    bool (*equalsOneOff)(struct WStringRec* others[]);

    bool (*startswith)(struct WStringRec* other);
    bool (*startswiths)(char* other);
    bool (*startswithws)(wchar_t* other);
    bool (*startswithi)(struct WStringRec* other);
    bool (*startswithsi)(char* other);
    bool (*startswithwsi)(wchar_t* other);

    bool (*endswith)(struct WStringRec* other);
    bool (*endswiths)(char* other);
    bool (*endswithws)(wchar_t* other);
    bool (*endswithi)(struct WStringRec* other);
    bool (*endswithsi)(char* other);
    bool (*endswithwsi)(wchar_t* other);

    bool (*contains)(struct WStringRec* other);
    bool (*containss)(char* other);
    bool (*containsws)(wchar_t* other);
    bool (*containsi)(struct WStringRec* other);
    bool (*containssi)(char* other);
    bool (*containswsi)(wchar_t* other);

    wchar_t* data;
    size_t capacity;
    size_t len;
} WString;

WString* WString_new();
