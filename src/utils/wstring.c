#include "wstring.h"

#include <stdlib.h>
#include <string.h>
#include <wctype.h>
#include <ctype.h>
#include <assert.h>

#define WSTRING_CAPACITY 16

static size_t WString_length(WString* self)
{
    return self->len;
}

static bool WString_empty(WString* self)
{
    return self->len == 0;
}

static void WString_clear(WString* self)
{
    self->len = 0;
    self->data[0] = L'\0';
}

static wchar_t WString_at(size_t index, WString* self)
{
    assert(index < self->len);
    return self->data[index];
}

static void WString_appendwc(wchar_t c, WString* self)
{
    if(self->len + 1 >= self->capacity)
    {
        self->capacity *= 2;
        self->data = realloc(self->data, sizeof(wchar_t) * self->capacity);
    }

    self->data[self->len++] = c;
    self->data[self->len] = L'\0';
}

static void WString_appendc(char c, WString* self)
{
    WString_appendwc((wchar_t)c, self);
}

static void WString_appendws(wchar_t* ws, WString* self)
{
    for(size_t i = 0; ws[i] != L'\0'; i++)
    {
        WString_appendwc(ws[i], self);
    }
}

static void WString_appends(char* s, WString* self)
{
    for(size_t i = 0; s[i] != '\0'; i++)
    {
        WString_appendc(s[i], self);
    }
}

static void WString_append(WString* other, WString* self)
{
    for(size_t i = 0; i < other->len; i++)
    {
        WString_appendwc(other->data[i], self);
    }
}

static wchar_t WString_popback(WString* self)
{
    if(self->len == 0)
    {
        return L'\0';
    }

    wchar_t c = self->data[--self->len];
    self->data[self->len] = L'\0';
    return c;
}

static wchar_t WString_popfront(WString* self)
{
    if(self->len == 0)
    {
        return L'\0';
    }

    wchar_t c = self->data[0];
    memmove(self->data, self->data + 1, sizeof(wchar_t) * self->len);
    self->data[--self->len] = L'\0';
    return c;
}

static WString* WString_substr(size_t start, size_t length, WString* self)
{
    WString* substr = WString_new();
    for(size_t i = start; i < start + length; i++)
    {
        WString_appendwc(self->data[i], substr);
    }

    return substr;
}

static WString* WString_copy(WString* self)
{
    WString* new = WString_new();
    new->append(self);
    return new;
}

static bool WString_equals(WString* other, WString* self)
{
    if(other->len != self->len)
    {
        return false;
    }

    for(size_t i = 0; i < self->len; i++)
    {
        if(other->data[i] != self->data[i])
        {
            return false;
        }
    }

    return true;
}

static bool WString_equalss(char* other, WString* self)
{
    size_t other_len = strlen(other);
    if(other_len != self->len)
    {
        return false;
    }

    for(size_t i = 0; i < self->len; i++)
    {
        if(other[i] != self->data[i])
        {
            return false;
        }
    }

    return true;
}

static bool WString_equalsws(wchar_t* other, WString* self)
{
    size_t other_len = wcslen(other);
    if(other_len != self->len)
    {
        return false;
    }

    for(size_t i = 0; i < self->len; i++)
    {
        if(other[i] != self->data[i])
        {
            return false;
        }
    }

    return true;
}

static bool WString_equalsi(WString* other, WString* self)
{
    if(other->len != self->len)
    {
        return false;
    }

    for(size_t i = 0; i < self->len; i++)
    {
        if(towlower(other->data[i]) != towlower(self->data[i]))
        {
            return false;
        }
    }

    return true;
}

static bool WString_equalssi(char* other, WString* self)
{
    size_t other_len = strlen(other);
    if(other_len != self->len)
    {
        return false;
    }

    for(size_t i = 0; i < self->len; i++)
    {
        if(tolower(other[i]) != towlower(self->data[i]))
        {
            return false;
        }
    }

    return true;
}

static bool WString_equalswsi(wchar_t* other, WString* self)
{
    size_t other_len = wcslen(other);
    if(other_len != self->len)
    {
        return false;
    }

    for(size_t i = 0; i < self->len; i++)
    {
        if(towlower(other[i]) != towlower(self->data[i]))
        {
            return false;
        }
    }

    return true;
}

static bool WString_equalsOneOff(WString* others[], WString* self)
{
    for(WString** other = others; *other; other++)
    {
        if(self->equals(*other))
        {
            return true;
        }
    }

    return false;
}

