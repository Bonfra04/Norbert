#include "tokenizer.h"

#include "codepoint.h"
#include "named_entities.h"

#include "utils/wstring.h"
#include "utils/vector.h"
#include "utils/queue.h"

#include <stdlib.h>
#include <stdio.h>
#include <wctype.h>

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

    state_NONE,
} states_t;

static stream_t* stream;
static states_t state;
static states_t returnState;
static wstring_t temporaryBuffer;
static wchar_t characterReferenceCode;
static queue_t tokens_queue;
static token_t* currentToken;

void tokenizer_init(stream_t* _stream)
{
    stream = _stream;
    state = state_Data;
    returnState = state_NONE;
    temporaryBuffer = wstring_new();
    characterReferenceCode = L'\x0000';
    tokens_queue = queue_create();
    currentToken = NULL;
}

static void parser_error(const char* error)
{
    fprintf(stderr, "Parser error: %s\n", error);
}

#define matchState(state) case state_##state:

#define on_wchar(condition) (stream_current(stream) == (wchar_t)(uint64_t)(condition))
#define on_function(condition) (((bool(*)(wchar_t))(condition))(stream_current(stream)))
#define on_wstring(condition) (stream_match(stream, (wchar_t*)condition, true, false))

#define checker(condition) _Generic((condition), wchar_t: on_wchar(condition), wchar_t*: on_wstring(condition), default: on_function(condition))
#define on(condition) if(checker(condition))
#define anythingElse()

#define switchTo(target) do { state = state_##target; goto emit_token; } while(0)
#define switchToReturnState() do { state = returnState; goto emit_token; } while(0)
#define reconsumeIn(target) do { stream_reconsume(stream); switchTo(target); } while(0)
#define reconsumeInReturnState() do { stream_reconsume(stream); state = returnState; goto emit_token; } while(0)

#define Token(ttype, ...) ({token_t* _ = calloc(sizeof(token_t), 1); *_ = (token_t){.type=token_##ttype, __VA_ARGS__}; _; })

#define prepareToken(ttype, ...) do { currentToken = (token_t*)Token(ttype, __VA_ARGS__); } while(0)
#define finalizeToken() do { queue_push(&tokens_queue, (uint64_t)currentToken); currentToken = NULL; } while(0)
#define enqueueToken(ttype, ...) do { prepareToken(ttype, __VA_ARGS__); finalizeToken(); } while(0)
#define emitToken(ttype, ...) do { enqueueToken(ttype, __VA_ARGS__); goto emit_token; } while(0)

#define emitEOFToken() emitToken(eof);
#define emitCurrentCharacterToken() emitToken(character, .value = stream_current(stream))

#define prepareAttribute() do { token_attribute_t _ = { wstring_new(), wstring_new() }; vector_append(currentToken->attributes, &_); } while(0)
#define appendAttributeName(c) do { token_attribute_t* _ = (token_attribute_t*)currentToken->attributes; wstring_append(_[vector_length(_) - 1].name, c); } while(0)
#define appendAttributeValue(c) do { token_attribute_t* _ = (token_attribute_t*)currentToken->attributes; wstring_append(_[vector_length(_) - 1].value, c); } while(0)

#define consumedAsPartOfAnAttribute() (returnState == state_AttributeValueDoubleQuoted || returnState == state_AttributeValueSingleQuoted || returnState == state_AttributeValueUnquoted)

#define flushConsumedCharacterReference() do {          \
size_t len = wcslen(temporaryBuffer);                   \
for(size_t i = 0; i < len; i++)                         \
    if(consumedAsPartOfAnAttribute())                   \
        appendAttributeValue(temporaryBuffer[i]);       \
    else                                                \
        enqueueToken(character, temporaryBuffer[i]);    \
} while(0)

#define redo() goto emit_token

