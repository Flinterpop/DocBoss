// The application window: file tree, outline, source editor and live preview
// -- or, when the open file is a PDF, the PDF viewer in their place.
//
// Modelled on MD Boss's MainFrame, which it started as a copy of: everything
// about Markdown documents behaves as it does there.  What DocBoss adds is the
// second kind of file.  The right-hand side is a book of two pages, the
// Markdown page (outline | editor | preview) and the PDF page (PdfView), and
// exactly one file is open at a time -- a document or a PDF, never both, so
// Save can never write the editor's text over a PDF.  Publish renders the open
// document through the preview's own print pipeline into the PDF beside it.
//
// This class is deliberately the only place that knows about all of them.  The
// panes are self-contained and report what happened through std::function
// hooks rather than reaching for the frame, so each can be read on its own;
// what they cannot do is decide policy.  Whether a changed file is reloaded,
// whether a document may be discarded, where a new file lands -- that lives
// here, because those answers depend on state no single pane holds.

#ifndef DOCBOSS_APP_MAIN_FRAME_H
#define DOCBOSS_APP_MAIN_FRAME_H

#include <wx/frame.h>
#include <wx/simplebook.h>
#include <wx/splitter.h>
#include <wx/stc/stc.h>
#include <wx/timer.h>

#include <string>

#include "PdfSettings.h"
#include "PdfView.h"
#include "mdboss/Config.h"
#include "mdboss/DocumentWatcher.h"
#include "mdboss/Updater.h"
#include "mdboss/FileScan.h"
#include "mdboss/FindBar.h"
#include "mdboss/FindInFilesDialog.h"
#include "DocTreePanel.h"
#include "mdboss/OutlinePanel.h"
#include "mdboss/PathListPanel.h"
#include "mdboss/PreviewPane.h"

namespace docboss {

using namespace mdboss;

class MainFrame : public wxFrame {
public:
    MainFrame();

    // Open `path`, replacing the current document.  Returns false and leaves
    // the current document intact if the file cannot be read.
    // How a document came to be opened, which decides what opening it costs.
    //
    // kBrowse is the arrow keys walking the tree: it must not push a row onto
    // the recents list, must not write config.json, and must not raise a
    // dialog -- one per keystroke is not a prompt, it is an obstacle.  It
    // gives up quietly instead, leaving Enter to do the full job.
    enum class OpenMode { kExplicit, kBrowse };
    bool open_path(const std::string& path,
                   OpenMode mode = OpenMode::kExplicit);

    // WM_COPYDATA carries a document path from a second launch; see
    // SingleInstance.h for why there is only ever one window.
    WXLRESULT MSWWindowProc(WXUINT message, WXWPARAM wparam,
                            WXLPARAM lparam) override;

private:
    void build_menu();
    void build_toolbar();
    void build_panes();
    void bind_events();

