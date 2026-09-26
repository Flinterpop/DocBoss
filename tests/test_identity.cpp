// DocBoss's identity must name nothing of MD Boss's.  Any leftover would make
// the pulled units read MD Boss's profile, share its single-instance slot,
// claim its ProgID, or update DocBoss from MD Boss's releases -- all silently.

#include <catch2/catch_test_macros.hpp>

#include <string>

#include "Identity.h"
#include "mdboss/PathUtf8.h"

namespace {

bool mentions_mdboss(const std::string& text)
{
    std::string lower;
    for (const char c : text) {   // bounded by the string
        lower += static_cast<char>((c >= 'A' && c <= 'Z') ? c - 'A' + 'a' : c);
    }
    return lower.find("mdboss") != std::string::npos ||
           lower.find("md boss") != std::string::npos;
}

bool mentions_mdboss(const std::wstring& text)
{
    std::string narrow;
    for (const wchar_t c : text) {   // bounded by the string
        narrow += static_cast<char>(c < 128 ? c : '?');
    }
    return mentions_mdboss(narrow);
}

}  // namespace

TEST_CASE("nothing in the identity is MD Boss's", "[identity]")
{
    const mdboss::AppIdentity identity = docboss::make_identity("");
    CHECK(identity.display_name == "DocBoss");
    for (const std::string& text :
         {identity.display_name, identity.data_dir, identity.staging_dir,
          identity.prog_id, identity.assoc_display_name,
          identity.assoc_app_name, identity.assoc_description,
          identity.releases_api_url, identity.releases_page_url,
          identity.setup_asset, identity.portable_asset, identity.exe_name,
          identity.portable_folder}) {
        CHECK_FALSE(mentions_mdboss(text));
    }
    CHECK_FALSE(mentions_mdboss(identity.instance_prop));
    CHECK_FALSE(mentions_mdboss(identity.instance_mutex));
    CHECK(identity.releases_api_url.find("/Flinterpop/DocBoss/") !=
          std::string::npos);
}

TEST_CASE("the default profile is DocBoss's own folder", "[identity]")
{
    const mdboss::AppIdentity identity = docboss::make_identity("");
    CHECK(mdboss::path_from_utf8(identity.data_dir).filename() == "DocBoss");
    CHECK(mdboss::path_from_utf8(identity.staging_dir).filename() == "DocBoss");
    CHECK(identity.data_dir != identity.staging_dir);
}

TEST_CASE("a profile folder moves everything under it", "[identity]")
{
    const mdboss::AppIdentity identity =
        docboss::make_identity("D:\\scratch\\profile");
    CHECK(identity.data_dir == "D:\\scratch\\profile");
    CHECK(identity.staging_dir == "D:\\scratch\\profile\\staging");
}
