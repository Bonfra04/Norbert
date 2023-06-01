#include <html/parser.h>

#include <html/tokenizer.h>
#include <codepoint.h>
#include <html/html.h>

#include "errors.h"

#include <assert.h>

typedef struct adjusted_insertion_location
{
    __DOM_Node* parent;
    size_t index;
} adjusted_insertion_location_t;

#define lastChild(node) ((adjusted_insertion_location_t){ (__DOM_Node*)node, ((__DOM_Node*)node)->childNodes()->length()})
#define locationDefault ((adjusted_insertion_location_t){ NULL, 0})

#define matchMode(imode) case HTMLParser_insertion_mode_##imode: process_##imode(self); break
#define processMode(mode) static void process_##mode(HTMLParser* self)

#define on(ttype) if(self->currentToken->type == HTMLTokenType_##ttype)
#define anythingElse() anythingElse:
#define actAsAnythingElse() goto anythingElse
#define oneOff(...) currentTagToken.name->equalsOneOff((DOMString[]){__VA_ARGS__, NULL})

#define switchTo(imode) (self->insertionMode = HTMLParser_insertion_mode_##imode)
#define switchToOriginalInsertionMode() (self->insertionMode = self->originalInsertionMode)
#define reprocessToken(imode) (self->reprocess = true)
#define useRulesOf(imode) process_##imode(self)
#define ignoreToken() 

#define currentCommentToken (self->currentToken->as.comment)
#define currentDoctypeToken (self->currentToken->as.doctype)
#define currentCharacterToken (self->currentToken->as.character)
#define currentTagToken self->currentToken->as.tag

#define currentNode ((__DOM_Node*)self->openElements->at(-1))
#define currentDocumentTypeNode (currentNode->as.DocumentType)
#define currentElementNode ((__DOM_Node_Element*)currentNode)

#define openElementsPush(element) self->openElements->push(element)
#define openElementsPop() self->openElements->pop()
#define openElementsRemove(element) self->openElements->remove(element)
#define openElementsContains(element) self->openElements->contains(element)
#define OpenElementsHasInScope(element) _Generic((element), DOMString: self->openElements->hasInScopeS(element), default: false)
#define OpenElementsHasInButtonScope(element) _Generic((element), DOMString: self->openElements->hasInButtonScopeS(element), default: false)
#define openElementsPopUntilInclusive(tagname) self->openElements->popUntilPopped(tagname)
#define openElementsPopUntilOneOffInclusive(...)  self->openElements->popUntilOneOffPopped((DOMString[]){__VA_ARGS__, NULL})

#define insertCharacter() insert_character(self, NULL)
// #define insertNodeAt(node, location) location.parent->childNodes->insert(location.index, node)
#define insertNodeAt(node, location) location.parent->appendChild((__DOM_Node*)node)
#define insertHtmlElementFor(token) insert_html_element(self, token)
#define insertComment() insert_comment(self, locationDefault)
#define insertCommentAs(location) insert_comment(self, location)

#define tokenizerSwithTo(state) self->tokenizer->SwitchTo(HTMLTokenizer_state_##state)

#define generateImpliedEndTags(exception) generate_implied_end_tags(self, exception)
#define closePElement() close_p_element(self)

static adjusted_insertion_location_t appropriate_place_for_inserting_node(HTMLParser* self, __DOM_Node_Element* overrideTarget)
{
    // https://html.spec.whatwg.org/#appropriate-place-for-inserting-a-node

    // 1. If there was an override target specified, then let target be the override target.
    //    Otherwise, let target be the current node.
    auto target = (__DOM_Node*)overrideTarget ?: currentNode;

    // 2. Determine the adjusted insertion location using the first matching steps from the following list:
    adjusted_insertion_location_t location;

    // If foster parenting is enabled and target is a table, tbody, tfoot, thead, or tr element
    if(false)
    {
        // TODO: foster parenting
    }
    // Otherwise
    else
    {
        // Let adjusted insertion location be inside target, after its last child (if any).
        location.parent = target;
        location.index = target->childNodes()->length();
    }

    // 3. If the adjusted insertion location is inside a template element, let it instead be inside the template element's template contents, after its last child (if any).
    // TODO

    // 4. Return the adjusted insertion location.
    return location;
}

static void insert_comment(HTMLParser* self, adjusted_insertion_location_t position)
{
    auto data = currentCommentToken.data;
    auto adjustedInsertionLocation = position.parent ? position : appropriate_place_for_inserting_node(self, NULL);
    auto comment = DOM.Comment.new(adjustedInsertionLocation.parent->ownerDocument(), data);
    insertNodeAt(comment, adjustedInsertionLocation);
}

static __DOM_Node_Element* create_element_for(HTMLToken* token, __DOM_Node* intendedParent)
{
    // https://html.spec.whatwg.org/#create-an-element-for-the-token

    // TODO 1, 2

    // 3. Let document be intended parent's node document.
    auto document = intendedParent->ownerDocument();

    // 4. Let local name be the tag name of the token.
    auto localName = token->as.tag.name;

    // 5. Let is be the value of the "is" attribute in the given token, if such an attribute exists, or null otherwise.
    // TODO

    // TODO 6

    // 7. If definition is non-null and the parser was not created as part of the HTML fragment parsing algorithm, then let will execute script be true. Otherwise, let it be false
    auto will_execute_script = false; // TODO

    // 8. If will execute script is true, then:
    if(will_execute_script)
    {
        // TODO
    }

    // 9. Let element be the result of creating an element given document, localName, given namespace, null, and is. If will execute script is true, set the synchronous custom elements flag; otherwise, leave it unset.
    auto element = DOM.create_element(document, localName); // TODO

    // 10. Append each attribute in the given token to element.
    for(size_t i = 0; i < token->as.tag.attributes->length(); i++)
    {
        HTMLTokenAttribute* attribute = token->as.tag.attributes->at(i);
        auto attr = DOM.Attr.new(document, attribute->name, attribute->value);
        element->attributes()->setNamedItem(attr);
    }

    // 11. If will execute script is true, then:
    if(will_execute_script)
    {
        // TODO
    }

    // TODO 12, 13, 14 

    // 15. Return element.
    return element;
}

static __DOM_Node_Element* insert_foreign_element(HTMLParser* self, HTMLToken* token)
{
    auto element = create_element_for(token, currentNode);
    currentNode->appendChild(element);
    openElementsPush(element);
    return element;
}

static __DOM_Node_Element* insert_html_element(HTMLParser* self, HTMLToken* token)
{
    return insert_foreign_element(self, token);
}

