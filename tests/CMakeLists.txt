find_package(Catch2 CONFIG REQUIRED)

# Links docboss_core only: no wxWidgets, no GUI.  The pulled sibling units'
# own tests live in their own repos and are run there; these cover what
# DocBoss adds and how it configures what it pulls.
add_executable(docboss_tests
    test_docpairs.cpp
    test_identity.cpp
    test_pdfsettings.cpp
    test_publish.cpp
    test_sources.cpp
    test_version.cpp
)

target_link_libraries(docboss_tests PRIVATE docboss_core Catch2::Catch2WithMain)

target_compile_definitions(docboss_tests PRIVATE
    DOCBOSS_REPO_DIR="${CMAKE_SOURCE_DIR}")

include(Catch)
catch_discover_tests(docboss_tests)
