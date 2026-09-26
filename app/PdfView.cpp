#include "PdfView.h"

#include <wx/app.h>
#include <wx/sizer.h>
#include <wx/textctrl.h>
#include <wx/treectrl.h>

#include <cassert>
#include <filesystem>
#include <thread>

#include "pdfboss/Metadata.h"
#include "pdfboss/PathUtf8.h"
#include "pdfboss/TocGen.h"

namespace docboss {
namespace {

// Default width of the topics column, when none has been recorded.
constexpr int kDefaultTopicsWidth = 260;

pdfboss::FitMode fit_from(const std::string& name)
{
    if (name == "page") {
        return pdfboss::FitMode::kPage;
    }
    if (name == "none") {
        return pdfboss::FitMode::kNone;
    }
    return pdfboss::FitMode::kWidth;
}

const char* fit_name(pdfboss::FitMode mode)
{
    switch (mode) {
    case pdfboss::FitMode::kPage:
        return "page";
    case pdfboss::FitMode::kNone:
        return "none";
    case pdfboss::FitMode::kWidth:
        break;
    }
    return "width";
}

}  // namespace

PdfView::PdfView(wxWindow* parent, PdfSettings& settings)
    : wxPanel(parent, wxID_ANY),
      settings_(settings),
      alive_(std::make_shared<std::atomic<bool>>(true))
{
    split_ = new wxSplitterWindow(this, wxID_ANY, wxDefaultPosition,
                                  wxDefaultSize,
                                  wxSP_LIVE_UPDATE | wxSP_3DSASH);
    split_->SetMinimumPaneSize(120);
    topics_ = new pdfboss::TopicsPane(split_);
    viewer_ = new pdfboss::ViewerPane(split_, fit_from(settings_.fit()));
    const int width = settings_.topics_sash() > 0 ? settings_.topics_sash()
                                                  : kDefaultTopicsWidth;
    split_->SplitVertically(topics_, viewer_, width);
    if (settings_.bookmarks_sash() > 0) {
        topics_->set_sash_position(settings_.bookmarks_sash());
    }

    auto* sizer = new wxBoxSizer(wxVERTICAL);
    sizer->Add(split_, 1, wxEXPAND);
    SetSizer(sizer);

    topics_->set_page_requested_handler([this](int page_1based) {
        viewer_->goto_page(page_1based - 1);
    });
    viewer_->set_page_changed_handler([this](int page_index) {
        if (!path_.empty() && page_index >= 0) {
            settings_.set_last_page(path_, page_index);
        }
    });
    viewer_->set_fit_changed_handler([this](pdfboss::FitMode mode) {
        settings_.set_fit(fit_name(mode));
    });
    viewer_->set_bookmark_requested_handler(
        [this](int page_1based) { topics_->add_bookmark(page_1based); });
    viewer_->set_documents_changed_handler([this]() {
        if (on_files_changed_) {
            on_files_changed_();
        }
    });
    // Page keys.  PDFBoss binds them as frame-wide accelerators; here that
    // would take Page Up/Down away from the Markdown editor, so they act only
    // while the keyboard is somewhere inside this page.
    Bind(wxEVT_CHAR_HOOK, &PdfView::on_char_hook, this);
    assert(topics_ != nullptr && viewer_ != nullptr);
}

void PdfView::on_char_hook(wxKeyEvent& event)
{
    // A text box or a list keeps its own keys: Home/End edit the search text,
    // Page Up/Down move through the topics.
    wxWindow* focus = wxWindow::FindFocus();
    const bool owns_keys = dynamic_cast<wxTextCtrl*>(focus) != nullptr ||
                           dynamic_cast<wxTreeCtrl*>(focus) != nullptr;
    if (owns_keys || !viewer_->has_document() || event.HasAnyModifiers()) {
        event.Skip();
        return;
    }
    switch (event.GetKeyCode()) {
    case WXK_PAGEDOWN:
    case WXK_SPACE:
        viewer_->next_page();
        return;
    case WXK_PAGEUP:
        viewer_->prev_page();
        return;
    case WXK_HOME:
        viewer_->goto_page(0);
        return;
    case WXK_END:
        viewer_->goto_page(viewer_->page_count() - 1);
        return;
    default:
        event.Skip();
        return;
    }
}

PdfView::~PdfView()
{
    // A topics build still running must not call back into a dead view.
    alive_->store(false);
}

bool PdfView::open(const std::string& pdf_path)
{
    assert(!pdf_path.empty() && "open needs a path");
    const std::filesystem::path file = pdfboss::path_from_utf8(pdf_path);
    const int page = settings_.last_page(pdf_path).value_or(0);
    if (!viewer_->load(file, page)) {
        path_.clear();
        topics_->clear();
        return false;
    }
    path_ = pdf_path;
    topics_->load(file);
    if (!pdfboss::find_metadata_path(file).has_value()) {
        build_topics_in_background(pdf_path);
    }
    assert(has_document());
    return true;
}

void PdfView::close()
{
    viewer_->unload();
    topics_->clear();
    path_.clear();
}

void PdfView::release()
{
    if (path_.empty()) {
        return;
    }
    released_page_ = viewer_->current_page();
    viewer_->unload();
}

void PdfView::reopen()
{
    if (path_.empty()) {
        return;
    }
    const std::filesystem::path file = pdfboss::path_from_utf8(path_);
    // The publication may have fewer pages now; load() clamps.
    if (viewer_->load(file, released_page_)) {
        topics_->load(file);
    }
}

bool PdfView::has_document() const
{
    return viewer_->has_document();
}

bool PdfView::maybe_save_annotations()
{
    return viewer_->maybe_save_annotations();
}

bool PdfView::dirty() const
{
    return viewer_->dirty();
}

void PdfView::focus_search()
{
    viewer_->focus_search();
}

void PdfView::find_next()
{
    viewer_->next_match();
}

void PdfView::find_previous()
{
    viewer_->prev_match();
}

void PdfView::bookmark_current_page()
{
    if (viewer_->has_document()) {
        topics_->add_bookmark(viewer_->current_page() + 1);
    }
}

void PdfView::remember_layout()
{
    if (split_->IsSplit()) {
        settings_.set_topics_sash(split_->GetSashPosition());
    }
    const std::optional<int> bookmarks = topics_->sash_position();
    if (bookmarks.has_value()) {
        settings_.set_bookmarks_sash(*bookmarks);
    }
}

void PdfView::build_topics_in_background(const std::string& pdf_path)
{
    std::shared_ptr<std::atomic<bool>> alive = alive_;
    // Detached, with its own PdfDocument inside write_toc -- and so its own
    // MuPDF context, which is the only way MuPDF may be used off the UI
    // thread.  Nothing may escape it: an uncaught exception on a detached
    // thread is a silent std::terminate.
    std::thread([this, alive, pdf_path] {
        bool ok = false;
        try {
            ok = pdfboss::write_toc(pdfboss::path_from_utf8(pdf_path)).ok;
        } catch (...) {
            ok = false;
        }
        if (!ok) {
            return;   // the pane keeps saying there are no topics
        }
        wxTheApp->CallAfter([this, alive, pdf_path] {
            // Only if the view still exists and still shows that PDF.
            if (alive->load() && path_ == pdf_path) {
                topics_->load(pdfboss::path_from_utf8(pdf_path));
            }
        });
    }).detach();
}

}  // namespace docboss