static void insert_character(HTMLParser* self, wchar_t* characters)
{
    adjusted_insertion_location_t adjustedInsertionLocation = appropriate_place_for_inserting_node(self, NULL);
    if(adjustedInsertionLocation.parent->nodeType() == DOM.Node.DOCUMENT_NODE)
        return;
    
    WString* data = WString_new();
    if(characters)
        data->appendws(characters);
    else
        data->appendwc(currentCharacterToken.value);

    auto siblings = adjustedInsertionLocation.parent->childNodes();
    if(siblings->length() > 0 && siblings->item(adjustedInsertionLocation.index - 1)->nodeType() == DOM.Node.TEXT_NODE)
    {
        auto textNode = (__DOM_Node_Text*)siblings->item(adjustedInsertionLocation.index - 1);
        textNode->_wholeText->append(data);
    }
    else
    {
        auto textNode = DOM.Text.new(adjustedInsertionLocation.parent->ownerDocument(), data);
        insertNodeAt(textNode, adjustedInsertionLocation);
    }

    data->delete();
}

static void reconstruct_active_formatting_elements()
{
    // TODO: implement this
    return;
}

static void generate_implied_end_tags(HTMLParser* self, DOMString exception)
{
    DOMString implied[] = { HTML.TagNames.dd, HTML.TagNames.dt, HTML.TagNames.li, HTML.TagNames.optgroup, HTML.TagNames.option, HTML.TagNames.p, HTML.TagNames.rb, HTML.TagNames.rp, HTML.TagNames.rt, HTML.TagNames.rtc, NULL };

    while(currentElementNode->localName()->equalsOneOff(implied) && (!exception || !currentElementNode->localName()->equals(exception)))
        openElementsPop();
}

static void close_p_element(HTMLParser* self)
{
    generateImpliedEndTags(HTML.TagNames.p);
    
    if(!currentElementNode->localName()->equals(HTML.TagNames.p))
        parser_error("generic");

    while(!currentElementNode->localName()->equals(HTML.TagNames.p))
        openElementsPop();
    openElementsPop();
}

// TODO: implement this
static bool is_parsing_stopped = false;
static void stop_parsing()
{
    is_parsing_stopped = true;
}

processMode(Initial);
processMode(BeforeHTML);
processMode(BeforeHead);
processMode(InHead);
processMode(InHeadNoScript);
processMode(AfterHead);
processMode(InBody);
processMode(AfterBody);
processMode(Text);
processMode(InTemplate);
processMode(InTable);
processMode(AfterAfterBody);