static bool WString_startswith(WString* other, WString* self)
{
    if(other->len > self->len)
    {
        return false;
    }

    for(size_t i = 0; i < other->len; i++)
    {
        if(other->data[i] != self->data[i])
        {
            return false;
        }
    }

    return true;
}

static bool WString_startswiths(char* other, WString* self)
{
    size_t other_len = strlen(other);
    if(other_len > self->len)
    {
        return false;
    }

    for(size_t i = 0; i < other_len; i++)
    {
        if(other[i] != self->data[i])
        {
            return false;
        }
    }

    return true;
}

static bool WString_startswithws(wchar_t* other, WString* self)
{
    size_t other_len = wcslen(other);
    if(other_len > self->len)
    {
        return false;
    }

    for(size_t i = 0; i < other_len; i++)
    {
        if(other[i] != self->data[i])
        {
            return false;
        }
    }

    return true;
}

static bool WString_startswithi(WString* other, WString* self)
{
    if(other->len > self->len)
    {
        return false;
    }

    for(size_t i = 0; i < other->len; i++)
    {
        if(towlower(other->data[i]) != towlower(self->data[i]))
        {
            return false;
        }
    }

    return true;
}

static bool WString_startswithsi(char* other, WString* self)
{
    size_t other_len = strlen(other);
    if(other_len > self->len)
    {
        return false;
    }

    for(size_t i = 0; i < other_len; i++)
    {
        if(tolower(other[i]) != towlower(self->data[i]))
        {
            return false;
        }
    }

    return true;
}

static bool WString_startswithwsi(wchar_t* other, WString* self)
{
    size_t other_len = wcslen(other);
    if(other_len > self->len)
    {
        return false;
    }

    for(size_t i = 0; i < other_len; i++)
    {
        if(towlower(other[i]) != towlower(self->data[i]))
        {
            return false;
        }
    }

    return true;
}

static bool WString_endswith(WString* other, WString* self)
{
    if(other->len > self->len)
    {
        return false;
    }

    for(size_t i = 0; i < other->len; i++)
    {
        if(other->data[other->len - i - 1] != self->data[self->len - i - 1])
        {
            return false;
        }
    }

    return true;
}

static bool WString_endswiths(char* other, WString* self)
{
    size_t other_len = strlen(other);
    if(other_len > self->len)
    {
        return false;
    }

    for(size_t i = 0; i < other_len; i++)
    {
        if(other[other_len - i - 1] != self->data[self->len - i - 1])
        {
            return false;
        }
    }

    return true;
}

static bool WString_endswithws(wchar_t* other, WString* self)
{
    size_t other_len = wcslen(other);
    if(other_len > self->len)
    {
        return false;
    }

    for(size_t i = 0; i < other_len; i++)
    {
        if(other[other_len - i - 1] != self->data[self->len - i - 1])
        {
            return false;
        }
    }

    return true;
}

static bool WString_endswithi(WString* other, WString* self)
{
    if(other->len > self->len)
    {
        return false;
    }

    for(size_t i = 0; i < other->len; i++)
    {
        if(towlower(other->data[other->len - i - 1]) != towlower(self->data[self->len - i - 1]))
        {
            return false;
        }
    }

    return true;
}

static bool WString_endswithsi(char* other, WString* self)
{
    size_t other_len = strlen(other);
    if(other_len > self->len)
    {
        return false;
    }

    for(size_t i = 0; i < other_len; i++)
    {
        if(tolower(other[other_len - i - 1]) != towlower(self->data[self->len - i - 1]))
        {
            return false;
        }
    }

    return true;
}

static bool WString_endswithwsi(wchar_t* other, WString* self)
{
    size_t other_len = wcslen(other);
    if(other_len > self->len)
    {
        return false;
    }

    for(size_t i = 0; i < other_len; i++)
    {
        if(towlower(other[other_len - i - 1]) != towlower(self->data[self->len - i - 1]))
        {
            return false;
        }
    }

    return true;
}

static bool WString_contains(WString* other, WString* self)
{
    if(other->len > self->len)
    {
        return false;
    }

    for(size_t i = 0; i < self->len - other->len + 1; i++)
    {
        for(size_t j = 0; j < other->len; j++)
        {
            if(other->data[j] != self->data[i + j])
            {
                break;
            }

            if(j == other->len - 1)
            {
                return true;
            }
        }
    }

    return false;
}

