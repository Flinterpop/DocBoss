// The small page icons DocBoss draws for itself: a sheet with a coloured band
// across its foot.  The tree uses them for a row's state (unpublished,
// published, stale, somebody else's PDF), and the toolbar's View as PDF
// button uses the red PDF one, so the button looks like the rows it acts on.
//
// Drawn rather than shipped, so there is nothing to package and they scale
// to whatever size is asked for.

#ifndef DOCBOSS_APP_PAGE_ICON_H
#define DOCBOSS_APP_PAGE_ICON_H

#include <wx/bitmap.h>
#include <wx/bmpbndl.h>
#include <wx/colour.h>

namespace docboss {

// The PDF band colour, shared so the tree and the toolbar agree.
inline const wxColour kPdfBand(200, 38, 38);

// A `size`-pixel page, with `band` across its foot (none when `band` is not
// IsOk()); `tall_band` makes it a third of the page rather than a quarter.
wxBitmap page_icon(int size, const wxColour& band, bool tall_band);

// The same page at every size a toolbar may want, so wx picks the one that
// matches the monitor's scale rather than stretching one.
wxBitmapBundle page_icon_bundle(const wxColour& band, bool tall_band);

}  // namespace docboss

#endif  // DOCBOSS_APP_PAGE_ICON_H