processMode(Initial)
{
    on(character && isAsciiWhitespace(currentCharacterToken.value))
    {
        ignoreToken();
        return;
    }
    on(comment)
    {
        insertCommentAs(lastChild(self->document));
        return;
    }
    on(doctype)
    {
        if (!currentDoctypeToken.name->equalss("html") || currentDoctypeToken.public_identifier != NULL || (currentDoctypeToken.system_identifier != NULL && !currentDoctypeToken.system_identifier->equalss("about:legacy-compat")))
        {
            parser_error("generic");
        }

        auto doctype = DOM.DocumentType.new(self->document, currentDoctypeToken.name, currentDoctypeToken.public_identifier, currentDoctypeToken.system_identifier);
        ((__DOM_Node*)self->document)->appendChild(doctype);

        self->document->_mode = no_quirks;
        if ( /* TODO not an iframe srcdoc*/ self->flags.parserCannotChangeTheMode == false)
        {
            // TODO: case insentiveness
            if ((currentDoctypeToken.forceQuirks)
                || !currentDoctypeToken.name->equalss("html")
                || (currentDoctypeToken.public_identifier && currentDoctypeToken.public_identifier->equalssi("-//W3O//DTD W3 HTML Strict 3.0//EN//"))
                || (currentDoctypeToken.public_identifier && currentDoctypeToken.public_identifier->equalssi("-/W3C/DTD HTML 4.0 Transitional/EN"))
                || (currentDoctypeToken.public_identifier && currentDoctypeToken.public_identifier->equalssi("HTML"))
                || (currentDoctypeToken.system_identifier && currentDoctypeToken.system_identifier->equalssi("http://www.ibm.com/data/dtd/v11/ibmxhtml1-transitional.dtd"))
                || (currentDoctypeToken.public_identifier && currentDoctypeToken.public_identifier->startswithsi("+//Silmaril//dtd html Pro v0r11 19970101//"))
                || (currentDoctypeToken.public_identifier && currentDoctypeToken.public_identifier->startswithsi("-//AS//DTD HTML 3.0 asWedit + extensions//"))
                || (currentDoctypeToken.public_identifier && currentDoctypeToken.public_identifier->startswithsi("-//AdvaSoft Ltd//DTD HTML 3.0 asWedit + extensions//"))
                || (currentDoctypeToken.public_identifier && currentDoctypeToken.public_identifier->startswithsi("-//IETF//DTD HTML 2.0 Level 1//"))
                || (currentDoctypeToken.public_identifier && currentDoctypeToken.public_identifier->startswithsi("-//IETF//DTD HTML 2.0 Level 2//"))
                || (currentDoctypeToken.public_identifier && currentDoctypeToken.public_identifier->startswithsi("-//IETF//DTD HTML 2.0 Strict Level 1//"))
                || (currentDoctypeToken.public_identifier && currentDoctypeToken.public_identifier->startswithsi("-//IETF//DTD HTML 2.0 Strict Level 2//"))
                || (currentDoctypeToken.public_identifier && currentDoctypeToken.public_identifier->startswithsi("-//IETF//DTD HTML 2.0 Strict//"))
                || (currentDoctypeToken.public_identifier && currentDoctypeToken.public_identifier->startswithsi("-//IETF//DTD HTML 2.0//"))
                || (currentDoctypeToken.public_identifier && currentDoctypeToken.public_identifier->startswithsi("-//IETF//DTD HTML 2.1E//"))
                || (currentDoctypeToken.public_identifier && currentDoctypeToken.public_identifier->startswithsi("-//IETF//DTD HTML 3.0//"))
                || (currentDoctypeToken.public_identifier && currentDoctypeToken.public_identifier->startswithsi("-//IETF//DTD HTML 3.2 Final//"))
                || (currentDoctypeToken.public_identifier && currentDoctypeToken.public_identifier->startswithsi("-//IETF//DTD HTML 3.2//"))
                || (currentDoctypeToken.public_identifier && currentDoctypeToken.public_identifier->startswithsi("-//IETF//DTD HTML 3//"))
                || (currentDoctypeToken.public_identifier && currentDoctypeToken.public_identifier->startswithsi("-//IETF//DTD HTML Level 0//"))
                || (currentDoctypeToken.public_identifier && currentDoctypeToken.public_identifier->startswithsi("-//IETF//DTD HTML Level 1//"))
                || (currentDoctypeToken.public_identifier && currentDoctypeToken.public_identifier->startswithsi("-//IETF//DTD HTML Level 2//"))
                || (currentDoctypeToken.public_identifier && currentDoctypeToken.public_identifier->startswithsi("-//IETF//DTD HTML Level 3//"))
                || (currentDoctypeToken.public_identifier && currentDoctypeToken.public_identifier->startswithsi("-//IETF//DTD HTML Strict Level 0//"))
                || (currentDoctypeToken.public_identifier && currentDoctypeToken.public_identifier->startswithsi("-//IETF//DTD HTML Strict Level 1//"))
                || (currentDoctypeToken.public_identifier && currentDoctypeToken.public_identifier->startswithsi("-//IETF//DTD HTML Strict Level 2//"))
                || (currentDoctypeToken.public_identifier && currentDoctypeToken.public_identifier->startswithsi("-//IETF//DTD HTML Strict Level 3//"))
                || (currentDoctypeToken.public_identifier && currentDoctypeToken.public_identifier->startswithsi("-//IETF//DTD HTML Strict//"))
                || (currentDoctypeToken.public_identifier && currentDoctypeToken.public_identifier->startswithsi("-//IETF//DTD HTML//"))
                || (currentDoctypeToken.public_identifier && currentDoctypeToken.public_identifier->startswithsi("-//Metrius//DTD Metrius Presentational//"))
                || (currentDoctypeToken.public_identifier && currentDoctypeToken.public_identifier->startswithsi("-//Microsoft//DTD Internet Explorer 2.0 HTML Strict//"))
                || (currentDoctypeToken.public_identifier && currentDoctypeToken.public_identifier->startswithsi("-//Microsoft//DTD Internet Explorer 2.0 HTML//"))
                || (currentDoctypeToken.public_identifier && currentDoctypeToken.public_identifier->startswithsi("-//Microsoft//DTD Internet Explorer 2.0 Tables//"))
                || (currentDoctypeToken.public_identifier && currentDoctypeToken.public_identifier->startswithsi("-//Microsoft//DTD Internet Explorer 3.0 HTML Strict//"))
                || (currentDoctypeToken.public_identifier && currentDoctypeToken.public_identifier->startswithsi("-//Microsoft//DTD Internet Explorer 3.0 HTML//"))
                || (currentDoctypeToken.public_identifier && currentDoctypeToken.public_identifier->startswithsi("-//Microsoft//DTD Internet Explorer 3.0 Tables//"))
                || (currentDoctypeToken.public_identifier && currentDoctypeToken.public_identifier->startswithsi("-//Netscape Comm. Corp.//DTD HTML//"))
                || (currentDoctypeToken.public_identifier && currentDoctypeToken.public_identifier->startswithsi("-//Netscape Comm. Corp.//DTD Strict HTML//"))
                || (currentDoctypeToken.public_identifier && currentDoctypeToken.public_identifier->startswithsi("-//O'Reilly and Associates//DTD HTML 2.0//"))
                || (currentDoctypeToken.public_identifier && currentDoctypeToken.public_identifier->startswithsi("-//O'Reilly and Associates//DTD HTML Extended 1.0//"))
                || (currentDoctypeToken.public_identifier && currentDoctypeToken.public_identifier->startswithsi("-//O'Reilly and Associates//DTD HTML Extended Relaxed 1.0//"))
                || (currentDoctypeToken.public_identifier && currentDoctypeToken.public_identifier->startswithsi("-//SQ//DTD HTML 2.0 HoTMetaL + extensions//"))
                || (currentDoctypeToken.public_identifier && currentDoctypeToken.public_identifier->startswithsi("-//SoftQuad Software//DTD HoTMetaL PRO 6.0::19990601::extensions to HTML 4.0//"))
                || (currentDoctypeToken.public_identifier && currentDoctypeToken.public_identifier->startswithsi("-//SoftQuad//DTD HoTMetaL PRO 4.0::19971010::extensions to HTML 4.0//"))
                || (currentDoctypeToken.public_identifier && currentDoctypeToken.public_identifier->startswithsi("-//Spyglass//DTD HTML 2.0 Extended//"))
                || (currentDoctypeToken.public_identifier && currentDoctypeToken.public_identifier->startswithsi("-//Sun Microsystems Corp.//DTD HotJava HTML//"))
                || (currentDoctypeToken.public_identifier && currentDoctypeToken.public_identifier->startswithsi("-//Sun Microsystems Corp.//DTD HotJava Strict HTML//"))
                || (currentDoctypeToken.public_identifier && currentDoctypeToken.public_identifier->startswithsi("-//W3C//DTD HTML 3 1995-03-24//"))
                || (currentDoctypeToken.public_identifier && currentDoctypeToken.public_identifier->startswithsi("-//W3C//DTD HTML 3.2 Draft//"))
                || (currentDoctypeToken.public_identifier && currentDoctypeToken.public_identifier->startswithsi("-//W3C//DTD HTML 3.2 Final//"))
                || (currentDoctypeToken.public_identifier && currentDoctypeToken.public_identifier->startswithsi("-//W3C//DTD HTML 3.2//"))
                || (currentDoctypeToken.public_identifier && currentDoctypeToken.public_identifier->startswithsi("-//W3C//DTD HTML 3.2S Draft//"))
                || (currentDoctypeToken.public_identifier && currentDoctypeToken.public_identifier->startswithsi("-//W3C//DTD HTML 4.0 Frameset//"))
                || (currentDoctypeToken.public_identifier && currentDoctypeToken.public_identifier->startswithsi("-//W3C//DTD HTML 4.0 Transitional//"))
                || (currentDoctypeToken.public_identifier && currentDoctypeToken.public_identifier->startswithsi("-//W3C//DTD HTML Experimental 19960712//"))
                || (currentDoctypeToken.public_identifier && currentDoctypeToken.public_identifier->startswithsi("-//W3C//DTD HTML Experimental 970421//"))
                || (currentDoctypeToken.public_identifier && currentDoctypeToken.public_identifier->startswithsi("-//W3C//DTD W3 HTML//"))
                || (currentDoctypeToken.public_identifier && currentDoctypeToken.public_identifier->startswithsi("-//W3O//DTD W3 HTML 3.0//"))
                || (currentDoctypeToken.public_identifier && currentDoctypeToken.public_identifier->startswithsi("-//WebTechs//DTD Mozilla HTML 2.0//"))
                || (currentDoctypeToken.public_identifier && currentDoctypeToken.public_identifier->startswithsi("-//WebTechs//DTD Mozilla HTML//" ))
                || (currentDoctypeToken.system_identifier == NULL && currentDoctypeToken.public_identifier && currentDoctypeToken.public_identifier->startswithsi("-//W3C//DTD HTML 4.01 Frameset//"))
                || (currentDoctypeToken.system_identifier == NULL && currentDoctypeToken.public_identifier && currentDoctypeToken.public_identifier->startswithsi("-//W3C//DTD HTML 4.01 Transitional//"))
            )
                self->document->_mode = quirks;
            else if (
                (currentDoctypeToken.public_identifier && currentDoctypeToken.public_identifier->startswithsi("-//W3C//DTD XHTML 1.0 Frameset//"))
                || (currentDoctypeToken.public_identifier && currentDoctypeToken.public_identifier->startswithsi("-//W3C//DTD XHTML 1.0 Transitional//"))
                || (currentDoctypeToken.system_identifier && currentDoctypeToken.public_identifier && currentDoctypeToken.public_identifier->startswithsi("-//W3C//DTD HTML 4.01 Frameset//"))
                || (currentDoctypeToken.system_identifier && currentDoctypeToken.public_identifier && currentDoctypeToken.public_identifier->startswithsi("-//W3C//DTD HTML 4.01 Transitional//"))
            )
                self->document->_mode = limited_quirks;
            else
                self->document->_mode = no_quirks;
        }

        switchTo(BeforeHTML);
        return;
    }
    anythingElse()
    {
        // TODO: not an iframe srcdoc document
        if(self->flags.parserCannotChangeTheMode == false)
        {
            self->document->_mode = quirks;
        }

        switchTo(BeforeHTML);
        reprocessToken();
        return;
    }
}

