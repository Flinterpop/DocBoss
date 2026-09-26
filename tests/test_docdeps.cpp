// What a document's PDF depends on besides its own text: the local images.

#include <catch2/catch_test_macros.hpp>

#include <chrono>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

#include "DocDeps.h"
#include "mdboss/PathUtf8.h"

namespace {

namespace fs = std::filesystem;

using Refs = std::vector<std::string>;

}  // namespace

TEST_CASE("image references of every form are found", "[docdeps]")
{
    const char* const text =
        "![shot](img/shot.png)\n"
        "![titled](fig.png \"Figure 1\")\n"
        "![spaced](<my shot.png>)\n"
        "<img src=\"pics/a.png\" style=\"zoom: 40%;\" />\n"
        "<img alt='x' src='pics/b.jpg'>\n"
        "![ref][logo]\n"
        "[logo]: ../shared/logo.svg\n"
        "[docs]: other-doc.md\n";
    CHECK(docboss::local_references(text) ==
          Refs{"img/shot.png", "fig.png", "my shot.png", "pics/a.png",
               "pics/b.jpg", "../shared/logo.svg"});
}

TEST_CASE("remote, inline and anchor targets are not local files",
          "[docdeps]")
{
    const char* const text =
        "![web](https://example.com/a.png)\n"
        "![inline](data:image/png;base64,AAAA)\n"
        "![anchor](#section)\n"
        "![drive](C:/figs/c.png)\n";
    // A drive letter is a path, not a URL scheme.
    CHECK(docboss::local_references(text) == Refs{"C:/figs/c.png"});
}

TEST_CASE("percent escapes, queries and duplicates are handled", "[docdeps]")
{
    const char* const text =
        "![a](my%20figure.png?v=2)\n"
        "![again](my%20figure.png)\n";
    CHECK(docboss::local_references(text) == Refs{"my figure.png"});
}

TEST_CASE("a newer image makes a current PDF stale", "[docdeps]")
{
    const fs::path dir = fs::temp_directory_path() / "docboss_deps_test";
    std::error_code ec;
    fs::remove_all(dir, ec);
    fs::create_directories(dir / "img", ec);
    {
        std::ofstream(dir / "guide.md") << "# Guide\n\n![f](img/f.png)\n";
        std::ofstream(dir / "img" / "f.png") << "png";
        std::ofstream(dir / "plain.md") << "# Plain\n";
    }
    const auto now = fs::file_time_type::clock::now();
    const std::int64_t pdf_time =
        (now - std::chrono::hours(1)).time_since_epoch().count();
    fs::last_write_time(dir / "img" / "f.png", now, ec);

    CHECK(docboss::newest_dependency(mdboss::path_to_utf8(dir / "guide.md")) >
          pdf_time);
    CHECK(docboss::newest_dependency(mdboss::path_to_utf8(dir / "plain.md")) ==
          0);

    docboss::Pairing pairing;
    for (const char* name : {"guide.md", "plain.md"}) {
        docboss::PdfLink link;
        link.md_path = mdboss::path_to_utf8(dir / name);
        link.pdf_path = link.md_path + ".pdf";
        link.pdf_modified = pdf_time;
        link.state = docboss::PdfState::kCurrent;
        pairing.published[name] = link;
    }
    docboss::apply_dependency_times(pairing);
    CHECK(pairing.published["guide.md"].state == docboss::PdfState::kStale);
    CHECK(pairing.published["plain.md"].state == docboss::PdfState::kCurrent);
    fs::remove_all(dir, ec);
}
