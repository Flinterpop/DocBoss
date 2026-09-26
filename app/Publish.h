// Where a document's published PDF lives: beside it, same name, .pdf.
//
// One rule, used by Publish (to write it), the tree (to find it) and the
// consolidation script's documentation (to explain it).  Keeping it a pure
// function is what lets a test pin the edge cases -- a stem with dots in it,
// a path outside ASCII -- that a GUI check would never exercise.

#ifndef DOCBOSS_APP_PUBLISH_H
#define DOCBOSS_APP_PUBLISH_H

#include <string>

namespace docboss {

// "C:\Docs\guide.md" -> "C:\Docs\guide.pdf".  UTF-8 in and out.  `md_path`
// must name a Markdown file (mdboss::is_markdown); the result is always in
// the same folder.
std::string published_pdf_path(const std::string& md_path);

// True for a ".pdf" name, any case.
bool is_pdf(const std::string& name);

}  // namespace docboss

#endif  // DOCBOSS_APP_PUBLISH_H