processMode(BeforeHTML)
{
    on(doctype)
    {
        parser_error("generic");
        ignoreToken();
        return;
    }
    on(comment)
    {
        insertCommentAs(lastChild(self->document));
        return;
    }
    on(character && isAsciiWhitespace(currentCharacterToken.value))
    {
        ignoreToken();
        return;
    }
    on(start_tag && oneOff(HTML.TagNames.html))
    {
        auto html = create_element_for(self->currentToken, (__DOM_Node*)self->document);
        ((__DOM_Node*)self->document)->appendChild(html);
        openElementsPush(html);
        switchTo(BeforeHead);
        return;
    }
    on(end_tag && oneOff(HTML.TagNames.head, HTML.TagNames.body, HTML.TagNames.html, HTML.TagNames.br))
    {
        actAsAnythingElse();
    }
    on(end_tag)
    {
        parser_error("generic");
        ignoreToken();
        return;
    }
    anythingElse()
    {
        auto html = DOM.create_element(self->document, HTML.TagNames.html);
        ((__DOM_Node*)self->document)->appendChild(html);
        openElementsPush(html);
        switchTo(BeforeHead);
        reprocessToken();
        return;
    }
}

processMode(BeforeHead)
{
    on(character && isAsciiWhitespace(currentCharacterToken.value))
    {
        ignoreToken();
        return;
    }
    on(comment)
    {
        insertComment();
        return;
    }
    on(doctype)
    {
        parser_error("generic");
        ignoreToken();
        return;
    }
    on(start_tag && oneOff(HTML.TagNames.html))
    {
        useRulesOf(InBody);
        return;
    }
    on(start_tag && oneOff(HTML.TagNames.head))
    {
        auto head = insertHtmlElementFor(self->currentToken);
        self->head = head;
        switchTo(InHead);
        return;
    }
    on(end_tag && oneOff(HTML.TagNames.head, HTML.TagNames.body, HTML.TagNames.html, HTML.TagNames.br))
    {
        actAsAnythingElse();
    }
    on(end_tag)
    {
        parser_error("generic");
        ignoreToken();
        return;
    }
    anythingElse()
    {
        HTMLToken dummyHead = { .type = HTMLTokenType_start_tag, .as.start_tag = { .name = HTML.TagNames.head, .selfClosing = false, .attributes = Vector_new() } };
        auto head = insertHtmlElementFor(&dummyHead);
        self->head = head;
        switchTo(InHead);
        reprocessToken();
        return;
    }
}

processMode(InHead)
{
    on(character && isAsciiWhitespace(currentCharacterToken.value))
    {
        insertCharacter();
        return;
    }
    on(comment)
    {
        insertComment();
        return;
    }
    on(doctype)
    {
        parser_error("generic");
        ignoreToken();
        return;
    }
    on(start_tag && oneOff(HTML.TagNames.html))
    {
        useRulesOf(InBody);
        return;
    }
    on(start_tag && oneOff(HTML.TagNames.base, HTML.TagNames.basefont, HTML.TagNames.bgsound, HTML.TagNames.link))
    {
        insertHtmlElementFor(self->currentToken);
        openElementsPop();
        currentTagToken.ackSelfClosing = true;
        return;
    }
    on(start_tag && oneOff(HTML.TagNames.meta))
    {
        insertHtmlElementFor(self->currentToken);
        openElementsPop();
        currentTagToken.ackSelfClosing = true;
        // TODO: charset and http-equiv

        return;
    }

    on(start_tag && oneOff(HTML.TagNames.title))
    {
        insertHtmlElementFor(self->currentToken);
        tokenizerSwithTo(RCDATA);
        self->originalInsertionMode = self->insertionMode;
        switchTo(Text);
        return;
    }
    on(start_tag && oneOff(HTML.TagNames.noscript) && self->flags.scripting == true)
    {
        insertHtmlElementFor(self->currentToken);
        tokenizerSwithTo(RAWTEXT);
        self->originalInsertionMode = self->insertionMode;
        switchTo(Text);
        return;
    }
    on(start_tag && oneOff(HTML.TagNames.noframes, HTML.TagNames.style))
    {
        insertHtmlElementFor(self->currentToken);
        tokenizerSwithTo(RAWTEXT);
        self->originalInsertionMode = self->insertionMode;
        switchTo(Text);
        return;
    }
    on(start_tag && oneOff(HTML.TagNames.noscript) && self->flags.scripting == false)
    {
        insertHtmlElementFor(self->currentToken);
        switchTo(InHeadNoScript);
        return;
    }
    // TODO: script
    on(end_tag && oneOff(HTML.TagNames.head))
    {
        openElementsPop();
        switchTo(AfterHead);
        return;
    }
    on(end_tag && oneOff(HTML.TagNames.body, HTML.TagNames.html, HTML.TagNames.br))
    {
        actAsAnythingElse();
    }
    // TODO: template
    on(start_tag && oneOff(HTML.TagNames.head))
    {
        parser_error("generic");
        ignoreToken();
        return;
    }
    on(end_tag)
    {
        parser_error("generic");
        ignoreToken();
        return;
    }
    anythingElse()
    {
        openElementsPop();
        switchTo(AfterHead);
        reprocessToken();
        return;
    }
}

