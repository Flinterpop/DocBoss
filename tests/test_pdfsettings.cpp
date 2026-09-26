// The PDF settings share config.json with mdboss::Config.  Each writer must
// keep the other's keys, or saving one would silently reset the other.

#include <catch2/catch_test_macros.hpp>

#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>

#include <nlohmann/json.hpp>

#include "PdfSettings.h"
#include "mdboss/PathUtf8.h"

namespace {

namespace fs = std::filesystem;

// A throwaway folder, removed afterwards.  Under the temp folder, which the
// test run points at build/claude-scratch/tmp (see CLAUDE.md).
class TempDir {
public:
    TempDir() : dir_(fs::temp_directory_path() / "docboss_settings_test")
    {
        std::error_code ec;
        fs::remove_all(dir_, ec);
        fs::create_directories(dir_, ec);
    }
    ~TempDir()
    {
        std::error_code ec;
        fs::remove_all(dir_, ec);
    }
    TempDir(const TempDir&) = delete;
    TempDir& operator=(const TempDir&) = delete;
    std::string file(const char* name) const
    {
        return mdboss::path_to_utf8(dir_ / name);
    }

private:
    fs::path dir_;
};

nlohmann::json read_json(const std::string& file)
{
    std::ifstream stream(mdboss::path_from_utf8(file), std::ios::binary);
    std::ostringstream buffer;
    buffer << stream.rdbuf();
    return nlohmann::json::parse(buffer.str());
}

}  // namespace

TEST_CASE("settings round-trip through the file", "[pdfsettings]")
{
    const TempDir dir;
    const std::string file = dir.file("config.json");
    {
        docboss::PdfSettings settings(file);
        settings.set_last_page("C:\\Docs\\a.pdf", 4);
        settings.set_fit("page");
        settings.set_topics_sash(300);
        REQUIRE(settings.save());
    }
    docboss::PdfSettings loaded(file);
    loaded.load();
    CHECK(loaded.last_page("C:\\Docs\\a.pdf") == 4);
    CHECK(loaded.last_page("c:\\docs\\A.PDF") == 4);   // same file on Windows
    CHECK_FALSE(loaded.last_page("C:\\Docs\\b.pdf").has_value());
    CHECK(loaded.fit() == "page");
    CHECK(loaded.topics_sash() == 300);
}

TEST_CASE("saving keeps every key it does not own", "[pdfsettings]")
{
    const TempDir dir;
    const std::string file = dir.file("config.json");
    {
        std::ofstream out(mdboss::path_from_utf8(file), std::ios::binary);
        out << R"({"roots": [{"name": "Notes", "path": "C:\\Notes"}],)"
            << R"( "wx_preview_theme": "notes"})";
    }
    docboss::PdfSettings settings(file);
    settings.load();
    settings.set_last_page("C:\\Notes\\x.pdf", 1);
    REQUIRE(settings.save());

    const nlohmann::json document = read_json(file);
    CHECK(document["wx_preview_theme"] == "notes");
    CHECK(document["roots"][0]["name"] == "Notes");
    CHECK(document.contains("docboss_pdf_pages"));
}

TEST_CASE("the most recent page wins and the list is bounded", "[pdfsettings]")
{
    const TempDir dir;
    docboss::PdfSettings settings(dir.file("config.json"));
    settings.set_last_page("C:\\a.pdf", 1);
    settings.set_last_page("C:\\a.pdf", 7);
    CHECK(settings.last_page("C:\\a.pdf") == 7);

    for (std::size_t i = 0; i < docboss::kMaxRememberedPages + 5; ++i) {
        settings.set_last_page("C:\\many\\" + std::to_string(i) + ".pdf", 0);
    }
    // The oldest fell off; the newest is kept.
    CHECK_FALSE(settings.last_page("C:\\a.pdf").has_value());
    CHECK(settings.last_page("C:\\many\\" +
                             std::to_string(docboss::kMaxRememberedPages + 4) +
                             ".pdf") == 0);
}

TEST_CASE("an unknown fit reads as width", "[pdfsettings]")
{
    const TempDir dir;
    docboss::PdfSettings settings(dir.file("config.json"));
    settings.set_fit("sideways");
    CHECK(settings.fit() == "width");
}

TEST_CASE("a missing or damaged file loads as defaults", "[pdfsettings]")
{
    const TempDir dir;
    const std::string file = dir.file("config.json");
    {
        std::ofstream out(mdboss::path_from_utf8(file), std::ios::binary);
        out << "{ not json";
    }
    docboss::PdfSettings settings(file);
    settings.load();
    CHECK(settings.fit() == "width");
    CHECK(settings.topics_sash() == 0);
}
