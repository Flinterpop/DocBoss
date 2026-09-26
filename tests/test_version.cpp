// Version lockstep: one number, bumped in every place it lives, in one
// commit.  Missing one ships an exe or installer whose version disagrees with
// the release -- and in the numeric FILEVERSION case, an installer that
// quietly refuses to upgrade.

#include <catch2/catch_test_macros.hpp>

#include <filesystem>
#include <fstream>
#include <string>

#include "Version.h"

namespace {

namespace fs = std::filesystem;

std::string read_file(const fs::path& path)
{
    std::ifstream stream(path, std::ios::binary);
    if (!stream) {
        return {};
    }
    return std::string(std::istreambuf_iterator<char>(stream),
                       std::istreambuf_iterator<char>());
}

// The text between `open` and the next `close`, or empty.
std::string between(const std::string& text, const std::string& open,
                    char close)
{
    const std::size_t start = text.find(open);
    if (start == std::string::npos) {
        return {};
    }
    const std::size_t from = start + open.size();
    const std::size_t end = text.find(close, from);
    return end == std::string::npos ? std::string()
                                    : text.substr(from, end - from);
}

}  // namespace

TEST_CASE("version is in lockstep across the repo", "[version]")
{
    const fs::path repo{DOCBOSS_REPO_DIR};
    const std::string expected = docboss::kAppVersion;
    REQUIRE_FALSE(expected.empty());

    const std::string cmake = read_file(repo / "CMakeLists.txt");
    REQUIRE_FALSE(cmake.empty());
    CHECK(between(cmake, "project(DocBoss VERSION ", ' ') == expected);

    const std::string iss = read_file(repo / "installer.iss");
    REQUIRE_FALSE(iss.empty());
    CHECK(between(iss, "#define AppVersion \"", '"') == expected);

    const std::string rc = read_file(repo / "app" / "DocBoss.rc");
    REQUIRE_FALSE(rc.empty());
    CHECK(between(rc, "VALUE \"FileVersion\",      \"", '"') == expected);
    CHECK(between(rc, "VALUE \"ProductVersion\",   \"", '"') == expected);
    // 1.0.0 -> 1,0,0,0
    std::string numeric = expected;
    for (char& ch : numeric) {   // bounded by the string
        if (ch == '.') {
            ch = ',';
        }
    }
    numeric += ",0";
    CHECK(between(rc, "FILEVERSION    ", '\n').find(numeric) == 0);
    CHECK(between(rc, "PRODUCTVERSION ", '\n').find(numeric) == 0);
}