processMode(InHeadNoScript)
{
    on(doctype)
    {
        parser_error("generic");
        ignoreToken();
        return;
    }
    on(start_tag && oneOff(HTML.TagNames.html))
    {
        useRulesOf(InBody);
        return;
    }
    on(end_tag && oneOff(HTML.TagNames.noscript))
    {
        openElementsPop();
        switchTo(InHead);
        return;
    }
    on(character && isAsciiWhitespace(currentCharacterToken.value))
    {
        useRulesOf(InHead);
        return;
    }
    on(comment)
    {
        useRulesOf(InHead);
        return;
    }
    on(start_tag && oneOff(HTML.TagNames.basefont, HTML.TagNames.bgsound, HTML.TagNames.link, HTML.TagNames.meta, HTML.TagNames.noframes, HTML.TagNames.style))
    {
        useRulesOf(InHead);
        return;
    }
    on(end_tag && oneOff(HTML.TagNames.br))
    {
        actAsAnythingElse();
    }
    on(start_tag && oneOff(HTML.TagNames.head, HTML.TagNames.noscript))
    {
        parser_error("generic");
        ignoreToken();
        return;
    }
    on(end_tag)
    {
        parser_error("generic");
        ignoreToken();
        return;
    }
    anythingElse()
    {
        parser_error("generic");
        openElementsPop();
        switchTo(InHead);
        reprocessToken();
        return;
    }
}

processMode(AfterHead)
{
    on(character && isAsciiWhitespace(currentCharacterToken.value))
    {
        insertCharacter();
        return;
    }
    on(comment)
    {
        insertComment();
        return;
    }
    on(doctype)
    {
        parser_error("generic");
        ignoreToken();
        return;
    }
    on(start_tag && oneOff(HTML.TagNames.html))
    {
        useRulesOf(InBody);
        return;
    }
    on(start_tag && oneOff(HTML.TagNames.body))
    {
        insertHtmlElementFor(self->currentToken);
        self->flags.framesetOk = false;
        switchTo(InBody);
        return;
    }
    // TODO: frameset
    on(start_tag && oneOff(HTML.TagNames.base, HTML.TagNames.basefont, HTML.TagNames.bgsound, HTML.TagNames.link, HTML.TagNames.meta, HTML.TagNames.noframes, HTML.TagNames.script, HTML.TagNames.style, HTML.TagNames.template, HTML.TagNames.title))
    {
        parser_error("generic");
        openElementsPush(self->head);
        useRulesOf(InHead);
        openElementsRemove(self->head);
        return;
    }
    on(end_tag && oneOff(HTML.TagNames.template))
    {
        useRulesOf(InHead);
        return;
    }
    on(end_tag && oneOff(HTML.TagNames.body, HTML.TagNames.html, HTML.TagNames.br))
    {
        actAsAnythingElse();
    }
    on(start_tag && oneOff(HTML.TagNames.head))
    {
        parser_error("generic");
        ignoreToken();
        return;
    }
    on(end_tag)
    {
        parser_error("generic");
        ignoreToken();
        return;
    }
    anythingElse()
    {
        HTMLToken dummyBody = { .type = HTMLTokenType_start_tag, .as.start_tag = { .name = HTML.TagNames.body, .selfClosing = false, .attributes = Vector_new() } };
        insertHtmlElementFor(&dummyBody);
        switchTo(InBody);
        reprocessToken();
        return;
    }
}

