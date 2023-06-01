#include <html/html.h>

#include <string.h>

struct __HTML HTML = {
    .TagNames = {
    },
    .SpecialTagNames = {
    }
};

#define add_tag_name(name) do { HTML.TagNames.name = WString_new(); HTML.TagNames.name->appends(#name); } while(0)

__attribute__((constructor))
static void __HTML_init()
{
    add_tag_name(applet);
    add_tag_name(area);
    add_tag_name(article);
    add_tag_name(aside);
    add_tag_name(base);
    add_tag_name(basefont);
    add_tag_name(bgsound);
    add_tag_name(blockquote);
    add_tag_name(body);
    add_tag_name(br);
    add_tag_name(button);    
    add_tag_name(caption);
    add_tag_name(center);
    add_tag_name(col);
    add_tag_name(colgroup);
    add_tag_name(dd);
    add_tag_name(details);
    add_tag_name(dialog);
    add_tag_name(dir);
    add_tag_name(div);
    add_tag_name(dl);
    add_tag_name(dt);
    add_tag_name(embed);
    add_tag_name(fieldset);
    add_tag_name(figcaption);
    add_tag_name(figure);
    add_tag_name(footer);
    add_tag_name(form);
    add_tag_name(frame);
    add_tag_name(frameset);
    add_tag_name(h1);
    add_tag_name(h2);
    add_tag_name(h3);
    add_tag_name(h4);
    add_tag_name(h5);
    add_tag_name(h6);
    add_tag_name(head);
    add_tag_name(header);
    add_tag_name(hgroup);
    add_tag_name(hr);
    add_tag_name(html);
    add_tag_name(keygen);
    add_tag_name(iframe);
    add_tag_name(image);
    add_tag_name(img);
    add_tag_name(input);
    add_tag_name(li);
    add_tag_name(link);
    add_tag_name(listing);
    add_tag_name(main);
    add_tag_name(marquee);
    add_tag_name(menu);
    add_tag_name(meta);
    add_tag_name(nav);
    add_tag_name(noembed);
    add_tag_name(noframes);
    add_tag_name(noscript);
    add_tag_name(object);
    add_tag_name(ol);
    add_tag_name(optgroup);
    add_tag_name(option);
    add_tag_name(p);
    add_tag_name(param);
    add_tag_name(plaintext);
    add_tag_name(pre);
    add_tag_name(rb);
    add_tag_name(rp);
    add_tag_name(rt);
    add_tag_name(rtc);
    add_tag_name(script);
    add_tag_name(search);
    add_tag_name(section);
    add_tag_name(select);
    add_tag_name(source);
    add_tag_name(style);
    add_tag_name(summary);
    add_tag_name(table);
    add_tag_name(tbody);
    add_tag_name(td);
    add_tag_name(template);
    add_tag_name(textarea);
    add_tag_name(tfoot);
    add_tag_name(th);
    add_tag_name(thead);
    add_tag_name(title);
    add_tag_name(tr);
    add_tag_name(track);
    add_tag_name(ul);
    add_tag_name(wbr);
    add_tag_name(xmp);

    DOMString specials[] = {
        HTML.TagNames.address, HTML.TagNames.applet, HTML.TagNames.area, HTML.TagNames.article, HTML.TagNames.aside, HTML.TagNames.base, HTML.TagNames.basefont, HTML.TagNames.bgsound, HTML.TagNames.blockquote, HTML.TagNames.body, HTML.TagNames.br, HTML.TagNames.button, HTML.TagNames.caption, HTML.TagNames.center, HTML.TagNames.col, HTML.TagNames.colgroup, HTML.TagNames.dd, HTML.TagNames.details, HTML.TagNames.dir, HTML.TagNames.div, HTML.TagNames.dl, HTML.TagNames.dt, HTML.TagNames.embed, HTML.TagNames.fieldset, HTML.TagNames.figcaption, HTML.TagNames.figure, HTML.TagNames.footer, HTML.TagNames.form, HTML.TagNames.frame, HTML.TagNames.frameset, HTML.TagNames.h1, HTML.TagNames.h2, HTML.TagNames.h3, HTML.TagNames.h4, HTML.TagNames.h5, HTML.TagNames.h6, HTML.TagNames.head, HTML.TagNames.header, HTML.TagNames.hgroup, HTML.TagNames.hr, HTML.TagNames.html, HTML.TagNames.iframe, HTML.TagNames.img, HTML.TagNames.input, HTML.TagNames.keygen, HTML.TagNames.li, HTML.TagNames.link, HTML.TagNames.listing, HTML.TagNames.main, HTML.TagNames.marquee, HTML.TagNames.menu, HTML.TagNames.meta, HTML.TagNames.nav, HTML.TagNames.noembed, HTML.TagNames.noframes, HTML.TagNames.noscript, HTML.TagNames.object, HTML.TagNames.ol, HTML.TagNames.p, HTML.TagNames.param, HTML.TagNames.plaintext, HTML.TagNames.pre, HTML.TagNames.script, HTML.TagNames.search, HTML.TagNames.section, HTML.TagNames.select, HTML.TagNames.source, HTML.TagNames.style, HTML.TagNames.summary, HTML.TagNames.table, HTML.TagNames.tbody, HTML.TagNames.td, HTML.TagNames.template, HTML.TagNames.textarea, HTML.TagNames.tfoot, HTML.TagNames.th, HTML.TagNames.thead, HTML.TagNames.title, HTML.TagNames.tr, HTML.TagNames.track, HTML.TagNames.ul, HTML.TagNames.wbr, HTML.TagNames.xmp
    };
    memcpy(HTML.SpecialTagNames, specials, sizeof(specials));
}
