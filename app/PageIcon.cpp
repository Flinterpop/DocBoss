#include "PageIcon.h"

#include <wx/image.h>
#include <wx/vector.h>

#include <algorithm>
#include <cassert>

namespace docboss {

// Built pixel by pixel as a wxImage with an explicit alpha channel: GDI
// drawing into a 32-bit bitmap leaves the alpha at zero, and the icon comes
// out invisible.  That happened first.
wxBitmap page_icon(int size, const wxColour& band, bool tall_band)
{
    assert(size > 0 && "an icon needs a size");
    wxImage image(size, size);
    image.InitAlpha();
    const int left = std::max(1, size / 6);
    const int right = size - 1 - std::max(1, size / 6);
    const int border = std::max(1, size / 16);
    const int band_top =
        band.IsOk() ? size - 1 - (tall_band ? size / 3 : size / 4) : size;
    for (int y = 0; y < size; ++y) {       // bounded by the icon size
        for (int x = 0; x < size; ++x) {   // bounded by the icon size
            const bool inside = x >= left && x <= right;
            if (!inside) {
                image.SetAlpha(x, y, 0);
                continue;
            }
            const bool edge = x < left + border || x > right - border ||
                              y < border || y > size - 1 - border;
            unsigned char r = 255;
            unsigned char g = 255;
            unsigned char b = 255;
            if (edge) {
                r = 90;
                g = 100;
                b = 115;
            } else if (y >= band_top) {
                r = band.Red();
                g = band.Green();
                b = band.Blue();
            }
            image.SetRGB(x, y, r, g, b);
            image.SetAlpha(x, y, 255);
        }
    }
    return wxBitmap(image);
}

wxBitmapBundle page_icon_bundle(const wxColour& band, bool tall_band)
{
    wxVector<wxBitmap> sizes;
    for (const int size : {16, 24, 32, 48}) {   // bounded: four sizes
        sizes.push_back(page_icon(size, band, tall_band));
    }
    assert(sizes.size() == 4);
    return wxBitmapBundle::FromBitmaps(sizes);
}

}  // namespace docboss
