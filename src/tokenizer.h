#pragma once

#include "stream/stream.h"

#include <wchar.h>
#include <stdbool.h>

typedef enum states
{
    state_Data,

    state_CharacterReference,
    state_NumericCharacterReference,
    state_NamedCharacterReference,
    state_AmbiguousAmpersand,
    state_HexadecimalCharacterReferenceStart,
    state_DecimalCharacterReferenceStart,
    state_HexadecimalCharacterReference,
    state_DecimalCharacterReference,
    state_NumericCharacterReferenceEnd,

    state_TagOpen,
    state_EndTagOpen,
    state_TagName,
    state_SelfClosingStartTag,

    state_BeforeAttributeName,
    state_AttributeName,
    state_AfterAttributeName,
    state_BeforeAttributeValue,
    state_AttributeValueDoubleQuoted,
    state_AttributeValueSingleQuoted,
    state_AttributeValueUnquoted,
    state_AfterAttributeValueQuoted,

    state_MarkupDeclarationOpen,
    
    state_CommentStart,
    state_BogusComment,
    state_CommentStartDash,
    state_Comment,
    state_CommentEnd,
    state_CommentLessThanSign,
    state_CommentLessThanSignBang,
    state_CommentLessThanSignBangDash,
    state_CommentLessThanSignBangDashDash,
    state_CommentEndDash,
    state_CommentEndBang,

    state_DOCTYPE,
    state_BeforeDOCTYPEName,
    state_DOCTYPEName,
    state_AfterDOCTYPEName,
    state_AfterDOCTYPEPublicKeyword,
    state_AfterDOCTYPESystemKeyword,
    state_BogusDOCTYPE,
    state_BeforeDOCTYPEPublicIdentifier,
    state_DOCTYPEPublicIdentifierDoubleQuoted,
    state_DOCTYPEPublicIdentifierSingleQuoted,
    state_AfterDOCTYPEPublicIdentifier,
    state_BetweenDOCTYPEPublicAndSystemIdentifiers,
    state_DOCTYPESystemIdentifierDoubleQuoted,
    state_DOCTYPESystemIdentifierSingleQuoted,
    state_BeforeDOCTYPESystemIdentifier,
    state_AfterDOCTYPESystemIdentifier,

    state_RCDATA,
    state_RCDATALessThanSign,
    state_RCDATAEndTagOpen,
    state_RCDATAEndTagName,

    state_RAWTEXT,
    state_RAWTEXTLessThanSign,
    state_RAWTEXTEndTagOpen,
    state_RAWTEXTEndTagName,

    state_NONE,
} states_t;

typedef enum token_type
{
    token_character,
    token_eof,
    token_start_tag,
    token_end_tag,
    token_comment,
    token_doctype
} token_type_t;

typedef struct token_attribute
{
    wchar_t* name;
    wchar_t* value;
} token_attribute_t;

typedef struct token
{
    token_type_t type;
    union
    {
        struct
        {
            wchar_t value;
        } character;
        struct
        {
            wchar_t* data;
        } comment;
        struct
        {
            wchar_t* name;
            bool selfClosing;
            bool ackSelfClosing;
            token_attribute_t* attributes;
        } tag, start_tag, end_tag;
        struct
        {
            wchar_t* name;
            bool forceQuirks;
            wchar_t* public_identifier;
            wchar_t* system_identifier;
        } doctype;
        struct
        {
        } eof;
    } as;
} token_t;

void tokenizer_init(stream_t* html);

void tokenizer_switch_to(states_t targetState);
token_t* tokenizer_emit_token();
void tokenizer_dispose_token(token_t* token);
