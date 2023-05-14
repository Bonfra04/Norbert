#include "parser.h"

#include "tokenizer.h"
#include "codepoint.h"
#include "html.h"

#include "errors.h"
#include "utils/vector.h"
#include "utils/wstring.h"
#include "utils/stack.h"

#include <assert.h>

typedef struct adjusted_insertion_location
{
    type(DOM.Node)* parent;
    size_t index;
} adjusted_insertion_location_t;

typedef enum insertion_modes
{
    insertion_mode_Initial,

    insertion_mode_BeforeHTML,
    insertion_mode_BeforeHead,
    insertion_mode_InHead,
    insertion_mode_InHeadNoscript,
    insertion_mode_AfterHead,
    
    insertion_mode_InBody,
    insertion_mode_AfterBody,
    insertion_mode_AfterAfterBody,
    
    insertion_mode_InTemplate,

    insertion_mode_InTable,

    insertion_mode_Text,

    insertion_mode_NONE,
} insertion_modes_t;

#define locationLastChild(node) ((adjusted_insertion_location_t){node, vector_length(node->childNodes)})
#define locationDefault ((adjusted_insertion_location_t){NULL, 0})

#define matchMode(imode) case insertion_mode_##imode: process_##imode(); break
#define processMode(mode) static void process_##mode()

#define on(ttype) if(currentToken->type == token_##ttype)
#define anythingElse() anythingElse:
#define actAsAnythingElse() goto anythingElse

#define switchTo(imode) do { insertion_mode = insertion_mode_##imode; return; } while(0)
#define switchToOriginalInsertionMode() do { insertion_mode = original_insertion_mode; return; } while(0)
#define reprocessIn(imode) do { insertion_mode = insertion_mode_##imode; process_##imode(); return; } while(0)
#define useRulesOf(imode) process_##imode()
#define ignoreToken() return

#define currentCommentToken (currentToken->as.comment)
#define currentDoctypeToken (currentToken->as.doctype)
#define currentCharacterToken (currentToken->as.character)
#define currentTagToken (currentToken->as.tag)

#define currentNode open_elements[vector_length(open_elements) - 1]
#define currentDocumentTypeNode (currentNode->as.DocumentType)
#define currentElementNode (currentNode->as.Element)
#define appendChild(node, child) do { type(DOM.Node)* _ = child; vector_append(node->childNodes, &_); } while(0)

#define openElementsPush(element) do { type(DOM.Node)* _ = element; vector_append(open_elements, &_); } while(0)
#define openElementsPop() vector_pop(open_elements)
#define openElementsRemove(element) vector_remove_first(open_elements, element)
#define openElementsContains(element) ({                                                                \
    bool _(void* e) { type(DOM.Node)* E = *(type(DOM.Node)**)(e); return wcscmp(E->as.Element.localName, element) == 0; }    \
    vector_find_first(open_elements, _);                                                                \
})

#define insertCharacter() insert_character(NULL)

#define activeFormattingElementsAddMarker() do { type(DOM.Node)* marker = NULL; vector_append(active_formatting_elements, &marker); } while(0)
#define activeFormattingElementsClearUpToMarker() do {                 \
while(vector_length(active_formatting_elements) > 0) {                  \
    if(vector_pop(active_formatting_elements) == NULL)                  \
        break;                                                          \
}} while(0)

static insertion_modes_t insertion_mode;
static insertion_modes_t original_insertion_mode;
static type(DOM.Node)* document;
static type(DOM.Node)** open_elements;
static token_t* currentToken;
static type(DOM.Node)* head;
static bool frameset_ok;
static bool parser_cannot_change_mode;
static bool scripting;
static type(DOM.Node)** active_formatting_elements;
static stack_t template_insertion_modes;

static adjusted_insertion_location_t appropriate_place_for_inserting_node(type(DOM.Node)* overrideTarget)
{
    // https://html.spec.whatwg.org/#appropriate-place-for-inserting-a-node

    // 1. If there was an override target specified, then let target be the override target.
    //    Otherwise, let target be the current node.
    type(DOM.Node)* target = overrideTarget ?: currentNode;

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
        location.index = vector_length(target->childNodes);
    }

    // 3. If the adjusted insertion location is inside a template element, let it instead be inside the template element's template contents, after its last child (if any).
    // TODO

    // 4. Return the adjusted insertion location.
    return location;
}

static void insert_node_at_location(type(DOM.Node)* node, adjusted_insertion_location_t location)
{
    vector_insert(location.parent->childNodes, location.index, &node);
}

static void insert_comment(adjusted_insertion_location_t position)
{
    wchar_t* data = currentCommentToken.data;
    adjusted_insertion_location_t adjustedInsertionLocation = position.parent ? position : appropriate_place_for_inserting_node(NULL);
    type(DOM.Node)* comment = DOM.Node.Comment.new(adjustedInsertionLocation.parent->ownerDocument, data);
    insert_node_at_location(comment, adjustedInsertionLocation);
}