static bool WString_containss(char* other, WString* self)
{
    size_t other_len = strlen(other);
    if(other_len > self->len)
    {
        return false;
    }

    for(size_t i = 0; i < self->len - other_len + 1; i++)
    {
        for(size_t j = 0; j < other_len; j++)
        {
            if(other[j] != self->data[i + j])
            {
                break;
            }

            if(j == other_len - 1)
            {
                return true;
            }
        }
    }

    return false;
}

static bool WString_containsws(wchar_t* other, WString* self)
{
    size_t other_len = wcslen(other);
    if(other_len > self->len)
    {
        return false;
    }

    for(size_t i = 0; i < self->len - other_len + 1; i++)
    {
        for(size_t j = 0; j < other_len; j++)
        {
            if(other[j] != self->data[i + j])
            {
                break;
            }

            if(j == other_len - 1)
            {
                return true;
            }
        }
    }

    return false;
}

static bool WString_containsi(WString* other, WString* self)
{
    if(other->len > self->len)
    {
        return false;
    }

    for(size_t i = 0; i < self->len - other->len + 1; i++)
    {
        for(size_t j = 0; j < other->len; j++)
        {
            if(towlower(other->data[j]) != towlower(self->data[i + j]))
            {
                break;
            }

            if(j == other->len - 1)
            {
                return true;
            }
        }
    }

    return false;
}

static bool WString_containssi(char* other, WString* self)
{
    size_t other_len = strlen(other);
    if(other_len > self->len)
    {
        return false;
    }

    for(size_t i = 0; i < self->len - other_len + 1; i++)
    {
        for(size_t j = 0; j < other_len; j++)
        {
            if(tolower(other[j]) != towlower(self->data[i + j]))
            {
                break;
            }

            if(j == other_len - 1)
            {
                return true;
            }
        }
    }

    return false;
}

static bool WString_containswsi(wchar_t* other, WString* self)
{
    size_t other_len = wcslen(other);
    if(other_len > self->len)
    {
        return false;
    }

    for(size_t i = 0; i < self->len - other_len + 1; i++)
    {
        for(size_t j = 0; j < other_len; j++)
        {
            if(towlower(other[j]) != towlower(self->data[i + j]))
            {
                break;
            }

            if(j == other_len - 1)
            {
                return true;
            }
        }
    }

    return false;
}

static void WString_destructor(WString* self)
{
    free(self->data);
    self->super.destruct();
}

WString* WString_new()
{
    WString* self = ObjectBase(WString, 38);

    self->data = malloc(sizeof(wchar_t) * WSTRING_CAPACITY);
    self->capacity = WSTRING_CAPACITY;
    self->len = 0;
    self->data[0] = L'\0';

    ObjectFunction(WString, length, 0);
    ObjectFunction(WString, empty, 0);
    ObjectFunction(WString, clear, 0);
    ObjectFunction(WString, at, 1);
    ObjectFunction(WString, substr, 2);
    ObjectFunction(WString, copy, 0);

    ObjectFunction(WString, appendwc, 1);
    ObjectFunction(WString, appendc, 1);
    ObjectFunction(WString, appendws, 1);
    ObjectFunction(WString, appends, 1);
    ObjectFunction(WString, append, 1);
    ObjectFunction(WString, popback, 0);
    ObjectFunction(WString, popfront, 0);

    ObjectFunction(WString, equals, 1);
    ObjectFunction(WString, equalss, 1);
    ObjectFunction(WString, equalsws, 1);
    ObjectFunction(WString, equalsi, 1);
    ObjectFunction(WString, equalssi, 1);
    ObjectFunction(WString, equalswsi, 1);

    ObjectFunction(WString, equalsOneOff, 1);

    ObjectFunction(WString, startswith, 1);
    ObjectFunction(WString, startswiths, 1);
    ObjectFunction(WString, startswithws, 1);
    ObjectFunction(WString, startswithi, 1);
    ObjectFunction(WString, startswithsi, 1);
    ObjectFunction(WString, startswithwsi, 1);

    ObjectFunction(WString, endswith, 1);
    ObjectFunction(WString, endswiths, 1);
    ObjectFunction(WString, endswithws, 1);
    ObjectFunction(WString, endswithi, 1);
    ObjectFunction(WString, endswithsi, 1);
    ObjectFunction(WString, endswithwsi, 1);

    ObjectFunction(WString, contains, 1);
    ObjectFunction(WString, containss, 1);
    ObjectFunction(WString, containsws, 1);
    ObjectFunction(WString, containsi, 1);
    ObjectFunction(WString, containssi, 1);
    ObjectFunction(WString, containswsi, 1);

    Object_prepare((Object*)&self->super);
    return self;
}
