#pragma once

#include <stdbool.h>
#include <wchar.h>

static inline bool codepoint_isControl(wchar_t c) { return c >= 0x7F && c <= 0x9F; }
#define Control codepoint_isControl
#define isControl codepoint_isControl
static inline bool codepoint_isAsciiWhitespace(wchar_t c) { return c == 0x09 || c == 0x0A || c == 0x0C || c == 0x0D || c == 0x20; }
#define AsciiWhitespace codepoint_isAsciiWhitespace
#define isAsciiWhitespace codepoint_isAsciiWhitespace
static inline bool codepoint_isNoncharacter(wchar_t c) { return (c >= 0xFDD0 && c <= 0xFDEF) || c == 0xFFFE || c == 0xFFFF || c == 0x1FFFE || c == 0x1FFFF || c == 0x2FFFE || c == 0x2FFFF || c == 0x3FFFE || c == 0x3FFFF || c == 0x4FFFE || c == 0x4FFFF || c == 0x5FFFE || c == 0x5FFFF || c == 0x6FFFE || c == 0x6FFFF || c == 0x7FFFE || c == 0x7FFFF || c == 0x8FFFE || c == 0x8FFFF || c == 0x9FFFE || c == 0x9FFFF || c == 0xAFFFE || c == 0xAFFFF || c == 0xBFFFE || c == 0xBFFFF || c == 0xCFFFE || c == 0xCFFFF || c == 0xDFFFE || c == 0xDFFFF || c == 0xEFFFE || c == 0xEFFFF || c == 0xFFFFE || c == 0xFFFFF || c == 0x10FFFE || c == 0x10FFFF; }
#define NoneCharacter codepoint_isNoncharacter
#define isNoncharacter codepoint_isNoncharacter
static inline bool codepoint_isSurrogate(wchar_t c) { return c >= 0xD800 && c <= 0xDFFF; }
#define Surrogate codepoint_isSurrogate
#define isSurrogate codepoint_isSurrogate
static inline bool codepoint_isAsciiDigit(wchar_t c) { return c >= 0x30 && c <= 0x39; }
#define AsciiDigit codepoint_isAsciiDigit
#define isAsciiDigit codepoint_isAsciiDigit
static inline bool codepoint_isAsciiLowerAlpha(wchar_t c) { return c >= 0x61 && c <= 0x7A; }
#define AsciiLowerAlpha codepoint_isAsciiLowerAlpha
#define isAsciiLowerAlpha codepoint_isAsciiLowerAlpha
static inline bool codepoint_isAsciiUpperAlpha(wchar_t c) { return c >= 0x41 && c <= 0x5A; }
#define AsciiUpperAlpha codepoint_isAsciiUpperAlpha
#define isAsciiUpperAlpha codepoint_isAsciiUpperAlpha
static inline bool codepoint_isAsciiAlpha(wchar_t c) { return codepoint_isAsciiLowerAlpha(c) || codepoint_isAsciiUpperAlpha(c); }
#define AsciiAlpha codepoint_isAsciiAlpha
#define isAsciiAlpha codepoint_isAsciiAlpha
static inline bool codepoint_isAsciiAlphanumeric(wchar_t c) { return codepoint_isAsciiAlpha(c) || codepoint_isAsciiDigit(c); }
#define AsciiAlphanumeric codepoint_isAsciiAlphanumeric
#define isAsciiAlphanumeric codepoint_isAsciiAlphanumeric
static inline bool codepoint_isAsciiUpperHexDigit(wchar_t c) { return codepoint_isAsciiDigit(c) || (c >= 0x41 && c <= 0x46); }
#define AsciiUpperHexDigit codepoint_isAsciiUpperHexDigit
#define isAsciiUpperHexDigit codepoint_isAsciiUpperHexDigit
static inline bool codepoint_isAsciiLowerHexDigit(wchar_t c) { return codepoint_isAsciiDigit(c) || (c >= 0x61 && c <= 0x66); }
#define AsciiLowerHexDigit codepoint_isAsciiLowerHexDigit
#define isAsciiLowerHexDigit codepoint_isAsciiLowerHexDigit
static inline bool codepoint_isAsciiHexDigit(wchar_t c) { return codepoint_isAsciiUpperHexDigit(c) || codepoint_isAsciiLowerHexDigit(c); }
#define AsciiHexDigit codepoint_isAsciiHexDigit
#define isAsciiHexDigit codepoint_isAsciiHexDigit