static type(DOM.Node)* create_element_for(token_t* token, type(DOM.Node)* intendedParent)
{
    // https://html.spec.whatwg.org/#create-an-element-for-the-token

    // TODO 1, 2

    // 3. Let document be intended parent's node document.
    type(DOM.Node)* document = intendedParent->ownerDocument;

    // 4. Let local name be the tag name of the token.
    DOMString localName = token->as.tag.name;

    // 5. Let is be the value of the "is" attribute in the given token, if such an attribute exists, or null otherwise.
    // TODO

    // TODO 6

    // 7. If definition is non-null and the parser was not created as part of the HTML fragment parsing algorithm, then let will execute script be true. Otherwise, let it be false
    bool will_execute_script = false; // TODO

    // 8. If will execute script is true, then:
    if(will_execute_script)
    {
        // TODO
    }

    // 9. Let element be the result of creating an element given document, localName, given namespace, null, and is. If will execute script is true, set the synchronous custom elements flag; otherwise, leave it unset.
    type(DOM.Node)* element = DOM.create_element(document, localName); // TODO

    // 10. Append each attribute in the given token to element.
    for(size_t i = 0; i < vector_length(token->as.tag.attributes); i++)
    {
        token_attribute_t* attribute = &token->as.tag.attributes[i];
        type(DOM.Node)* attr = DOM.Node.Attr.new(document, attribute->name, attribute->value);
        DOM.NamedNodeMap.setNamedItem(element->as.Element.attributes, attr);
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

static type(DOM.Node)* insert_foreign_element(token_t* token)
{
    type(DOM.Node)* element = create_element_for(token, currentNode);
    appendChild(currentNode, element);
    openElementsPush(element);
    return element;
}

static type(DOM.Node)* insert_html_element(token_t* token)
{
    return insert_foreign_element(token);
}

static void insert_character(wchar_t* characters)
{
    adjusted_insertion_location_t adjustedInsertionLocation = appropriate_place_for_inserting_node(NULL);
    if(adjustedInsertionLocation.parent->nodeType == DOM.Node.DOCUMENT_NODE)
        return;
    
    wstring_t data = wstring_new();
    if(characters)
        wstring_appends(data, characters);
    else
        wstring_append(data, currentCharacterToken.value);

    type(DOM.Node)** siblings = adjustedInsertionLocation.parent->childNodes;
    if(vector_length(siblings) > 0 && siblings[adjustedInsertionLocation.index - 1]->nodeType == DOM.Node.TEXT_NODE)
    {
        type(DOM.Node)* textNode = siblings[adjustedInsertionLocation.index - 1];
        wstring_appends(textNode->as.Text.wholeText, data);
    }
    else
    {
        type(DOM.Node)* textNode = DOM.Node.Text.new(adjustedInsertionLocation.parent->ownerDocument, data);
        insert_node_at_location(textNode, adjustedInsertionLocation);
    }

    wstring_free(data);
}

static void reconstruct_active_formatting_elements()
{
    size_t len = vector_length(active_formatting_elements);
    if(len == 0)
        return;

    // TODO: fully implement this
    assert(false);
}

static bool one_off(DOMString what, wchar_t** options)
{
    for(wchar_t** option = options; *option; option++)
        if(wcscmp(what, *option) == 0)
            return true;
    return false;
}
#define oneOff(...) one_off(currentTagToken.name, (wchar_t*[]){__VA_ARGS__, NULL})

static bool open_elements_has_in_scope_specific_str(DOMString tagName, DOMString* scope)
{
    for(int64_t i = vector_length(open_elements); i > 0; i--)
    {
        type(DOM.Node)* node = open_elements[i - 1];
        if(wcscmp(node->as.Element.localName, tagName) == 0)
            return true;
        DOMString* list = scope;
        while(*list)
        {
            if(wcscmp(*list, node->as.Element.localName) == 0)
                return false;
            list++;
        }
    }
}

#define basicScopeList HTML.TagNames.applet, HTML.TagNames.caption, HTML.TagNames.html, HTML.TagNames.table, HTML.TagNames.td, HTML.TagNames.th, HTML.TagNames.marquee, HTML.TagNames.object, HTML.TagNames.template

static bool open_elements_has_in_scope_str(DOMString tagName)
{
    DOMString base_list[] = { basicScopeList, NULL };
    return open_elements_has_in_scope_specific_str(tagName, base_list);
}

static bool open_elements_has_in_button_scope_str(DOMString tagName)
{
    DOMString base_list[] = { HTML.TagNames.button, basicScopeList, NULL };
    return open_elements_has_in_scope_specific_str(tagName, base_list);
}

#define OpenElementsHasInScope(element) _Generic((element), DOMString: open_elements_has_in_scope_str(element), default: false)
#define OpenElementsHasInButtonScope(element) _Generic((element), DOMString: open_elements_has_in_button_scope_str(element), default: false)

#define openElementsPopUntilInclusive(tagname) do {                 \
while(wcscmp(currentElementNode.localName, HTML.TagNames.p) != 0)   \
    openElementsPop();                                              \
openElementsPop();                                                  \
} while(0)

#define openElementsPopUntilOneOffInclusive(...) do {                           \
while(!one_off(currentElementNode.localName, (wchar_t*[]){__VA_ARGS__, NULL}))  \
    openElementsPop();                                                          \
openElementsPop();                                                              \
} while(0)

static void generate_implied_end_tag(DOMString exception)
{
    DOMString implied[] = { HTML.TagNames.dd, HTML.TagNames.dt, HTML.TagNames.li, HTML.TagNames.optgroup, HTML.TagNames.option, HTML.TagNames.p, HTML.TagNames.rb, HTML.TagNames.rp, HTML.TagNames.rt, HTML.TagNames.rtc, NULL };

    while(one_off(currentElementNode.localName, implied) && (!exception || wcscmp(currentElementNode.localName, exception) != 0))
        openElementsPop();
}

