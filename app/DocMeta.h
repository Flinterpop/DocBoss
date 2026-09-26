// What a published PDF says about itself, taken from the document's YAML
// front matter: the page header and footer (title, version, date, page
// numbers) and the PDF's own properties (Title, Author, Subject, Keywords).
//
// Pure and wx-free.  The front matter is read the way the house templates
// write it -- `key: value` lines between `---` fences at the very top -- and
// nothing cleverer: a value is one line, surrounding quotes are dropped, a
// YAML list `[a, b]` is kept as its text.

#ifndef DOCBOSS_APP_DOC_META_H
#define DOCBOSS_APP_DOC_META_H

#include <string>
#include <string_view>

namespace docboss {

struct DocMeta {
    std::string title;      // front matter title, else first heading, else fallback
    std::string author;
    std::string subject;
    std::string keywords;
    std::string version;
    std::string date;       // the front matter's date: value, as written
};

// `fallback_title` is used when the document names no title at all -- the
// caller passes the file's stem.
DocMeta read_doc_meta(std::string_view md_text,
                      const std::string& fallback_title);

// One front-matter value by key (case-insensitive), or empty.  Exposed for
// the tests; read_doc_meta is what the app uses.
std::string front_matter_value(std::string_view md_text, std::string_view key);

// `html` (a full page from mdrender::render_document) with a print-only
// stylesheet added that puts the title and version in the top margin and the
// date and "Page N of M" in the bottom one, as CSS @page margin boxes.  The
// screen is untouched; only a printed page shows them.  `today` is the date
// used when the document has none, in the house format ("26 Sep 2026").
std::string add_page_margin_boxes(const std::string& html, const DocMeta& meta,
                                  const std::string& today);

// A string made safe inside a CSS "..." literal.
std::string css_string(std::string_view text);

}  // namespace docboss

#endif  // DOCBOSS_APP_DOC_META_H