token_t* tokenizer_emit_token()
{
    emit_token:
    if(tokens_queue.size > 0)
        return (token_t*)queue_pop(&tokens_queue);

    switch (state)
    {
        matchState(Data)
        {
            stream_consume(stream);
            on(L'&')
            {
                returnState = state_Data;
                switchTo(CharacterReference);
            }
            on(L'<')
            {
                switchTo(TagOpen);
            }
            on(L'\0')
            {
                parser_error("null-character-in-input-stream");
                emitCurrentCharacterToken();
            }
            on(-1)
            {
                emitEOFToken();
            }
            anythingElse()
            {
                emitCurrentCharacterToken();
            }
        }
    
        matchState(CharacterReference)
        {
            wstring_clear(temporaryBuffer);
            wstring_append(temporaryBuffer, L'&');

            stream_consume(stream);
            on(AsciiAlphanumeric)
            {
                reconsumeIn(NamedCharacterReference);
            }
            on(L'#')
            {
                wstring_append(temporaryBuffer, stream_current(stream));
                switchTo(NumericCharacterReference);
            }
            anythingElse()
            {
                flushConsumedCharacterReference();
                reconsumeInReturnState();
            }
        }

        matchState(NamedCharacterReference)
        {
            bool something = false;
            while(true)
            {
                if(stream_consume(stream) == -1)
                    break;

                wstring_append(temporaryBuffer, stream_current(stream));

                size_t buffLen = wstring_length(temporaryBuffer);

                bool match = false;
                for(size_t i = 0; i < NAMED_ENTITIES_COUNT; i++)
                {
                    size_t len = wcslen(entities[i].name);

                    bool correspondence = buffLen <= len;
                    if(correspondence)
                        for(size_t j = 0; j < buffLen; j++)
                            if(temporaryBuffer[j] != entities[i].name[j])
                            {
                                correspondence = false;
                                break;
                            }
                    
                    if(correspondence)
                    {
                        match = true;
                        if(len == buffLen)
                        {
                            entities[i].flag = true;
                            something = true;
                        }
                    }
                }

                if(!match)
                {
                    stream_reconsume(stream);
                    wstring_popback(temporaryBuffer);
                    break;
                }
            };

            if(something)
            {
                int64_t i = NAMED_ENTITIES_COUNT - 1;
                for(; i >= 0; i--)
                    if(entities[i].flag)
                    {
                        size_t len = wcslen(entities[i].name);
                        
                        if(consumedAsPartOfAnAttribute())
                            if(entities[i].name[len - 1] != L';' && (stream_current(stream) == L'=' || isAsciiAlphanumeric(stream_current(stream))))
                            {
                                flushConsumedCharacterReference();
                                switchToReturnState();
                            }

                        if(entities[i].name[len - 1] != ';')
                            parser_error("missing-semicolon-after-character-reference");

                        wstring_clear(temporaryBuffer);
                        wchar_t* value = entities[i].value;
                        wstring_append(temporaryBuffer, value[0]);
                        if(value[1] != 0)
                            wstring_append(temporaryBuffer, value[1]);
                        break;
                    }
                for(; i >= 0; i--)
                    entities[i].flag = false;
            }

            flushConsumedCharacterReference();
            if(something)
                switchToReturnState();
            else
                switchTo(AmbiguousAmpersand);
        }

        matchState(AmbiguousAmpersand)
        {
            stream_consume(stream);
            on(AsciiAlphanumeric)
            {
                if(consumedAsPartOfAnAttribute())
                    appendAttributeValue(stream_current(stream));
                else
                    emitCurrentCharacterToken();
            }
            on(L';')
            {
                parser_error("unknown-named-character-reference");
                reconsumeInReturnState();
            }
            anythingElse()
            {
                reconsumeInReturnState();
            }
        }

        matchState(NumericCharacterReference)
        {
            characterReferenceCode = 0;

            stream_consume(stream);
            on(L'X')
            {
                wstring_append(temporaryBuffer, stream_current(stream));
                switchTo(HexadecimalCharacterReferenceStart);
            }
            on(L'x')
            {
                wstring_append(temporaryBuffer, stream_current(stream));
                switchTo(HexadecimalCharacterReferenceStart);
            }
            anythingElse()
            {
                reconsumeIn(DecimalCharacterReferenceStart);
            }
        }

        matchState(HexadecimalCharacterReferenceStart)
        {
            stream_consume(stream);
            on(AsciiHexDigit)
            {
                reconsumeIn(HexadecimalCharacterReference);
            }
            anythingElse()
            {
                parser_error("absence-of-digits-in-numeric-character-reference");
                flushConsumedCharacterReference();
                reconsumeInReturnState();
            }
        }

        matchState(DecimalCharacterReferenceStart)
        {
            stream_consume(stream);
            on(AsciiDigit)
            {
                reconsumeIn(DecimalCharacterReference);
            }
            anythingElse()
            {
                parser_error("absence-of-digits-in-numeric-character-reference");
                flushConsumedCharacterReference();
                reconsumeInReturnState();
            }
        }

        matchState(HexadecimalCharacterReference)
        {
            stream_consume(stream);
            on(AsciiDigit)
            {
                characterReferenceCode *= 16;
                characterReferenceCode += stream_current(stream) - L'0';
                redo();
            }
            on(AsciiUpperHexDigit)
            {
                characterReferenceCode *= 16;
                characterReferenceCode += stream_current(stream) - L'A' + 10;
                redo();
            }
            on(AsciiLowerHexDigit)
            {
                characterReferenceCode *= 16;
                characterReferenceCode += stream_current(stream) - L'a' + 10;
                redo();
            }
            on(L';')
            {
                switchTo(NumericCharacterReferenceEnd);
            }
            anythingElse()
            {
                parser_error("missing-semicolon-after-character-reference");
                reconsumeIn(NumericCharacterReferenceEnd);
            }
        }

        matchState(DecimalCharacterReference)
        {
            stream_consume(stream);
            on(AsciiDigit)
            {
                characterReferenceCode *= 10;
                characterReferenceCode += stream_current(stream) - L'0';
                redo();
            }
            on(L';')
            {
                switchTo(NumericCharacterReferenceEnd);
            }
            anythingElse()
            {
                parser_error("missing-semicolon-after-character-reference");
                reconsumeIn(NumericCharacterReferenceEnd);
            }
        }

        matchState(NumericCharacterReferenceEnd)
        {
            if(characterReferenceCode == 0)
            {
                parser_error("null-character-reference");
                characterReferenceCode = 0xFFFD;
            }
            else if(characterReferenceCode > 0x10FFFF)
            {
                parser_error("character-reference-outside-unicode-range");
                characterReferenceCode = 0xFFFD;
            }
            else if(isSurrogate(characterReferenceCode))
            {
                parser_error("surrogate-character-reference");
                characterReferenceCode = 0xFFFD;
            }
            else if(isNoncharacter(characterReferenceCode))
            {
                parser_error("noncharacter-character-reference");
            }
            else if(characterReferenceCode == 0x0D || (isControl(characterReferenceCode) && !isAsciiWhitespace(characterReferenceCode)))
            {
                parser_error("control-character-reference");
                for(size_t i = 0; i < CONTROL_CHARACTERS_COUNT; i++)
                    if(characterReferenceCode == control_characters[i].value)
                    {
                        characterReferenceCode = control_characters[i].replacement;
                        break;
                    }
            }

            wstring_clear(temporaryBuffer);
            wstring_append(temporaryBuffer, characterReferenceCode);
            flushConsumedCharacterReference();
            switchToReturnState();
        }

        matchState(TagOpen)
        {
            stream_consume(stream);
            on(L'!')
            {
                switchTo(MarkupDeclarationOpen);
            }
            on(L'/')
            {
                switchTo(EndTagOpen);
            }
            on(AsciiAlpha)
            {
                prepareToken(start_tag, .name = wstring_new(), .selfClosing = false, .attributes = vector_new(sizeof(token_attribute_t)));
                reconsumeIn(TagName);
            }
            on(L'?')
            {
                parser_error("unexpected-question-mark-instead-of-tag-name");
                prepareToken(comment, .data = wstring_new());
                reconsumeIn(BogusComment);
            }
            on(-1)
            {
                parser_error("eof-before-tag-name");
                enqueueToken(character, L'<');
                emitEOFToken();
            }
            anythingElse()
            {
                parser_error("invalid-first-character-of-tag-name");
                enqueueToken(character, L'<');
                reconsumeIn(Data);
            }
        }

        matchState(EndTagOpen)
        {
            stream_consume(stream);
            on(AsciiAlpha)
            {
                prepareToken(end_tag, .name = wstring_new(), .selfClosing = false, .attributes = vector_new(sizeof(token_attribute_t)));
                reconsumeIn(TagName);
            }
            on(L'>')
            {
                parser_error("missing-end-tag-name");
                switchTo(Data);
            }
            on(-1)
            {
                parser_error("eof-before-tag-name");
                enqueueToken(character, L'<');
                enqueueToken(character, L'/');
                emitEOFToken();
            }
            anythingElse()
            {
                parser_error("invalid-first-character-of-tag-name");
                prepareToken(comment, .data = wstring_new());
                reconsumeIn(BogusComment);
            }
        }

        matchState(TagName)
        {
            stream_consume(stream);
            on(AsciiWhitespace)
            {
                switchTo(BeforeAttributeName);
            }
            on(L'/')
            {
                switchTo(SelfClosingStartTag);
            }
            on(L'>')
            {
                finalizeToken();
                switchTo(Data);
            }
            on(AsciiUpperAlpha)
            {
                wstring_append(currentToken->name, towlower(stream_current(stream)));
                redo();
            }
            on(L'\0')
            {
                parser_error("unexpected-null-character");
                wstring_append(currentToken->name, L'\uFFFD');
                redo();
            }
            on(-1)
            {
                parser_error("eof-in-tag");
                emitEOFToken();
            }
            anythingElse()
            {
                wstring_append(currentToken->name, stream_current(stream));
                redo();
            }
        }

        matchState(SelfClosingStartTag)
        {
            stream_consume(stream);
            on(L'>')
            {
                currentToken->selfClosing = true;
                finalizeToken();
                switchTo(Data);
            }
            on(-1)
            {
                parser_error("eof-in-tag");
                emitEOFToken();
            }
            anythingElse()
            {
                parser_error("unexpected-solidus-in-tag");
                reconsumeIn(BeforeAttributeName);
            }
        }

        matchState(BeforeAttributeName)
        {
            stream_consume(stream);
            on(AsciiWhitespace)
            {
                redo();
            }
            on(L'/')
            {
                reconsumeIn(AfterAttributeName);
            }
            on(L'>')
            {
                reconsumeIn(AfterAttributeName);
            }
            on(-1)
            {
                reconsumeIn(AfterAttributeName);
            }
            on(L'=')
            {
                parser_error("unexpected-equals-sign-before-attribute-name");
                prepareAttribute();
                appendAttributeName(stream_current(stream));
                switchTo(AttributeName);
            }
            anythingElse()
            {
                prepareAttribute();
                reconsumeIn(AttributeName);
            }
        }

        matchState(AttributeName)
        {
            stream_consume(stream);
            on(AsciiWhitespace)
            {
                reconsumeIn(AfterAttributeName);
            }
            on(L'>')
            {
                reconsumeIn(AfterAttributeName);
            }
            on(-1)
            {
                reconsumeIn(AfterAttributeName);
            }
            on(L'=')
            {
                switchTo(BeforeAttributeValue);
            }
            on(AsciiUpperAlpha)
            {
                appendAttributeName(towlower(stream_current(stream)));
                redo();
            }
            on('\0')
            {
                parser_error("unexpected-null-character");
                appendAttributeName(L'\uFFFD');
                redo();
            }
            on(L'"')
            {
                parser_error("unexpected-character-in-attribute-name");
                appendAttributeName(stream_current(stream));
                redo();
            }
            on(L'\'')
            {
                parser_error("unexpected-character-in-attribute-name");
                appendAttributeName(stream_current(stream));
                redo();
            }
            on(L'<')
            {
                parser_error("unexpected-character-in-attribute-name");
                appendAttributeName(stream_current(stream));
                redo();
            }
            anythingElse()
            {
                appendAttributeName(stream_current(stream));
                redo();
            }
        }

        matchState(AfterAttributeName)
        {
            stream_consume(stream);
            on(AsciiWhitespace)
            {
                redo();
            }
            on(L'/')
            {
                switchTo(SelfClosingStartTag);
            }
            on(L'=')
            {
                switchTo(BeforeAttributeValue);
            }
            on(L'>')
            {
                finalizeToken();
                switchTo(Data);
            }
            on(-1)
            {
                parser_error("eof-in-tag");
                emitEOFToken();
            }
            anythingElse()
            {
                prepareAttribute();
                reconsumeIn(AttributeName);
            }
        }

        matchState(BeforeAttributeValue)
        {
            stream_consume(stream);
            on(AsciiWhitespace)
            {
                redo();
            }
            on(L'"')
            {
                switchTo(AttributeValueDoubleQuoted);
            }
            on(L'\'')
            {
                switchTo(AttributeValueSingleQuoted);
            }
            on(L'>')
            {
                parser_error("missing-attribute-value");
                finalizeToken();
                switchTo(Data);
            }
            anythingElse()
            {
                reconsumeIn(AttributeValueUnquoted);
            }
        }

        matchState(AttributeValueDoubleQuoted)
        {
            stream_consume(stream);
            on(L'"')
            {
                switchTo(AfterAttributeValueQuoted);
            }
            on(L'&')
            {
                returnState = state_AttributeValueDoubleQuoted;
                switchTo(CharacterReference);
            }
            on(L'\0')
            {
                parser_error("unexpected-null-character");
                appendAttributeValue(L'\uFFFD');
                redo();
            }
            on(-1)
            {
                parser_error("eof-in-tag");
                emitEOFToken();
            }
            anythingElse()
            {
                appendAttributeValue(stream_current(stream));
                redo();
            }
        }

        matchState(AttributeValueSingleQuoted)
        {
            stream_consume(stream);
            on(L'\'')
            {
                switchTo(AfterAttributeValueQuoted);
            }
            on(L'&')
            {
                returnState = state_AttributeValueSingleQuoted;
                switchTo(CharacterReference);
            }
            on(L'\0')
            {
                parser_error("unexpected-null-character");
                appendAttributeValue(L'\uFFFD');
                redo();
            }
            on(-1)
            {
                parser_error("eof-in-tag");
                emitEOFToken();
            }
            anythingElse()
            {
                appendAttributeValue(stream_current(stream));
                redo();
            }
        }

        matchState(AttributeValueUnquoted)
        {
            stream_consume(stream);
            on(AsciiWhitespace)
            {
                switchTo(BeforeAttributeName);
            }
            on(L'&')
            {
                returnState = state_AttributeValueUnquoted;
                switchTo(CharacterReference);
            }
            on(L'>')
            {
                finalizeToken();
                switchTo(Data);
            }
            on(L'\0')
            {
                parser_error("unexpected-null-character");
                appendAttributeValue(L'\uFFFD');
                redo();
            }
            on(L'"')
            {
                parser_error("unexpected-character-in-unquoted-attribute-value");
                appendAttributeValue(stream_current(stream));
                redo();
            }
            on(L'\'')
            {
                parser_error("unexpected-character-in-unquoted-attribute-value");
                appendAttributeValue(stream_current(stream));
                redo();
            }
            on(L'<')
            {
                parser_error("unexpected-character-in-unquoted-attribute-value");
                appendAttributeValue(stream_current(stream));
                redo();
            }
            on(L'=')
            {
                parser_error("unexpected-character-in-unquoted-attribute-value");
                appendAttributeValue(stream_current(stream));
                redo();
            }
            on(L'`')
            {
                parser_error("unexpected-character-in-unquoted-attribute-value");
                appendAttributeValue(stream_current(stream));
                redo();
            }
            on(-1)
            {
                parser_error("eof-in-tag");
                emitEOFToken();
            }
            anythingElse()
            {
                appendAttributeValue(stream_current(stream));
                redo();
            }
        }

        matchState(AfterAttributeValueQuoted)
        {
            stream_consume(stream);
            on(AsciiWhitespace)
            {
                switchTo(BeforeAttributeName);
            }
            on(L'/')
            {
                switchTo(SelfClosingStartTag);
            }
            on(L'>')
            {
                finalizeToken();
                switchTo(Data);
            }
            on(-1)
            {
                parser_error("eof-in-tag");
                emitEOFToken();
            }
            anythingElse()
            {
                parser_error("missing-whitespace-between-attributes");
                reconsumeIn(BeforeAttributeName);
            }
        }

        matchState(MarkupDeclarationOpen)
        {
            on(L"--")
            {
                prepareToken(comment, .data = wstring_new());
                switchTo(CommentStart);
            }
            on(L"DOCTYPE")
            {
                switchTo(DOCTYPE);
            }
            // TODO: L"[CDATA["
            anythingElse()
            {
                parser_error("incorrectly-opened-comment");
                prepareToken(comment, .data = wstring_new());
                switchTo(BogusComment);
            }
        }

        matchState(CommentStart)
        {
            stream_consume(stream);
            on(L'-')
            {
                switchTo(CommentStartDash);
            }
            on(L'>')
            {
                parser_error("abrupt-closing-of-empty-comment");
                finalizeToken();
                switchTo(Data);
            }
            anythingElse()
            {
                reconsumeIn(Comment);
            }
        }

        matchState(CommentStartDash)
        {
            stream_consume(stream);
            on(L'-')
            {
                switchTo(CommentEnd);
            }
            on(L'>')
            {
                parser_error("abrupt-closing-of-empty-comment");
                finalizeToken();
                switchTo(Data);
            }
            on(-1)
            {
                parser_error("eof-in-comment");
                finalizeToken();
                emitEOFToken();
            }
            anythingElse()
            {
                wstring_append(currentToken->data, L'-');
                reconsumeIn(Comment);
            }
        }

        matchState(Comment)
        {
            stream_consume(stream);
            on(L'<')
            {
                wstring_append(currentToken->data, stream_current(stream));
                switchTo(CommentLessThanSign);
            }
            on(L'-')
            {
                switchTo(CommentEndDash);
            }
            on(L'\0')
            {
                parser_error("unexpected-null-character");
                wstring_append(currentToken->data, L'\uFFFD');
                redo();
            }
            on(-1)
            {
                parser_error("eof-in-comment");
                finalizeToken();
                emitEOFToken();
            }
            anythingElse()
            {
                wstring_append(currentToken->data, stream_current(stream));
                redo();
            }
        }

        matchState(CommentLessThanSign)
        {
            stream_consume(stream);
            on(L'!')
            {
                wstring_append(currentToken->data, stream_current(stream));
                switchTo(CommentLessThanSignBang);
            }
            on('<')
            {
                wstring_append(currentToken->data, stream_current(stream));
                redo();
            }
            anythingElse()
            {
                reconsumeIn(Comment);
            }
        }

        matchState(CommentLessThanSignBang)
        {
            stream_consume(stream);
            on(L'-')
            {
                switchTo(CommentLessThanSignBangDash);
            }
            anythingElse()
            {
                reconsumeIn(Comment);
            }
        }

        matchState(CommentLessThanSignBangDash)
        {
            stream_consume(stream);
            on(L'-')
            {
                switchTo(CommentLessThanSignBangDashDash);
            }
            anythingElse()
            {
                reconsumeIn(CommentEndDash);
            }
        }

        matchState(CommentLessThanSignBangDashDash)
        {
            stream_consume(stream);
            on(L'>')
            {
                reconsumeIn(CommentEnd);
            }
            on(-1)
            {
                reconsumeIn(CommentEnd);
            }
            anythingElse()
            {
                parser_error("nested-comment");
                reconsumeIn(CommentEnd);
            }
        }

        matchState(CommentEndDash)
        {
            stream_consume(stream);
            on(L'-')
            {
                switchTo(CommentEnd);
            }
            on(-1)
            {
                parser_error("eof-in-comment");
                finalizeToken();
                emitEOFToken();
            }
            anythingElse()
            {
                wstring_append(currentToken->data, L'-');
                reconsumeIn(Comment);
            }
        }

        matchState(CommentEnd)
        {
            stream_consume(stream);
            on(L'>')
            {
                finalizeToken();
                switchTo(Data);
            }
            on(L'!')
            {
                switchTo(CommentEndBang);
            }
            on(L'-')
            {
                wstring_append(currentToken->data, L'-');
                redo();
            }
            on(-1)
            {
                parser_error("eof-in-comment");
                finalizeToken();
                emitEOFToken();
            }
            anythingElse()
            {
                wstring_append(currentToken->data, L'-');
                reconsumeIn(Comment);
            }
        }

        matchState(CommentEndBang)
        {
            stream_consume(stream);
            on(L'-')
            {
                wstring_append(currentToken->data, L'-');
                wstring_append(currentToken->data, L'-');
                wstring_append(currentToken->data, L'!');
                switchTo(CommentEndDash);
            }
            on(L'>')
            {
                parser_error("incorrectly-closed-comment");
                finalizeToken();
                switchTo(Data);
            }
            on(-1)
            {
                parser_error("eof-in-comment");
                finalizeToken();
                emitEOFToken();
            }
            anythingElse()
            {
                wstring_append(currentToken->data, L'-');
                wstring_append(currentToken->data, L'-');
                wstring_append(currentToken->data, L'!');
                reconsumeIn(Comment);
            }
        }

        matchState(BogusComment)
        {
            stream_consume(stream);
            on(L'>')
            {
                finalizeToken();
                switchTo(Data);
            }
            on(-1)
            {
                finalizeToken();
                emitEOFToken();
            }
            on(L'\0')
            {
                parser_error("unexpected-null-character");
                wstring_append(currentToken->data, L'\uFFFD');
                redo();
            }
            anythingElse()
            {
                wstring_append(currentToken->data, stream_current(stream));
                redo();
            }
        }

        matchState(DOCTYPE)
        {
            stream_consume(stream);
            on(AsciiWhitespace)
            {
                switchTo(BeforeDOCTYPEName);
            }
            on(L'>')
            {
                reconsumeIn(BeforeDOCTYPEName);
            }
            on(-1)
            {
                parser_error("eof-in-doctype");
                prepareToken(doctype, .forceQuirks = true);
                finalizeToken();
                emitEOFToken();
            }
            anythingElse()
            {
                parser_error("missing-whitespace-before-doctype-name");
                reconsumeIn(BeforeDOCTYPEName);
            }
        }

        matchState(BeforeDOCTYPEName)
        {
            stream_consume(stream);
            on(AsciiWhitespace)
            {
                redo();
            }
            on(AsciiUpperAlpha)
            {
                prepareToken(doctype, .name = wstring_new());
                wstring_append(currentToken->name, stream_current(stream) + 0x20);
                switchTo(DOCTYPEName);
            }
            on(L'\0')
            {
                parser_error("unexpected-null-character");
                prepareToken(doctype, .name = wstring_new());
                wstring_append(currentToken->name, L'\uFFFD');
                switchTo(DOCTYPEName);
            }
            on(L'>')
            {
                parser_error("missing-doctype-name");
                prepareToken(doctype, .forceQuirks = true);
                finalizeToken();
                switchTo(Data);
            }
            on(-1)
            {
                parser_error("eof-in-doctype");
                prepareToken(doctype, .forceQuirks = true);
                finalizeToken();
                emitEOFToken();
            }
            anythingElse()
            {
                prepareToken(doctype, .name = wstring_new());
                wstring_append(currentToken->name, stream_current(stream));
                switchTo(DOCTYPEName);
            }
        }

        matchState(DOCTYPEName)
        {
            stream_consume(stream);
            on(AsciiWhitespace)
            {
                switchTo(AfterDOCTYPEName);
            }
            on(L'>')
            {
                finalizeToken();
                switchTo(Data);
            }
            on(AsciiUpperAlpha)
            {
                wstring_append(currentToken->name, stream_current(stream) + 0x20);
                redo();
            }
            on(L'\0')
            {
                parser_error("unexpected-null-character");
                wstring_append(currentToken->name, L'\uFFFD');
                redo();
            }
            on(-1)
            {
                parser_error("eof-in-doctype");
                currentToken->forceQuirks = true;
                finalizeToken();
                emitEOFToken();
            }
            anythingElse()
            {
                wstring_append(currentToken->name, stream_current(stream));
                redo();
            }
        }

        matchState(AfterDOCTYPEName)
        {
            stream_consume(stream);
            on(AsciiWhitespace)
            {
                redo();
            }
            on(L'>')
            {
                finalizeToken();
                switchTo(Data);
            }
            on(-1)
            {
                parser_error("eof-in-doctype");
                currentToken->forceQuirks = true;
                finalizeToken();
                emitEOFToken();
            }
            anythingElse()
            {
                if(towlower(stream_current(stream)) == L's')
                {
                    on(L"YSTEM")
                    {
                        switchTo(AfterDOCTYPESystemKeyword);
                    }
                }
                else if(towlower(stream_current(stream)) == L'p')
                {
                    on(L"UBLIC")
                    {
                        switchTo(AfterDOCTYPEPublicKeyword);
                    }
                }
                else
                {
                    parser_error("expected-space-or-right-bracket-in-doctype");
                    currentToken->forceQuirks = true;
                    reconsumeIn(BogusDOCTYPE);
                }
            }
        }

        matchState(AfterDOCTYPEPublicKeyword)
        {
            stream_consume(stream);
            on(AsciiWhitespace)
            {
                switchTo(BeforeDOCTYPEPublicIdentifier);
            }
            on(L'"')
            {
                parser_error("missing-whitespace-after-doctype-public-keyword");
                currentToken->public_identifier = wstring_new();
                switchTo(DOCTYPEPublicIdentifierDoubleQuoted);
            }
            on(L'\'')
            {
                parser_error("missing-whitespace-after-doctype-public-keyword");
                currentToken->public_identifier = wstring_new();
                switchTo(DOCTYPEPublicIdentifierSingleQuoted);
            }
            on(L'>')
            {
                parser_error("missing-doctype-public-identifier");
                currentToken->forceQuirks = true;
                finalizeToken();
                switchTo(Data);
            }
            on(-1)
            {
                parser_error("eof-in-doctype");
                currentToken->forceQuirks = true;
                finalizeToken();
                emitEOFToken();
            }
            anythingElse()
            {
                parser_error("missing-quote-before-doctype-public-identifier");
                currentToken->forceQuirks = true;
                reconsumeIn(BogusDOCTYPE);
            }
        }

        matchState(BeforeDOCTYPEPublicIdentifier)
        {
            stream_consume(stream);
            on(AsciiWhitespace)
            {
                redo();
            }
            on(L'"')
            {
                currentToken->public_identifier = wstring_new();
                switchTo(DOCTYPEPublicIdentifierDoubleQuoted);
            }
            on(L'\'')
            {
                currentToken->public_identifier = wstring_new();
                switchTo(DOCTYPEPublicIdentifierSingleQuoted);
            }
            on(L'>')
            {
                parser_error("missing-doctype-public-identifier");
                currentToken->forceQuirks = true;
                finalizeToken();
                switchTo(Data);
            }
            on(-1)
            {
                parser_error("eof-in-doctype");
                currentToken->forceQuirks = true;
                finalizeToken();
                emitEOFToken();
            }
            anythingElse()
            {
                parser_error("missing-quote-before-doctype-public-identifier");
                currentToken->forceQuirks = true;
                reconsumeIn(BogusDOCTYPE);
            }
        }

        matchState(DOCTYPEPublicIdentifierDoubleQuoted)
        {
            stream_consume(stream);
            on(L'"')
            {
                switchTo(AfterDOCTYPEPublicIdentifier);
            }
            on(L'\0')
            {
                parser_error("unexpected-null-character");
                wstring_append(currentToken->public_identifier, L'\uFFFD');
                redo();
            }
            on(L'>')
            {
                parser_error("abrupt-doctype-public-identifier");
                currentToken->forceQuirks = true;
                finalizeToken();
                switchTo(Data);
            }
            on(-1)
            {
                parser_error("eof-in-doctype");
                currentToken->forceQuirks = true;
                finalizeToken();
                emitEOFToken();
            }
            anythingElse()
            {
                wstring_append(currentToken->public_identifier, stream_current(stream));
                redo();
            }
        }

        matchState(DOCTYPEPublicIdentifierSingleQuoted)
        {
            stream_consume(stream);
            on(L'\'')
            {
                switchTo(AfterDOCTYPEPublicIdentifier);
            }
            on(L'\0')
            {
                parser_error("unexpected-null-character");
                wstring_append(currentToken->public_identifier, L'\uFFFD');
                redo();
            }
            on(L'>')
            {
                parser_error("abrupt-doctype-public-identifier");
                currentToken->forceQuirks = true;
                finalizeToken();
                switchTo(Data);
            }
            on(-1)
            {
                parser_error("eof-in-doctype");
                currentToken->forceQuirks = true;
                finalizeToken();
                emitEOFToken();
            }
            anythingElse()
            {
                wstring_append(currentToken->public_identifier, stream_current(stream));
                redo();
            }
        }

        matchState(AfterDOCTYPEPublicIdentifier)
        {
            stream_consume(stream);
            on(AsciiWhitespace)
            {
                switchTo(BetweenDOCTYPEPublicAndSystemIdentifiers);
            }
            on(L'>')
            {
                finalizeToken();
                switchTo(Data);
            }
            on(L'"')
            {
                parser_error("missing-whitespace-between-doctype-public-and-system-identifiers");
                currentToken->system_identfier = wstring_new();
                switchTo(DOCTYPESystemIdentifierDoubleQuoted);
            }
            on(L'\'')
            {
                parser_error("missing-whitespace-between-doctype-public-and-system-identifiers");
                currentToken->system_identfier = wstring_new();
                switchTo(DOCTYPESystemIdentifierSingleQuoted);
            }
            on(-1)
            {
                parser_error("eof-in-doctype");
                currentToken->forceQuirks = true;
                finalizeToken();
                emitEOFToken();
            }
            anythingElse()
            {
                parser_error("missing-quote-before-doctype-system-identifier");
                currentToken->forceQuirks = true;
                reconsumeIn(BogusDOCTYPE);
            }
        }

        matchState(BetweenDOCTYPEPublicAndSystemIdentifiers)
        {
            stream_consume(stream);
            on(AsciiWhitespace)
            {
                redo();
            }
            on(L'>')
            {
                finalizeToken();
                switchTo(Data);
            }
            on(L'"')
            {
                currentToken->system_identfier = wstring_new();
                switchTo(DOCTYPESystemIdentifierDoubleQuoted);
            }
            on(L'\'')
            {
                currentToken->system_identfier = wstring_new();
                switchTo(DOCTYPESystemIdentifierSingleQuoted);
            }
            on(-1)
            {
                parser_error("eof-in-doctype");
                currentToken->forceQuirks = true;
                finalizeToken();
                emitEOFToken();
            }
            anythingElse()
            {
                parser_error("missing-quote-before-doctype-system-identifier");
                currentToken->forceQuirks = true;
                reconsumeIn(BogusDOCTYPE);
            }
        }

        matchState(AfterDOCTYPESystemKeyword)
        {
            stream_consume(stream);
            on(AsciiWhitespace)
            {
                switchTo(BeforeDOCTYPESystemIdentifier);
            }
            on(L'"')
            {
                parser_error("missing-whitespace-after-doctype-system-keyword");
                currentToken->system_identfier = wstring_new();
                switchTo(DOCTYPESystemIdentifierDoubleQuoted);
            }
            on(L'\'')
            {
                parser_error("missing-whitespace-after-doctype-system-keyword");
                currentToken->system_identfier = wstring_new();
                switchTo(DOCTYPESystemIdentifierSingleQuoted);
            }
            on(L'>')
            {
                parser_error("missing-doctype-system-identifier");
                currentToken->forceQuirks = true;
                finalizeToken();
                switchTo(Data);
            }
            on(-1)
            {
                parser_error("eof-in-doctype");
                currentToken->forceQuirks = true;
                finalizeToken();
                emitEOFToken();
            }
            anythingElse()
            {
                parser_error("missing-quote-before-doctype-system-identifier");
                currentToken->forceQuirks = true;
                reconsumeIn(BogusDOCTYPE);
            }
        }

        matchState(BeforeDOCTYPESystemIdentifier)
        {
            stream_consume(stream);
            on(AsciiWhitespace)
            {
                redo();
            }
            on(L'"')
            {
                currentToken->system_identfier = wstring_new();
                switchTo(DOCTYPESystemIdentifierDoubleQuoted);
            }
            on(L'\'')
            {
                currentToken->system_identfier = wstring_new();
                switchTo(DOCTYPESystemIdentifierSingleQuoted);
            }
            on(L'>')
            {
                parser_error("missing-doctype-system-identifier");
                currentToken->forceQuirks = true;
                finalizeToken();
                switchTo(Data);
            }
            on(-1)
            {
                parser_error("eof-in-doctype");
                currentToken->forceQuirks = true;
                finalizeToken();
                emitEOFToken();
            }
            anythingElse()
            {
                parser_error("missing-quote-before-doctype-system-identifier");
                currentToken->forceQuirks = true;
                reconsumeIn(BogusDOCTYPE);
            }
        }

        matchState(DOCTYPESystemIdentifierDoubleQuoted)
        {
            stream_consume(stream);
            on(L'"')
            {
                switchTo(AfterDOCTYPESystemIdentifier);
            }
            on(L'\0')
            {
                parser_error("unexpected-null-character");
                wstring_append(currentToken->system_identfier, L'\uFFFD');
                redo();
            }
            on(L'>')
            {
                parser_error("abrupt-doctype-system-identifier");
                currentToken->forceQuirks = true;
                finalizeToken();
                switchTo(Data);
            }
            on(-1)
            {
                parser_error("eof-in-doctype");
                currentToken->forceQuirks = true;
                finalizeToken();
                emitEOFToken();
            }
            anythingElse()
            {
                wstring_append(currentToken->system_identfier, stream_current(stream));
                redo();
            }
        }

        matchState(DOCTYPESystemIdentifierSingleQuoted)
        {
            stream_consume(stream);
            on(L'\'')
            {
                switchTo(AfterDOCTYPESystemIdentifier);
            }
            on(L'\0')
            {
                parser_error("unexpected-null-character");
                wstring_append(currentToken->system_identfier, L'\uFFFD');
                redo();
            }
            on(L'>')
            {
                parser_error("abrupt-doctype-system-identifier");
                currentToken->forceQuirks = true;
                finalizeToken();
                switchTo(Data);
            }
            on(-1)
            {
                parser_error("eof-in-doctype");
                currentToken->forceQuirks = true;
                finalizeToken();
                emitEOFToken();
            }
            anythingElse()
            {
                wstring_append(currentToken->system_identfier, stream_current(stream));
                redo();
            }
        }

        matchState(AfterDOCTYPESystemIdentifier)
        {
            stream_consume(stream);
            on(AsciiWhitespace)
            {
                redo();
            }
            on(L'>')
            {
                finalizeToken();
                switchTo(Data);
            }
            on(-1)
            {
                parser_error("eof-in-doctype");
                currentToken->forceQuirks = true;
                finalizeToken();
                emitEOFToken();
            }
            anythingElse()
            {
                parser_error("unexpected-char-after-doctype-system-identifier");
                reconsumeIn(BogusDOCTYPE);
            }
        }

        matchState(BogusDOCTYPE)
        {
            stream_consume(stream);
            on(L'>')
            {
                finalizeToken();
                switchTo(Data);
            }
            on(L'\0')
            {
                parser_error("unexpected-null-character");
                redo();
            }
            on(-1)
            {
                finalizeToken();
                emitEOFToken();
            }
            anythingElse()
            {
                redo();
            }
        }

    default:
        printf("Unknown state %d\n", state);
        exit(-1);
    }
}

void free_token(token_t* token)
{
    if(!token)
        return;

    if (token->name) wstring_free(token->name);
    if (token->attributes) vector_free(token->attributes);
    if (token->system_identfier) wstring_free(token->system_identfier);
    if (token->public_identifier) wstring_free(token->public_identifier);
    free(token);
}
