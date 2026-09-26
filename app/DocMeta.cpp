#include "DocMeta.h"

#include <cassert>
#include <cstddef>

#include "mdrender/MdRender.h"

namespace docboss {
namespace {

// Bound on how many front-matter lines are looked at (Rule of 10).  Real
// blocks are a dozen lines; this only stops an unclosed fence from reading
// the whole document as metadata.
constexpr std::size_t kMaxFrontMatterLines = 200;

std::string lower(std::string_view text)
{
    std::string out(text);
    for (char& c : out) {   // bounded by the string
        if (c >= 'A' && c <= 'Z') {
            c = static_cast<char>(c - 'A' + 'a');
        }
    }
    return out;
}

std::string_view trim(std::string_view text)
{
    const std::size_t first = text.find_first_not_of(" \t\r");
    if (first == std::string_view::npos) {
        return {};
    }
    const std::size_t last = text.find_last_not_of(" \t\r");
    return text.substr(first, last - first + 1);
}

// Drop one pair of matching surrounding quotes.
std::string unquote(std::string_view value)
{
    if (value.size() >= 2 &&
        ((value.front() == '"' && value.back() == '"') ||
         (value.front() == '\'' && value.back() == '\''))) {
        return std::string(value.substr(1, value.size() - 2));
    }
    return std::string(value);
}

// The lines between the opening and closing fence, or empty when the
// document does not start with one.  A BOM before the fence is allowed.
std::string_view front_matter_block(std::string_view text)
{
    if (text.substr(0, 3) == "\xEF\xBB\xBF") {
        text.remove_prefix(3);
    }
    if (text.substr(0, 3) != "---") {
        return {};
    }
    const std::size_t first_end = text.find('\n');
    if (first_end == std::string_view::npos ||
        !trim(text.substr(3, first_end - 3)).empty()) {
        return {};   // "---" must be the whole first line
    }
    std::size_t pos = first_end + 1;
    for (std::size_t n = 0; n < kMaxFrontMatterLines && pos < text.size();
         ++n) {
        std::size_t end = text.find('\n', pos);
        if (end == std::string_view::npos) {
            end = text.size();
        }
        const std::string_view line = trim(text.substr(pos, end - pos));
        if (line == "---" || line == "...") {
            return text.substr(first_end + 1, pos - (first_end + 1));
        }
        pos = end + 1;
    }
    return {};   // unclosed: treated as no front matter, like mdrender does
}

}  // namespace

std::string front_matter_value(std::string_view md_text, std::string_view key)
{
    assert(!key.empty() && "a key is needed");
    const std::string_view block = front_matter_block(md_text);
    const std::string wanted = lower(key);
    std::size_t pos = 0;
    for (std::size_t n = 0; n < kMaxFrontMatterLines && pos < block.size();
         ++n) {
        std::size_t end = block.find('\n', pos);
        if (end == std::string_view::npos) {
            end = block.size();
        }
        const std::string_view line = block.substr(pos, end - pos);
        pos = end + 1;
        // A top-level key starts in column 0; an indented one belongs to some
        // nested mapping and is not ours.
        if (line.empty() || line.front() == ' ' || line.front() == '\t') {
            continue;
        }
        const std::size_t colon = line.find(':');
        if (colon == std::string_view::npos) {
            continue;
        }
        if (lower(trim(line.substr(0, colon))) == wanted) {
            return unquote(trim(line.substr(colon + 1)));
        }
    }
    return {};
}

DocMeta read_doc_meta(std::string_view md_text, const std::string& fallback_title)
{
    DocMeta meta;
    // mdrender's own rule for a title: the front matter's (including the
    // Typora-style bare first line), else the first heading.
    meta.title = mdrender::document_title(md_text);
    if (meta.title.empty()) {
        meta.title = fallback_title;
    }
    meta.author = front_matter_value(md_text, "author");
    meta.subject = front_matter_value(md_text, "subject");
    meta.keywords = front_matter_value(md_text, "keywords");
    meta.version = front_matter_value(md_text, "version");
    meta.date = front_matter_value(md_text, "date");
    return meta;
}

std::string css_string(std::string_view text)
{
    std::string out;
    out.reserve(text.size() + 8);
    for (const char c : text) {   // bounded by the string
        if (c == '\\' || c == '"') {
            out += '\\';
            out += c;
        } else if (c == '\n' || c == '\r') {
            out += ' ';   // a CSS string may not hold a raw line break
        } else if (c == '<') {
            // Never lets the value close the <style> element it sits in.
            out += "\\3C ";
        } else {
            out += c;
        }
    }
    return out;
}

std::string add_page_margin_boxes(const std::string& html, const DocMeta& meta,
                                  const std::string& today)
{
    const std::size_t head_end = html.find("</head>");
    if (head_end == std::string::npos) {
        return html;   // not a page we recognise; print it as it is
    }
    const std::string version =
        meta.version.empty() ? std::string() : "Version " + meta.version;
    const std::string date = meta.date.empty() ? today : meta.date;

    std::string style =
        "<style media=\"print\">\n"
        "@page {\n"
        "  @top-left { content: \"" + css_string(meta.title) + "\";"
        " font: 8pt 'Segoe UI', sans-serif; color: #57606a; }\n"
        "  @top-right { content: \"" + css_string(version) + "\";"
        " font: 8pt 'Segoe UI', sans-serif; color: #57606a; }\n"
        "  @bottom-left { content: \"" + css_string(date) + "\";"
        " font: 8pt 'Segoe UI', sans-serif; color: #57606a; }\n"
        "  @bottom-right { content: \"Page \" counter(page) \" of \" "
        "counter(pages); font: 8pt 'Segoe UI', sans-serif; color: #57606a; }\n"
        "}\n"
        "</style>\n";
    std::string out = html;
    out.insert(head_end, style);
    assert(out.size() > html.size() && "the stylesheet went in");
    return out;
}

}  // namespace docboss
