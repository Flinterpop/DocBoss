// The PDF half of DocBoss's settings: the page each PDF was left on, the fit
// preference, and the viewer's sash positions.
//
// Stored in the SAME config.json as everything else (mdboss::Config owns the
// rest of it), under docboss_* keys.  That works because both writers do a
// read-modify-write: mdboss::Config::save() re-reads the file and keeps every
// key it does not own, and so does this.  Neither can lose the other's keys.
//
// Wx-free so a test can round-trip it.

#ifndef DOCBOSS_APP_PDF_SETTINGS_H
#define DOCBOSS_APP_PDF_SETTINGS_H

#include <cstddef>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace docboss {

// How many PDFs' last pages are remembered.  PDFBoss's cap, for the same
// reason: bounded (Rule of 10), and far more than anyone reopens.
inline constexpr std::size_t kMaxRememberedPages = 200;

class PdfSettings {
public:
    // `file` is the config.json to read and merge into (UTF-8).
    explicit PdfSettings(std::string file);

    void load();
    // Merge this object's keys into the file on disk.  False on a write
    // failure; nothing else in the file is changed either way.
    bool save() const;

    // 0-based page, if this PDF has been opened before.
    std::optional<int> last_page(const std::string& pdf_path) const;
    // Most recent first; the oldest entry falls off past the cap.
    void set_last_page(const std::string& pdf_path, int page_index);

    // "width", "page" or "none"; anything else reads as "width".
    const std::string& fit() const { return fit_; }
    void set_fit(const std::string& fit);

    // Pixels; 0 means "not recorded, use the default".
    int topics_sash() const { return topics_sash_; }
    void set_topics_sash(int pixels) { topics_sash_ = pixels; }
    int bookmarks_sash() const { return bookmarks_sash_; }
    void set_bookmarks_sash(int pixels) { bookmarks_sash_ = pixels; }

private:
    std::string file_;
    // (normalised path, 0-based page), most recent first.
    std::vector<std::pair<std::string, int>> pages_;
    std::string fit_ = "width";
    int topics_sash_ = 0;
    int bookmarks_sash_ = 0;
};

}  // namespace docboss

#endif  // DOCBOSS_APP_PDF_SETTINGS_H
