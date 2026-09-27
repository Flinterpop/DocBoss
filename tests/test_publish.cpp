// Where a published PDF goes, and what counts as a PDF.

#include <catch2/catch_test_macros.hpp>

#include <filesystem>
#include <fstream>
#include <string>

#include "Publish.h"
#include "mdboss/PathUtf8.h"

TEST_CASE("the PDF goes beside its source", "[publish]")
{
    CHECK(docboss::published_pdf_path("C:\\Docs\\guide.md") ==
          "C:\\Docs\\guide.pdf");
    CHECK(docboss::published_pdf_path("C:\\Docs\\spec.markdown") ==
          "C:\\Docs\\spec.pdf");
}

TEST_CASE("only the last extension is replaced", "[publish]")
{
    CHECK(docboss::published_pdf_path("C:\\Docs\\v1.2-notes.md") ==
          "C:\\Docs\\v1.2-notes.pdf");
}

TEST_CASE("spaces and non-ASCII names survive", "[publish]")
{
    CHECK(docboss::published_pdf_path("C:\\My Docs\\install guide.md") ==
          "C:\\My Docs\\install guide.pdf");
    // UTF-8 in, UTF-8 out: "café.md" -> "café.pdf".
    CHECK(docboss::published_pdf_path("C:\\Docs\\caf\xC3\xA9.md") ==
          "C:\\Docs\\caf\xC3\xA9.pdf");
}

TEST_CASE("a PDF finds the document it was published from", "[publish]")
{
    namespace fs = std::filesystem;
    const fs::path dir = fs::temp_directory_path() / "docboss_source_test";
    std::error_code ec;
    fs::remove_all(dir, ec);
    fs::create_directories(dir, ec);
    std::ofstream(dir / "guide.md") << "# Guide\n";
    std::ofstream(dir / "spec.markdown") << "# Spec\n";
    std::ofstream(dir / "guide.pdf") << "%PDF";

    const std::string guide = mdboss::path_to_utf8(dir / "guide.pdf");
    CHECK(docboss::source_document_for(guide) ==
          mdboss::path_to_utf8(dir / "guide.md"));
    CHECK(docboss::source_document_for(mdboss::path_to_utf8(dir / "spec.pdf")) ==
          mdboss::path_to_utf8(dir / "spec.markdown"));
    CHECK(docboss::source_document_for(mdboss::path_to_utf8(dir / "orphan.pdf"))
              .empty());
    // Round trip with published_pdf_path.
    CHECK(docboss::published_pdf_path(docboss::source_document_for(guide)) ==
          guide);
    fs::remove_all(dir, ec);
}

TEST_CASE("is_pdf is case-insensitive and exact", "[publish]")
{
    CHECK(docboss::is_pdf("a.pdf"));
    CHECK(docboss::is_pdf("A.PDF"));
    CHECK(docboss::is_pdf("x.Pdf"));
    CHECK_FALSE(docboss::is_pdf("a.pdfx"));
    CHECK_FALSE(docboss::is_pdf("a.md"));
    CHECK_FALSE(docboss::is_pdf("pdf"));
    CHECK_FALSE(docboss::is_pdf(""));
}
