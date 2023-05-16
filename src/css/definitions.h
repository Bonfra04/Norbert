#pragma once

#include <wchar.h>
#include <stdbool.h>

typedef wchar_t CodePoint;

static inline bool css_codepoint_isNewline(CodePoint c) { return c == 0x0A; }
#define isNewline css_codepoint_isNewline
#define Newline css_codepoint_isNewline

static inline bool css_codepoint_isWhitespace(CodePoint c) { return isNewline(c) || c == 0x09 || c == 0x20; }
#define isWhitespace css_codepoint_isWhitespace
#define Whitespace css_codepoint_isWhitespace

static inline bool css_codepoint_isDigit(CodePoint c) { return c >= L'0' && c <= L'9'; }
#define isDigit css_codepoint_isDigit
#define Digit css_codepoint_isDigit

static inline bool css_codepoint_isHexDigit(CodePoint c) { return isDigit(c) || (c >= L'A' && c <= L'F') || (c >= L'a' && c <= L'f'); }
#define isHexDigit css_codepoint_isHexDigit
#define HexDigit css_codepoint_isHexDigit

static inline bool css_codepoint_isLeadingSurrogate(CodePoint c) { return c >= 0xD800 && c <= 0xDBFF; }
#define isLeadingSurrogate css_codepoint_isLeadingSurrogate
#define LeadingSurrogate css_codepoint_isLeadingSurrogate

static inline bool css_codepoint_isTrailingSurrogate(CodePoint c) { return c >= 0xDC00 && c <= 0xDFFF; }
#define isTrailingSurrogate css_codepoint_isTrailingSurrogate
#define TrailingSurrogate css_codepoint_isTrailingSurrogate

static inline bool css_codepoint_isSurrogate(CodePoint c) { return isLeadingSurrogate(c) || isTrailingSurrogate(c); }
#define isSurrogate css_codepoint_isSurrogate
#define Surrogate css_codepoint_isSurrogate

static inline bool css_codepoint_isUppercaseLetter(CodePoint c) { return c >= L'A' && c <= L'Z'; }
#define isUppercaseLetter css_codepoint_isUppercaseLetter
#define UppercaseLetter css_codepoint_isUppercaseLetter

static inline bool css_codepoint_isLowercaseLetter(CodePoint c) { return c >= L'a' && c <= L'z'; }
#define isLowercaseLetter css_codepoint_isLowercaseLetter
#define LowercaseLetter css_codepoint_isLowercaseLetter

static inline bool css_codepoint_isLetter(CodePoint c) { return isUppercaseLetter(c) || isLowercaseLetter(c); }
#define isLetter css_codepoint_isLetter
#define Letter css_codepoint_isLetter

static inline bool css_codepoint_isNonASCIICodePoint(CodePoint c) { return c >= 0x80; }
#define isNonASCIICodePoint css_codepoint_isNonASCIICodePoint
#define NonASCIICodePoint css_codepoint_isNonASCIICodePoint

static inline bool css_codepoint_isIdentStart(CodePoint c) { return isLetter(c) || isNonASCIICodePoint(c) || c == L'_'; }
#define isIdentStart css_codepoint_isIdentStart
#define IdentStart css_codepoint_isIdentStart

static inline bool css_codepoint_isIdent(CodePoint c) { return isIdentStart(c) || isDigit(c) || c == L'-'; }
#define isIdent css_codepoint_isIdent
#define Ident css_codepoint_isIdent

static inline bool css_codepoint_isNonPrintable(CodePoint c) { return (c >= 0x00 && c <= 0x08) || c == 0x0B || (c >= 0x0E && c <= 0x1F) || c == 0x7F; }
#define isNonPrintable css_codepoint_isNonPrintable
#define NonPrintable css_codepoint_isNonPrintable
