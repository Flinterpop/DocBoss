// Pairing documents with their published PDFs, and telling stale from current.
//
// Pure: classify_pairs() never touches the disk, so every case is built from
// DocEntry values with the times written in.

#include <catch2/catch_test_macros.hpp>

#include <string>
#include <vector>

#include "DocPairs.h"

namespace {

mdboss::DocEntry entry(const std::string& path, std::int64_t modified)
{
    mdboss::DocEntry e;
    e.path = path;
    const std::size_t slash = path.find_last_of('\\');
    e.name = (slash == std::string::npos) ? path : path.substr(slash + 1);
    e.modified = modified;
    return e;
}

}  // namespace

TEST_CASE("a PDF newer than its source is current", "[pairs]")
{
    const auto pairing = docboss::classify_pairs(
        {entry("C:\\Docs\\guide.md", 100)}, {entry("C:\\Docs\\guide.pdf", 200)});
    CHECK(docboss::pdf_state(pairing, "C:\\Docs\\guide.md") ==
          docboss::PdfState::kCurrent);
    CHECK(pairing.loose_pdfs.empty());
    CHECK(pairing.published.begin()->second.pdf_path == "C:\\Docs\\guide.pdf");
}

TEST_CASE("a source edited after its PDF is stale", "[pairs]")
{
    const auto pairing = docboss::classify_pairs(
        {entry("C:\\Docs\\guide.md", 300)}, {entry("C:\\Docs\\guide.pdf", 200)});
    CHECK(docboss::pdf_state(pairing, "C:\\Docs\\guide.md") ==
          docboss::PdfState::kStale);
}

TEST_CASE("the same instant counts as current", "[pairs]")
{
    // A PDF exported in the same clock tick as the save IS that save.
    const auto pairing = docboss::classify_pairs(
        {entry("C:\\Docs\\a.md", 500)}, {entry("C:\\Docs\\a.pdf", 500)});
    CHECK(docboss::pdf_state(pairing, "C:\\Docs\\a.md") ==
          docboss::PdfState::kCurrent);
}

TEST_CASE("an unreadable source time never reads as stale", "[pairs]")
{
    const auto pairing = docboss::classify_pairs(
        {entry("C:\\Docs\\a.md", 0)}, {entry("C:\\Docs\\a.pdf", 500)});
    CHECK(docboss::pdf_state(pairing, "C:\\Docs\\a.md") ==
          docboss::PdfState::kCurrent);
}

TEST_CASE("a document with no PDF is unpublished", "[pairs]")
{
    const auto pairing =
        docboss::classify_pairs({entry("C:\\Docs\\notes.md", 1)}, {});
    CHECK(docboss::pdf_state(pairing, "C:\\Docs\\notes.md") ==
          docboss::PdfState::kNone);
    CHECK(pairing.published.empty());
}

TEST_CASE("a PDF with no source is loose", "[pairs]")
{
    const auto pairing = docboss::classify_pairs(
        {entry("C:\\Docs\\notes.md", 1)},
        {entry("C:\\Docs\\vendor-manual.pdf", 1)});
    REQUIRE(pairing.loose_pdfs.size() == 1);
    CHECK(pairing.loose_pdfs[0].name == "vendor-manual.pdf");
    CHECK(pairing.published.empty());
}

TEST_CASE("pairing ignores case, as Windows does", "[pairs]")
{
    const auto pairing = docboss::classify_pairs(
        {entry("C:\\Docs\\Install Guide.md", 1)},
        {entry("C:\\docs\\INSTALL GUIDE.PDF", 2)});
    CHECK(docboss::pdf_state(pairing, "c:\\docs\\install guide.md") ==
          docboss::PdfState::kCurrent);
    CHECK(pairing.loose_pdfs.empty());
}

TEST_CASE("only the same folder pairs", "[pairs]")
{
    // The old layout this app replaces: sources in one tree, PDFs in another.
    const auto pairing = docboss::classify_pairs(
        {entry("C:\\Docs\\src\\guide.md", 1)},
        {entry("C:\\Docs\\pdf\\guide.pdf", 2)});
    CHECK(docboss::pdf_state(pairing, "C:\\Docs\\src\\guide.md") ==
          docboss::PdfState::kNone);
    CHECK(pairing.loose_pdfs.size() == 1);
}

TEST_CASE("a stem with dots pairs on the whole stem", "[pairs]")
{
    const auto pairing = docboss::classify_pairs(
        {entry("C:\\Docs\\v1.2-notes.md", 1), entry("C:\\Docs\\v1.md", 1)},
        {entry("C:\\Docs\\v1.2-notes.pdf", 2)});
    CHECK(docboss::pdf_state(pairing, "C:\\Docs\\v1.2-notes.md") ==
          docboss::PdfState::kCurrent);
    CHECK(docboss::pdf_state(pairing, "C:\\Docs\\v1.md") ==
          docboss::PdfState::kNone);
}

TEST_CASE("stale documents are listed, in path order, by folder", "[pairs]")
{
    const auto pairing = docboss::classify_pairs(
        {entry("C:\\Docs\\b.md", 9), entry("C:\\Docs\\a.md", 9),
         entry("C:\\Docs\\current.md", 1), entry("C:\\Docs\\Sub\\c.md", 9),
         entry("C:\\Docs2\\d.md", 9), entry("C:\\Docs\\never.md", 9)},
        {entry("C:\\Docs\\b.pdf", 5), entry("C:\\Docs\\a.pdf", 5),
         entry("C:\\Docs\\current.pdf", 5), entry("C:\\Docs\\Sub\\c.pdf", 5),
         entry("C:\\Docs2\\d.pdf", 5)});

    // Everything stale, never-published and current ones left out.  The
    // paths come back as scanned, not lower-cased.
    const std::vector<std::string> all = docboss::stale_documents(pairing, "");
    REQUIRE(all.size() == 4);
    CHECK(all[0] == "C:\\Docs\\a.md");
    CHECK(all[1] == "C:\\Docs\\b.md");
    CHECK(all[2] == "C:\\Docs\\Sub\\c.md");
    CHECK(all[3] == "C:\\Docs2\\d.md");

    // A folder takes its subfolders but not a sibling that shares its name
    // as a prefix.
    const std::vector<std::string> docs =
        docboss::stale_documents(pairing, "c:\\docs");
    CHECK(docs.size() == 3);
    const std::vector<std::string> sub =
        docboss::stale_documents(pairing, "C:\\Docs\\Sub\\");
    REQUIRE(sub.size() == 1);
    CHECK(sub[0] == "C:\\Docs\\Sub\\c.md");
}

TEST_CASE("a .markdown source pairs like a .md one", "[pairs]")
{
    const auto pairing = docboss::classify_pairs(
        {entry("C:\\Docs\\spec.markdown", 1)}, {entry("C:\\Docs\\spec.pdf", 2)});
    CHECK(docboss::pdf_state(pairing, "C:\\Docs\\spec.markdown") ==
          docboss::PdfState::kCurrent);
}
