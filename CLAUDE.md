# CLAUDE.md

*Last updated: 26 Sep 2026*

Guidance for Claude Code working in this repository.

## What this repo is

**DocBoss** — Markdown sources and the PDFs published from them, in one folder tree. Created 25 Sep 2026 because the author kept sources and exported PDFs in two parallel trees and wanted one. It is a **new app, not a merge**: MD Boss (Markdown only, MIT) and PDFBoss (PDF only, AGPL) keep shipping on their own for people who need just one of them.

The repo is **public** (`github.com/Flinterpop/DocBoss`, branch `main`) — created and first released (v1.0.0, installer + portable zip) on 26 Sep 2026. Licence **AGPL-3.0-or-later**, forced by the statically linked MuPDF.

## The one rule the app is built on

A document's publication is **`<same folder>\<same stem>.pdf`** (`Publish.cpp`, `published_pdf_path`). Pairing (`DocPairs.cpp`) is folder + stem, case-insensitive; a PDF is **stale** when the source's last write time is strictly newer. Everything else follows: the tree folds a publication into its document's row, lists every other PDF as a row of its own ("loose"), and a move or rename of a document carries its PDF and PDFBoss's sidecars (`.toc`, `.json`, `.bookmarks.json`) along (`carry_publication` in `DocTreePanel.cpp`).

## How it is built: a source pull from two siblings

Most of the code is not in this repo. `cmake/Siblings.cmake` compiles units straight out of `C:\source\MDBoss\MDBossCpp` and `C:\source\PDFBoss\PDFBossCpp` by absolute path, each pinned by `*_EXPECTED_COMMIT` (warns at configure time, never fails) — the workspace's usual pinned source pull. **What gets built is always the sibling's HEAD**; `release.ps1` refuses unless both siblings are exactly at their pins with no tracked changes.

| From | Units | How |
|---|---|---|
| MD Boss | `mdrender/` | `add_subdirectory`, guarded `if(NOT TARGET mdrender)` |
| MD Boss | `AppIdentity FileScan TextSearch LinkTarget PathUtf8 InternalNotes TechNotes Templates Favorites FileAssoc Updater Config` | headless, into `docboss_core` |
| MD Boss | `PreviewPane FindBar FindInFilesDialog DocumentWatcher OutlinePanel PathListPanel InternalDialogs SingleInstance UpdaterHttp DropTarget FoldersDialog` | GUI, into the exe |
| PDFBoss | `PdfDocument TocGen Metadata PathUtf8` | headless, into `docboss_core` |
| PDFBoss | `ViewerPane TopicsPane` | GUI, into the exe |
| PDFBoss | `cmake/MuPdf.cmake` | `include()` — the `mupdf` target and the 1.28.2 pin |
| MD Boss | `assets/` | copied beside the exe post-build; shipped by the installer |

Three things make two apps' code safe in one binary, and all three matter:

- **Namespaces.** MD Boss's code is `mdboss::`, PDFBoss's is `pdfboss::`, DocBoss's is `docboss::`. Both siblings have a `PathUtf8.cpp` and both are linked.
- **A pulled unit keeps its own includes.** A quoted `#include` resolves in the including file's folder first, so a pulled `.cpp` always gets its own sibling's headers.
- **DocBoss's own code never includes a sibling header by bare name.** Both siblings ship `Config.h`, `Version.h`, `Updater.h`, `PathUtf8.h`… so `Siblings.cmake` generates forwarding headers under `build/pull/mdboss/` and `build/pull/pdfboss/`, and DocBoss writes `#include "mdboss/FileScan.h"`. Only listed units get a forwarding header.

**A new `#include` in a pulled sibling unit breaks DocBoss at LINK time**, as undefined symbols, because DocBoss lists the `.cpp` files it compiles. Add the new unit to the list in `Siblings.cmake`. When bumping a pin, diff the pulled files, not just the SHA (`git -C ..\MDBoss diff <old>..<new> -- MDBossCpp/app MDBossCpp/mdrender assets`).

`WIN32_LEAN_AND_MEAN` is set **only on PDFBoss's sources** (they are built with it at home); MD Boss's preview needs the COM headers lean mode drops. `NOMINMAX` is global.

### The seams added to MD Boss for this (upstream first, 25 Sep 2026)

- **`app/AppIdentity.h`** (MD Boss `e448fd1`): profile folder, preview staging folder, ProgID and association names, single-instance mutex and window property, release URLs, asset names, and the exe the portable update checks for. MD Boss never sets it; every default is the literal it replaced. **DocBoss sets it first thing in `OnInit`** (`Identity.cpp`), before anything reads it — including the `--register-file-types` switches. Without it DocBoss would silently read MD Boss's `config.json`, share its single-instance slot and update itself from MD Boss's releases. `tests/test_identity.cpp` checks no field still names MD Boss.
- **`Templates` no longer sees `Config`** (same commit): `seed_templates()` takes a `SeededTemplates` record.
- **`scan_root(..., companion_exts)`** (same commit): `.pdf` files from the same walk, reported in `RootScan::companions`, kept out of `entries` and `counts`; plus each entry's last write time.
- **`PreviewPane::set_on_page_loaded`** (MD Boss `be2800b`): raised on `NavigationCompleted`. Publish waits for it.

PDFBoss needed no change: its pulled units were already leaves free of `Config`, `Updater` and `Version.h`.

### What DocBoss owns

