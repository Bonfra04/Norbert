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

    type(DOM.Node)* target = overrideTarget ?: currentNode;

    adjusted_insertion_location_t location;

    if(false)
    {
        // TODO: foster parenting
    }
    else
    {
        location.parent = target;
        location.index = vector_length(target->childNodes);
    }

    // TODO: template
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

    // TODO: fully implement this
    type(DOM.Node)* document = intendedParent->ownerDocument;
    DOMString localName = token->as.tag.name;

    type(DOM.Node)* result = DOM.create_element(document, localName);

    for(size_t i = 0; i < vector_length(token->as.tag.attributes); i++)
    {
        token_attribute_t* attribute = &token->as.tag.attributes[i];
        type(DOM.Node)* attr = DOM.Node.Attr.new(document, attribute->name, attribute->value);
        DOM.NamedNodeMap.setNamedItem(result->as.Element.attributes, attr);
    }

    return result;
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

static bool one_off(wchar_t** options)
{
    for(wchar_t** option = options; *option; option++)
        if(wcscmp(currentTagToken.name, *option) == 0)
            return true;
    return false;
}
#define oneOff(...) one_off((wchar_t*[]){__VA_ARGS__, NULL})

processMode(Initial);
processMode(BeforeHTML);
processMode(BeforeHead);
processMode(InHead);
processMode(InHeadNoscript);
processMode(AfterHead);
processMode(InBody);
processMode(Text);

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
        // TODO: parser errors

        type(DOM.Node)* doctype = DOM.Node.DocumentType.new(document, currentDoctypeToken.name, currentDoctypeToken.public_identifier, currentDoctypeToken.system_identifier);
        appendChild(document, doctype);

        // TODO: lot of quirk stuffs

        switchTo(BeforeHTML);
    }
    anythingElse()
    {
        // TODO: some quirk stuffs
        // TODO: parser errors

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
    // TODO: body
    // TODO: frameset
    // 
}

processMode(Text)
{
    on(character)
    {
        insert_character(NULL);
        return;
    }
    // TODO: rest of the shit
    anythingElse()
    {
        openElementsPop();
        switchToOriginalInsertionMode();
    }
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
    bool isEOF = false;
    do
    {
        currentToken = tokenizer_emit_token();
        isEOF = currentToken->type == token_eof;

        if(!isEOF)
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
        
        tokenizer_dispose_token(currentToken);
    } while (!isEOF);

    return document;
}
