#include "tokenizer.h"

#include "codepoint.h"
#include "named_entities.h"

#include "errors.h"
#include "utils/wstring.h"
#include "utils/vector.h"
#include "utils/queue.h"

#include <stdlib.h>
#include <wctype.h>

static stream_t* stream;
static states_t state;
static states_t returnState;
static wstring_t temporaryBuffer;
static wchar_t characterReferenceCode;
static queue_t tokens_queue;
static token_t* currentToken;
static wstring_t lastStartTagName;

void tokenizer_init(stream_t* _stream)
{
    stream = _stream;
    state = state_Data;
    returnState = state_NONE;
    temporaryBuffer = wstring_new();
    characterReferenceCode = L'\x0000';
    tokens_queue = queue_create();
    currentToken = NULL;
    lastStartTagName = wstring_new();
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
#define redo() goto emit_token

#define consumeInputCharacter() stream_consume(stream)
#define currentInputCharacter stream_current(stream)

#define temporaryBufferClear() wstring_clear(temporaryBuffer)
#define temporaryBufferAppend(c) wstring_append(temporaryBuffer, c)
#define temporaryBufferAppendCurrent() temporaryBufferAppend(currentInputCharacter)

#define Token(ttype, ...) ({token_t* _ = calloc(sizeof(token_t), 1); *_ = (token_t){.type=token_##ttype, .as = { .ttype =  {__VA_ARGS__}}}; _; })

#define prepareToken(ttype, ...) do { currentToken = (token_t*)Token(ttype, __VA_ARGS__); } while(0)
#define finalizeToken() do { queue_push(&tokens_queue, (uint64_t)currentToken); currentToken = NULL; } while(0)
#define enqueueToken(ttype, ...) do { prepareToken(ttype, __VA_ARGS__); finalizeToken(); } while(0)
#define emitToken(ttype, ...) do { enqueueToken(ttype, __VA_ARGS__); goto emit_token; } while(0)

#define emitEOFToken() emitToken(eof);
#define emitCurrentCharacterToken() emitToken(character, .value = stream_current(stream))

#define currentTagToken (currentToken->as.tag)
#define currentCommentToken (currentToken->as.comment)
#define currentDoctypeToken (currentToken->as.doctype)

#define prepareAttribute() do { token_attribute_t _ = { wstring_new(), wstring_new() }; vector_append(currentTagToken.attributes, &_); } while(0)
#define appendAttributeName(c) wstring_append(currentTagToken.attributes[vector_length(currentTagToken.attributes) - 1].name, c)
#define appendAttributeValue(c) wstring_append(currentTagToken.attributes[vector_length(currentTagToken.attributes) - 1].value, c)
#define appendTagName(c) wstring_append(currentTagToken.name, c)
#define appendCommentData(c) wstring_append(currentCommentToken.data, c)
#define appendDoctypeName(c) wstring_append(currentDoctypeToken.name, c)
#define appendDoctypePublicIdentifier(c) wstring_append(currentDoctypeToken.public_identifier, c)
#define appendDoctypeSystemIdentifier(c) wstring_append(currentDoctypeToken.system_identifier, c)

#define consumedAsPartOfAnAttribute() (returnState == state_AttributeValueDoubleQuoted || returnState == state_AttributeValueSingleQuoted || returnState == state_AttributeValueUnquoted)
#define flushConsumedCharacterReference() do {          \
size_t len = wcslen(temporaryBuffer);                   \
for(size_t i = 0; i < len; i++)                         \
    if(consumedAsPartOfAnAttribute())                   \
        appendAttributeValue(temporaryBuffer[i]);       \
    else                                                \
        enqueueToken(character, temporaryBuffer[i]);    \
} while(0)

#define emitTemporaryBufferAsCharacters() do {      \
size_t len = wcslen(temporaryBuffer);               \
for(size_t i = 0; i < len; i++)                     \
    enqueueToken(character, temporaryBuffer[i]);    \
} while(0)

#define isAppropriateEndTagToken(token) (wcscmp(token->as.tag.name, lastStartTagName) == 0)

void tokenizer_switch_to(states_t targetState)
{
    state = targetState;
}

