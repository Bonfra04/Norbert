#pragma once

#include <oop/object.h>

#include <utils/stream/wc_stream.h>
#include <utils/wstring.h>
#include <utils/queue.h>
#include <utils/vector.h>

#include <wchar.h>
#include <stdbool.h>

typedef enum HTMLTokenizer_states
{
    HTMLTokenizer_state_Data,

    HTMLTokenizer_state_CharacterReference,
    HTMLTokenizer_state_NumericCharacterReference,
    HTMLTokenizer_state_NamedCharacterReference,
    HTMLTokenizer_state_AmbiguousAmpersand,
    HTMLTokenizer_state_HexadecimalCharacterReferenceStart,
    HTMLTokenizer_state_DecimalCharacterReferenceStart,
    HTMLTokenizer_state_HexadecimalCharacterReference,
    HTMLTokenizer_state_DecimalCharacterReference,
    HTMLTokenizer_state_NumericCharacterReferenceEnd,

    HTMLTokenizer_state_TagOpen,
    HTMLTokenizer_state_EndTagOpen,
    HTMLTokenizer_state_TagName,
    HTMLTokenizer_state_SelfClosingStartTag,

    HTMLTokenizer_state_BeforeAttributeName,
    HTMLTokenizer_state_AttributeName,
    HTMLTokenizer_state_AfterAttributeName,
    HTMLTokenizer_state_BeforeAttributeValue,
    HTMLTokenizer_state_AttributeValueDoubleQuoted,
    HTMLTokenizer_state_AttributeValueSingleQuoted,
    HTMLTokenizer_state_AttributeValueUnquoted,
    HTMLTokenizer_state_AfterAttributeValueQuoted,

    HTMLTokenizer_state_MarkupDeclarationOpen,
    
    HTMLTokenizer_state_CommentStart,
    HTMLTokenizer_state_BogusComment,
    HTMLTokenizer_state_CommentStartDash,
    HTMLTokenizer_state_Comment,
    HTMLTokenizer_state_CommentEnd,
    HTMLTokenizer_state_CommentLessThanSign,
    HTMLTokenizer_state_CommentLessThanSignBang,
    HTMLTokenizer_state_CommentLessThanSignBangDash,
    HTMLTokenizer_state_CommentLessThanSignBangDashDash,
    HTMLTokenizer_state_CommentEndDash,
    HTMLTokenizer_state_CommentEndBang,

    HTMLTokenizer_state_DOCTYPE,
    HTMLTokenizer_state_BeforeDOCTYPEName,
    HTMLTokenizer_state_DOCTYPEName,
    HTMLTokenizer_state_AfterDOCTYPEName,
    HTMLTokenizer_state_AfterDOCTYPEPublicKeyword,
    HTMLTokenizer_state_AfterDOCTYPESystemKeyword,
    HTMLTokenizer_state_BogusDOCTYPE,
    HTMLTokenizer_state_BeforeDOCTYPEPublicIdentifier,
    HTMLTokenizer_state_DOCTYPEPublicIdentifierDoubleQuoted,
    HTMLTokenizer_state_DOCTYPEPublicIdentifierSingleQuoted,
    HTMLTokenizer_state_AfterDOCTYPEPublicIdentifier,
    HTMLTokenizer_state_BetweenDOCTYPEPublicAndSystemIdentifiers,
    HTMLTokenizer_state_DOCTYPESystemIdentifierDoubleQuoted,
    HTMLTokenizer_state_DOCTYPESystemIdentifierSingleQuoted,
    HTMLTokenizer_state_BeforeDOCTYPESystemIdentifier,
    HTMLTokenizer_state_AfterDOCTYPESystemIdentifier,

    HTMLTokenizer_state_RCDATA,
    HTMLTokenizer_state_RCDATALessThanSign,
    HTMLTokenizer_state_RCDATAEndTagOpen,
    HTMLTokenizer_state_RCDATAEndTagName,

    HTMLTokenizer_state_RAWTEXT,
    HTMLTokenizer_state_RAWTEXTLessThanSign,
    HTMLTokenizer_state_RAWTEXTEndTagOpen,
    HTMLTokenizer_state_RAWTEXTEndTagName,

    HTMLTokenizer_state_NONE,
} HTMLTokenizer_states;

typedef enum HTMLTokenType
{
    HTMLTokenType_character,
    HTMLTokenType_eof,
    HTMLTokenType_start_tag,
    HTMLTokenType_end_tag,
    HTMLTokenType_comment,
    HTMLTokenType_doctype
} HTMLTokenType;

typedef struct HTMLTokenAttribute
{
    WString* name;
    WString* value;
} HTMLTokenAttribute;

typedef struct HTMLToken
{
    HTMLTokenType type;
    union
    {
        struct
        {
            wchar_t value;
        } character;
        struct
        {
            WString* data;
        } comment;
        struct
        {
            WString* name;
            bool selfClosing;
            bool ackSelfClosing;
            Vector* attributes;
        } tag, start_tag, end_tag;
        struct
        {
            WString* name;
            bool forceQuirks;
            WString* public_identifier;
            WString* system_identifier;
        } doctype;
        struct
        {
        } eof;
    } as;
} HTMLToken;

typedef struct HTMLTokenizerRec
{
    ObjectExtends(Object);

    HTMLToken* (*EmitToken)();
    void (*DisposeToken)(HTMLToken* token);
    void (*SwitchTo)(HTMLTokenizer_states targetState);

    HTMLTokenizer_states state;
    HTMLTokenizer_states returnState;
    WString* temporaryBuffer;
    wchar_t characterReferenceCode;
    WCStream* stream;
    Queue* tokensQueue;
    HTMLToken* currentToken;
    WString* lastStartTagName;
} HTMLTokenizer;

HTMLTokenizer* HTMLTokenizer_new(WCStream* stream);