processMode(InBody)
{
    on(character && currentCharacterToken.value == L'\0')
    {
        parser_error("generic");
        ignoreToken();
        return;
    }
    on(character && isAsciiWhitespace(currentCharacterToken.value))
    {
        reconstruct_active_formatting_elements();
        insertCharacter();
        return;
    }
    on(character)
    {
        reconstruct_active_formatting_elements();
        insertCharacter();
        self->flags.framesetOk = false;
        return;
    }
    on(comment)
    {
        insertComment();
        return;
    }
    on(doctype)
    {
        parser_error("generic");
        ignoreToken();
        return;
    }
    on(start_tag && oneOff(HTML.TagNames.html))
    {
        parser_error("generic");

        if(openElementsContains(HTML.TagNames.template))
        {
            ignoreToken();
            return;
        }

        for(size_t i = 0; i < currentTagToken.attributes->length(); i++)
        {
            WString* name = ((HTMLTokenAttribute*)currentTagToken.attributes->at(i))->name;
            WString* value = ((HTMLTokenAttribute*)currentTagToken.attributes->at(i))->value;
            auto element = self->openElements->at(0);
            if(element->attributes()->getNamedItem(name) == NULL)
            {
                auto attr = DOM.Attr.new(((__DOM_Node*)element)->ownerDocument(), name, value);
                element->attributes()->setNamedItem(attr);
            }
        }

        return;
    }
    on(start_tag && oneOff(HTML.TagNames.base, HTML.TagNames.basefont, HTML.TagNames.bgsound, HTML.TagNames.link, HTML.TagNames.meta, HTML.TagNames.noframes, HTML.TagNames.script, HTML.TagNames.style, HTML.TagNames.template, HTML.TagNames.title))
    {
        useRulesOf(InHead);
        return;
    }
    on(end_tag && oneOff(HTML.TagNames.template))
    {
        useRulesOf(InHead);
        return;
    }
    on(start_tag && oneOff(HTML.TagNames.body))
    {
        parser_error("generic");

        // TODO: fragment case

        self->flags.framesetOk = false;
        for (size_t i = 0; i < currentTagToken.attributes->length(); i++)
        {
            WString* name = ((HTMLTokenAttribute*)currentTagToken.attributes->at(i))->name;
            WString* value = ((HTMLTokenAttribute*)currentTagToken.attributes->at(i))->value;
            auto element = self->openElements->at(1);

            if(element->attributes()->getNamedItem(name) == NULL)
            {
                auto attr = DOM.Attr.new(((__DOM_Node*)element)->ownerDocument(), name, value);
                element->attributes()->setNamedItem(attr);
            }
        }

        return;
    }
    // TODO: frameset    
    on(eof)
    {
        if(self->templateInsertionModes->size() > 0)
        {
            useRulesOf(InTemplate);
            return;
        }

        bool contains = openElementsContains(HTML.TagNames.dd) ||
                        openElementsContains(HTML.TagNames.dt) ||
                        openElementsContains(HTML.TagNames.li) ||
                        openElementsContains(HTML.TagNames.optgroup) ||
                        openElementsContains(HTML.TagNames.option) ||
                        openElementsContains(HTML.TagNames.p) ||
                        openElementsContains(HTML.TagNames.rb) ||
                        openElementsContains(HTML.TagNames.rp) ||
                        openElementsContains(HTML.TagNames.rt) ||
                        openElementsContains(HTML.TagNames.rtc) ||
                        openElementsContains(HTML.TagNames.tbody) ||
                        openElementsContains(HTML.TagNames.td) ||
                        openElementsContains(HTML.TagNames.tfoot) ||
                        openElementsContains(HTML.TagNames.th) ||
                        openElementsContains(HTML.TagNames.thead) ||
                        openElementsContains(HTML.TagNames.tr) ||
                        openElementsContains(HTML.TagNames.body) ||
                        openElementsContains(HTML.TagNames.html);
        if (!contains)
        {
            parser_error("generic");
        }

        stop_parsing();
        return;
    }
    on(end_tag && oneOff(HTML.TagNames.body))
    {
        if(!OpenElementsHasInScope(HTML.TagNames.body))
        {
            parser_error("generic");
            ignoreToken();
            return;
        }

        bool contains = openElementsContains(HTML.TagNames.dd) ||
                        openElementsContains(HTML.TagNames.dt) ||
                        openElementsContains(HTML.TagNames.li) ||
                        openElementsContains(HTML.TagNames.optgroup) ||
                        openElementsContains(HTML.TagNames.option) ||
                        openElementsContains(HTML.TagNames.p) ||
                        openElementsContains(HTML.TagNames.rb) ||
                        openElementsContains(HTML.TagNames.rp) ||
                        openElementsContains(HTML.TagNames.rt) ||
                        openElementsContains(HTML.TagNames.rtc) ||
                        openElementsContains(HTML.TagNames.tbody) ||
                        openElementsContains(HTML.TagNames.td) ||
                        openElementsContains(HTML.TagNames.tfoot) ||
                        openElementsContains(HTML.TagNames.th) ||
                        openElementsContains(HTML.TagNames.thead) ||
                        openElementsContains(HTML.TagNames.tr) ||
                        openElementsContains(HTML.TagNames.body) ||
                        openElementsContains(HTML.TagNames.html);
        if (!contains)
        {
            parser_error("generic");
        }

        switchTo(AfterBody);
        return;
    }
    on(end_tag && oneOff(HTML.TagNames.html))
    {
        if(!OpenElementsHasInScope(HTML.TagNames.body))
        {
            parser_error("generic");
            ignoreToken();
            return;
        }  
        
        bool contains = openElementsContains(HTML.TagNames.dd) ||
                        openElementsContains(HTML.TagNames.dt) ||
                        openElementsContains(HTML.TagNames.li) ||
                        openElementsContains(HTML.TagNames.optgroup) ||
                        openElementsContains(HTML.TagNames.option) ||
                        openElementsContains(HTML.TagNames.p) ||
                        openElementsContains(HTML.TagNames.rb) ||
                        openElementsContains(HTML.TagNames.rp) ||
                        openElementsContains(HTML.TagNames.rt) ||
                        openElementsContains(HTML.TagNames.rtc) ||
                        openElementsContains(HTML.TagNames.tbody) ||
                        openElementsContains(HTML.TagNames.td) ||
                        openElementsContains(HTML.TagNames.tfoot) ||
                        openElementsContains(HTML.TagNames.th) ||
                        openElementsContains(HTML.TagNames.thead) ||
                        openElementsContains(HTML.TagNames.tr) ||
                        openElementsContains(HTML.TagNames.body) ||
                        openElementsContains(HTML.TagNames.html);
        if (!contains)
        {
            parser_error("generic");
        }

        switchTo(AfterBody);
        reprocessToken();
        return;
    }
    on(start_tag && oneOff(HTML.TagNames.address, HTML.TagNames.article, HTML.TagNames.aside, HTML.TagNames.blockquote, HTML.TagNames.center, HTML.TagNames.details, HTML.TagNames.dialog, HTML.TagNames.dir, HTML.TagNames.div, HTML.TagNames.dl, HTML.TagNames.fieldset, HTML.TagNames.figcaption, HTML.TagNames.figure, HTML.TagNames.footer, HTML.TagNames.header, HTML.TagNames.hgroup, HTML.TagNames.main, HTML.TagNames.menu, HTML.TagNames.nav, HTML.TagNames.ol, HTML.TagNames.p, HTML.TagNames.search, HTML.TagNames.section, HTML.TagNames.summary, HTML.TagNames.ul))
    {
        if(OpenElementsHasInButtonScope(HTML.TagNames.p))
        {
            closePElement();
        }
        
        insertHtmlElementFor(self->currentToken);
        return;
    }
    on(start_tag && oneOff(HTML.TagNames.h1, HTML.TagNames.h2, HTML.TagNames.h3, HTML.TagNames.h4, HTML.TagNames.h5, HTML.TagNames.h6))
    {
        if(OpenElementsHasInButtonScope(HTML.TagNames.p))
        {
            closePElement();
        }

        if(currentElementNode->localName()->equalsOneOff((DOMString[]){HTML.TagNames.h2, HTML.TagNames.h3, HTML.TagNames.h4, HTML.TagNames.h5, HTML.TagNames.h6}))
        {
            parser_error("generic");
            openElementsPop();
        }

        insertHtmlElementFor(self->currentToken);
        return;
    }
    on(start_tag && oneOff(HTML.TagNames.pre, HTML.TagNames.listing))
    {
        if(OpenElementsHasInButtonScope(HTML.TagNames.p))
        {
            closePElement();
        }

        insertHtmlElementFor(self->currentToken);

        // TODO: next token things

        self->flags.framesetOk = false;
        return;
    }
    // TODO: form
    on(start_tag && oneOff(HTML.TagNames.li))
    {
        self->flags.framesetOk = false;

        for (int64_t i = self->openElements->length() - 1; i >= 0; i--)
        {
            auto node = self->openElements->at(i);

            if(node->localName()->equals(HTML.TagNames.li))
            {
                generateImpliedEndTags(HTML.TagNames.li);
                if(!currentElementNode->localName()->equals(HTML.TagNames.li))
                {
                    parser_error("generic");
                }
                openElementsPopUntilInclusive(HTML.TagNames.li);
                break;
            }

            if(node->localName()->equalsOneOff(HTML.SpecialTagNames) && !node->localName()->equalsOneOff((DOMString[]){HTML.TagNames.address, HTML.TagNames.div, HTML.TagNames.p}))
            {
                break;
            }
        }

        if(OpenElementsHasInButtonScope(HTML.TagNames.p))
        {
            closePElement();
        }

        insertHtmlElementFor(self->currentToken);
        return;
    }
    on(start_tag && oneOff(HTML.TagNames.dd, HTML.TagNames.dt))
    {
        self->flags.framesetOk = false;
    
        for (int64_t i = self->openElements->length() - 1; i >= 0; i--)
        {
            auto node = self->openElements->at(i);

            if(node->localName()->equals(HTML.TagNames.dd))
            {
                generateImpliedEndTags(HTML.TagNames.dd);
                if(!currentElementNode->localName()->equals(HTML.TagNames.dd))
                {
                    parser_error("generic");
                }
                openElementsPopUntilInclusive(HTML.TagNames.dd);
                break;
            }
            if(node->localName()->equals(HTML.TagNames.dt))
            {
                generateImpliedEndTags(HTML.TagNames.dt);
                if(!currentElementNode->localName()->equals(HTML.TagNames.dt))
                {
                    parser_error("generic");
                }
                openElementsPopUntilInclusive(HTML.TagNames.dt);
                break;
            }
            if(node->localName()->equalsOneOff(HTML.SpecialTagNames) && !node->localName()->equalsOneOff((DOMString[]){HTML.TagNames.address, HTML.TagNames.div, HTML.TagNames.p}))
            {
                break;
            }
        }

        if(OpenElementsHasInButtonScope(HTML.TagNames.p))
        {
            closePElement();
        }

        insertHtmlElementFor(self->currentToken);
        return;
    }
    // TODO: plaintext
    on(start_tag && oneOff(HTML.TagNames.button))
    {
        if(OpenElementsHasInScope(HTML.TagNames.button))
        {
            parser_error("generic");
            generateImpliedEndTags(NULL);
            openElementsPopUntilInclusive(HTML.TagNames.button);
        }
        reconstruct_active_formatting_elements();
        insertHtmlElementFor(self->currentToken);
        self->flags.framesetOk = false;
        return;
    }
    on(end_tag && oneOff(HTML.TagNames.address, HTML.TagNames.article, HTML.TagNames.aside, HTML.TagNames.blockquote, HTML.TagNames.button, HTML.TagNames.center, HTML.TagNames.details, HTML.TagNames.dialog, HTML.TagNames.dir, HTML.TagNames.div, HTML.TagNames.dl, HTML.TagNames.fieldset, HTML.TagNames.figcaption, HTML.TagNames.figure, HTML.TagNames.footer, HTML.TagNames.header, HTML.TagNames.hgroup, HTML.TagNames.listing, HTML.TagNames.main, HTML.TagNames.menu, HTML.TagNames.nav, HTML.TagNames.ol, HTML.TagNames.pre, HTML.TagNames.search, HTML.TagNames.section, HTML.TagNames.summary, HTML.TagNames.ul))
    {
        if(!OpenElementsHasInScope(currentTagToken.name))
        {
            parser_error("generic");
            ignoreToken();
            return;
        }
        else
        {
            generateImpliedEndTags(NULL);
            if(!currentElementNode->localName()->equals(currentTagToken.name))
            {
                parser_error("generic");
            }
            openElementsPopUntilInclusive(currentTagToken.name);
            return;
        }
    }
    // TODO: form
    on(end_tag && oneOff(HTML.TagNames.p))
    {
        if(!OpenElementsHasInButtonScope(HTML.TagNames.p))
        {
            parser_error("generic");
            HTMLToken dummyP = { .type = HTMLTokenType_start_tag, .as.start_tag = { .name = HTML.TagNames.p, .selfClosing = false, .attributes = Vector_new() } };
            insertHtmlElementFor(&dummyP);
        }
        closePElement();
        return;
    }
    on(end_tag && oneOff(HTML.TagNames.li))
    {
        if(!OpenElementsHasInScope(HTML.TagNames.li))
        {
            parser_error("generic");
            ignoreToken();
            return;
        }

        generateImpliedEndTags(HTML.TagNames.li);
        if(!currentElementNode->localName()->equals(HTML.TagNames.li))
        {
            parser_error("generic");
        }
        openElementsPopUntilInclusive(HTML.TagNames.li);
        return;
    }
    on(end_tag && oneOff(HTML.TagNames.dd, HTML.TagNames.dt))
    {
        if(!OpenElementsHasInScope(currentTagToken.name))
        {
            parser_error("generic");
            ignoreToken();
        }

        generateImpliedEndTags(currentTagToken.name);
        if(!currentElementNode->localName()->equals(currentTagToken.name))
        {
            parser_error("generic");
        }
        openElementsPopUntilInclusive(currentTagToken.name);
        return;
    }
    on(end_tag && oneOff(HTML.TagNames.h1, HTML.TagNames.h2, HTML.TagNames.h3, HTML.TagNames.h4, HTML.TagNames.h5, HTML.TagNames.h6))
    {
        if(!OpenElementsHasInScope(HTML.TagNames.h1)
            && !OpenElementsHasInScope(HTML.TagNames.h2)
            && !OpenElementsHasInScope(HTML.TagNames.h3)
            && !OpenElementsHasInScope(HTML.TagNames.h4)
            && !OpenElementsHasInScope(HTML.TagNames.h5)
            && !OpenElementsHasInScope(HTML.TagNames.h6))
        {
            parser_error("generic");
            ignoreToken();
            return;
        }

        generateImpliedEndTags(NULL);
        if(!currentElementNode->localName()->equals(currentTagToken.name))
        {
            parser_error("generic");
        }
        openElementsPopUntilOneOffInclusive(HTML.TagNames.h1, HTML.TagNames.h2, HTML.TagNames.h3, HTML.TagNames.h4, HTML.TagNames.h5, HTML.TagNames.h6);
        return;
    }
    // TODO: "a"
    // TODO: "b", "big", "code", "em", "font", "i", "s", "small", "strike", "strong", "tt", "u"
    // TODO: "nobr"
    // TODO: "a", "b", "big", "code", "em", "font", "i", "nobr", "s", "small", "strike", "strong", "tt", "u"
    // TODO: open/close "applet", "marquee", "object"
    on(start_tag && oneOff(HTML.TagNames.table))
    {
        if(self->document->mode() != quirks && OpenElementsHasInButtonScope(HTML.TagNames.p))
        {
            closePElement();
        }

        insertHtmlElementFor(self->currentToken);
        self->flags.framesetOk = false;
        switchTo(InTable);
        return;
    }
    on(end_tag && oneOff(HTML.TagNames.br))
    {
        parser_error("generic");
        // TODO: drop attributes
        goto normalBr;
    }
    on(start_tag && oneOff(HTML.TagNames.area, HTML.TagNames.br, HTML.TagNames.embed, HTML.TagNames.img, HTML.TagNames.keygen, HTML.TagNames.wbr))
    {
        normalBr:
        reconstruct_active_formatting_elements();
        insertHtmlElementFor(self->currentToken);
        openElementsPop();
        currentTagToken.ackSelfClosing = true;
        self->flags.framesetOk = false;
        return;
    }
    on(start_tag && oneOff(HTML.TagNames.input))
    {
        reconstruct_active_formatting_elements();
        insertHtmlElementFor(self->currentToken);
        openElementsPop();
        currentTagToken.ackSelfClosing = true;

        size_t len = currentTagToken.attributes->length();
        for(size_t i = 0; i < len; i++)
        {
            HTMLTokenAttribute* attr = currentTagToken.attributes->at(i);
            if(attr->name->equalss("type"))
            {
                if(attr->value->equalss("hidden"))
                {
                    self->flags.framesetOk = false;
                }
                break;
            }
        }
        return;
    }
    on(start_tag && oneOff(HTML.TagNames.param, HTML.TagNames.source, HTML.TagNames.track))
    {
        insert_html_element(self, self->currentToken);
        openElementsPop();
        currentTagToken.ackSelfClosing = true;
        return;
    }
    on(start_tag && oneOff(HTML.TagNames.hr))
    {
        if(OpenElementsHasInButtonScope(HTML.TagNames.p))
        {
            closePElement();
        }

        insert_html_element(self, self->currentToken);
        openElementsPop();
        currentTagToken.ackSelfClosing = true;
        self->flags.framesetOk = false;
        return;
    }
    on(start_tag && oneOff(HTML.TagNames.image))
    {
        parser_error("generic");
        currentTagToken.name->clear();
        currentTagToken.name->append(HTML.TagNames.img);
        switchTo(InBody);
        reprocessToken();
        return;
    }
    // TODO: the rest
    on(start_tag)
    {
        reconstruct_active_formatting_elements();
        insertHtmlElementFor(self->currentToken);
        return;
    }
    on(end_tag)
    {  
        size_t len = self->openElements->length();
        for(int64_t i = len - 1; i >= 0; i--)
        {
            auto node = self->openElements->at(i);
            if(node->localName()->equals(currentTagToken.name))
            {
                generateImpliedEndTags(currentTagToken.name);
                if(!currentElementNode->localName()->equals(currentTagToken.name))
                {
                    parser_error("generic");
                }
                openElementsPopUntilInclusive(currentTagToken.name);
                break;
            }
            else if(node->localName()->equalsOneOff(HTML.SpecialTagNames))
            {
                parser_error("generic");
                return;
            }
        }
        return;
    }
}