`MainFrame` and `DocTreePanel` started as **copies** of MD Boss's `MainFrame` / `FileTreePanel` (25 Sep 2026) and have diverged on purpose: the right-hand side is a `wxSimplebook` (Markdown page | `PdfView`), the tree scans for PDFs and pairs them, and there is Publish. **Nothing warns when MD Boss fixes something in its own copies** — they are windows, not pullable units. When MD Boss's `MainFrame.cpp` or `FileTreePanel.cpp` change, diff them against these by hand and port what applies. `HelpDialog` is a copy too. `PdfView`, `DocPairs`, `Publish`, `PdfSettings`, `Identity` are new.

`mdboss::Config` is **pulled, not copied**: once `AppIdentity` existed its file follows DocBoss's data folder. DocBoss's PDF settings (`PdfSettings`) live in the **same `config.json`** under `docboss_*` keys; both writers re-read and merge, so neither drops the other's keys (`test_pdfsettings.cpp`).

## Behaviour that is deliberate

- **One file open at a time**, a document or a PDF, never both. Opening a PDF clears the editor (after `confirm_discard`), so Ctrl+S can never write editor text over a PDF. Ctrl+S with a PDF showing saves highlights instead.
- **Publish** refuses while the document is dirty or untitled, re-renders, waits for `set_on_page_loaded` plus a 600 ms settle (mermaid and KaTeX draw on DOMContentLoaded), then prints; a 5 s fallback prints anyway so Publish can never stick. It overwrites without asking — published PDFs are output (user ruling, 25 Sep 2026). Highlights added to a published PDF are lost on the next publish; HELP.md says so.
- **Topics are built on a worker with no dialog** when a PDF without a `.toc` is opened (`PdfView::build_topics_in_background`). PDFBoss asks and shows a modal progress dialog because it builds many; here arrow-key browsing opens PDFs one keystroke at a time.
- **Page keys** (PgUp/PgDn/Space/Home/End) act only while focus is inside the PDF page (`wxEVT_CHAR_HOOK` on `PdfView`), never frame-wide, or they would be taken from the editor.
- **Tree state icons** are built pixel by pixel as a `wxImage` with alpha. Drawing with a `wxMemoryDC` into a 32-bit bitmap leaves alpha at 0 and the icons come out invisible — that happened first.
- **`.pdf` is never claimed** as a file association; only `.md` et al., as ProgID `DocBoss.Markdown`, and only when the installer task (unchecked by default) or File types… asks.

## Building and testing

```powershell
cmake --preset windows-static
cmake --build build --config Release
ctest --test-dir build -C Release          # with TMP/TEMP pointed at build\claude-scratch\tmp
```

- **No Debug build.** MuPDF is built Release-only (`/MT`); a Debug exe is `/MTd` and fails to link on RuntimeLibrary / `_ITERATOR_DEBUG_LEVEL`. For assertions, use a second tree: `cmake --preset windows-static -B build-asserts -DDOCBOSS_RELEASE_ASSERTS=ON`. Run the tests **and the app** from it when touching assertions.
- The tests write throwaway files under `fs::temp_directory_path()` — `%TEMP%`, which is inside AppData here. Always run `ctest` with `TMP` and `TEMP` set to `build\claude-scratch\tmp`.
- **Running the app:** always `DocBoss.exe --profile build\claude-scratch\profile` with `APPDATA`, `TEMP` and `TMP` also pointed into `build\claude-scratch`. `--profile` moves the settings and the preview's staging (and WebView2 profile) under that folder; nothing then touches AppData. Point the profile's roots at a scratch folder of neutral documents — never screenshot against the real profile.
- The preview's **network lock** must hold: a document with a remote `<img>`/`<script>` aimed at a loopback listener must produce no connection, with a positive control proving the listener counts. Re-run it when anything around `PreviewPane` changes (MD Boss's CLAUDE.md has the full procedure).
- Verified 25 Sep 2026: Release and asserts builds clean at `/W4 /WX`; 25 tests pass; `dumpbin /dependents` shows no VCRUNTIME/MSVCP; one window on a simultaneous double launch; publish → green, edit → orange, republish → green; published PDF matches the preview including mermaid; loopback lock check 0 connections (control 1); installer compiles. **Not yet verified:** the updater from one real release to the next (v1.0.0 is the only one), and an install/uninstall run of the installer.

## Releasing

`.\release.ps1 <version>` and only that. It bumps the four version files (test_version.cpp checks them), refuses unless both siblings are at their pins and clean, builds, tests, runs ISCC (candidate list, `C:\bin\InnoSetup6` first) and zips the portable build, commits, pushes and publishes `DocBoss-Setup.exe` + `DocBoss-Portable.zip`. The updater matches those names exactly (`Identity.cpp`); never publish a sibling's asset name here.

## ITAR and the public repo

Everything under `C:\source` is ITAR-controlled; the global rules apply. This repo is **public**, and like MD Boss **the leak path is documentation and examples, not code**: sample paths, screenshots, test fixtures, README examples. Use neutral names (`install-guide.md`, a root called `Notes`). Before every commit, grep the staged diff for real project names, unit identifiers, root paths and document titles. The app itself will be pointed at controlled documents, so `.gitignore` refuses root-level `*.md`/`*.pdf` (bar README/HELP/CLAUDE), `*.toc`, `*.bookmarks.json` and anything named like a password file.

`tools/consolidate_pdfs.ps1` prints real filenames when run on real trees; it writes no log file by design, and its output should not be pasted anywhere outside this machine.
