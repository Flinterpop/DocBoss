#include "Publish.h"

#include <cassert>
#include <filesystem>

#include "mdboss/FileScan.h"
#include "mdboss/PathUtf8.h"

namespace docboss {

std::string published_pdf_path(const std::string& md_path)
{
    const std::filesystem::path source = mdboss::path_from_utf8(md_path);
    assert(mdboss::is_markdown(mdboss::path_to_utf8(source.filename())) &&
           "only a Markdown document is published");
    std::filesystem::path target = source;
    // replace_extension swaps only the LAST extension, so "v1.2-notes.md"
    // becomes "v1.2-notes.pdf", not "v1.pdf".
    target.replace_extension(".pdf");
    assert(target.parent_path() == source.parent_path() &&
           "a published PDF sits beside its source");
    return mdboss::path_to_utf8(target);
}

bool is_pdf(const std::string& name)
{
    if (name.size() < 4) {
        return false;
    }
    const std::string ext = name.substr(name.size() - 4);
    bool match = true;
    for (std::size_t i = 0; i < 4; ++i) {   // bounded: four characters
        const char c = ext[i];
        const char lower = (c >= 'A' && c <= 'Z') ? static_cast<char>(c - 'A' + 'a') : c;
        match = match && lower == ".pdf"[i];
    }
    return match;
}

}  // namespace docboss
