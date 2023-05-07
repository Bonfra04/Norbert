#pragma once

#include <wchar.h>

typedef wchar_t* wstring_t;

wstring_t wstring_new();
wstring_t wstring_copy(const wstring_t str);
size_t wstring_length(const wstring_t str);
void wstring_free(wstring_t str);
void wstring_append(wstring_t str, wchar_t append);
void wstring_appends(wstring_t str, wchar_t* append);
void wstring_clear(wstring_t str);
void wstring_popback(wstring_t str);
void wstring_popfront(wstring_t str);
