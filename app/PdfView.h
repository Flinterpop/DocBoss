// What the right-hand side shows when the open file is a PDF: PDFBoss's topics
// and bookmarks pane beside PDFBoss's viewer, both compiled from the PDFBoss
// tree.  This class only joins them and keeps their state in PdfSettings.
//
// Two things it does that PDFBoss does differently:
//
// - A PDF with no topics file gets one written on a WORKER, with no progress
//   dialog.  PDFBoss asks first and shows a modal dialog because it builds
//   many at once; here one PDF is opened at a time, often by arrowing through
//   the tree, and a dialog per keystroke is an obstacle, not a prompt.
// - release() lets go of the file.  MuPDF holds the PDF open while it is
//   shown, and Publish writes over exactly that file, so the frame releases it
//   first and reopens it after.

#ifndef DOCBOSS_APP_PDF_VIEW_H
#define DOCBOSS_APP_PDF_VIEW_H

#include <wx/panel.h>
#include <wx/splitter.h>

#include <atomic>
#include <functional>
#include <memory>
#include <string>

#include "PdfSettings.h"
#include "pdfboss/TopicsPane.h"
#include "pdfboss/ViewerPane.h"

namespace docboss {

class PdfView : public wxPanel {
public:
    // `settings` must outlive the view; the frame owns both.
    PdfView(wxWindow* parent, PdfSettings& settings);
    ~PdfView() override;

    PdfView(const PdfView&) = delete;
    PdfView& operator=(const PdfView&) = delete;

    // Show `pdf_path` at the page it was last left on.  False, with the
    // viewer showing why, when it cannot be opened.
    bool open(const std::string& pdf_path);
    // Nothing shown.
    void close();
    // Let go of the file (see the header comment); reopen() shows it again
    // at the same page.
    void release();
    void reopen();

    bool has_document() const;
    // The PDF on show, UTF-8; empty when none.
    const std::string& path() const { return path_; }

    // Unsaved highlights: offer to save them.  False when the user cancelled
    // and the caller should abandon what it was about to do.
    bool maybe_save_annotations();
    bool dirty() const;

    void focus_search();
    void find_next();
    void find_previous();
    void bookmark_current_page();

    // Record sash positions into the settings (not saved to disk here).
    void remember_layout();

    // Raised after the viewer writes a new file (an "(ann)" copy), so the tree
    // can pick it up.
    void set_on_files_changed(std::function<void()> handler)
    {
        on_files_changed_ = std::move(handler);
    }

    pdfboss::ViewerPane& viewer() { return *viewer_; }

private:
    void build_topics_in_background(const std::string& pdf_path);
    void on_char_hook(wxKeyEvent& event);

    PdfSettings& settings_;
    wxSplitterWindow* split_ = nullptr;
    pdfboss::TopicsPane* topics_ = nullptr;
    pdfboss::ViewerPane* viewer_ = nullptr;
    std::string path_;
    int released_page_ = 0;
    std::shared_ptr<std::atomic<bool>> alive_;
    std::function<void()> on_files_changed_;
};

}  // namespace docboss

#endif  // DOCBOSS_APP_PDF_VIEW_H