    void on_open(wxCommandEvent& event);
    void on_save(wxCommandEvent& event);
    void on_exit(wxCommandEvent& event);
    void on_toggle_front_matter(wxCommandEvent& event);
    void on_manage_folders(wxCommandEvent& event);
    void on_new(wxCommandEvent& event);
    void on_new_from_template(wxCommandEvent& event);
    // Close the document without closing the window: the editor and preview
    // go back to empty and no file is open.
    void on_close_document(wxCommandEvent& event);
    // Insert one of the fixed snippet blocks at the caret; see kSnippets.
    void on_snippet(wxCommandEvent& event);
    // Pick an image file and insert a Markdown reference to it.
    void on_insert_image(wxCommandEvent& event);
    // Drop `body` in at the caret as its own block, adding the blank lines
    // around it that make Markdown treat it as one.
    void insert_block(const std::string& body);
    void on_open_templates_folder(wxCommandEvent& event);
    // Offer any starter template this profile has not seen, and save.
    void seed_starter_templates();
    // The three MD_Internal lists, and a way to reach the folder itself.
    void on_add_login(wxCommandEvent& event);
    void on_add_todo(wxCommandEvent& event);
    void on_add_diary(wxCommandEvent& event);
    // One dated fact, appended as a row of MD_Internal\Facts.md.
    void on_add_fact(wxCommandEvent& event);
    void on_open_internal_folder(wxCommandEvent& event);
    // Rebuild MD_Internal\TechNotes.md from the documents on disk and open it.
    void on_tech_notes(wxCommandEvent& event);
    // Rebuild it without opening it, and reload it in place when it is the
    // document already on screen.  Separate from Show because the list is
    // derived and goes stale as soon as a note is added, renamed or deleted.
    void on_refresh_tech_notes(wxCommandEvent& event);
    // The shared half: rebuild and write MD_Internal\TechNotes.md, reporting
    // where it went.  False when nothing was written, having already said why.
    bool rebuild_tech_note_index(std::string* index_path);
    // Add the front matter that makes a document a tech note: a GUID, a number
    // for its year, and the TechNote keyword.  Only ever adds -- see
    // promote_to_tech_note().  Raised from the Lists menu for the open
    // document, and from the tree's context menu for any document.
    void on_promote_document(wxCommandEvent& event);
    // Find in this document: the bar along the bottom of the window.  Ctrl+F
    // opens it, F3 and Shift+F3 step through, and both of those open it when
    // there is nothing yet to repeat.
    void on_find(wxCommandEvent& event);
    void on_find_next(wxCommandEvent& event);
    void on_find_previous(wxCommandEvent& event);
    void step_find(int direction, wxCommandEvent& event);
    // Do what the bar is asking and report the count back to it.  The bar
    // knows nothing about the editor; this is the only place the two meet.
    void find_in_document(const FindRequest& request);
    // Where the caret is, as a byte offset -- Scintilla positions and the
    // matcher's offsets are the same thing, which is why no conversion
    // appears anywhere in this feature.
    std::size_t editor_selection_start() const;
    // Find in all documents: the modeless results list.
    void on_find_in_files(wxCommandEvent& event);
    // Open the document a result names and select the match inside it.
    void open_match(const DocumentMatch& match, const std::string& needle,
                    const SearchOptions& options);
    // Back: return to the document shown before this one.
    //
    // A history of what was SHOWN, browsing included -- after arrowing through
    // a folder, "the one I was just looking at" is the only reading that
    // matches the screen.  That makes it a different list from recents, which
    // records what was chosen.
    void on_back(wxCommandEvent& event);
    void on_forward(wxCommandEvent& event);
    // Both are one call into this: `step` is -1 for Back, +1 for Forward.
    void navigate_history(int step);
    void push_history(const std::string& path);
    void update_nav_state();
    void promote_tech_note(const std::string& path);
    // Export the rendered preview to PDF via WebView2's own print pipeline.
    void on_export_pdf(wxCommandEvent& event);
    // Publish the open document: render it to <same folder>\<stem>.pdf,
    // overwriting, without a dialog.  Refused while the document is unsaved
    // -- the PDF must say what the file says -- and for an untitled buffer.
    void on_publish(wxCommandEvent& event);
    // The same, for any document: opens it first if it is not the open one.
    void publish_document(const std::string& md_path);
    // Show the PDF published from the open document.
    void on_open_published(wxCommandEvent& event);
    // Open a PDF in the viewer page.  Same contract as open_path(), which
    // routes here for a .pdf.
    bool open_pdf(const std::string& path, OpenMode mode);
    // Leave the PDF page for the Markdown page, closing any PDF.  False when
    // the user cancelled saving its highlights.
    bool leave_pdf(OpenMode mode);
    // What the window is showing: the open document, or the open PDF.
    std::string shown_path() const;
    // The two halves of a publish that waits for its page to load.
    void on_page_loaded();
    void on_publish_timer(wxTimerEvent& event);
    // Back to the user's own Hide YAML setting after a publish.
    void end_publish_render();
    // Switch the preview stylesheet (View menu). Persists and re-renders.
    void on_preview_theme(wxCommandEvent& event);
    // A document moved on disk (dragged in the tree, or renamed): rewrite the
    // absolute paths held in favorites and recents, and follow it if it is the
    // document currently open.
    void on_path_moved(const std::string& from, const std::string& to);
    // Shared tail of the three: append, report a failure, refresh the tree.
    void save_internal_entry(const std::string& filename,
                             const std::string& seed,
                             const std::string& block, const wxString& what);
    void on_refresh(wxCommandEvent& event);
    void on_toggle_files(wxCommandEvent& event);
    void on_toggle_outline(wxCommandEvent& event);
    void on_toggle_editor(wxCommandEvent& event);
    // Hide the rendered preview, leaving the editor the whole pane.  Shares
    // one splitter with the editor, so the two toggles are not independent:
    // hiding both would leave nothing, and either toggle restores the pair.
    void on_toggle_preview(wxCommandEvent& event);
    void on_file_types(wxCommandEvent& event);
    void on_help(wxCommandEvent& event);
    void on_about(wxCommandEvent& event);
    void on_check_updates(wxCommandEvent& event);
    // Download the right asset for this copy -- the installer, or the
    // portable zip when no uninstaller sits beside the exe -- then hand over
    // to a batch and exit.  Split in two because the download is
    // asynchronous: the second half runs from its completion callback.
    void install_update(const ReleaseInfo& info, bool portable);
    void hand_off_to_installer(const std::string& setup_path);
    void hand_off_to_portable(const std::string& zip_path);
    // Shared tail of both hand-offs: write the batch, spawn it hidden, and
    // close the app so the batch's wait loop can finish.
    void spawn_handoff_and_close(const std::string& batch_path,
                                 const std::string& text);
    void on_toggle_favorite(wxCommandEvent& event);
    void on_export_favorites();
    void on_import_favorites();
    // Copy chosen Markdown files into MD_Inbox and open the first, which is
    // what makes the command feel like an import rather than a file copy.
    void on_import_to_inbox();
    void refresh_lists();
    void on_text_changed(wxStyledTextEvent& event);
    void on_editor_scrolled(wxStyledTextEvent& event);
    void on_render_timer(wxTimerEvent& event);
    // Drops the guard on the preview's scroll echo once it can no longer be
    // ours; see kScrollEchoMs.
    void on_scroll_echo_timer(wxTimerEvent& event);
    void on_close(wxCloseEvent& event);

