// What a document's PDF depends on besides the document itself: the local
// images it shows.  Replacing a figure changes the published PDF just as
// surely as editing the text, so a figure newer than the PDF makes it stale.
//
// Two halves.  local_references() is pure: which local files a Markdown text
// points at.  newest_dependency() reads the document (bounded) and stats
// those files.  The tree calls it only for documents that HAVE a current
// PDF -- the scan itself never opens a file, and this keeps the reading to
// the documents where it can change the answer.

#ifndef DOCBOSS_APP_DOC_DEPS_H
#define DOCBOSS_APP_DOC_DEPS_H

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

#include "DocPairs.h"

namespace docboss {

// Targets of `![alt](target)`, `<img src="target">` and reference
// definitions `[id]: target` that name an image, in order of appearance,
// duplicates removed.  Anything with a URL scheme (http:, data:, ...) or
// starting with '#' is not a local file and is left out.  `<...>` wrappers,
// a trailing "title", and ?query / #fragment are stripped; %XX is decoded.
std::vector<std::string> local_references(std::string_view md_text);

// The newest last-write time (file_time_type ticks) among the files
// `md_path` references, resolved against its folder.  0 when there are
// none, or none that exist.
std::int64_t newest_dependency(const std::string& md_path);

// Mark stale every publication whose document references a file newer than
// the PDF.  Reads each such document; call it on a worker.
void apply_dependency_times(Pairing& pairing);

// One document's publication state, straight from the disk, by the same
// rules the tree uses: kNone when <stem>.pdf is not beside it; kStale when
// the document, or a local image it shows, was written after the PDF;
// kCurrent otherwise.  Reads that one document (1 MB cap), so it is cheap
// enough for the UI thread; the frame uses it for the open document.
PdfState publication_state_on_disk(const std::string& md_path);

}  // namespace docboss

#endif  // DOCBOSS_APP_DOC_DEPS_H
