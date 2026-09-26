// Reading a document's front matter for the page margins and the PDF's
// properties, and building the print-only margin stylesheet.

#include <catch2/catch_test_macros.hpp>

#include <string>

#include "DocMeta.h"

namespace {

const char* const kTechNote =
    "---\n"
    "title: Install Guide\n"
    "author: B. Graham\n"
    "version: 1.2\n"
    "subject: \"Overview\"\n"
    "keywords: [Radar, TechNote]\n"
    "date: 3 Jan 2026\n"
    "nested:\n"
    "  author: not this one\n"
    "---\n"
    "# Heading\n";

}  // namespace

TEST_CASE("front matter values are read by key", "[docmeta]")
{
    const docboss::DocMeta meta = docboss::read_doc_meta(kTechNote, "stem");
    CHECK(meta.title == "Install Guide");
    CHECK(meta.author == "B. Graham");
    CHECK(meta.version == "1.2");
    CHECK(meta.subject == "Overview");   // quotes dropped
    CHECK(meta.keywords == "[Radar, TechNote]");
    CHECK(meta.date == "3 Jan 2026");
}

TEST_CASE("keys are case-insensitive and nested ones are ignored", "[docmeta]")
{
    CHECK(docboss::front_matter_value(kTechNote, "AUTHOR") == "B. Graham");
    CHECK(docboss::front_matter_value(kTechNote, "missing").empty());
}

TEST_CASE("without front matter the title comes from the heading or the file",
          "[docmeta]")
{
    CHECK(docboss::read_doc_meta("# Ops notes\n\ntext\n", "ops").title ==
          "Ops notes");
    const docboss::DocMeta bare = docboss::read_doc_meta("just text\n", "ops");
    CHECK(bare.title == "ops");
    CHECK(bare.author.empty());
    CHECK(bare.version.empty());
}

TEST_CASE("a block that is not at the very top is not front matter",
          "[docmeta]")
{
    const char* const text = "intro\n---\nauthor: x\n---\n";
    CHECK(docboss::front_matter_value(text, "author").empty());
}

TEST_CASE("an unclosed block is not front matter", "[docmeta]")
{
    CHECK(docboss::front_matter_value("---\nauthor: x\n# body\n", "author")
              .empty());
}

TEST_CASE("CSS strings are escaped and cannot close the style element",
          "[docmeta]")
{
    CHECK(docboss::css_string("a \"b\" \\c") == "a \\\"b\\\" \\\\c");
    CHECK(docboss::css_string("x</style>") == "x\\3C /style>");
    CHECK(docboss::css_string("line\nbreak") == "line break");
}

TEST_CASE("the margin stylesheet is print-only and carries the fields",
          "[docmeta]")
{
    const std::string page = "<html><head><title>t</title></head><body/></html>";
    docboss::DocMeta meta;
    meta.title = "Install Guide";
    meta.version = "1.2";
    const std::string out =
        docboss::add_page_margin_boxes(page, meta, "26 Sep 2026");
    CHECK(out.find("<style media=\"print\">") < out.find("</head>"));
    CHECK(out.find("@top-left { content: \"Install Guide\"") !=
          std::string::npos);
    CHECK(out.find("\"Version 1.2\"") != std::string::npos);
    // No date in the document: today's is used.
    CHECK(out.find("\"26 Sep 2026\"") != std::string::npos);
    CHECK(out.find("counter(pages)") != std::string::npos);

    meta.date = "3 Jan 2026";
    CHECK(docboss::add_page_margin_boxes(page, meta, "26 Sep 2026")
              .find("\"3 Jan 2026\"") != std::string::npos);
    // Not a page it recognises: unchanged.
    CHECK(docboss::add_page_margin_boxes("<p>x</p>", meta, "d") == "<p>x</p>");
}
