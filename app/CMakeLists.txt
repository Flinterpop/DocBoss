# docboss_core -- everything headless: the pulled sibling units that do not
# touch the GUI, and DocBoss's own logic.  The tests link only this.

find_package(md4c CONFIG REQUIRED)

add_library(docboss_core STATIC
    ${MDBOSS_CORE_SOURCES}
    ${PDFBOSS_CORE_SOURCES}
    DocDeps.cpp
    DocDeps.h
    DocMeta.cpp
    DocMeta.h
    DocPairs.cpp
    DocPairs.h
    PdfInfo.cpp
    PdfInfo.h
    Identity.cpp
    Identity.h
    PdfSettings.cpp
    PdfSettings.h
    Publish.cpp
    Publish.h
    Version.h
)

# PDFBoss's units are written for, and built by PDFBoss with, these two.
set_source_files_properties(${PDFBOSS_CORE_SOURCES} ${PDFBOSS_GUI_SOURCES}
    PROPERTIES COMPILE_DEFINITIONS "WIN32_LEAN_AND_MEAN")

target_include_directories(docboss_core PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}"
    "${DOCBOSS_PULL_INCLUDE}")

target_link_libraries(docboss_core
    PUBLIC
        mdrender
        nlohmann_json::nlohmann_json
        # PUBLIC, not PRIVATE as in PDFBoss: the exe compiles ViewerPane, which
        # includes PdfDocument.h, which needs MuPDF's feature defines to agree
        # with the library about which functions exist.
        mupdf)

# -- The application ---------------------------------------------------------

find_package(wxWidgets CONFIG REQUIRED COMPONENTS core base html net stc)

add_executable(DocBoss WIN32
    main.cpp
    MainFrame.cpp
    MainFrame.h
    DocTreePanel.cpp
    DocTreePanel.h
    PdfView.cpp
    PdfView.h
    HelpDialog.cpp
    HelpDialog.h
    ${MDBOSS_GUI_SOURCES}
    ${PDFBOSS_GUI_SOURCES}
    DocBoss.rc
    DocBoss.manifest
)

target_link_libraries(DocBoss PRIVATE
    docboss_core
    wx::core
    wx::base
    wx::html
    wx::net
    wx::stc
    "${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/lib/WebView2LoaderStatic.lib"
)

# Deliberately NOT wx::webview -- MDBoss's PreviewPane owns its WebView2,
# because wxWebView's Edge backend never delivers WebResourceRequested and so
# cannot enforce the preview's network lock.

# The render assets are MDBoss's; put a copy beside the exe so a build tree
# runs exactly as an install does.
add_custom_command(TARGET DocBoss POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory "${MDBOSS_ASSETS}"
            "$<TARGET_FILE_DIR:DocBoss>/assets"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${CMAKE_SOURCE_DIR}/HELP.md" "$<TARGET_FILE_DIR:DocBoss>/HELP.md"
    VERBATIM)

set_target_properties(DocBoss PROPERTIES
    VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}")