    void render_preview();
    void update_title();
    // Back to no document open.  Asks nothing -- the caller decides whether
    // unsaved work needs confirming first.
    void clear_document();
    // Grey the Close command out when there is nothing to close.  Called from
    // update_title(), which every document-state change already ends in.
    void update_close_enabled();
    // Keep the View menu item and the toolbar button showing the same state:
    // they are two check controls sharing one id and wx syncs neither.
    void sync_front_matter_checks(bool hide);
    // Record which panes are showing, the moment it changes.
    void save_pane_visibility();
    // The configured root folders, as plain paths.
    std::vector<std::string> root_paths() const;
    // The open document changed underneath us.  Never clobbers unsaved work:
    // a modified buffer is kept and the user told, because the edits in the
    // editor are the only copy of themselves and the file on disk is not.
    void on_document_changed(const std::string& path, bool still_exists);
    void reload_from_disk();
    bool save_to(const std::string& path);
    bool confirm_discard();
    void sync_preview_from_editor();
    void on_preview_scrolled(double ratio);

    Config config_;
    // The PDF half of the settings, merged into the same config.json.
    PdfSettings pdf_settings_;
    // Page 0 is outline_split_ (the Markdown page), page 1 is pdf_view_.
    wxSimplebook* content_ = nullptr;
    PdfView* pdf_view_ = nullptr;
    bool showing_pdf_ = false;
    // Left column is Recent over Favorites over Files; that column then sits
    // left of Outline, which sits left of the editor/preview pair.  Five
    // splitters, because wxSplitterWindow only ever holds two panes.
    wxSplitterWindow* files_split_ = nullptr;
    wxSplitterWindow* recent_split_ = nullptr;
    wxSplitterWindow* favorites_split_ = nullptr;
    wxSplitterWindow* outline_split_ = nullptr;
    wxSplitterWindow* split_ = nullptr;
    PathListPanel* recent_ = nullptr;
    PathListPanel* favorites_ = nullptr;
    DocTreePanel* files_ = nullptr;
    OutlinePanel* outline_ = nullptr;
    wxStyledTextCtrl* editor_ = nullptr;
    PreviewPane* preview_ = nullptr;
    FindBar* find_bar_ = nullptr;
    // Built on first use and then kept, hidden between uses, so a results
    // list survives going away to read one of the documents in it.
    FindInFilesDialog* find_in_files_ = nullptr;
    // Where the current incremental search started, so typing refines the
    // same search instead of walking forward through the document.
    std::size_t find_origin_ = 0;
    wxTimer render_timer_;
    wxTimer scroll_echo_timer_;
    wxTimer publish_timer_;
    // The document a publish is waiting to print, or empty.
    std::string pending_publish_;
    // Set from the start of a publish until its PDF is written (or the
    // publish is cancelled): the preview renders without YAML front matter
    // for the whole of that, so the print cannot catch it.
    bool publish_render_ = false;
    DocumentWatcher watcher_;

    std::string current_path_;
    // Tech-note numbers handed out this session.  A new note's number is
    // derived from the notes on disk, and a note that has not been saved yet
    // is not on disk -- so without this, two notes created back to back and
    // saved afterwards would both claim the same number.
    std::vector<std::string> issued_tn_indices_;
    // Documents shown this session, oldest first, with a CURSOR rather than a
    // stack top: Forward needs the entries after the current one to survive
    // going Back, which a stack that popped them could not offer.  Bounded,
    // and re-showing the current document is not a move.
    std::vector<std::string> history_;
    // Index into history_ of the document on screen.  Meaningless while
    // history_ is empty, which is the only time it may equal size().
    std::size_t history_pos_ = 0;
    // Set while Back or Forward is doing the opening, so the move does not
    // record itself and leave the pair shuffling two documents for ever.
    bool navigating_ = false;
    bool dirty_ = false;
    // Set once the installer has been handed the job, so the close that
    // follows does not ask about unsaved work a second time.
    bool updating_ = false;
    // Sash positions remembered across a hide, so showing a pane again
    // restores the width the user had chosen rather than a default.
    int hidden_files_sash_ = 0;
    int hidden_outline_sash_ = 0;
    int hidden_editor_sash_ = 0;
    // Scroll sync echoes: a programmatic scroll on one side raises the same
    // event a user scroll would, so each side ignores movement it caused.
    bool suppress_editor_scroll_ = false;
    bool suppress_preview_scroll_ = false;
};

}  // namespace docboss

#endif  // DOCBOSS_APP_MAIN_FRAME_H
