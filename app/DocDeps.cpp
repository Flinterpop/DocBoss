#include "DocDeps.h"

#include <algorithm>
#include <cassert>
#include <filesystem>
#include <fstream>
#include <set>

#include "mdboss/PathUtf8.h"

namespace docboss {
namespace {

// Bounds (Rule of 10).  A document bigger than this is read only this far;
// its figures are nearly always near where they are first discussed.
constexpr std::size_t kMaxDocumentBytes = 1024 * 1024;
constexpr std::size_t kMaxReferences = 500;

bool has_scheme(std::string_view target)
{
    // "c:/..." or "C:\..." is a drive, not a scheme.
    const std::size_t colon = target.find(':');
    if (colon == std::string_view::npos || colon < 2) {
        return false;
    }
    for (std::size_t i = 0; i < colon; ++i) {   // bounded by the target
        const char c = target[i];
        const bool ok = (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
                        (i > 0 && ((c >= '0' && c <= '9') || c == '+' ||
                                   c == '-' || c == '.'));
        if (!ok) {
            return false;
        }
    }
    return true;
}

int hex_value(char c)
{
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
}

// A raw link destination to a local path, or empty when it is not one.
std::string clean_target(std::string_view raw)
{
    std::string_view t = raw;
    while (!t.empty() && (t.front() == ' ' || t.front() == '\t')) {
        t.remove_prefix(1);
    }
    if (!t.empty() && t.front() == '<') {
        const std::size_t close = t.find('>');
        t = (close == std::string_view::npos) ? t.substr(1)
                                              : t.substr(1, close - 1);
    } else {
        // A title after the destination: ![a](fig.png "Figure 1").
        const std::size_t space = t.find_first_of(" \t");
        if (space != std::string_view::npos) {
            t = t.substr(0, space);
        }
    }
    if (t.empty() || t.front() == '#' || has_scheme(t)) {
        return {};
    }
    const std::size_t cut = t.find_first_of("?#");
    if (cut != std::string_view::npos) {
        t = t.substr(0, cut);
    }
    std::string out;
    for (std::size_t i = 0; i < t.size(); ++i) {   // bounded by the target
        if (t[i] == '%' && i + 2 < t.size() && hex_value(t[i + 1]) >= 0 &&
            hex_value(t[i + 2]) >= 0) {
            out += static_cast<char>(hex_value(t[i + 1]) * 16 +
                                     hex_value(t[i + 2]));
            i += 2;
        } else {
            out += t[i];
        }
    }
    return out;
}

bool looks_like_image(std::string_view target)
{
    const std::size_t dot = target.rfind('.');
    if (dot == std::string_view::npos) {
        return false;
    }
    std::string ext(target.substr(dot));
    for (char& c : ext) {   // bounded by the extension
        if (c >= 'A' && c <= 'Z') {
            c = static_cast<char>(c - 'A' + 'a');
        }
    }
    return ext == ".png" || ext == ".jpg" || ext == ".jpeg" || ext == ".gif" ||
           ext == ".svg" || ext == ".bmp" || ext == ".webp";
}

}  // namespace

std::vector<std::string> local_references(std::string_view md_text)
{
    std::vector<std::string> out;
    std::set<std::string> seen;
    const auto add = [&](std::string_view raw) {
        const std::string target = clean_target(raw);
        if (!target.empty() && out.size() < kMaxReferences &&
            seen.insert(target).second) {
            out.push_back(target);
        }
    };

    // ![alt](target ...)
    std::size_t pos = 0;
    while (out.size() < kMaxReferences) {   // bounded: each pass advances
        const std::size_t bang = md_text.find("![", pos);
        if (bang == std::string_view::npos) {
            break;
        }
        const std::size_t close = md_text.find("](", bang + 2);
        const std::size_t line_end = md_text.find('\n', bang);
        if (close == std::string_view::npos ||
            (line_end != std::string_view::npos && close > line_end)) {
            pos = bang + 2;
            continue;
        }
        const std::size_t end = md_text.find(')', close + 2);
        if (end == std::string_view::npos) {
            break;
        }
        add(md_text.substr(close + 2, end - close - 2));
        assert(end + 1 > pos && "the scan moves forward");
        pos = end + 1;
    }

    // <img ... src="target" ...>, either quote.
    pos = 0;
    while (out.size() < kMaxReferences) {   // bounded: each pass advances
        const std::size_t img = md_text.find("<img", pos);
        if (img == std::string_view::npos) {
            break;
        }
        const std::size_t tag_end = md_text.find('>', img);
        const std::size_t src = md_text.find("src=", img);
        pos = img + 4;
        if (src == std::string_view::npos ||
            (tag_end != std::string_view::npos && src > tag_end) ||
            src + 5 >= md_text.size()) {
            continue;
        }
        const char quote = md_text[src + 4];
        if (quote != '"' && quote != '\'') {
            continue;
        }
        const std::size_t value_end = md_text.find(quote, src + 5);
        if (value_end == std::string_view::npos) {
            continue;
        }
        add(md_text.substr(src + 5, value_end - src - 5));
    }

    // [id]: target -- a reference definition at the start of a line, kept
    // only when it names an image (it may just as well be a link elsewhere).
    pos = 0;
    while (pos < md_text.size() && out.size() < kMaxReferences) {
        std::size_t end = md_text.find('\n', pos);
        if (end == std::string_view::npos) {
            end = md_text.size();
        }
        std::string_view line = md_text.substr(pos, end - pos);
        pos = end + 1;
        while (!line.empty() && line.front() == ' ') {
            line.remove_prefix(1);
        }
        const std::size_t colon = line.find("]:");
        if (line.empty() || line.front() != '[' || line.substr(0, 2) == "[^" ||
            colon == std::string_view::npos) {
            continue;
        }
        const std::string target = clean_target(line.substr(colon + 2));
        if (looks_like_image(target)) {
            add(line.substr(colon + 2));
        }
    }
    return out;
}

std::int64_t newest_dependency(const std::string& md_path)
{
    namespace fs = std::filesystem;
    const fs::path doc = mdboss::path_from_utf8(md_path);
    std::ifstream stream(doc, std::ios::binary);
    if (!stream) {
        return 0;
    }
    std::string text(kMaxDocumentBytes, '\0');
    stream.read(text.data(), static_cast<std::streamsize>(text.size()));
    text.resize(static_cast<std::size_t>(stream.gcount()));

    std::int64_t newest = 0;
    for (const std::string& ref : local_references(text)) {   // bounded
        const fs::path target = mdboss::path_from_utf8(ref);
        const fs::path file =
            target.is_absolute() ? target : doc.parent_path() / target;
        std::error_code ec;
        const fs::file_time_type when = fs::last_write_time(file, ec);
        if (!ec) {
            newest = std::max<std::int64_t>(
                newest, static_cast<std::int64_t>(
                            when.time_since_epoch().count()));
        }
    }
    assert(newest >= 0);
    return newest;
}

PdfState publication_state_on_disk(const std::string& md_path)
{
    namespace fs = std::filesystem;
    assert(!md_path.empty() && "a document is needed");
    const fs::path doc = mdboss::path_from_utf8(md_path);
    fs::path pdf = doc;
    pdf.replace_extension(".pdf");
    std::error_code ec;
    const fs::file_time_type pdf_time = fs::last_write_time(pdf, ec);
    if (ec) {
        return PdfState::kNone;   // no PDF beside it (or unreadable)
    }
    const fs::file_time_type doc_time = fs::last_write_time(doc, ec);
    const std::int64_t pdf_ticks =
        static_cast<std::int64_t>(pdf_time.time_since_epoch().count());
    if (!ec && static_cast<std::int64_t>(doc_time.time_since_epoch().count()) >
                   pdf_ticks) {
        return PdfState::kStale;
    }
    return newest_dependency(md_path) > pdf_ticks ? PdfState::kStale
                                                  : PdfState::kCurrent;
}

void apply_dependency_times(Pairing& pairing)
{
    for (auto& [key, link] : pairing.published) {   // bounded by the scan
        if (link.state != PdfState::kCurrent) {
            continue;   // already stale: reading the document cannot change it
        }
        if (newest_dependency(link.md_path) > link.pdf_modified) {
            link.state = PdfState::kStale;
        }
    }
}

}  // namespace docboss