processMode(AfterBody)
{
    on(character && isAsciiWhitespace(currentCharacterToken.value))
    {
        useRulesOf(InBody);
        return;
    }
    on(comment)
    {
        insertCommentAs(lastChild(self->openElements->at(0)));
        return;
    }
    on(doctype)
    {
        parser_error("generic");
        return;
    }
    on(start_tag && oneOff(HTML.TagNames.html))
    {
        useRulesOf(InBody);
        return;
    }
    on(end_tag && oneOff(HTML.TagNames.html))
    {
        // TODO fragment case

        switchTo(AfterAfterBody);
        return;
    }
    on(eof)
    {
        stop_parsing();
        return;
    }
    anythingElse()
    {
        parser_error("generic");
        switchTo(InBody);
        reprocessToken();
        return;
    }
}

processMode(AfterAfterBody)
{
    on(comment)
    {
        insertCommentAs(lastChild(self->document));
        return;
    }
    on(doctype)
    {
        useRulesOf(InBody);
        return;
    }
    on(character && isAsciiWhitespace(currentCharacterToken.value))
    {
        useRulesOf(InBody);
        return;
    }
    on(start_tag && oneOff(HTML.TagNames.html))
    {
        useRulesOf(InBody);
        return;
    }
    on(eof)
    {
        stop_parsing();
        return;
    }
    anythingElse()
    {
        parser_error("generic");
        switchTo(InBody);
        reprocessToken();
        return;
    }
}

