# DocBoss -- Markdown sources and the PDFs published from them, in one tree.
#
# DocBoss is built mostly out of its two siblings, MDBoss (the Markdown
# editor, preview and document tools) and PDFBoss (the MuPDF viewer, topics,
# bookmarks and highlights), compiled by SOURCE PULL from their trees at
# pinned commits -- see cmake/Siblings.cmake for exactly what is taken and
# why.  What lives here is the window that joins them, the tree that shows
# both kinds of file, publishing, and packaging.
#
# Licence: AGPL-3.0-or-later, because MuPDF is statically linked.
#
# Dependencies come from vcpkg in CLASSIC mode, exactly as for the siblings:
#
#   C:\vcpkg\vcpkg.exe install wxwidgets:x64-windows-static ^
#                              md4c:x64-windows-static catch2:x64-windows-static ^
#                              nlohmann-json:x64-windows-static ^
#                              webview2:x64-windows-static
#
# MuPDF is built from the pinned tree at C:\source\mupdf; see PDFBoss's
# cmake/MuPdf.cmake for the msbuild line.
#
# Configure FROM SCRATCH WITH THE PRESET (the toolchain file is only honoured
# on a build directory's first configure):
#
#   cmake --preset windows-static
#   cmake --build build --config Release
#   ctest --test-dir build -C Release --output-on-failure

cmake_minimum_required(VERSION 3.21)

# Kept in step with app/Version.h, app/DocBoss.rc and installer.iss by
# release.ps1; tests/test_version.cpp enforces it.
project(DocBoss VERSION 1.0.2 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Static CRT so the shipped exe needs no VC++ redistributable, and because
# MuPDF is built /MT: mixing the two is two heaps and a crash at the first
# cross-module free.  Verify with `dumpbin /dependents`.
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

# First-party code, and the pulled sibling code, is warning-clean at /W4 /WX.
# Third-party headers come in as SYSTEM includes.
add_compile_options(/W4 /WX /utf-8 /permissive- /external:W0)

# windows.h's min/max macros break std::min/std::max wherever it leaks in.
# PDFBoss builds with NOMINMAX everywhere and its units assume it; MDBoss's are
# indifferent to it.  WIN32_LEAN_AND_MEAN is NOT global: PDFBoss's units are
# built with it (app/CMakeLists.txt sets it on their sources), MDBoss's are
# not, and its preview needs the COM headers lean mode drops.
add_compile_definitions(NOMINMAX)

# MuPDF's static libraries are built with /GL, so the link needs /LTCG.
add_link_options($<$<CONFIG:Release>:/LTCG>)

# There is no Debug build of the app.  The pinned MuPDF is built Release-only
# (/MT), and a Debug exe is /MTd: the link fails on RuntimeLibrary and
# _ITERATOR_DEBUG_LEVEL mismatches, as PDFBoss's would.  Release defines
# NDEBUG, which removes every assert() -- so without this, no assertion here
# or in the pulled units would ever be checked.  Configure a SEPARATE tree:
#
#   cmake --preset windows-static -B build-asserts -DDOCBOSS_RELEASE_ASSERTS=ON
#   cmake --build build-asserts --config Release
#   ctest --test-dir build-asserts -C Release
option(DOCBOSS_RELEASE_ASSERTS "Keep assert() live in the Release build" OFF)
if(DOCBOSS_RELEASE_ASSERTS)
    string(REPLACE "/DNDEBUG" "" CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE}")
    message(STATUS "DocBoss: assertions are live in this Release build")
endif()

enable_testing()

find_package(nlohmann_json CONFIG REQUIRED)

include(cmake/Siblings.cmake)

add_subdirectory(app)
add_subdirectory(tests)
