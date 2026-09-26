// Stamp a printed PDF's document properties -- Title, Author, Subject,
// Keywords, Creator -- from the document it was printed from.
//
// WebView2's PrintToPdf writes only what Chromium knows: the page's <title>
// and a Chromium producer.  A reader's Properties dialog, a search index and
// a document list all show those fields, so a published PDF titled
// "install-guide.md" and authored by nobody is worth fixing.  Done with
// MuPDF after the print, as an incremental save of the file in place.

#ifndef DOCBOSS_APP_PDF_INFO_H
#define DOCBOSS_APP_PDF_INFO_H

#include <string>

#include "DocMeta.h"

namespace docboss {

// Empty on success, else what went wrong.  A field that is empty in `meta`
// is left as the PDF already has it.
std::string write_pdf_info(const std::string& pdf_path, const DocMeta& meta);

}  // namespace docboss

#endif  // DOCBOSS_APP_PDF_INFO_H
