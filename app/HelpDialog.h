// The Help window: HELP.md rendered with the app's own renderer.
//
// Deliberately the real document rather than a copy of its text in a message
// box, so there is one HELP.md and it cannot drift.  It renders through
// PreviewPane, which means the same network lock applies here as in the
// preview -- a Help page can no more reach the network than a document can.

#ifndef DOCBOSS_APP_HELP_DIALOG_H
#define DOCBOSS_APP_HELP_DIALOG_H

#include <wx/dialog.h>

#include <string>

namespace docboss {

class HelpDialog : public wxDialog {
public:
    explicit HelpDialog(wxWindow* parent);
};

// Absolute path to HELP.md if it can be found beside the executable or in the
// source tree above it, else an empty string.
std::string find_help_document();

// Show the standard About box.
void show_about_box(wxWindow* parent);

}  // namespace docboss

#endif  // DOCBOSS_APP_HELP_DIALOG_H
