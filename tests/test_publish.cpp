// Where a published PDF goes, and what counts as a PDF.

#include <catch2/catch_test_macros.hpp>

#include <string>

#include "Publish.h"

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
