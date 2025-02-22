#pragma once

#include "infra/infra.hpp"

namespace Norbert::CSS::Definitions
{
    constexpr Infra::code_point EOF = 0;
    constexpr uint32_t maximumAllowedCodePoint = 0x10FFFF;

    inline bool isDigit(Infra::code_point cp) { return cp >= '0' && cp <= '9'; }
    #define digit '0' ... '9'
    inline bool isHexDigit(Infra::code_point cp) { return isDigit(cp) || (cp >= 'A' && cp <= 'F') || (cp >= 'a' && cp <= 'f'); }
    #define hexDigit digit: case 'A' ... 'F': case 'a' ... 'f'
    inline bool isUppercaseLetter(Infra::code_point cp) { return cp >= 'A' && cp <= 'Z'; }
    #define uppercaseLetter 'A' ... 'Z'
    inline bool isLowercaseLetter(Infra::code_point cp) { return cp >= 'a' && cp <= 'z'; }
    #define lowercaseLetter 'a' ... 'z'
    inline bool isLetter(Infra::code_point cp) { return isUppercaseLetter(cp) || isLowercaseLetter(cp); }
    #define letter uppercaseLetter: case lowercaseLetter
    inline bool isNonAsciiIdentCodePoint(Infra::code_point cp) { return cp == 0xB7 || (cp >= 0xC0 && cp <= 0xD6) || (cp >= 0xD8 && cp <= 0xF6) || (cp >= 0xF8 && cp <= 0x037D) || (cp >= 0x37F && cp <= 0x1FFF) || cp == 0x200C || cp == 0x200D || cp == 0x203F || cp == 0x2040 || (cp >= 0x2070 && cp <= 0x218F) || (cp >= 0x2C00 && cp <= 0x2FEF) || (cp >= 0x3001 && cp <= 0xD7FF) || (cp >= 0xF900 && cp <= 0xFDCF) || (cp >= 0xFDF0 && cp <= 0xFFFD) || cp >= 0x10000; }
    #define nonAsciiIdentCodePoint 0xB7: case 0xC0 ... 0xD6: case 0xD8 ... 0xF6: case 0xF8 ... 0x037D: case 0x37F ... 0x1FFF: case 0x200C: case 0x200D: case 0x203F: case 0x2040: case 0x2070 ... 0x218F: case 0x2C00 ... 0x2FEF: case 0x3001 ... 0xD7FF: case 0xF900 ... 0xFDCF: case 0xFDF0 ... 0xFFFD: case 0x10000 ... maximumAllowedCodePoint
    inline bool isIdentStartCodePoint(Infra::code_point cp) { return isLetter(cp) || isNonAsciiIdentCodePoint(cp) || cp == '_'; }
    #define identStartCodePoint letter: case nonAsciiIdentCodePoint: case '_'
    inline bool isIdentCodePoint(Infra::code_point cp) { return isIdentStartCodePoint(cp) || isDigit(cp) || cp == '-'; }
    #define identCodePoint identStartCodePoint: case digit: case '-'
    inline bool isNonPrintableCodePoint(Infra::code_point cp) { return (cp >= 0x00 && cp <= 0x08) || cp == 0xB || (cp >= 0x0E && cp <= 0x1F) || cp == 0x7f; }
    #define nonPrintableCodePoint 0x00 ... 0x08: case 0x0B: case 0x0E ... 0x1F: case 0x7F
    #define nonPrintableCodePoint_exceptEOF 0x01 ... 0x08: case 0x0B: case 0x0E ... 0x1F: case 0x7F
    inline bool isNewLine(Infra::code_point cp) { return cp == '\n'; }
    #define newline '\n'
    inline bool isWhitespace(Infra::code_point cp) { return isNewLine(cp) || cp == '\t' || cp == ' '; }
    #define whitespace newline: case '\t': case ' '
    inline bool isWhitespace(Infra::string s) { for (Infra::code_point cp : s.asCodePoints()) if (!isWhitespace(cp)) return false; return true; }

}