processMode(Text)
{
    on(character)
    {
        insertCharacter();
        return;
    }
    on(eof)
    {
        parser_error("generic");
        if(currentElementNode->localName()->equals(HTML.TagNames.script))
        {
            // TODO: set its already started flag to true
        }
        openElementsPop();
        switchToOriginalInsertionMode();
        return;
    }
    on(end_tag && oneOff(HTML.TagNames.script))
    {
        // TODO implement this
        assert(false);
    }
    anythingElse()
    {
        openElementsPop();
        switchToOriginalInsertionMode();
        return;
    }
}

processMode(InTemplate)
{
    assert(false);
}

processMode(InTable)
{
    assert(false);
}

static __DOM_Node_Document* HTMLParser_parse(HTMLParser* self)
{
    do
    {
        if(self->reprocess == true)
        {
            self->reprocess = false;
        }
        else
        {
            self->currentToken = self->tokenizer->EmitToken();
        }

        switch(self->insertionMode)
        {
            matchMode(Initial);
            matchMode(BeforeHTML);
            matchMode(BeforeHead);
            matchMode(InHead);
            matchMode(InHeadNoScript);
            matchMode(Text);
            matchMode(AfterHead);
            matchMode(InBody);
            matchMode(AfterBody);
            matchMode(AfterAfterBody);
            matchMode(InTable);
            matchMode(InTemplate);

            case HTMLParser_insertion_mode_NONE:
                assert(false);
        }
        
        if(self->reprocess == false)
            self->tokenizer->DisposeToken(self->currentToken);

    } while(!is_parsing_stopped);

    return self->document;
}

static void HTMLParser_destructor(HTMLParser* self)
{
    self->tokenizer->delete();
    self->openElements->delete();
    self->super.destruct();
}

HTMLParser* HTMLParser_new(WCStream* stream)
{
    HTMLParser* self = ObjectBase(HTMLParser, 1);

    self->tokenizer = HTMLTokenizer_new(stream);
    self->insertionMode = HTMLParser_insertion_mode_Initial;
    self->originalInsertionMode = HTMLParser_insertion_mode_NONE;
    self->currentToken = NULL;
    self->document = DOM.Document.new();
    self->head = NULL;
    self->openElements = StackOfOpenElements_new();
    self->templateInsertionModes = Stack_new();

    self->reprocess = false;
    self->flags.parserCannotChangeTheMode = false;
    self->flags.framesetOk = true;
    self->flags.scripting = false;

    ObjectFunction(HTMLParser, parse, 0);

    Object_prepare((Object*)&self->super);
    return self;
}

