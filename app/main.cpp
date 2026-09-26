// DocBoss entry point.
//
// The first thing it does is set the identity the pulled MD Boss units run
// under (Identity.h), because every one of them -- the settings file, the
// preview's staging folder, the single-instance slot, the file association,
// the updater -- reads it the first time it is used.  Setting it any later
// would let one of them act as MD Boss first.

#include <wx/app.h>
#include <wx/cmdline.h>
#include <wx/filename.h>
#include <wx/image.h>
#include <wx/stdpaths.h>

#include <cassert>
#include <string>

#include "Identity.h"
#include "MainFrame.h"
#include "mdboss/AppIdentity.h"
#include "mdboss/FileAssoc.h"
#include "mdboss/SingleInstance.h"
#include "mdrender/MdRender.h"

namespace {

constexpr const char* kRegisterFlag = "--register-file-types";
constexpr const char* kUnregisterFlag = "--unregister-file-types";
constexpr const char* kProfileFlag = "--profile";

// "--profile <dir>" or "--profile=<dir>", read straight from argv because it
// has to be known before wx's parser runs -- the registration switches below
// act before any GUI exists, and they need the identity already set.
std::string profile_argument(int argc, wxChar** argv)
{
    for (int i = 1; i < argc; ++i) {   // bounded by argc
        const wxString arg(argv[i]);
        if (arg == kProfileFlag && i + 1 < argc) {
            return std::string(wxString(argv[i + 1]).ToUTF8());
        }
        if (arg.StartsWith(wxString(kProfileFlag) + "=")) {
            return std::string(
                arg.Mid(std::string(kProfileFlag).size() + 1).ToUTF8());
        }
    }
    return {};
}

// Handle the registration switches before any GUI exists: the installer runs
// them silently, and putting up a window would be wrong there.  Returns an
// exit code, or -1 to carry on and start normally.
int handle_registration_flags(int argc, wxChar** argv)
{
    for (int i = 1; i < argc; ++i) {
        const wxString arg(argv[i]);
        if (arg == kRegisterFlag) {
            const mdboss::RegPlan plan = mdboss::current_registration_plan();
            const bool ok = mdboss::apply_registration(plan);
            mdboss::notify_assoc_changed();
            return ok ? 0 : 1;
        }
        if (arg == kUnregisterFlag) {
            mdboss::remove_registration(mdboss::current_registration_plan());
            mdboss::notify_assoc_changed();
            return 0;
        }
    }
    return -1;
}

// The render assets (MD Boss's) sit beside the executable, installed or in a
// build tree -- the build copies them there.  The fallbacks cover a build tree
// whose copy step has not run.
void locate_assets()
{
    const wxFileName exe(wxStandardPaths::Get().GetExecutablePath());
    const wxString candidates[] = {
        exe.GetPath() + "\\assets",
        exe.GetPath() + "\\..\\..\\..\\..\\MDBoss\\assets",
        exe.GetPath() + "\\..\\..\\..\\..\\..\\MDBoss\\assets",
    };
    for (const wxString& candidate : candidates) {
        wxFileName dir(candidate + "\\");
        dir.Normalize(wxPATH_NORM_DOTS | wxPATH_NORM_ABSOLUTE);
        if (mdrender::set_asset_dir(std::string(dir.GetPath().ToUTF8()))) {
            return;
        }
    }
    // Left unset: render_document() then returns an empty page rather than a
    // half-built one, which is visible rather than silently wrong.
}

class DocBossApp : public wxApp {
public:
    // wx's default parser rejects any argument it was not told about, and
    // OnInit() would then return false -- so the app would exit silently for a
    // file handed over by the shell, or for --profile.
    void OnInitCmdLine(wxCmdLineParser& parser) override
    {
        wxApp::OnInitCmdLine(parser);
        parser.AddOption("", "profile",
                         "Keep settings and preview staging in this folder "
                         "instead of the per-user default");
        parser.AddParam("Markdown document or PDF to open",
                        wxCMD_LINE_VAL_STRING, wxCMD_LINE_PARAM_OPTIONAL);
    }

    bool OnCmdLineParsed(wxCmdLineParser& parser) override
    {
        if (parser.GetParamCount() > 0) {
            document_ = std::string(parser.GetParam(0).ToUTF8());
        }
        return wxApp::OnCmdLineParsed(parser);
    }

    // Returning false from OnInit() makes wxEntry return -1 (exit code 255),
    // so a successful --register-file-types would look like a failure to the
    // installer.  OnInit() succeeds instead and OnRun() returns the real code.
    int OnRun() override
    {
        if (exit_immediately_) {
            return exit_code_;
        }
        return wxApp::OnRun();
    }

    bool OnInit() override
    {
        mdboss::set_app_identity(docboss::make_identity(profile_argument(argc, argv)));
        assert(mdboss::app_identity().display_name == "DocBoss" &&
               "the identity is set before anything reads it");

        const int registration = handle_registration_flags(argc, argv);
        if (registration >= 0) {
            exit_immediately_ = true;
            exit_code_ = registration;
            return true;
        }
        if (!wxApp::OnInit()) {
            return false;
        }
        // PDFBoss registers every handler: the viewer's page bitmaps and the
        // toolbar art go through wxImage.
        wxInitAllImageHandlers();
        locate_assets();

        // One window per session: hand the file to the instance that is
        // already up rather than starting a rival that would overwrite its
        // settings on exit.
        if (mdboss::forward_to_running(document_)) {
            exit_immediately_ = true;
            exit_code_ = 0;
            return true;
        }

        auto* frame = new docboss::MainFrame();
        frame->Show(true);
        mdboss::mark_as_instance(frame->GetHandle());
        if (!document_.empty()) {
            frame->open_path(document_);
        }
        return true;
    }

private:
    std::string document_;
    bool exit_immediately_ = false;
    int exit_code_ = 0;
};

}  // namespace

wxIMPLEMENT_APP(DocBossApp);
