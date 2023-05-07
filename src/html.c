#include "html.h"

struct __namespace_HTML HTML = {
    .TagNames = {
    },
};

#define add_tag_name(name, value) do { HTML.TagNames.name = wstring_new(); wstring_appends(HTML.TagNames.name, value); } while(0)

__attribute__((constructor))
static void __namespace_HTML_init()
{
    add_tag_name(html, L"html");
    add_tag_name(head, L"head");
    add_tag_name(body, L"body");
    add_tag_name(br, L"br");
    add_tag_name(base, L"base");
    add_tag_name(basefont, L"basefont");
    add_tag_name(bgsound, L"bgsound");
    add_tag_name(link, L"link");
    add_tag_name(meta, L"meta");
    add_tag_name(title, L"title");
    add_tag_name(noframes, L"noframes");
    add_tag_name(script, L"script");
    add_tag_name(style, L"style");
    add_tag_name(template, L"template");
    add_tag_name(noscript, L"noscript");
    add_tag_name(caption, L"caption");
    add_tag_name(colgroup, L"colgroup");
    add_tag_name(tbody, L"tbody");
    add_tag_name(tfoot, L"tfoot");
    add_tag_name(thead, L"thead");
}