static void close_p_element()
{
    generate_implied_end_tag(HTML.TagNames.p);
    
    if(wcscmp(currentElementNode.localName, HTML.TagNames.p) != 0)
        parser_error("generic");

    while(wcscmp(currentElementNode.localName, HTML.TagNames.p) != 0)
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
processMode(InHeadNoscript);
processMode(AfterHead);
processMode(InBody);
processMode(AfterBody);
processMode(Text);
processMode(InTemplate);
processMode(InTable);
processMode(AfterAfterBody);

#define insertComment() do { insert_comment(locationDefault); return; } while(0)
#define insertCommentAt(location) do { insert_comment(location); return; } while(0)

processMode(Initial)
{
    on(character && isAsciiWhitespace(currentCharacterToken.value))
    {
        ignoreToken();
    }
    on(comment)
    {
        insertCommentAt(locationLastChild(document));
    }
    on(doctype)
    {
        if (wcscmp(currentDoctypeToken.name, L"html") != 0 || currentDoctypeToken.public_identifier != NULL || (currentDoctypeToken.system_identifier != NULL && wcscmp(currentDoctypeToken.system_identifier, L"about:legacy-compat") != 0))
            parser_error("generic");

        type(DOM.Node)* doctype = DOM.Node.DocumentType.new(document, currentDoctypeToken.name, currentDoctypeToken.public_identifier, currentDoctypeToken.system_identifier);
        appendChild(document, doctype);

        document->as.Document.mode = no_quirks;
        if ( /* TODO not an iframe srcdoc*/ parser_cannot_change_mode == false)
        {
            // TODO: case insentiveness
            if ((currentDoctypeToken.forceQuirks)
                || (wcscmp(currentDoctypeToken.name, L"html") != 0)
                || (currentDoctypeToken.public_identifier && wcscmp(currentDoctypeToken.public_identifier, L"-//W3O//DTD W3 HTML Strict 3.0//EN//") == 0)
                || (currentDoctypeToken.public_identifier && wcscmp(currentDoctypeToken.public_identifier, L"-/W3C/DTD HTML 4.0 Transitional/EN") == 0)
                || (currentDoctypeToken.public_identifier && wcscmp(currentDoctypeToken.public_identifier, L"HTML") == 0)
                || (currentDoctypeToken.system_identifier && wcscmp(currentDoctypeToken.system_identifier, L"http://www.ibm.com/data/dtd/v11/ibmxhtml1-transitional.dtd") == 0)
                || (currentDoctypeToken.public_identifier && wcsncmp(currentDoctypeToken.public_identifier, L"+//Silmaril//dtd html Pro v0r11 19970101//", 42) == 0)
                || (currentDoctypeToken.public_identifier && wcsncmp(currentDoctypeToken.public_identifier, L"-//AS//DTD HTML 3.0 asWedit + extensions//", 42) == 0)
                || (currentDoctypeToken.public_identifier && wcsncmp(currentDoctypeToken.public_identifier, L"-//AdvaSoft Ltd//DTD HTML 3.0 asWedit + extensions//", 52) == 0)
                || (currentDoctypeToken.public_identifier && wcsncmp(currentDoctypeToken.public_identifier, L"-//IETF//DTD HTML 2.0 Level 1//", 31) == 0)
                || (currentDoctypeToken.public_identifier && wcsncmp(currentDoctypeToken.public_identifier, L"-//IETF//DTD HTML 2.0 Level 2//", 31) == 0)
                || (currentDoctypeToken.public_identifier && wcsncmp(currentDoctypeToken.public_identifier, L"-//IETF//DTD HTML 2.0 Strict Level 1//", 38) == 0)
                || (currentDoctypeToken.public_identifier && wcsncmp(currentDoctypeToken.public_identifier, L"-//IETF//DTD HTML 2.0 Strict Level 2//", 38) == 0)
                || (currentDoctypeToken.public_identifier && wcsncmp(currentDoctypeToken.public_identifier, L"-//IETF//DTD HTML 2.0 Strict//", 30) == 0)
                || (currentDoctypeToken.public_identifier && wcsncmp(currentDoctypeToken.public_identifier, L"-//IETF//DTD HTML 2.0//", 23) == 0)
                || (currentDoctypeToken.public_identifier && wcsncmp(currentDoctypeToken.public_identifier, L"-//IETF//DTD HTML 2.1E//", 24) == 0)
                || (currentDoctypeToken.public_identifier && wcsncmp(currentDoctypeToken.public_identifier, L"-//IETF//DTD HTML 3.0//", 23) == 0)
                || (currentDoctypeToken.public_identifier && wcsncmp(currentDoctypeToken.public_identifier, L"-//IETF//DTD HTML 3.2 Final//", 29) == 0)
                || (currentDoctypeToken.public_identifier && wcsncmp(currentDoctypeToken.public_identifier, L"-//IETF//DTD HTML 3.2//", 23) == 0)
                || (currentDoctypeToken.public_identifier && wcsncmp(currentDoctypeToken.public_identifier, L"-//IETF//DTD HTML 3//", 21) == 0)
                || (currentDoctypeToken.public_identifier && wcsncmp(currentDoctypeToken.public_identifier, L"-//IETF//DTD HTML Level 0//", 27) == 0)
                || (currentDoctypeToken.public_identifier && wcsncmp(currentDoctypeToken.public_identifier, L"-//IETF//DTD HTML Level 1//", 27) == 0)
                || (currentDoctypeToken.public_identifier && wcsncmp(currentDoctypeToken.public_identifier, L"-//IETF//DTD HTML Level 2//", 27) == 0)
                || (currentDoctypeToken.public_identifier && wcsncmp(currentDoctypeToken.public_identifier, L"-//IETF//DTD HTML Level 3//", 27) == 0)
                || (currentDoctypeToken.public_identifier && wcsncmp(currentDoctypeToken.public_identifier, L"-//IETF//DTD HTML Strict Level 0//", 34) == 0)
                || (currentDoctypeToken.public_identifier && wcsncmp(currentDoctypeToken.public_identifier, L"-//IETF//DTD HTML Strict Level 1//", 34) == 0)
                || (currentDoctypeToken.public_identifier && wcsncmp(currentDoctypeToken.public_identifier, L"-//IETF//DTD HTML Strict Level 2//", 34) == 0)
                || (currentDoctypeToken.public_identifier && wcsncmp(currentDoctypeToken.public_identifier, L"-//IETF//DTD HTML Strict Level 3//", 34) == 0)
                || (currentDoctypeToken.public_identifier && wcsncmp(currentDoctypeToken.public_identifier, L"-//IETF//DTD HTML Strict//", 26) == 0)
                || (currentDoctypeToken.public_identifier && wcsncmp(currentDoctypeToken.public_identifier, L"-//IETF//DTD HTML//", 19) == 0)
                || (currentDoctypeToken.public_identifier && wcsncmp(currentDoctypeToken.public_identifier, L"-//Metrius//DTD Metrius Presentational//", 40) == 0)
                || (currentDoctypeToken.public_identifier && wcsncmp(currentDoctypeToken.public_identifier, L"-//Microsoft//DTD Internet Explorer 2.0 HTML Strict//", 53) == 0)
                || (currentDoctypeToken.public_identifier && wcsncmp(currentDoctypeToken.public_identifier, L"-//Microsoft//DTD Internet Explorer 2.0 HTML//", 46) == 0)
                || (currentDoctypeToken.public_identifier && wcsncmp(currentDoctypeToken.public_identifier, L"-//Microsoft//DTD Internet Explorer 2.0 Tables//", 48) == 0)
                || (currentDoctypeToken.public_identifier && wcsncmp(currentDoctypeToken.public_identifier, L"-//Microsoft//DTD Internet Explorer 3.0 HTML Strict//", 53) == 0)
                || (currentDoctypeToken.public_identifier && wcsncmp(currentDoctypeToken.public_identifier, L"-//Microsoft//DTD Internet Explorer 3.0 HTML//", 46) == 0)
                || (currentDoctypeToken.public_identifier && wcsncmp(currentDoctypeToken.public_identifier, L"-//Microsoft//DTD Internet Explorer 3.0 Tables//", 48) == 0)
                || (currentDoctypeToken.public_identifier && wcsncmp(currentDoctypeToken.public_identifier, L"-//Netscape Comm. Corp.//DTD HTML//", 35) == 0)
                || (currentDoctypeToken.public_identifier && wcsncmp(currentDoctypeToken.public_identifier, L"-//Netscape Comm. Corp.//DTD Strict HTML//", 42) == 0)
                || (currentDoctypeToken.public_identifier && wcsncmp(currentDoctypeToken.public_identifier, L"-//O'Reilly and Associates//DTD HTML 2.0//", 42) == 0)
                || (currentDoctypeToken.public_identifier && wcsncmp(currentDoctypeToken.public_identifier, L"-//O'Reilly and Associates//DTD HTML Extended 1.0//", 51) == 0)
                || (currentDoctypeToken.public_identifier && wcsncmp(currentDoctypeToken.public_identifier, L"-//O'Reilly and Associates//DTD HTML Extended Relaxed 1.0//", 59) == 0)
                || (currentDoctypeToken.public_identifier && wcsncmp(currentDoctypeToken.public_identifier, L"-//SQ//DTD HTML 2.0 HoTMetaL + extensions//", 43) == 0)
                || (currentDoctypeToken.public_identifier && wcsncmp(currentDoctypeToken.public_identifier, L"-//SoftQuad Software//DTD HoTMetaL PRO 6.0::19990601::extensions to HTML 4.0//", 78) == 0)
                || (currentDoctypeToken.public_identifier && wcsncmp(currentDoctypeToken.public_identifier, L"-//SoftQuad//DTD HoTMetaL PRO 4.0::19971010::extensions to HTML 4.0//", 69) == 0)
                || (currentDoctypeToken.public_identifier && wcsncmp(currentDoctypeToken.public_identifier, L"-//Spyglass//DTD HTML 2.0 Extended//", 36) == 0)
                || (currentDoctypeToken.public_identifier && wcsncmp(currentDoctypeToken.public_identifier, L"-//Sun Microsystems Corp.//DTD HotJava HTML//", 45) == 0)
                || (currentDoctypeToken.public_identifier && wcsncmp(currentDoctypeToken.public_identifier, L"-//Sun Microsystems Corp.//DTD HotJava Strict HTML//", 52) == 0)
                || (currentDoctypeToken.public_identifier && wcsncmp(currentDoctypeToken.public_identifier, L"-//W3C//DTD HTML 3 1995-03-24//", 31) == 0)
                || (currentDoctypeToken.public_identifier && wcsncmp(currentDoctypeToken.public_identifier, L"-//W3C//DTD HTML 3.2 Draft//", 28) == 0)
                || (currentDoctypeToken.public_identifier && wcsncmp(currentDoctypeToken.public_identifier, L"-//W3C//DTD HTML 3.2 Final//", 28) == 0)
                || (currentDoctypeToken.public_identifier && wcsncmp(currentDoctypeToken.public_identifier, L"-//W3C//DTD HTML 3.2//", 22) == 0)
                || (currentDoctypeToken.public_identifier && wcsncmp(currentDoctypeToken.public_identifier, L"-//W3C//DTD HTML 3.2S Draft//", 29) == 0)
                || (currentDoctypeToken.public_identifier && wcsncmp(currentDoctypeToken.public_identifier, L"-//W3C//DTD HTML 4.0 Frameset//", 31) == 0)
                || (currentDoctypeToken.public_identifier && wcsncmp(currentDoctypeToken.public_identifier, L"-//W3C//DTD HTML 4.0 Transitional//", 35) == 0)
                || (currentDoctypeToken.public_identifier && wcsncmp(currentDoctypeToken.public_identifier, L"-//W3C//DTD HTML Experimental 19960712//", 40) == 0)
                || (currentDoctypeToken.public_identifier && wcsncmp(currentDoctypeToken.public_identifier, L"-//W3C//DTD HTML Experimental 970421//", 38) == 0)
                || (currentDoctypeToken.public_identifier && wcsncmp(currentDoctypeToken.public_identifier, L"-//W3C//DTD W3 HTML//", 21) == 0)
                || (currentDoctypeToken.public_identifier && wcsncmp(currentDoctypeToken.public_identifier, L"-//W3O//DTD W3 HTML 3.0//", 25) == 0)
                || (currentDoctypeToken.public_identifier && wcsncmp(currentDoctypeToken.public_identifier, L"-//WebTechs//DTD Mozilla HTML 2.0//", 35) == 0)
                || (currentDoctypeToken.public_identifier && wcsncmp(currentDoctypeToken.public_identifier, L"-//WebTechs//DTD Mozilla HTML//" , 31) == 0)
                || (currentDoctypeToken.system_identifier == NULL && currentDoctypeToken.public_identifier && wcsncmp(currentDoctypeToken.public_identifier, L"-//W3C//DTD HTML 4.01 Frameset//", 32) == 0)
                || (currentDoctypeToken.system_identifier == NULL && currentDoctypeToken.public_identifier && wcsncmp(currentDoctypeToken.public_identifier, L"-//W3C//DTD HTML 4.01 Transitional//", 36) == 0)
            )
                document->as.Document.mode = quirks;
            else if (
                (currentDoctypeToken.public_identifier && wcsncmp(currentDoctypeToken.public_identifier, L"-//W3C//DTD XHTML 1.0 Frameset//" , 32) == 0)
                || (currentDoctypeToken.public_identifier && wcsncmp(currentDoctypeToken.public_identifier, L"-//W3C//DTD XHTML 1.0 Transitional//", 36) == 0)
                || (currentDoctypeToken.system_identifier && currentDoctypeToken.public_identifier && wcsncmp(currentDoctypeToken.public_identifier, L"-//W3C//DTD HTML 4.01 Frameset//", 32) == 0)
                || (currentDoctypeToken.system_identifier && currentDoctypeToken.public_identifier && wcsncmp(currentDoctypeToken.public_identifier, L"-//W3C//DTD HTML 4.01 Transitional//", 36) == 0)
            )
                document->as.Document.mode = limited_quirks;
            else
                document->as.Document.mode = no_quirks;
        }

        switchTo(BeforeHTML);
    }
    anythingElse()
    {
        // TODO: not an iframe srcdoc document
        if(parser_cannot_change_mode == false)
            document->as.Document.mode = quirks;

        reprocessIn(BeforeHTML);
    }
}

processMode(BeforeHTML)
{
    on(doctype)
    {
        parser_error("generic");
        ignoreToken();
    }
    on(comment)
    {
        insertCommentAt(locationLastChild(document));
    }
    on(character && isAsciiWhitespace(currentCharacterToken.value))
    {
        ignoreToken();
    }
    on(start_tag && oneOff(HTML.TagNames.html))
    {
        type(DOM.Node)* html = create_element_for(currentToken, document);
        appendChild(document, html);
        openElementsPush(html);
        switchTo(BeforeHead);
    }
    on(end_tag && oneOff(HTML.TagNames.head, HTML.TagNames.body, HTML.TagNames.html, HTML.TagNames.br))
    {
        actAsAnythingElse();
    }
    on(end_tag)
    {
        parser_error("generic");
        ignoreToken();
    }
    anythingElse()
    {
        type(DOM.Node)* html = DOM.create_element(document, HTML.TagNames.html);
        appendChild(document, html);
        openElementsPush(html);
        reprocessIn(BeforeHead);
    }
}

processMode(BeforeHead)
{
    on(character && isAsciiWhitespace(currentCharacterToken.value))
    {
        ignoreToken();
    }
    on(comment)
    {
        insertComment();
    }
    on(doctype)
    {
        parser_error("generic");
        ignoreToken();
    }
    on(start_tag && oneOff(HTML.TagNames.html))
    {
        useRulesOf(InBody);
        return;
    }
    on(start_tag && oneOff(HTML.TagNames.head))
    {
        type(DOM.Node)* element = insert_html_element(currentToken);
        head = element;
        switchTo(InHead);
    }
    on(end_tag && oneOff(HTML.TagNames.head, HTML.TagNames.body, HTML.TagNames.html, HTML.TagNames.br))
    {
        actAsAnythingElse();
    }
    on(end_tag)
    {
        parser_error("generic");
        ignoreToken();
    }
    anythingElse()
    {
        token_t dummyHead = { .type = token_start_tag, .as.start_tag = { .name = HTML.TagNames.head, .selfClosing = false, .attributes = NULL } };
        type(DOM.Node)* element = insert_html_element(&dummyHead);
        head = element;
        reprocessIn(InHead);
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
    }
    on(doctype)
    {
        parser_error("generic");
        ignoreToken();
    }
    on(start_tag && oneOff(HTML.TagNames.html))
    {
        useRulesOf(InBody);
        return;
    }
    on(start_tag && oneOff(HTML.TagNames.base, HTML.TagNames.basefont, HTML.TagNames.bgsound, HTML.TagNames.link))
    {
        insert_html_element(currentToken);
        openElementsPop();
        currentTagToken.ackSelfClosing = true;
        return;
    }
    on(start_tag && oneOff(HTML.TagNames.meta))
    {
        insert_html_element(currentToken);
        openElementsPop();
        currentTagToken.ackSelfClosing = true;
        // TODO: charset and http-equiv

        return;
    }
    on(start_tag && oneOff(HTML.TagNames.title))
    {
        insert_html_element(currentToken);
        tokenizer_switch_to(state_RCDATA);
        original_insertion_mode = insertion_mode;
        switchTo(Text);
    }
    on(start_tag && oneOff(HTML.TagNames.noscript) && scripting == true)
    {
        insert_html_element(currentToken);
        tokenizer_switch_to(state_RAWTEXT);
        original_insertion_mode = insertion_mode;
        switchTo(Text);
    }
    on(start_tag && oneOff(HTML.TagNames.noframes, HTML.TagNames.style))
    {
        insert_html_element(currentToken);
        tokenizer_switch_to(state_RAWTEXT);
        original_insertion_mode = insertion_mode;
        switchTo(Text);
    }
    on(start_tag && oneOff(HTML.TagNames.noscript) && scripting == false)
    {
        insert_html_element(currentToken);
        switchTo(InHeadNoscript);
    }
    // TODO: script
    on(end_tag && oneOff(HTML.TagNames.head))
    {
        openElementsPop();
        switchTo(AfterHead);
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
    }
    on(end_tag)
    {
        parser_error("generic");
        ignoreToken();
    }
    anythingElse()
    {
        openElementsPop();
        reprocessIn(AfterHead);
    }
}

processMode(InHeadNoscript)
{
    on(doctype)
    {
        parser_error("generic");
        ignoreToken();
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
    }
    on(character && isAsciiWhitespace(currentCharacterToken.value))
    {
        useRulesOf(InHead);
        return;
    }
    on(comment)
    {
        useRulesOf(InHead);
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
    }
    on(end_tag)
    {
        parser_error("generic");
        ignoreToken();
    }
    anythingElse()
    {
        parser_error("generic");
        openElementsPop();
        reprocessIn(InHead);
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
    }
    on(doctype)
    {
        parser_error("generic");
        ignoreToken();
    }
    on(start_tag && oneOff(HTML.TagNames.html))
    {
        useRulesOf(InBody);
        return;
    }
    on(start_tag && oneOff(HTML.TagNames.body))
    {
        insert_html_element(currentToken);
        frameset_ok = false;
        switchTo(InBody);
    }
    // TODO: frameset
    on(start_tag && oneOff(HTML.TagNames.base, HTML.TagNames.basefont, HTML.TagNames.bgsound, HTML.TagNames.link, HTML.TagNames.meta, HTML.TagNames.noframes, HTML.TagNames.script, HTML.TagNames.style, HTML.TagNames.template, HTML.TagNames.title))
    {
        parser_error("generic");
        openElementsPush(head);
        useRulesOf(InHead);
        openElementsRemove(head);
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
    }
    on(end_tag)
    {
        parser_error("generic");
        ignoreToken();
    }
    anythingElse()
    {
        token_t dummyBody = { .type = token_start_tag, .as.start_tag = { .name = HTML.TagNames.body, .selfClosing = false, .attributes = vector_new(sizeof(token_attribute_t)) } };
        insert_html_element(&dummyBody);
        reprocessIn(InBody);
    }
}

processMode(InBody)
{
    on(character && currentCharacterToken.value == L'\0')
    {
        parser_error("generic");
        ignoreToken();
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
        frameset_ok = false;
        return;
    }
    on(comment)
    {
        insertComment();
    }
    on(doctype)
    {
        parser_error("generic");
        ignoreToken();
    }
    on(start_tag && oneOff(HTML.TagNames.html))
    {
        parser_error("generic");

        if(openElementsContains(HTML.TagNames.template))
            ignoreToken();

        size_t nattrs = vector_length(currentTagToken.attributes);
        for(size_t i = 0; i < nattrs; i++)
            if(DOM.NamedNodeMap.getNamedItem(open_elements[0]->as.Element.attributes, currentTagToken.attributes[i].name) == NULL)
            {
                type(DOM.Node)* attr = DOM.Node.Attr.new(open_elements[0]->ownerDocument, currentTagToken.attributes[i].name, currentTagToken.attributes[i].value);
                DOM.NamedNodeMap.setNamedItem(open_elements[0]->as.Element.attributes, attr);
            }

        return;
    }
    on(start_tag && oneOff(HTML.TagNames.base, HTML.TagNames.basefont, HTML.TagNames.bgsound, HTML.TagNames.link, HTML.TagNames.meta, HTML.TagNames.noframes, HTML.TagNames.script, HTML.TagNames.style, HTML.TagNames.template, HTML.TagNames.title))
    {
        useRulesOf(InHead);
        return;
    }
    on(start_tag && oneOff(HTML.TagNames.body))
    {
        parser_error("generic");

       // TODO: fragment case

       frameset_ok = false;
       for (size_t i = 0; i < vector_length(currentTagToken.attributes); i++)
           if (DOM.NamedNodeMap.getNamedItem(open_elements[0]->as.Element.attributes, currentTagToken.attributes[i].name) == NULL)
           {
               type(DOM.Node)* attr = DOM.Node.Attr.new(open_elements[0]->ownerDocument, currentTagToken.attributes[i].name, currentTagToken.attributes[i].value);
               DOM.NamedNodeMap.setNamedItem(open_elements[0]->as.Element.attributes, attr);
           }

        return;
    }
    // TODO: frameset    
    on(eof)
    {
        if(template_insertion_modes.size > 0)
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
            parser_error("generic");

        switchTo(AfterBody);
    }
    on(end_tag && oneOff(HTML.TagNames.html))
    {
        if(!OpenElementsHasInScope(HTML.TagNames.body))
        {
            parser_error("generic");
            ignoreToken();
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

        reprocessIn(AfterBody);
    }
    on(start_tag && oneOff(HTML.TagNames.address, HTML.TagNames.article, HTML.TagNames.aside, HTML.TagNames.blockquote, HTML.TagNames.center, HTML.TagNames.details, HTML.TagNames.dialog, HTML.TagNames.dir, HTML.TagNames.div, HTML.TagNames.dl, HTML.TagNames.fieldset, HTML.TagNames.figcaption, HTML.TagNames.figure, HTML.TagNames.footer, HTML.TagNames.header, HTML.TagNames.hgroup, HTML.TagNames.main, HTML.TagNames.menu, HTML.TagNames.nav, HTML.TagNames.ol, HTML.TagNames.p, HTML.TagNames.search, HTML.TagNames.section, HTML.TagNames.summary, HTML.TagNames.ul))
    {
        if(OpenElementsHasInButtonScope(HTML.TagNames.p))
            close_p_element();
        
        insert_html_element(currentToken);
        return;
    }
    on(start_tag && oneOff(HTML.TagNames.h1, HTML.TagNames.h2, HTML.TagNames.h3, HTML.TagNames.h4, HTML.TagNames.h5, HTML.TagNames.h6))
    {
        if(OpenElementsHasInButtonScope(HTML.TagNames.p))
            close_p_element();

        if(one_off(currentElementNode.localName, (wchar_t*[]){HTML.TagNames.h2, HTML.TagNames.h3, HTML.TagNames.h4, HTML.TagNames.h5, HTML.TagNames.h6}))
        {
            parser_error("generic");
            openElementsPop();
        }

        insert_html_element(currentToken);
        return;
    }
    on(start_tag && oneOff(HTML.TagNames.pre, HTML.TagNames.listing))
    {
        if(OpenElementsHasInButtonScope(HTML.TagNames.p))
            close_p_element();

        insert_html_element(currentToken);

        // TODO: next token things

        frameset_ok = false;
        return;
    }
    // TODO: form
    on(start_tag && oneOff(HTML.TagNames.li))
    {
        frameset_ok = false;

        for (int64_t i = vector_length(open_elements) - 1; i >= 0; i--)
        {
            type(DOM.Node)* node = open_elements[i];

            if(wcscmp(node->as.Element.localName, HTML.TagNames.li) == 0)
            {
                generate_implied_end_tag(HTML.TagNames.li);
                if(wcscmp(currentElementNode.localName, HTML.TagNames.li) != 0)
                    parser_error("generic");
                openElementsPopUntilInclusive(HTML.TagNames.li);
                break;
            }
            if(one_off(node->as.Element.localName, HTML.SpecialTagNames) && !one_off(node->as.Element.localName, (wchar_t*[]){HTML.TagNames.address, HTML.TagNames.div, HTML.TagNames.p}))
                break;
        }

        if(OpenElementsHasInButtonScope(HTML.TagNames.p))
            close_p_element();

        insert_html_element(currentToken);
        return;
    }
    on(start_tag && oneOff(HTML.TagNames.dd, HTML.TagNames.dt))
    {
        frameset_ok = false;
    
        for (int64_t i = vector_length(open_elements) - 1; i >= 0; i--)
        {
            type(DOM.Node)* node = open_elements[i];

            if(wcscmp(node->as.Element.localName, HTML.TagNames.dd) == 0)
            {
                generate_implied_end_tag(HTML.TagNames.dd);
                if(wcscmp(currentElementNode.localName, HTML.TagNames.dd) != 0)
                    parser_error("generic");
                openElementsPopUntilInclusive(HTML.TagNames.dd);
                break;
            }
            if(wcscmp(node->as.Element.localName, HTML.TagNames.dt) == 0)
            {
                generate_implied_end_tag(HTML.TagNames.dt);
                if(wcscmp(currentElementNode.localName, HTML.TagNames.dt) != 0)
                    parser_error("generic");
                openElementsPopUntilInclusive(HTML.TagNames.dt);
                break;
            }
            if(one_off(node->as.Element.localName, HTML.SpecialTagNames) && !one_off(node->as.Element.localName, (wchar_t*[]){HTML.TagNames.address, HTML.TagNames.div, HTML.TagNames.p}))
                break;
        }

        if(OpenElementsHasInButtonScope(HTML.TagNames.p))
            close_p_element();

        insert_html_element(currentToken);
        return;
    }
    // TODO: plaintext
    on(start_tag && oneOff(HTML.TagNames.button))
    {
        if(OpenElementsHasInScope(HTML.TagNames.button))
        {
            parser_error("generic");
            generate_implied_end_tag(NULL);
            openElementsPopUntilInclusive(HTML.TagNames.button);
        }
        reconstruct_active_formatting_elements();
        insert_html_element(currentToken);
        frameset_ok = false;
        return;
    }
    on(end_tag && oneOff(HTML.TagNames.address, HTML.TagNames.article, HTML.TagNames.aside, HTML.TagNames.blockquote, HTML.TagNames.button, HTML.TagNames.center, HTML.TagNames.details, HTML.TagNames.dialog, HTML.TagNames.dir, HTML.TagNames.div, HTML.TagNames.dl, HTML.TagNames.fieldset, HTML.TagNames.figcaption, HTML.TagNames.figure, HTML.TagNames.footer, HTML.TagNames.header, HTML.TagNames.hgroup, HTML.TagNames.listing, HTML.TagNames.main, HTML.TagNames.menu, HTML.TagNames.nav, HTML.TagNames.ol, HTML.TagNames.pre, HTML.TagNames.search, HTML.TagNames.section, HTML.TagNames.summary, HTML.TagNames.ul))
    {
        if(!OpenElementsHasInScope(currentTagToken.name))
        {
            parser_error("generic");
            ignoreToken();
        }
        else
        {
            generate_implied_end_tag(NULL);
            if(wcscmp(currentElementNode.localName, currentTagToken.name) != 0)
                parser_error("generic");
            openElementsPopUntilInclusive(currentTagToken.name);
        }
        return;
    }
    // TODO: form
    on(end_tag && oneOff(HTML.TagNames.p))
    {
        if(!OpenElementsHasInButtonScope(HTML.TagNames.p))
        {
            parser_error("generic");
            token_t dummyP = { .type = token_start_tag, .as.start_tag = { .name = HTML.TagNames.p, .selfClosing = false, .attributes = vector_new(sizeof(token_attribute_t)) } };
            insert_html_element(currentToken);
        }
        close_p_element();
        return;
    }
    on(end_tag && oneOff(HTML.TagNames.li))
    {
        if(!OpenElementsHasInScope(HTML.TagNames.li))
        {
            parser_error("generic");
            ignoreToken();
        }

        generate_implied_end_tag(HTML.TagNames.li);
        if(wcscmp(currentElementNode.localName, HTML.TagNames.li) != 0)
            parser_error("generic");
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

        generate_implied_end_tag(currentTagToken.name);
        if(wcscmp(currentElementNode.localName, currentTagToken.name) != 0)
            parser_error("generic");
        openElementsPopUntilInclusive(currentTagToken.name);
        return;
    }
    on(start_tag && oneOff(HTML.TagNames.h1, HTML.TagNames.h2, HTML.TagNames.h3, HTML.TagNames.h4, HTML.TagNames.h5, HTML.TagNames.h6))
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
        }

        generate_implied_end_tag(NULL);
        if(wcscmp(currentElementNode.localName, currentTagToken.name) != 0)
            parser_error("generic");
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
        if(document->as.Document.mode != quirks && OpenElementsHasInButtonScope(HTML.TagNames.p))
            close_p_element();

        insert_html_element(currentToken);
        frameset_ok = false;
        switchTo(InTable);
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
        insert_html_element(currentToken);
        openElementsPop();
        currentTagToken.ackSelfClosing = true;
        frameset_ok = false;
        return;
    }
    on(start_tag && oneOff(HTML.TagNames.input))
    {
        reconstruct_active_formatting_elements();
        insert_html_element(currentToken);
        openElementsPop();
        currentTagToken.ackSelfClosing = true;

        size_t len = vector_length(currentTagToken.attributes);
        for(size_t i = 0; i < len; i++)
        {
            token_attribute_t* attr = &currentTagToken.attributes[i];
            if(wcscmp(attr->name, L"type") == 0)
            {
                if(wcscmp(attr->value, L"hidden") == 0)
                    frameset_ok = false;
                break;
            }
        }
        return;
    }
    on(start_tag && oneOff(HTML.TagNames.param, HTML.TagNames.source, HTML.TagNames.track))
    {
        insert_html_element(currentToken);
        openElementsPop();
        currentTagToken.ackSelfClosing = true;
        return;
    }
    on(start_tag && oneOff(HTML.TagNames.hr))
    {
        if(OpenElementsHasInButtonScope(HTML.TagNames.p))
            close_p_element();

        insert_html_element(currentToken);
        openElementsPop();
        currentTagToken.ackSelfClosing = true;
        frameset_ok = false;
        return;
    }
    on(start_tag && oneOff(HTML.TagNames.image))
    {
        parser_error("generic");
        wstring_clear(currentTagToken.name);
        wstring_appends(currentTagToken.name, HTML.TagNames.img);
        reprocessIn(InBody);
    }
    // TODO: the rest
    on(start_tag)
    {
        reconstruct_active_formatting_elements();
        insert_html_element(currentToken);
        return;
    }
    on(end_tag)
    {  
        size_t len = vector_length(open_elements);
        for(int64_t i = len - 1; i >= 0; i--)
        {
            type(DOM.Node)* node = open_elements[i];
            if(wcscmp(node->as.Element.localName, currentTagToken.name) == 0)
            {
                generate_implied_end_tag(currentTagToken.name);
                if(wcscmp(currentElementNode.localName, currentTagToken.name) != 0)
                    parser_error("generic");
                openElementsPopUntilInclusive(currentTagToken.name);
                break;
            }
            else if(one_off(node->as.Element.localName, HTML.SpecialTagNames))
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
        insert_comment(locationLastChild(open_elements[0]));
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
    }
    on(eof)
    {
        stop_parsing();
        return;
    }
    anythingElse()
    {
        parser_error("generic");
        reprocessIn(InBody);
    }
}

processMode(AfterAfterBody)
{
    on(comment)
    {
        insert_comment(locationLastChild(document));
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
        reprocessIn(InBody);
    }
}

processMode(Text)
{
    on(character)
    {
        insert_character(NULL);
        return;
    }
    on(eof)
    {
        parser_error("generic");
        if(wcscmp(currentElementNode.localName, HTML.TagNames.script) == 0)
        {
            // TODO: set its already started flag to true
        }
        openElementsPop();
        switchToOriginalInsertionMode();
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

void parser_init()
{
    insertion_mode = insertion_mode_Initial;
    original_insertion_mode = insertion_mode_NONE;
    document = DOM.Node.Document.new();
    open_elements = vector_new(sizeof(type(DOM.Node)*));
    currentToken = NULL;
    head = NULL;
    frameset_ok = true;
    parser_cannot_change_mode = false;
    scripting = false;
    active_formatting_elements = vector_new(sizeof(type(DOM.Node)*));
    template_insertion_modes = stack_create();
}

type(DOM.Node)* parser_parse()
{
    do
    {
        currentToken = tokenizer_emit_token();

        switch (insertion_mode)
        {
            matchMode(Initial);
            matchMode(BeforeHTML);
            matchMode(BeforeHead);
            matchMode(InHead);
            matchMode(Text);
            matchMode(AfterHead);
            matchMode(InBody);
        }
        
        if(currentToken->type == token_eof)
            is_parsing_stopped = true;
        tokenizer_dispose_token(currentToken);

    } while (!is_parsing_stopped);

    return document;
}
