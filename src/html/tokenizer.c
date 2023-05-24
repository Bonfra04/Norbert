#include <html/tokenizer.h>

#include <codepoint.h>
#include <html/named_entities.h>

#include "errors.h"

#include <stdlib.h>
#include <wctype.h>
#include <stdint.h>

#define matchState(state) case HTMLTokenizer_state_##state:

#define on_wchar(condition) (self->stream->Current() == (wchar_t)(uint64_t)(condition))
#define on_function(condition) (((bool(*)(wchar_t))(condition))(self->stream->Current()))
#define on_wstring(condition) (self->stream->Match((wchar_t*)condition, true, false))
#define checker(condition) (_Generic((condition), wchar_t: on_wchar(condition), wchar_t*: on_wstring(condition), default: on_function(condition)))
#define on(condition) if(checker(condition))
#define anythingElse() if(1)

#define next() goto emit_token
#define switchTo(target) do { self->state = HTMLTokenizer_state_##target; next(); } while(0)
#define switchToReturnState() do { self->state = self->returnState; next(); } while(0)
#define reconsumeIn(target) do { self->stream->Reconsume(); switchTo(target); } while(0)
#define reconsumeInReturnState() do { self->stream->Reconsume(); self->state = self->returnState; next(); } while(0)

#define consumeInputCharacter() (self->stream->Consume())
#define currentInputCharacter (self->stream->Current())

#define temporaryBufferClear() (self->temporaryBuffer->clear())
#define temporaryBufferAppend(c) (self->temporaryBuffer->appendwc(c))
#define temporaryBufferAppendCurrent() temporaryBufferAppend(currentInputCharacter)

#define Token(ttype, ...) ({HTMLToken* _ = calloc(sizeof(HTMLToken), 1); *_ = (HTMLToken){.type=HTMLTokenType_##ttype, .as = { .ttype =  {__VA_ARGS__}}}; _; })

#define prepareToken(ttype, ...) do { self->currentToken = Token(ttype, __VA_ARGS__); } while(0)
#define finalizeToken() do { self->tokensQueue->enqueue(self->currentToken); self->currentToken = NULL; } while(0)
#define enqueueToken(ttype, ...) do { prepareToken(ttype, __VA_ARGS__); finalizeToken(); } while(0)
#define emitToken(ttype, ...) do { enqueueToken(ttype, __VA_ARGS__); goto emit_token; } while(0)

#define emitEOFToken() emitToken(eof);
#define emitCurrentCharacterToken() emitToken(character, .value = self->stream->Current())

#define currentTagToken (self->currentToken->as.tag)
#define currentCommentToken (self->currentToken->as.comment)
#define currentDoctypeToken (self->currentToken->as.doctype)

#define prepareAttribute() do { HTMLTokenAttribute* _ = malloc(sizeof(HTMLTokenAttribute)); *_ = (HTMLTokenAttribute){ WString_new(), WString_new() }; currentTagToken.attributes->append(_); } while(0)
#define appendAttributeName(c) (((HTMLTokenAttribute*)currentTagToken.attributes->at(-1))->name->appendwc(c))
#define appendAttributeValue(c) (((HTMLTokenAttribute*)currentTagToken.attributes->at(-1))->value->appendwc(c))
#define appendTagName(c) currentTagToken.name->appendwc(c)
#define appendCommentData(c) currentCommentToken.data->appendwc(c)
#define appendDoctypeName(c) currentDoctypeToken.name->appendwc(c)
#define appendDoctypePublicIdentifier(c) currentDoctypeToken.public_identifier->appendwc(c)
#define appendDoctypeSystemIdentifier(c) currentDoctypeToken.system_identifier->appendwc(c)

#define consumedAsPartOfAnAttribute() (self->returnState == HTMLTokenizer_state_AttributeValueDoubleQuoted || self->returnState == HTMLTokenizer_state_AttributeValueSingleQuoted || self->returnState == HTMLTokenizer_state_AttributeValueUnquoted)
#define flushConsumedCharacterReference() do {                  \
for(size_t i = 0; i < self->temporaryBuffer->length(); i++)     \
    if(consumedAsPartOfAnAttribute())                           \
        appendAttributeValue(self->temporaryBuffer->at(i));     \
    else                                                        \
        enqueueToken(character, self->temporaryBuffer->at(i));  \
} while(0)