token_t* tokenizer_emit_token()
{
    emit_token:
    if(tokens_queue.size > 0)
    {
        token_t* result = (token_t*)queue_pop(&tokens_queue);

        if(result->type == token_start_tag)
        {
            wstring_clear(lastStartTagName);
            wstring_appends(lastStartTagName, result->as.tag.name);
        }

        return result;
    }

    switch (state)
    {
        matchState(Data)
        {
            consumeInputCharacter();
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
            temporaryBufferClear();
            temporaryBufferAppend(L'&');

            consumeInputCharacter();
            on(AsciiAlphanumeric)
            {
                reconsumeIn(NamedCharacterReference);
            }
            on(L'#')
            {
                temporaryBufferAppendCurrent();
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
                if(consumeInputCharacter() == -1)
                    break;

                temporaryBufferAppendCurrent();

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
                            if(entities[i].name[len - 1] != L';' && (currentInputCharacter == L'=' || isAsciiAlphanumeric(currentInputCharacter)))
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
            consumeInputCharacter();
            on(AsciiAlphanumeric)
            {
                if(consumedAsPartOfAnAttribute())
                    appendAttributeValue(currentInputCharacter);
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

            consumeInputCharacter();
            on(L'X')
            {
                temporaryBufferAppendCurrent();
                switchTo(HexadecimalCharacterReferenceStart);
            }
            on(L'x')
            {
                temporaryBufferAppendCurrent();
                switchTo(HexadecimalCharacterReferenceStart);
            }
            anythingElse()
            {
                reconsumeIn(DecimalCharacterReferenceStart);
            }
        }

        matchState(HexadecimalCharacterReferenceStart)
        {
            consumeInputCharacter();
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
            consumeInputCharacter();
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
            consumeInputCharacter();
            on(AsciiDigit)
            {
                characterReferenceCode *= 16;
                characterReferenceCode += currentInputCharacter - L'0';
                redo();
            }
            on(AsciiUpperHexDigit)
            {
                characterReferenceCode *= 16;
                characterReferenceCode += currentInputCharacter - L'A' + 10;
                redo();
            }
            on(AsciiLowerHexDigit)
            {
                characterReferenceCode *= 16;
                characterReferenceCode += currentInputCharacter - L'a' + 10;
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
            consumeInputCharacter();
            on(AsciiDigit)
            {
                characterReferenceCode *= 10;
                characterReferenceCode += currentInputCharacter - L'0';
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

            temporaryBufferClear();
            temporaryBufferAppend(characterReferenceCode);
            flushConsumedCharacterReference();
            switchToReturnState();
        }

        matchState(TagOpen)
        {
            consumeInputCharacter();
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
                prepareToken(start_tag, .name = wstring_new(), .selfClosing = false, .ackSelfClosing = false, .attributes = vector_new(sizeof(token_attribute_t)));
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
            consumeInputCharacter();
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
            consumeInputCharacter();
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
                appendTagName(towlower(currentInputCharacter));
                redo();
            }
            on(L'\0')
            {
                parser_error("unexpected-null-character");
                appendTagName(L'\uFFFD');
                redo();
            }
            on(-1)
            {
                parser_error("eof-in-tag");
                emitEOFToken();
            }
            anythingElse()
            {
                appendTagName(currentInputCharacter);
                redo();
            }
        }

        matchState(SelfClosingStartTag)
        {
            consumeInputCharacter();
            on(L'>')
            {
                currentTagToken.selfClosing = true;
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
            consumeInputCharacter();
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
                appendAttributeName(currentInputCharacter);
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
            consumeInputCharacter();
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
                appendAttributeName(towlower(currentInputCharacter));
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
                appendAttributeName(currentInputCharacter);
                redo();
            }
            on(L'\'')
            {
                parser_error("unexpected-character-in-attribute-name");
                appendAttributeName(currentInputCharacter);
                redo();
            }
            on(L'<')
            {
                parser_error("unexpected-character-in-attribute-name");
                appendAttributeName(currentInputCharacter);
                redo();
            }
            anythingElse()
            {
                appendAttributeName(currentInputCharacter);
                redo();
            }
        }

        matchState(AfterAttributeName)
        {
            consumeInputCharacter();
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
            consumeInputCharacter();
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
            consumeInputCharacter();
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
                appendAttributeValue(currentInputCharacter);
                redo();
            }
        }

        matchState(AttributeValueSingleQuoted)
        {
            consumeInputCharacter();
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
                appendAttributeValue(currentInputCharacter);
                redo();
            }
        }

        matchState(AttributeValueUnquoted)
        {
            consumeInputCharacter();
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
                appendAttributeValue(currentInputCharacter);
                redo();
            }
            on(L'\'')
            {
                parser_error("unexpected-character-in-unquoted-attribute-value");
                appendAttributeValue(currentInputCharacter);
                redo();
            }
            on(L'<')
            {
                parser_error("unexpected-character-in-unquoted-attribute-value");
                appendAttributeValue(currentInputCharacter);
                redo();
            }
            on(L'=')
            {
                parser_error("unexpected-character-in-unquoted-attribute-value");
                appendAttributeValue(currentInputCharacter);
                redo();
            }
            on(L'`')
            {
                parser_error("unexpected-character-in-unquoted-attribute-value");
                appendAttributeValue(currentInputCharacter);
                redo();
            }
            on(-1)
            {
                parser_error("eof-in-tag");
                emitEOFToken();
            }
            anythingElse()
            {
                appendAttributeValue(currentInputCharacter);
                redo();
            }
        }

        matchState(AfterAttributeValueQuoted)
        {
            consumeInputCharacter();
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
            consumeInputCharacter();
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
            consumeInputCharacter();
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
                appendCommentData(L'-');
                reconsumeIn(Comment);
            }
        }

        matchState(Comment)
        {
            consumeInputCharacter();
            on(L'<')
            {
                appendCommentData(currentInputCharacter);
                switchTo(CommentLessThanSign);
            }
            on(L'-')
            {
                switchTo(CommentEndDash);
            }
            on(L'\0')
            {
                parser_error("unexpected-null-character");
                appendCommentData(L'\uFFFD');
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
                appendCommentData(currentInputCharacter);
                redo();
            }
        }

        matchState(CommentLessThanSign)
        {
            consumeInputCharacter();
            on(L'!')
            {
                appendCommentData(currentInputCharacter);
                switchTo(CommentLessThanSignBang);
            }
            on('<')
            {
                appendCommentData(currentInputCharacter);
                redo();
            }
            anythingElse()
            {
                reconsumeIn(Comment);
            }
        }

        matchState(CommentLessThanSignBang)
        {
            consumeInputCharacter();
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
            consumeInputCharacter();
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
            consumeInputCharacter();
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
            consumeInputCharacter();
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
                appendCommentData(L'-');
                reconsumeIn(Comment);
            }
        }

        matchState(CommentEnd)
        {
            consumeInputCharacter();
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
                appendCommentData(L'-');
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
                appendCommentData(L'-');
                reconsumeIn(Comment);
            }
        }

        matchState(CommentEndBang)
        {
            consumeInputCharacter();
            on(L'-')
            {
                appendCommentData(L'-');
                appendCommentData(L'-');
                appendCommentData(L'!');
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
                appendCommentData(L'-');
                appendCommentData(L'-');
                appendCommentData(L'!');
                reconsumeIn(Comment);
            }
        }

        matchState(BogusComment)
        {
            consumeInputCharacter();
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
                appendCommentData(L'\uFFFD');
                redo();
            }
            anythingElse()
            {
                appendCommentData(currentInputCharacter);
                redo();
            }
        }

        matchState(DOCTYPE)
        {
            consumeInputCharacter();
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
            consumeInputCharacter();
            on(AsciiWhitespace)
            {
                redo();
            }
            on(AsciiUpperAlpha)
            {
                prepareToken(doctype, .name = wstring_new());
                appendDoctypeName(towlower(currentInputCharacter));
                switchTo(DOCTYPEName);
            }
            on(L'\0')
            {
                parser_error("unexpected-null-character");
                prepareToken(doctype, .name = wstring_new());
                appendDoctypeName(L'\uFFFD');
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
                appendDoctypeName(currentInputCharacter);
                switchTo(DOCTYPEName);
            }
        }

        matchState(DOCTYPEName)
        {
            consumeInputCharacter();
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
                appendDoctypeName(towlower(currentInputCharacter));
                redo();
            }
            on(L'\0')
            {
                parser_error("unexpected-null-character");
                appendDoctypeName(L'\uFFFD');
                redo();
            }
            on(-1)
            {
                parser_error("eof-in-doctype");
                currentDoctypeToken.forceQuirks = true;
                finalizeToken();
                emitEOFToken();
            }
            anythingElse()
            {
                appendDoctypeName(currentInputCharacter);
                redo();
            }
        }

        matchState(AfterDOCTYPEName)
        {
            consumeInputCharacter();
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
                currentDoctypeToken.forceQuirks = true;
                finalizeToken();
                emitEOFToken();
            }
            anythingElse()
            {
                if(towlower(currentInputCharacter) == L's')
                {
                    on(L"YSTEM")
                    {
                        switchTo(AfterDOCTYPESystemKeyword);
                    }
                }
                else if(towlower(currentInputCharacter) == L'p')
                {
                    on(L"UBLIC")
                    {
                        switchTo(AfterDOCTYPEPublicKeyword);
                    }
                }
                else
                {
                    parser_error("expected-space-or-right-bracket-in-doctype");
                    currentDoctypeToken.forceQuirks = true;
                    reconsumeIn(BogusDOCTYPE);
                }
            }
        }

        matchState(AfterDOCTYPEPublicKeyword)
        {
            consumeInputCharacter();
            on(AsciiWhitespace)
            {
                switchTo(BeforeDOCTYPEPublicIdentifier);
            }
            on(L'"')
            {
                parser_error("missing-whitespace-after-doctype-public-keyword");
                currentDoctypeToken.public_identifier = wstring_new();
                switchTo(DOCTYPEPublicIdentifierDoubleQuoted);
            }
            on(L'\'')
            {
                parser_error("missing-whitespace-after-doctype-public-keyword");
                currentDoctypeToken.public_identifier = wstring_new();
                switchTo(DOCTYPEPublicIdentifierSingleQuoted);
            }
            on(L'>')
            {
                parser_error("missing-doctype-public-identifier");
                currentDoctypeToken.forceQuirks = true;
                finalizeToken();
                switchTo(Data);
            }
            on(-1)
            {
                parser_error("eof-in-doctype");
                currentDoctypeToken.forceQuirks = true;
                finalizeToken();
                emitEOFToken();
            }
            anythingElse()
            {
                parser_error("missing-quote-before-doctype-public-identifier");
                currentDoctypeToken.forceQuirks = true;
                reconsumeIn(BogusDOCTYPE);
            }
        }

        matchState(BeforeDOCTYPEPublicIdentifier)
        {
            consumeInputCharacter();
            on(AsciiWhitespace)
            {
                redo();
            }
            on(L'"')
            {
                currentDoctypeToken.public_identifier = wstring_new();
                switchTo(DOCTYPEPublicIdentifierDoubleQuoted);
            }
            on(L'\'')
            {
                currentDoctypeToken.public_identifier = wstring_new();
                switchTo(DOCTYPEPublicIdentifierSingleQuoted);
            }
            on(L'>')
            {
                parser_error("missing-doctype-public-identifier");
                currentDoctypeToken.forceQuirks = true;
                finalizeToken();
                switchTo(Data);
            }
            on(-1)
            {
                parser_error("eof-in-doctype");
                currentDoctypeToken.forceQuirks = true;
                finalizeToken();
                emitEOFToken();
            }
            anythingElse()
            {
                parser_error("missing-quote-before-doctype-public-identifier");
                currentDoctypeToken.forceQuirks = true;
                reconsumeIn(BogusDOCTYPE);
            }
        }

        matchState(DOCTYPEPublicIdentifierDoubleQuoted)
        {
            consumeInputCharacter();
            on(L'"')
            {
                switchTo(AfterDOCTYPEPublicIdentifier);
            }
            on(L'\0')
            {
                parser_error("unexpected-null-character");
                appendDoctypePublicIdentifier(L'\uFFFD');
                redo();
            }
            on(L'>')
            {
                parser_error("abrupt-doctype-public-identifier");
                currentDoctypeToken.forceQuirks = true;
                finalizeToken();
                switchTo(Data);
            }
            on(-1)
            {
                parser_error("eof-in-doctype");
                currentDoctypeToken.forceQuirks = true;
                finalizeToken();
                emitEOFToken();
            }
            anythingElse()
            {
                appendDoctypePublicIdentifier(currentInputCharacter);
                redo();
            }
        }

        matchState(DOCTYPEPublicIdentifierSingleQuoted)
        {
            consumeInputCharacter();
            on(L'\'')
            {
                switchTo(AfterDOCTYPEPublicIdentifier);
            }
            on(L'\0')
            {
                parser_error("unexpected-null-character");
                appendDoctypePublicIdentifier(L'\uFFFD');
                redo();
            }
            on(L'>')
            {
                parser_error("abrupt-doctype-public-identifier");
                currentDoctypeToken.forceQuirks = true;
                finalizeToken();
                switchTo(Data);
            }
            on(-1)
            {
                parser_error("eof-in-doctype");
                currentDoctypeToken.forceQuirks = true;
                finalizeToken();
                emitEOFToken();
            }
            anythingElse()
            {
                appendDoctypePublicIdentifier(currentInputCharacter);
                redo();
            }
        }

        matchState(AfterDOCTYPEPublicIdentifier)
        {
            consumeInputCharacter();
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
                currentDoctypeToken.system_identifier = wstring_new();
                switchTo(DOCTYPESystemIdentifierDoubleQuoted);
            }
            on(L'\'')
            {
                parser_error("missing-whitespace-between-doctype-public-and-system-identifiers");
                currentDoctypeToken.system_identifier = wstring_new();
                switchTo(DOCTYPESystemIdentifierSingleQuoted);
            }
            on(-1)
            {
                parser_error("eof-in-doctype");
                currentDoctypeToken.forceQuirks = true;
                finalizeToken();
                emitEOFToken();
            }
            anythingElse()
            {
                parser_error("missing-quote-before-doctype-system-identifier");
                currentDoctypeToken.forceQuirks = true;
                reconsumeIn(BogusDOCTYPE);
            }
        }

        matchState(BetweenDOCTYPEPublicAndSystemIdentifiers)
        {
            consumeInputCharacter();
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
                currentDoctypeToken.system_identifier = wstring_new();
                switchTo(DOCTYPESystemIdentifierDoubleQuoted);
            }
            on(L'\'')
            {
                currentDoctypeToken.system_identifier = wstring_new();
                switchTo(DOCTYPESystemIdentifierSingleQuoted);
            }
            on(-1)
            {
                parser_error("eof-in-doctype");
                currentDoctypeToken.forceQuirks = true;
                finalizeToken();
                emitEOFToken();
            }
            anythingElse()
            {
                parser_error("missing-quote-before-doctype-system-identifier");
                currentDoctypeToken.forceQuirks = true;
                reconsumeIn(BogusDOCTYPE);
            }
        }

        matchState(AfterDOCTYPESystemKeyword)
        {
            consumeInputCharacter();
            on(AsciiWhitespace)
            {
                switchTo(BeforeDOCTYPESystemIdentifier);
            }
            on(L'"')
            {
                parser_error("missing-whitespace-after-doctype-system-keyword");
                currentDoctypeToken.system_identifier = wstring_new();
                switchTo(DOCTYPESystemIdentifierDoubleQuoted);
            }
            on(L'\'')
            {
                parser_error("missing-whitespace-after-doctype-system-keyword");
                currentDoctypeToken.system_identifier = wstring_new();
                switchTo(DOCTYPESystemIdentifierSingleQuoted);
            }
            on(L'>')
            {
                parser_error("missing-doctype-system-identifier");
                currentDoctypeToken.forceQuirks = true;
                finalizeToken();
                switchTo(Data);
            }
            on(-1)
            {
                parser_error("eof-in-doctype");
                currentDoctypeToken.forceQuirks = true;
                finalizeToken();
                emitEOFToken();
            }
            anythingElse()
            {
                parser_error("missing-quote-before-doctype-system-identifier");
                currentDoctypeToken.forceQuirks = true;
                reconsumeIn(BogusDOCTYPE);
            }
        }

        matchState(BeforeDOCTYPESystemIdentifier)
        {
            consumeInputCharacter();
            on(AsciiWhitespace)
            {
                redo();
            }
            on(L'"')
            {
                currentDoctypeToken.system_identifier = wstring_new();
                switchTo(DOCTYPESystemIdentifierDoubleQuoted);
            }
            on(L'\'')
            {
                currentDoctypeToken.system_identifier = wstring_new();
                switchTo(DOCTYPESystemIdentifierSingleQuoted);
            }
            on(L'>')
            {
                parser_error("missing-doctype-system-identifier");
                currentDoctypeToken.forceQuirks = true;
                finalizeToken();
                switchTo(Data);
            }
            on(-1)
            {
                parser_error("eof-in-doctype");
                currentDoctypeToken.forceQuirks = true;
                finalizeToken();
                emitEOFToken();
            }
            anythingElse()
            {
                parser_error("missing-quote-before-doctype-system-identifier");
                currentDoctypeToken.forceQuirks = true;
                reconsumeIn(BogusDOCTYPE);
            }
        }

        matchState(DOCTYPESystemIdentifierDoubleQuoted)
        {
            consumeInputCharacter();
            on(L'"')
            {
                switchTo(AfterDOCTYPESystemIdentifier);
            }
            on(L'\0')
            {
                parser_error("unexpected-null-character");
                appendDoctypeSystemIdentifier(L'\uFFFD');
                redo();
            }
            on(L'>')
            {
                parser_error("abrupt-doctype-system-identifier");
                currentDoctypeToken.forceQuirks = true;
                finalizeToken();
                switchTo(Data);
            }
            on(-1)
            {
                parser_error("eof-in-doctype");
                currentDoctypeToken.forceQuirks = true;
                finalizeToken();
                emitEOFToken();
            }
            anythingElse()
            {
                appendDoctypeSystemIdentifier(currentInputCharacter);
                redo();
            }
        }

        matchState(DOCTYPESystemIdentifierSingleQuoted)
        {
            consumeInputCharacter();
            on(L'\'')
            {
                switchTo(AfterDOCTYPESystemIdentifier);
            }
            on(L'\0')
            {
                parser_error("unexpected-null-character");
                appendDoctypeSystemIdentifier(L'\uFFFD');
                redo();
            }
            on(L'>')
            {
                parser_error("abrupt-doctype-system-identifier");
                currentDoctypeToken.forceQuirks = true;
                finalizeToken();
                switchTo(Data);
            }
            on(-1)
            {
                parser_error("eof-in-doctype");
                currentDoctypeToken.forceQuirks = true;
                finalizeToken();
                emitEOFToken();
            }
            anythingElse()
            {
                appendDoctypeSystemIdentifier(currentInputCharacter);
                redo();
            }
        }

        matchState(AfterDOCTYPESystemIdentifier)
        {
            consumeInputCharacter();
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
                currentDoctypeToken.forceQuirks = true;
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
            consumeInputCharacter();
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

        matchState(RCDATA)
        {
            consumeInputCharacter();
            on(L'&')
            {
                returnState = state_RCDATA;
                switchTo(CharacterReference);
            }
            on(L'<')
            {
                switchTo(RCDATALessThanSign);
            }
            on(L'\0')
            {
                parser_error("unexpected-null-character");
                emitToken(character, L'\uFFFD');
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

        matchState(RCDATALessThanSign)
        {
            consumeInputCharacter();
            on(L'/')
            {
                temporaryBufferClear();
                switchTo(RCDATAEndTagOpen);
            }
            anythingElse()
            {
                enqueueToken(character, L'<');
                reconsumeIn(RCDATA);
            }
        }

        matchState(RCDATAEndTagOpen)
        {
            consumeInputCharacter();
            on(AsciiAlpha)
            {
                prepareToken(end_tag, .name = wstring_new(), .selfClosing = false, .attributes = vector_new(sizeof(token_attribute_t)));
                reconsumeIn(RCDATAEndTagName);
            }
            anythingElse()
            {
                enqueueToken(character, L'<');
                enqueueToken(character, L'/');
                reconsumeIn(RCDATA);
            }
        }

        matchState(RCDATAEndTagName)
        {
            consumeInputCharacter();
            on(AsciiWhitespace)
            {
                if(isAppropriateEndTagToken(currentToken))
                    switchTo(BeforeAttributeName);
                else
                {
                    enqueueToken(character, L'<');
                    enqueueToken(character, L'/');
                    emitTemporaryBufferAsCharacters();
                    reconsumeIn(RCDATA);
                }
            }
            on(L'/')
            {
                if(isAppropriateEndTagToken(currentToken))
                    switchTo(SelfClosingStartTag);
                else
                {
                    enqueueToken(character, L'<');
                    enqueueToken(character, L'/');
                    emitTemporaryBufferAsCharacters();
                    reconsumeIn(RCDATA);
                }
            }
            on(L'>')
            {
                if(isAppropriateEndTagToken(currentToken)) 
                {
                    finalizeToken();
                    switchTo(Data);
                }
                else
                {
                    enqueueToken(character, L'<');
                    enqueueToken(character, L'/');
                    emitTemporaryBufferAsCharacters();
                    reconsumeIn(RCDATA);
                }
            }
            on(AsciiUpperAlpha)
            {
                appendTagName(towlower(currentInputCharacter));
                temporaryBufferAppend(currentInputCharacter);
                redo();
            }
            on(AsciiLowerAlpha)
            {
                appendTagName(currentInputCharacter);
                temporaryBufferAppend(currentInputCharacter);
                redo();
            }
            anythingElse()
            {
                enqueueToken(character, L'<');
                enqueueToken(character, L'/');
                emitTemporaryBufferAsCharacters();
                reconsumeIn(RCDATA);
            }
        }

        matchState(RAWTEXT)
        {
            consumeInputCharacter();
            on(L'<')
            {
                switchTo(RAWTEXTLessThanSign);
            }
            on(L'\0')
            {
                parser_error("unexpected-null-character");
                emitToken(character, L'\uFFFD');
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

        matchState(RAWTEXTLessThanSign)
        {
            consumeInputCharacter();
            on(L'/')
            {
                temporaryBufferClear();
                switchTo(RAWTEXTEndTagOpen);
            }
            anythingElse()
            {
                enqueueToken(character, L'<');
                reconsumeIn(RAWTEXT);
            }
        }

        matchState(RAWTEXTEndTagOpen)
        {
            consumeInputCharacter();
            on(AsciiAlpha)
            {
                prepareToken(end_tag, .name = wstring_new(), .selfClosing = false, .attributes = vector_new(sizeof(token_attribute_t)));
                reconsumeIn(RAWTEXTEndTagName);
            }
            anythingElse()
            {
                enqueueToken(character, L'<');
                enqueueToken(character, L'/');
                reconsumeIn(RAWTEXT);
            }
        }

        matchState(RAWTEXTEndTagName)
        {
            on(AsciiWhitespace)
            {
                if(isAppropriateEndTagToken(currentToken))
                    switchTo(BeforeAttributeName);
                else
                {
                    enqueueToken(character, L'<');
                    enqueueToken(character, L'/');
                    emitTemporaryBufferAsCharacters();
                    reconsumeIn(RAWTEXT);
                }
            }
            on(L'/')
            {
                if(isAppropriateEndTagToken(currentToken))
                    switchTo(SelfClosingStartTag);
                else
                {
                    enqueueToken(character, L'<');
                    enqueueToken(character, L'/');
                    emitTemporaryBufferAsCharacters();
                    reconsumeIn(RAWTEXT);
                }
            }
            on(L'>')
            {
                if(isAppropriateEndTagToken(currentToken)) 
                {
                    finalizeToken();
                    switchTo(Data);
                }
                else
                {
                    enqueueToken(character, L'<');
                    enqueueToken(character, L'/');
                    emitTemporaryBufferAsCharacters();
                    reconsumeIn(RAWTEXT);
                }
            }
            on(AsciiUpperAlpha)
            {
                appendTagName(towlower(currentInputCharacter));
                temporaryBufferAppend(currentInputCharacter);
                redo();
            }
            on(AsciiLowerAlpha)
            {
                appendTagName(currentInputCharacter);
                temporaryBufferAppend(currentInputCharacter);
                redo();
            }
            anythingElse()
            {
                enqueueToken(character, L'<');
                enqueueToken(character, L'/');
                emitTemporaryBufferAsCharacters();
                reconsumeIn(RAWTEXT);
            }
        }

    default:
        exit(-1);
    }
}

void tokenizer_dispose_token(token_t* token)
{
    if(!token)
        return;

    if(token->type == token_start_tag && token->as.start_tag.selfClosing && token->as.start_tag.ackSelfClosing == false)
    {
        parser_error("non-void-html-element-start-tag-with-trailing-solidus");
    }

    switch (token->type)
    {
    case token_start_tag:
    case token_end_tag:
        if(token->as.tag.name) wstring_free(token->as.tag.name);
        if(token->as.tag.attributes) vector_free(token->as.tag.attributes);
        break;

    case token_doctype:
        if(token->as.doctype.name) wstring_free(token->as.doctype.name);
        if(token->as.doctype.public_identifier) wstring_free(token->as.doctype.public_identifier);
        if(token->as.doctype.system_identifier) wstring_free(token->as.doctype.system_identifier);
        break;

    case token_comment:
        if(token->as.comment.data) wstring_free(token->as.comment.data);
        break;
    }

    free(token);
}
