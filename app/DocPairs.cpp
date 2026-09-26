#include "DocPairs.h"

#include <algorithm>
#include <cassert>
#include <filesystem>
#include <utility>

#include "mdboss/PathUtf8.h"

namespace docboss {
namespace {

// Folder plus stem, normalised the same way for both kinds of file, so the
// match is "same folder, same name ignoring case and extension".
std::string pair_key(const std::string& path)
{
    const std::filesystem::path p = mdboss::path_from_utf8(path);
    const std::filesystem::path key = p.parent_path() / p.stem();
    return mdboss::norm_path(mdboss::path_to_utf8(key));
}

}  // namespace

Pairing classify_pairs(const std::vector<mdboss::DocEntry>& documents,
                       const std::vector<mdboss::DocEntry>& pdfs)
{
    // Both lists are already bounded by the scan (kMaxEntriesPerRoot), so
    // every loop below is too.
    std::map<std::string, const mdboss::DocEntry*> by_key;
    for (const mdboss::DocEntry& pdf : pdfs) {
        by_key.emplace(pair_key(pdf.path), &pdf);
    }

    Pairing out;
    std::map<std::string, bool> claimed;
    for (const mdboss::DocEntry& doc : documents) {
        const std::string key = pair_key(doc.path);
        const auto found = by_key.find(key);
        if (found == by_key.end()) {
            continue;
        }
        const mdboss::DocEntry& pdf = *found->second;
        PdfLink link;
        link.md_path = doc.path;
        link.pdf_path = pdf.path;
        // A time that could not be read is 0; treat an unknown source time
        // as "not newer", so a failure to read never cries stale.
        link.state = (doc.modified > pdf.modified) ? PdfState::kStale
                                                   : PdfState::kCurrent;
        out.published[mdboss::norm_path(doc.path)] = link;
        claimed[key] = true;
    }

    for (const mdboss::DocEntry& pdf : pdfs) {
        if (claimed.count(pair_key(pdf.path)) == 0) {
            out.loose_pdfs.push_back(pdf);
        }
    }
    assert(out.published.size() <= documents.size() &&
           "at most one publication per document");
    assert(out.loose_pdfs.size() <= pdfs.size());
    return out;
}

PdfState pdf_state(const Pairing& pairing, const std::string& md_path)
{
    const auto found = pairing.published.find(mdboss::norm_path(md_path));
    return found == pairing.published.end() ? PdfState::kNone
                                            : found->second.state;
}

std::vector<std::string> stale_documents(const Pairing& pairing,
                                         const std::string& folder)
{
    // A trailing separator on the prefix, so "C:\Docs" does not also claim
    // "C:\Docs2".
    std::string prefix;
    if (!folder.empty()) {
        prefix = mdboss::norm_path(folder);
        if (!prefix.empty() && prefix.back() != '\\') {
            prefix += '\\';
        }
    }
    // Sorted on the normalised path with the separator ranked below every
    // other character, so a folder's contents stay together: plain string
    // order would put "docs2\..." between "docs\a" and "docs\sub\...".
    std::vector<std::pair<std::string, std::string>> keyed;
    for (const auto& [key, link] : pairing.published) {   // bounded by the scan
        if (link.state != PdfState::kStale) {
            continue;
        }
        if (!prefix.empty() && key.compare(0, prefix.size(), prefix) != 0) {
            continue;
        }
        std::string order = key;
        std::replace(order.begin(), order.end(), '\\', '\x01');
        keyed.emplace_back(std::move(order), link.md_path);
    }
    std::sort(keyed.begin(), keyed.end());
    std::vector<std::string> out;
    out.reserve(keyed.size());
    for (auto& item : keyed) {   // bounded by the scan
        out.push_back(std::move(item.second));
    }
    assert(out.size() <= pairing.published.size());
    return out;
}

}  // namespace docboss
