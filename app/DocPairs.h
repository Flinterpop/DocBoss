// Pairing each Markdown document with the PDF published from it.
//
// A PDF is a document's publication when it sits in the same folder with the
// same stem ("guide.md" and "guide.pdf"), compared case-insensitively because
// Windows is.  Such a PDF is folded under its source in the tree rather than
// listed beside it; a PDF with no source is listed on its own, since it is
// somebody else's document and the viewer is for exactly those.
//
// A publication is STALE when the source was written after it: whatever was
// handed out no longer says what the document says.  Equal times count as
// current -- a PDF exported in the same clock tick as a save is that save.

#ifndef DOCBOSS_APP_DOC_PAIRS_H
#define DOCBOSS_APP_DOC_PAIRS_H

#include <map>
#include <string>
#include <vector>

#include "mdboss/FileScan.h"

namespace docboss {

enum class PdfState {
    kNone,      // never published
    kCurrent,   // the PDF is at least as new as the source
    kStale,     // the source changed after the PDF was written
};

struct PdfLink {
    std::string pdf_path;   // UTF-8, absolute
    PdfState state = PdfState::kNone;
};

struct Pairing {
    // Keyed by mdboss::norm_path() of the Markdown document.  A document with
    // no entry here has never been published.
    std::map<std::string, PdfLink> published;
    // PDFs with no source beside them, in the scan's order.
    std::vector<mdboss::DocEntry> loose_pdfs;
};

// `documents` and `pdfs` as one scan_root() returned them (entries and
// companions).  Pure: no disk access, so the times it compares are the ones
// the scan recorded.
Pairing classify_pairs(const std::vector<mdboss::DocEntry>& documents,
                       const std::vector<mdboss::DocEntry>& pdfs);

// The state a document's publication is in, from a pairing.  kNone when the
// document is not in it.
PdfState pdf_state(const Pairing& pairing, const std::string& md_path);

}  // namespace docboss

#endif  // DOCBOSS_APP_DOC_PAIRS_H
