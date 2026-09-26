// Stamping a printed PDF's document properties from the front matter.

#include <catch2/catch_test_macros.hpp>

#include <filesystem>
#include <string>

#include <mupdf/fitz.h>

#include "PdfInfo.h"
#include "mdboss/PathUtf8.h"

// See PdfInfo.cpp: nothing with a destructor lives inside an fz_try.
#pragma warning(disable : 4611)

namespace {

namespace fs = std::filesystem;

// One metadata field as MuPDF reads it back, or "" on any failure.
std::string read_field(const std::string& pdf, const char* key)
{
    fz_context* ctx = fz_new_context(nullptr, nullptr, FZ_STORE_DEFAULT);
    if (ctx == nullptr) {
        return {};
    }
    fz_register_document_handlers(ctx);
    char buffer[512] = {};
    fz_document* doc = nullptr;
    fz_var(doc);
    fz_try(ctx) {
        doc = fz_open_document(ctx, pdf.c_str());
        fz_lookup_metadata(ctx, doc, key, buffer, sizeof(buffer));
    }
    fz_always(ctx) {
        fz_drop_document(ctx, doc);
    }
    fz_catch(ctx) {
        buffer[0] = '\0';
    }
    fz_drop_context(ctx);
    return buffer;
}

}  // namespace

TEST_CASE("document properties are written into the PDF", "[pdfinfo]")
{
    const fs::path dir = fs::temp_directory_path() / "docboss_pdfinfo_test";
    std::error_code ec;
    fs::remove_all(dir, ec);
    fs::create_directories(dir, ec);
    const fs::path pdf = dir / "guide.pdf";
    fs::copy_file(fs::path(DOCBOSS_PDF_FIXTURE), pdf, ec);
    REQUIRE_FALSE(ec);

    docboss::DocMeta meta;
    meta.title = "Install Guide";
    meta.author = "B. Graham";
    meta.subject = "Overview";
    meta.keywords = "Radar, TechNote";
    const std::string path = mdboss::path_to_utf8(pdf);
    CHECK(docboss::write_pdf_info(path, meta).empty());

    CHECK(read_field(path, FZ_META_INFO_TITLE) == "Install Guide");
    CHECK(read_field(path, FZ_META_INFO_AUTHOR) == "B. Graham");
    CHECK(read_field(path, FZ_META_INFO_SUBJECT) == "Overview");
    CHECK(read_field(path, FZ_META_INFO_KEYWORDS) == "Radar, TechNote");
    CHECK(read_field(path, FZ_META_INFO_CREATOR) == "DocBoss");
    fs::remove_all(dir, ec);
}

TEST_CASE("an unreadable file is reported, not thrown", "[pdfinfo]")
{
    docboss::DocMeta meta;
    meta.title = "x";
    CHECK_FALSE(docboss::write_pdf_info("Z:\\definitely\\not\\here.pdf", meta)
                    .empty());
}
