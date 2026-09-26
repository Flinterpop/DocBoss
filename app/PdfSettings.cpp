#include "PdfSettings.h"

#include <cassert>
#include <filesystem>
#include <fstream>
#include <sstream>

#include <nlohmann/json.hpp>

#include "mdboss/FileScan.h"
#include "mdboss/PathUtf8.h"

namespace docboss {
namespace {

using json = nlohmann::json;

constexpr const char* kPagesKey = "docboss_pdf_pages";
constexpr const char* kFitKey = "docboss_pdf_fit";
constexpr const char* kTopicsSashKey = "docboss_topics_sash";
constexpr const char* kBookmarksSashKey = "docboss_bookmarks_sash";

bool valid_fit(const std::string& fit)
{
    return fit == "width" || fit == "page" || fit == "none";
}

// The whole file as JSON, or an empty object on any failure -- a settings
// file is never load bearing.
json read_document(const std::string& file)
{
    std::ifstream stream(mdboss::path_from_utf8(file), std::ios::binary);
    if (!stream) {
        return json::object();
    }
    std::ostringstream buffer;
    buffer << stream.rdbuf();
    json document =
        json::parse(mdboss::strip_utf8_bom(buffer.str()), nullptr, false);
    if (document.is_discarded() || !document.is_object()) {
        return json::object();
    }
    return document;
}

int int_or(const json& document, const char* key, int fallback)
{
    if (document.contains(key) && document[key].is_number_integer()) {
        return document[key].get<int>();
    }
    return fallback;
}

}  // namespace

PdfSettings::PdfSettings(std::string file) : file_(std::move(file))
{
    assert(!file_.empty() && "settings need a file");
}

void PdfSettings::load()
{
    const json document = read_document(file_);
    pages_.clear();
    if (document.contains(kPagesKey) && document[kPagesKey].is_array()) {
        for (const json& item : document[kPagesKey]) {
            if (pages_.size() >= kMaxRememberedPages) {
                break;
            }
            if (item.is_object() && item.contains("path") &&
                item["path"].is_string() && item.contains("page") &&
                item["page"].is_number_integer() &&
                item["page"].get<int>() >= 0) {
                pages_.emplace_back(item["path"].get<std::string>(),
                                    item["page"].get<int>());
            }
        }
    }
    if (document.contains(kFitKey) && document[kFitKey].is_string()) {
        set_fit(document[kFitKey].get<std::string>());
    }
    topics_sash_ = int_or(document, kTopicsSashKey, 0);
    bookmarks_sash_ = int_or(document, kBookmarksSashKey, 0);
    assert(pages_.size() <= kMaxRememberedPages);
}

bool PdfSettings::save() const
{
    json document = read_document(file_);
    json pages = json::array();
    for (const auto& [path, page] : pages_) {
        pages.push_back(json{{"path", path}, {"page", page}});
    }
    document[kPagesKey] = pages;
    document[kFitKey] = fit_;
    document[kTopicsSashKey] = topics_sash_;
    document[kBookmarksSashKey] = bookmarks_sash_;

    const std::filesystem::path file = mdboss::path_from_utf8(file_);
    std::error_code ec;
    std::filesystem::create_directories(file.parent_path(), ec);
    return mdboss::write_text_file_checked(file_, document.dump(2)).empty();
}

std::optional<int> PdfSettings::last_page(const std::string& pdf_path) const
{
    const std::string key = mdboss::norm_path(pdf_path);
    for (const auto& [path, page] : pages_) {
        if (path == key) {
            return page;
        }
    }
    return std::nullopt;
}

void PdfSettings::set_last_page(const std::string& pdf_path, int page_index)
{
    assert(page_index >= 0 && "pages are 0-based and never negative");
    const std::string key = mdboss::norm_path(pdf_path);
    for (auto it = pages_.begin(); it != pages_.end(); ++it) {
        if (it->first == key) {
            pages_.erase(it);
            break;
        }
    }
    pages_.insert(pages_.begin(), {key, page_index});
    if (pages_.size() > kMaxRememberedPages) {
        pages_.resize(kMaxRememberedPages);
    }
    assert(!pages_.empty() && pages_.front().first == key);
}

void PdfSettings::set_fit(const std::string& fit)
{
    fit_ = valid_fit(fit) ? fit : std::string("width");
}

}  // namespace docboss
