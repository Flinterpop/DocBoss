# DocBoss

[![Latest release](https://img.shields.io/github/v/release/Flinterpop/DocBoss?sort=semver)](https://github.com/Flinterpop/DocBoss/releases/latest)
[![License: AGPL-3.0](https://img.shields.io/github/license/Flinterpop/DocBoss)](LICENSE)

*Last updated: 25 Sep 2026*

**Markdown sources and the PDFs published from them, in one folder tree.** Write a document with a live, offline GitHub-style preview; press **Publish** and its PDF is written right beside it; and the tree shows at a glance which documents are published, which PDFs are out of date, and which PDFs came from somewhere else. Windows, 100% offline.

DocBoss is built from two sibling apps, which stay available on their own:

- [MD Boss](https://github.com/Flinterpop/MDBoss) — the Markdown editor, preview, outline, search, templates, tech notes and lists.
- [PDFBoss](https://github.com/Flinterpop/PDFBoss) — the MuPDF viewer, topics, bookmarks, search and highlighting.

## The one rule

A document's PDF has the **same name, in the same folder**: `install-guide.md` publishes to `install-guide.pdf`. That is all DocBoss needs to pair them — there is no index to keep up to date. Each document row in the tree shows whether it is unpublished, published (green), or edited since it was published (orange); PDFs with no document beside them (red) are listed as documents in their own right and open in the viewer.

Keeping sources and PDFs in two parallel trees until now? `tools/consolidate_pdfs.ps1` moves each PDF beside its source, dry run first. See [HELP.md](HELP.md).

## Features

- Everything MD Boss does: editor and live preview side by side, mermaid, KaTeX, highlight.js, GitHub alerts and admonitions, outline, find in document and across all documents, a filterable tree over up to five root folders, favourites and recents, templates, tech-note index and numbering, the Lists menu, and a network-locked preview that never fetches a remote byte.
- **Publish PDF** (Ctrl+Shift+P): the preview printed to `<name>.pdf` beside the source, no dialog, replacing the last one. Refused while the document has unsaved edits, so a PDF always matches its file.
- **Published / stale / foreign** markers on every row, from one scan of the disk.
- Everything PDFBoss does for reading: page view, topic lists generated on first open, your own bookmarks, text search, highlights saved into the PDF or an `(ann)` copy.
- A document that is moved or renamed takes its PDF (and the PDF's topics and bookmarks) with it.

## Install

Download `DocBoss-Setup.exe` from the [latest release](https://github.com/Flinterpop/DocBoss/releases/latest) — it asks whether to install for you or for everyone — or `DocBoss-Portable.zip` to run from a folder. **Help ▸ Check for updates** updates either kind in place. The preview needs the Microsoft Edge WebView2 runtime, which Windows 10 and 11 already have.

## Build

DocBoss compiles its two siblings **by source**, from checkouts beside it at pinned commits (see `cmake/Siblings.cmake`):

```text
C:\source\DocBoss     this repo
C:\source\MDBoss      github.com/Flinterpop/MDBoss
C:\source\PDFBoss     github.com/Flinterpop/PDFBoss
C:\source\mupdf       MuPDF 1.28.2 source, built as PDFBoss's cmake/MuPdf.cmake describes
```

Dependencies come from vcpkg in classic mode (`x64-windows-static`): `wxwidgets`, `md4c`, `nlohmann-json`, `catch2`, `webview2`. Then, with Visual Studio 2026:

```powershell
cmake --preset windows-static
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
```

The exe is fully static (no VC++ runtime needed). There is no Debug build of the app, because MuPDF is built Release-only; configure a second tree with `-DDOCBOSS_RELEASE_ASSERTS=ON` to run with assertions live.

## Licence

DocBoss is licensed **AGPL-3.0-or-later** ([LICENSE](LICENSE)), because it statically links MuPDF, which is AGPL.

| Component | Licence |
|---|---|
| MuPDF | AGPL-3.0 |
| PDFBoss code compiled in | AGPL-3.0-or-later |
| MD Boss code compiled in | MIT |
| wxWidgets | wxWindows Library Licence (LGPL with a static-linking exception) |
| md4c, nlohmann-json | MIT |
| highlight.js, KaTeX, mermaid (bundled assets) | BSD-3 / MIT / MIT |
