#include "html.h"

#include <string.h>

struct __namespace_HTML HTML = {
    .TagNames = {
    },
    .SpecialTagNames = {
    }
};

#define add_tag_name(name, value) do { HTML.TagNames.name = wstring_new(); wstring_appends(HTML.TagNames.name, value); } while(0)

__attribute__((constructor))
static void __namespace_HTML_init()
{
    add_tag_name(address, L"address");
    add_tag_name(applet, L"applet");
    add_tag_name(area, L"area");
    add_tag_name(article, L"article");
    add_tag_name(aside, L"aside");
    add_tag_name(base, L"base");
    add_tag_name(basefont, L"basefont");
    add_tag_name(bgsound, L"bgsound");
    add_tag_name(blockquote, L"blockquote");
    add_tag_name(body, L"body");
    add_tag_name(br, L"br");
    add_tag_name(button, L"button");    
    add_tag_name(caption, L"caption");
    add_tag_name(center, L"center");
    add_tag_name(col, L"col");
    add_tag_name(colgroup, L"colgroup");
    add_tag_name(dd, L"dd");
    add_tag_name(details, L"details");
    add_tag_name(dialog, L"dialog");
    add_tag_name(dir, L"dir");
    add_tag_name(div, L"div");
    add_tag_name(dl, L"dl");
    add_tag_name(dt, L"dt");
    add_tag_name(embed, L"embed");
    add_tag_name(fieldset, L"fieldset");
    add_tag_name(figcaption, L"figcaption");
    add_tag_name(figure, L"figure");
    add_tag_name(footer, L"footer");
    add_tag_name(form, L"form");
    add_tag_name(frame, L"frame");
    add_tag_name(frameset, L"frameset");
    add_tag_name(h1, L"h1");
    add_tag_name(h2, L"h2");
    add_tag_name(h3, L"h3");
    add_tag_name(h4, L"h4");
    add_tag_name(h5, L"h5");
    add_tag_name(h6, L"h6");
    add_tag_name(head, L"head");
    add_tag_name(header, L"header");
    add_tag_name(hgroup, L"hgroup");
    add_tag_name(hr, L"hr");
    add_tag_name(html, L"html");
    add_tag_name(keygen, L"keygen");
    add_tag_name(iframe, L"iframe");
    add_tag_name(image, L"image");
    add_tag_name(img, L"img");
    add_tag_name(input, L"input");
    add_tag_name(li, L"li");
    add_tag_name(link, L"link");
    add_tag_name(listing, L"listing");
    add_tag_name(main, L"main");
    add_tag_name(marquee, L"marquee");
    add_tag_name(menu, L"menu");
    add_tag_name(meta, L"meta");
    add_tag_name(nav, L"nav");
    add_tag_name(noembed, L"noembed");
    add_tag_name(noframes, L"noframes");
    add_tag_name(noscript, L"noscript");
    add_tag_name(object, L"object");
    add_tag_name(ol, L"ol");
    add_tag_name(optgroup, L"optgroup");
    add_tag_name(option, L"option");
    add_tag_name(p, L"p");
    add_tag_name(param, L"param");
    add_tag_name(plaintext, L"plaintext");
    add_tag_name(pre, L"pre");
    add_tag_name(rb, L"rb");
    add_tag_name(rp, L"rp");
    add_tag_name(rt, L"rt");
    add_tag_name(rtc, L"rtc");
    add_tag_name(script, L"script");
    add_tag_name(search, L"search");
    add_tag_name(section, L"section");
    add_tag_name(select, L"select");
    add_tag_name(source, L"source");
    add_tag_name(style, L"style");
    add_tag_name(summary, L"summary");
    add_tag_name(table, L"table");
    add_tag_name(tbody, L"tbody");
    add_tag_name(td, L"td");
    add_tag_name(template, L"template");
    add_tag_name(textarea, L"textarea");
    add_tag_name(tfoot, L"tfoot");
    add_tag_name(th, L"th");
    add_tag_name(thead, L"thead");
    add_tag_name(title, L"title");
    add_tag_name(tr, L"tr");
    add_tag_name(track, L"track");
    add_tag_name(ul, L"ul");
    add_tag_name(wbr, L"wbr");
    add_tag_name(xmp, L"xmp");

    DOMString specials[] = {
        HTML.TagNames.address, HTML.TagNames.applet, HTML.TagNames.area, HTML.TagNames.article, HTML.TagNames.aside, HTML.TagNames.base, HTML.TagNames.basefont, HTML.TagNames.bgsound, HTML.TagNames.blockquote, HTML.TagNames.body, HTML.TagNames.br, HTML.TagNames.button, HTML.TagNames.caption, HTML.TagNames.center, HTML.TagNames.col, HTML.TagNames.colgroup, HTML.TagNames.dd, HTML.TagNames.details, HTML.TagNames.dir, HTML.TagNames.div, HTML.TagNames.dl, HTML.TagNames.dt, HTML.TagNames.embed, HTML.TagNames.fieldset, HTML.TagNames.figcaption, HTML.TagNames.figure, HTML.TagNames.footer, HTML.TagNames.form, HTML.TagNames.frame, HTML.TagNames.frameset, HTML.TagNames.h1, HTML.TagNames.h2, HTML.TagNames.h3, HTML.TagNames.h4, HTML.TagNames.h5, HTML.TagNames.h6, HTML.TagNames.head, HTML.TagNames.header, HTML.TagNames.hgroup, HTML.TagNames.hr, HTML.TagNames.html, HTML.TagNames.iframe, HTML.TagNames.img, HTML.TagNames.input, HTML.TagNames.keygen, HTML.TagNames.li, HTML.TagNames.link, HTML.TagNames.listing, HTML.TagNames.main, HTML.TagNames.marquee, HTML.TagNames.menu, HTML.TagNames.meta, HTML.TagNames.nav, HTML.TagNames.noembed, HTML.TagNames.noframes, HTML.TagNames.noscript, HTML.TagNames.object, HTML.TagNames.ol, HTML.TagNames.p, HTML.TagNames.param, HTML.TagNames.plaintext, HTML.TagNames.pre, HTML.TagNames.script, HTML.TagNames.search, HTML.TagNames.section, HTML.TagNames.select, HTML.TagNames.source, HTML.TagNames.style, HTML.TagNames.summary, HTML.TagNames.table, HTML.TagNames.tbody, HTML.TagNames.td, HTML.TagNames.template, HTML.TagNames.textarea, HTML.TagNames.tfoot, HTML.TagNames.th, HTML.TagNames.thead, HTML.TagNames.title, HTML.TagNames.tr, HTML.TagNames.track, HTML.TagNames.ul, HTML.TagNames.wbr, HTML.TagNames.xmp
    };
    memcpy(HTML.SpecialTagNames, specials, sizeof(specials));
}