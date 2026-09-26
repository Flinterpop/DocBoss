#include "PdfInfo.h"

#include <cassert>
#include <utility>

#include <mupdf/fitz.h>
#include <mupdf/pdf.h>

// fz_try/fz_catch are setjmp/longjmp, and MSVC warns (C4611) about any
// function that mixes them with C++ objects.  The rule that makes it safe is
// kept by hand, as in PDFBoss's PdfDocument.cpp: nothing with a destructor is
// created inside an fz_try, so a longjmp never skips one.
#pragma warning(push)
#pragma warning(disable : 4611)

namespace docboss {
namespace {

// One MuPDF context for the length of one call, dropped however it leaves.
class Context {
public:
    Context() : ctx_(fz_new_context(nullptr, nullptr, FZ_STORE_DEFAULT))
    {
        if (ctx_ != nullptr) {
            fz_register_document_handlers(ctx_);
        }
    }
    ~Context()
    {
        if (ctx_ != nullptr) {
            fz_drop_context(ctx_);
        }
    }
    Context(const Context&) = delete;
    Context& operator=(const Context&) = delete;
    fz_context* get() const { return ctx_; }

private:
    fz_context* ctx_;
};

}  // namespace

std::string write_pdf_info(const std::string& pdf_path, const DocMeta& meta)
{
    assert(!pdf_path.empty() && "a PDF to stamp is needed");
    const Context context;
    fz_context* ctx = context.get();
    if (ctx == nullptr) {
        return "MuPDF could not start";
    }

    const std::pair<const char*, const std::string*> fields[] = {
        {FZ_META_INFO_TITLE, &meta.title},
        {FZ_META_INFO_AUTHOR, &meta.author},
        {FZ_META_INFO_SUBJECT, &meta.subject},
        {FZ_META_INFO_KEYWORDS, &meta.keywords},
    };

    // fz_try/fz_catch are setjmp-based: nothing with a destructor may be
    // created between them, so every std::string above is built first.
    std::string error;
    fz_document* doc = nullptr;
    fz_var(doc);
    fz_try(ctx) {
        // MuPDF's Windows build takes UTF-8 filenames and widens them itself.
        doc = fz_open_document(ctx, pdf_path.c_str());
        pdf_document* pdf = pdf_specifics(ctx, doc);
        if (pdf == nullptr) {
            fz_throw(ctx, FZ_ERROR_ARGUMENT, "not a PDF");
        }
        for (const auto& field : fields) {   // bounded: four
            if (!field.second->empty()) {
                fz_set_metadata(ctx, doc, field.first, field.second->c_str());
            }
        }
        fz_set_metadata(ctx, doc, FZ_META_INFO_CREATOR, "DocBoss");
        // The path spelled out: incremental mode APPENDS to it, and MuPDF
        // throws "no output to write to" when given none.
        pdf_write_options options = pdf_default_write_options;
        options.do_incremental = 1;
        pdf_save_document(ctx, pdf, pdf_path.c_str(), &options);
    }
    fz_always(ctx) {
        fz_drop_document(ctx, doc);
    }
    fz_catch(ctx) {
        error = fz_caught_message(ctx);
        if (error.empty()) {
            error = "MuPDF could not update the file";
        }
    }
    return error;
}

}  // namespace docboss

#pragma warning(pop)
