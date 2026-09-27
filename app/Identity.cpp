#include "Identity.h"

#include <cassert>
#include <cstdint>
#include <cstdlib>
#include <filesystem>

#include "mdboss/PathUtf8.h"

namespace docboss {
namespace {

// getenv() is deprecated under /W4 /WX on MSVC; _dupenv_s hands back an
// allocation the caller owns.
std::string environment(const char* name)
{
    assert(name != nullptr && *name != '\0');
    char* value = nullptr;
    std::size_t size = 0;
    if (_dupenv_s(&value, &size, name) != 0 || value == nullptr) {
        return {};
    }
    std::string out(value);
    std::free(value);
    return out;
}

// The first of `names` that is set, else ".".
std::string first_set(const char* first, const char* second)
{
    std::string value = environment(first);
    if (value.empty()) {
        value = environment(second);
    }
    return value.empty() ? std::string(".") : value;
}

// A short, stable fingerprint of a profile folder for kernel object names:
// FNV-1a over the case-folded path, as hex.  Not security -- only distinct
// per folder, and free of the backslashes a mutex name may not contain.
std::wstring profile_key(const std::string& profile_dir)
{
    std::uint64_t hash = 1469598103934665603ULL;
    for (const char c : profile_dir) {   // bounded by the path
        const char folded =
            (c >= 'A' && c <= 'Z') ? static_cast<char>(c - 'A' + 'a')
                                   : (c == '/' ? '\\' : c);
        hash ^= static_cast<unsigned char>(folded);
        hash *= 1099511628211ULL;
    }
    wchar_t text[17] = {};
    for (int i = 15; i >= 0; --i) {   // bounded: 16 hex digits
        text[i] = L"0123456789abcdef"[hash & 0xF];
        hash >>= 4;
    }
    return std::wstring(text, 16);
}

std::string join(const std::string& dir, const char* leaf)
{
    return mdboss::path_to_utf8(mdboss::path_from_utf8(dir) / leaf);
}

}  // namespace

mdboss::AppIdentity make_identity(const std::string& profile_dir)
{
    mdboss::AppIdentity identity;
    identity.display_name = "DocBoss";
    if (profile_dir.empty()) {
        identity.data_dir = join(first_set("APPDATA", "USERPROFILE"), "DocBoss");
        identity.staging_dir = join(first_set("TEMP", "TMP"), "DocBoss");
    } else {
        identity.data_dir = profile_dir;
        identity.staging_dir = join(profile_dir, "staging");
    }

    identity.prog_id = "DocBoss.Markdown";
    identity.assoc_display_name = "DocBoss";
    identity.assoc_app_name = "DocBoss";
    identity.assoc_description =
        "Markdown sources and the PDFs published from them, in one tree.";

    identity.instance_prop = L"DocBoss.Instance";
    identity.instance_mutex = L"Local\\DocBoss.SingleInstance";
    if (!profile_dir.empty()) {
        // A separate profile is a separate instance.  The slot is about two
        // copies overwriting one config.json on exit; two profiles never share
        // one, so they must not share the slot either.  Sharing it handed a
        // scratch copy's command-line document to the user's own running
        // window (27 Sep 2026).  Keyed on the profile folder, case-folded as
        // Windows compares paths.
        const std::wstring suffix = L"." + profile_key(profile_dir);
        identity.instance_prop += suffix;
        identity.instance_mutex += suffix;
    }

    identity.releases_api_url =
        "https://api.github.com/repos/Flinterpop/DocBoss/releases/latest";
    identity.releases_page_url = "https://github.com/Flinterpop/DocBoss/releases";
    identity.setup_asset = "DocBoss-Setup.exe";
    identity.portable_asset = "DocBoss-Portable.zip";
    identity.exe_name = "DocBoss.exe";
    identity.portable_folder = "DocBoss";

    assert(!identity.data_dir.empty() && !identity.staging_dir.empty() &&
           "both folders always resolve");
    assert(identity.data_dir != identity.staging_dir &&
           "the preview's staging must not land in the settings folder");
    return identity;
}

}  // namespace docboss
