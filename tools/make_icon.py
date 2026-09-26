"""Draw docboss.ico: a page with Markdown-ish text lines over a red PDF band.

Run with the machine's Python (C:\\Program Files\\Python314\\python.exe) from
the repo root.  The icon is committed; this is how to regenerate it.
"""
from PIL import Image, ImageDraw

SIZE = 256


def draw() -> Image.Image:
    img = Image.new("RGBA", (SIZE, SIZE), (0, 0, 0, 0))
    d = ImageDraw.Draw(img)
    # The sheet, with a folded corner.
    left, top, right, bottom, fold = 40, 16, 216, 240, 52
    sheet = [(left, top), (right - fold, top), (right, top + fold),
             (right, bottom), (left, bottom)]
    d.polygon(sheet, fill=(255, 255, 255, 255), outline=(60, 72, 88, 255))
    d.line(sheet + [sheet[0]], fill=(60, 72, 88, 255), width=8)
    d.polygon([(right - fold, top), (right - fold, top + fold),
               (right, top + fold)], fill=(210, 218, 228, 255),
              outline=(60, 72, 88, 255))
    # A heading and body lines: the Markdown source.
    d.rectangle([64, 60, 150, 76], fill=(9, 105, 218, 255))
    for i, width in enumerate((120, 104, 128, 88)):
        y = 92 + i * 20
        d.rectangle([64, y, 64 + width, y + 8], fill=(120, 132, 148, 255))
    # The publication: a red band across the foot of the page.
    d.rectangle([28, 178, 228, 226], fill=(200, 38, 38, 255))
    d.text((70, 186), "PDF", fill=(255, 255, 255, 255),
           font_size=36)
    return img


if __name__ == "__main__":
    image = draw()
    image.save("app/docboss.ico",
               sizes=[(16, 16), (24, 24), (32, 32), (48, 48), (64, 64),
                      (128, 128), (256, 256)])
    print("wrote app/docboss.ico")