#define emitTemporaryBufferAsCharacters() do {              \
for(size_t i = 0; i < self->temporaryBuffer->length(); i++) \
    enqueueToken(character, self->temporaryBuffer->at(i));  \
} while(0)

#define isAppropriateEndTagToken(token) token->as.tag.name->equals(self->lastStartTagName)

static void HTMLTokenizer_SwitchTo(HTMLTokenizer_states targetState, HTMLTokenizer* self)
{
    self->state = targetState;
}

static HTMLToken* HTMLTokenizer_EmitToken(HTMLTokenizer* self)
{
    emit_token:
    if(self->tokensQueue->size() > 0)
    {
        HTMLToken* result = self->tokensQueue->dequeue();

        if(result->type == HTMLTokenType_start_tag)
        {
            self->lastStartTagName->clear();
            self->lastStartTagName->append(result->as.tag.name);
        }

        return result;
    }

    switch (self->state)
    {
        matchState(Data)
        {
            consumeInputCharacter();
            on(L'&')
            {
                self->returnState = HTMLTokenizer_state_Data;
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
                {
                    break;
                }

                temporaryBufferAppendCurrent();

                size_t buffLen = self->temporaryBuffer->length();

                bool match = false;
                for(size_t i = 0; i < NAMED_ENTITIES_COUNT; i++)
                {
                    size_t len = wcslen(entities[i].name);

                    bool correspondence = buffLen <= len;
                    if(correspondence == true)
                    {
                        for(size_t j = 0; j < buffLen; j++)
                        {
                            if(self->temporaryBuffer->at(j) != entities[i].name[j])
                            {
                                correspondence = false;
                                break;
                            }
                        }

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
                    self->stream->Reconsume();
                    self->temporaryBuffer->popback();
                    break;
                }
            }

            if(something == true)
            {
                int64_t i = NAMED_ENTITIES_COUNT - 1;
                for(; i >= 0; i--)
                    if(entities[i].flag)
                    {
                        size_t len = wcslen(entities[i].name);
                        
                        if(consumedAsPartOfAnAttribute())
                        {
                            if(entities[i].name[len - 1] != L';' && (currentInputCharacter == L'=' || isAsciiAlphanumeric(currentInputCharacter)))
                            {
                                flushConsumedCharacterReference();
                                switchToReturnState();
                            }
                        }

                        if(entities[i].name[len - 1] != ';')
                        {
                            parser_error("missing-semicolon-after-character-reference");
                        }

                        self->temporaryBuffer->clear();
                        wchar_t* value = entities[i].value;
                        self->temporaryBuffer->appendwc(value[0]);
                        if(value[1] != 0)
                        {
                            self->temporaryBuffer->appendwc(value[1]);
                        }
                        break;
                    }
                for(; i >= 0; i--)
                {
                    entities[i].flag = false;
                }
            }

            flushConsumedCharacterReference();
            if(something == true)
            {
                switchToReturnState();
            }
            else
            {
                switchTo(AmbiguousAmpersand);
            }
        }

        matchState(AmbiguousAmpersand)
        {
            consumeInputCharacter();
            on(AsciiAlphanumeric)
            {
                if(consumedAsPartOfAnAttribute())
                {
                    appendAttributeValue(currentInputCharacter);
                }
                else
                {
                    emitCurrentCharacterToken();
                }
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
            self->characterReferenceCode = 0;

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
                self->characterReferenceCode *= 16;
                self->characterReferenceCode += currentInputCharacter - L'0';
                next();
            }
            on(AsciiUpperHexDigit)
            {
                self->characterReferenceCode *= 16;
                self->characterReferenceCode += currentInputCharacter - L'A' + 10;
                next();
            }
            on(AsciiLowerHexDigit)
            {
                self->characterReferenceCode *= 16;
                self->characterReferenceCode += currentInputCharacter - L'a' + 10;
                next();
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
                self->characterReferenceCode *= 10;
                self->characterReferenceCode += currentInputCharacter - L'0';
                next();
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
            if(self->characterReferenceCode == 0)
            {
                parser_error("null-character-reference");
                self->characterReferenceCode = 0xFFFD;
            }
            else if(self->characterReferenceCode > 0x10FFFF)
            {
                parser_error("character-reference-outside-unicode-range");
                self->characterReferenceCode = 0xFFFD;
            }
            else if(isSurrogate(self->characterReferenceCode))
            {
                parser_error("surrogate-character-reference");
                self->characterReferenceCode = 0xFFFD;
            }
            else if(isNoncharacter(self->characterReferenceCode))
            {
                parser_error("noncharacter-character-reference");
            }
            else if(self->characterReferenceCode == 0x0D || (isControl(self->characterReferenceCode) && !isAsciiWhitespace(self->characterReferenceCode)))
            {
                parser_error("control-character-reference");
                for(size_t i = 0; i < CONTROL_CHARACTERS_COUNT; i++)
                {
                    if(self->characterReferenceCode == control_characters[i].value)
                    {
                        self->characterReferenceCode = control_characters[i].replacement;
                        break;
                    }
                }
            }

            temporaryBufferClear();
            temporaryBufferAppend(self->characterReferenceCode);
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
                prepareToken(start_tag, .name = WString_new(), .selfClosing = false, .ackSelfClosing = false, .attributes = Vector_new());
                reconsumeIn(TagName);
            }
            on(L'?')
            {
                parser_error("unexpected-question-mark-instead-of-tag-name");
                prepareToken(comment, .data = WString_new());
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
                prepareToken(end_tag, .name = WString_new(), .selfClosing = false, .attributes = Vector_new());
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
                prepareToken(comment, .data = WString_new());
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
                next();
            }
            on(L'\0')
            {
                parser_error("unexpected-null-character");
                appendTagName(L'\uFFFD');
                next();
            }
            on(-1)
            {
                parser_error("eof-in-tag");
                emitEOFToken();
            }
            anythingElse()
            {
                appendTagName(currentInputCharacter);
                next();
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
                next();
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
                next();
            }
            on('\0')
            {
                parser_error("unexpected-null-character");
                appendAttributeName(L'\uFFFD');
                next();
            }
            on(L'"')
            {
                parser_error("unexpected-character-in-attribute-name");
                appendAttributeName(currentInputCharacter);
                next();
            }
            on(L'\'')
            {
                parser_error("unexpected-character-in-attribute-name");
                appendAttributeName(currentInputCharacter);
                next();
            }
            on(L'<')
            {
                parser_error("unexpected-character-in-attribute-name");
                appendAttributeName(currentInputCharacter);
                next();
            }
            anythingElse()
            {
                appendAttributeName(currentInputCharacter);
                next();
            }
        }

        matchState(AfterAttributeName)
        {
            consumeInputCharacter();
            on(AsciiWhitespace)
            {
                next();
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
                next();
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
                self->returnState = HTMLTokenizer_state_AttributeValueDoubleQuoted;
                switchTo(CharacterReference);
            }
            on(L'\0')
            {
                parser_error("unexpected-null-character");
                appendAttributeValue(L'\uFFFD');
                next();
            }
            on(-1)
            {
                parser_error("eof-in-tag");
                emitEOFToken();
            }
            anythingElse()
            {
                appendAttributeValue(currentInputCharacter);
                next();
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
                self->returnState = HTMLTokenizer_state_AttributeValueSingleQuoted;
                switchTo(CharacterReference);
            }
            on(L'\0')
            {
                parser_error("unexpected-null-character");
                appendAttributeValue(L'\uFFFD');
                next();
            }
            on(-1)
            {
                parser_error("eof-in-tag");
                emitEOFToken();
            }
            anythingElse()
            {
                appendAttributeValue(currentInputCharacter);
                next();
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
                self->returnState = HTMLTokenizer_state_AttributeValueUnquoted;
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
                next();
            }
            on(L'"')
            {
                parser_error("unexpected-character-in-unquoted-attribute-value");
                appendAttributeValue(currentInputCharacter);
                next();
            }
            on(L'\'')
            {
                parser_error("unexpected-character-in-unquoted-attribute-value");
                appendAttributeValue(currentInputCharacter);
                next();
            }
            on(L'<')
            {
                parser_error("unexpected-character-in-unquoted-attribute-value");
                appendAttributeValue(currentInputCharacter);
                next();
            }
            on(L'=')
            {
                parser_error("unexpected-character-in-unquoted-attribute-value");
                appendAttributeValue(currentInputCharacter);
                next();
            }
            on(L'`')
            {
                parser_error("unexpected-character-in-unquoted-attribute-value");
                appendAttributeValue(currentInputCharacter);
                next();
            }
            on(-1)
            {
                parser_error("eof-in-tag");
                emitEOFToken();
            }
            anythingElse()
            {
                appendAttributeValue(currentInputCharacter);
                next();
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
                prepareToken(comment, .data = WString_new());
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
                prepareToken(comment, .data = WString_new());
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
                next();
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
                next();
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
                next();
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
                next();
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
                next();
            }
            anythingElse()
            {
                appendCommentData(currentInputCharacter);
                next();
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
                next();
            }
            on(AsciiUpperAlpha)
            {
                prepareToken(doctype, .name = WString_new());
                appendDoctypeName(towlower(currentInputCharacter));
                switchTo(DOCTYPEName);
            }
            on(L'\0')
            {
                parser_error("unexpected-null-character");
                prepareToken(doctype, .name = WString_new());
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
                prepareToken(doctype, .name = WString_new());
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
                next();
            }
            on(L'\0')
            {
                parser_error("unexpected-null-character");
                appendDoctypeName(L'\uFFFD');
                next();
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
                next();
            }
        }

        matchState(AfterDOCTYPEName)
        {
            consumeInputCharacter();
            on(AsciiWhitespace)
            {
                next();
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
                currentDoctypeToken.public_identifier = WString_new();
                switchTo(DOCTYPEPublicIdentifierDoubleQuoted);
            }
            on(L'\'')
            {
                parser_error("missing-whitespace-after-doctype-public-keyword");
                currentDoctypeToken.public_identifier = WString_new();
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
                next();
            }
            on(L'"')
            {
                currentDoctypeToken.public_identifier = WString_new();
                switchTo(DOCTYPEPublicIdentifierDoubleQuoted);
            }
            on(L'\'')
            {
                currentDoctypeToken.public_identifier = WString_new();
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
                next();
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
                next();
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
                next();
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
                next();
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
                currentDoctypeToken.system_identifier = WString_new();
                switchTo(DOCTYPESystemIdentifierDoubleQuoted);
            }
            on(L'\'')
            {
                parser_error("missing-whitespace-between-doctype-public-and-system-identifiers");
                currentDoctypeToken.system_identifier = WString_new();
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
                next();
            }
            on(L'>')
            {
                finalizeToken();
                switchTo(Data);
            }
            on(L'"')
            {
                currentDoctypeToken.system_identifier = WString_new();
                switchTo(DOCTYPESystemIdentifierDoubleQuoted);
            }
            on(L'\'')
            {
                currentDoctypeToken.system_identifier = WString_new();
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
                currentDoctypeToken.system_identifier = WString_new();
                switchTo(DOCTYPESystemIdentifierDoubleQuoted);
            }
            on(L'\'')
            {
                parser_error("missing-whitespace-after-doctype-system-keyword");
                currentDoctypeToken.system_identifier = WString_new();
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
                next();
            }
            on(L'"')
            {
                currentDoctypeToken.system_identifier = WString_new();
                switchTo(DOCTYPESystemIdentifierDoubleQuoted);
            }
            on(L'\'')
            {
                currentDoctypeToken.system_identifier = WString_new();
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
                next();
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
                next();
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
                next();
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
                next();
            }
        }

        matchState(AfterDOCTYPESystemIdentifier)
        {
            consumeInputCharacter();
            on(AsciiWhitespace)
            {
                next();
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
                next();
            }
            on(-1)
            {
                finalizeToken();
                emitEOFToken();
            }
            anythingElse()
            {
                next();
            }
        }

        matchState(RCDATA)
        {
            consumeInputCharacter();
            on(L'&')
            {
                self->returnState = HTMLTokenizer_state_RCDATA;
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
                prepareToken(end_tag, .name = WString_new(), .selfClosing = false, .attributes = Vector_new());
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
                if(isAppropriateEndTagToken(self->currentToken))
                {
                    switchTo(BeforeAttributeName);
                }
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
                if(isAppropriateEndTagToken(self->currentToken))
                {
                    switchTo(SelfClosingStartTag);
                }
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
                if(isAppropriateEndTagToken(self->currentToken)) 
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
                next();
            }
            on(AsciiLowerAlpha)
            {
                appendTagName(currentInputCharacter);
                temporaryBufferAppend(currentInputCharacter);
                next();
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
                prepareToken(end_tag, .name = WString_new(), .selfClosing = false, .attributes = Vector_new());
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
                if(isAppropriateEndTagToken(self->currentToken))
                {
                    switchTo(BeforeAttributeName);
                }
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
                if(isAppropriateEndTagToken(self->currentToken))
                {
                    switchTo(SelfClosingStartTag);
                }
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
                if(isAppropriateEndTagToken(self->currentToken)) 
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
                next();
            }
            on(AsciiLowerAlpha)
            {
                appendTagName(currentInputCharacter);
                temporaryBufferAppend(currentInputCharacter);
                next();
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

static void HTMLTokenizer_DisposeToken(HTMLToken* token, HTMLTokenizer* self)
{
    if(token == NULL)
    {
        return;
    }

    if(token->type == HTMLTokenType_start_tag && token->as.start_tag.selfClosing && token->as.start_tag.ackSelfClosing == false)
    {
        parser_error("non-void-html-element-start-tag-with-trailing-solidus");
    }

    switch (token->type)
    {
    case HTMLTokenType_start_tag:
    case HTMLTokenType_end_tag:
        if(token->as.tag.name) token->as.tag.name->delete();
        if(token->as.tag.attributes) token->as.tag.attributes->delete();
        break;

    case HTMLTokenType_doctype:
        if(token->as.doctype.name) token->as.doctype.name->delete();
        if(token->as.doctype.public_identifier) token->as.doctype.public_identifier->delete();
        if(token->as.doctype.system_identifier) token->as.doctype.system_identifier->delete();
        break;

    case HTMLTokenType_comment:
        if(token->as.comment.data) token->as.comment.data->delete();
        break;

    default:
        break;
    }

    free(token);
}

static void HTMLTokenizer_destructor(HTMLTokenizer* self)
{
    self->temporaryBuffer->delete();
    self->tokensQueue->delete();
    self->lastStartTagName->delete();
    self->super.destruct();
}

HTMLTokenizer* HTMLTokenizer_new(WCStream* stream)
{
    HTMLTokenizer* self = ObjectBase(HTMLTokenizer, 3);

    self->stream = stream;
    self->state = HTMLTokenizer_state_Data;
    self->returnState = HTMLTokenizer_state_NONE;
    self->temporaryBuffer = WString_new();
    self->characterReferenceCode = L'\0';
    self->tokensQueue = Queue_new();
    self->currentToken = NULL;
    self->lastStartTagName = WString_new();
    
    ObjectFunction(HTMLTokenizer, EmitToken, 0);
    ObjectFunction(HTMLTokenizer, DisposeToken, 1);
    ObjectFunction(HTMLTokenizer, SwitchTo, 1);

    Object_prepare((Object*)&self->super);
    return self;
}
