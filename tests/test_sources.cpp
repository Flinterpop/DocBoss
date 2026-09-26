// Source hygiene, as MD Boss enforces it for the same two reasons:
//
// - A narrow string literal holding non-ASCII text is handed to wxString as
//   bytes and decoded in the ANSI code page, so "…" shows up as mojibake in
//   the UI.  Non-ASCII UI text must be a WIDE literal.
// - A PowerShell rewrite once double-encoded every dash in an MD Boss file;
//   the tell-tale byte sequences are cheap to look for.

#include <catch2/catch_test_macros.hpp>

#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

namespace {

namespace fs = std::filesystem;

std::string read_bytes(const fs::path& file)
{
    std::ifstream stream(file, std::ios::binary);
    std::ostringstream buffer;
    buffer << stream.rdbuf();
    return buffer.str();
}

// First non-ASCII byte inside a narrow "..." literal on this line, ignoring
// anything after a // comment.  -1 when the line is clean.
bool narrow_literal_has_non_ascii(const std::string& line)
{
    const std::string code = line.substr(0, line.find("//"));
    bool inside = false;
    bool wide = false;
    for (std::size_t i = 0; i < code.size(); ++i) {   // bounded by the line
        const unsigned char ch = static_cast<unsigned char>(code[i]);
        if (ch == '"' && (i == 0 || code[i - 1] != '\\')) {
            if (inside) {
                inside = false;
            } else {
                inside = true;
                wide = i > 0 && code[i - 1] == 'L';
            }
        } else if (inside && !wide && ch > 127) {
            return true;
        }
    }
    return false;
}

std::vector<fs::path> sources()
{
    std::vector<fs::path> out;
    const fs::path repo{DOCBOSS_REPO_DIR};
    std::error_code ec;
    for (const char* dir : {"app", "tests"}) {
        for (const fs::directory_entry& entry :
             fs::directory_iterator(repo / dir, ec)) {
            const std::string ext = entry.path().extension().string();
            if (ext == ".cpp" || ext == ".h") {
                out.push_back(entry.path());
            }
        }
    }
    return out;
}

}  // namespace

TEST_CASE("no narrow string literal holds non-ASCII text", "[sources]")
{
    const std::vector<fs::path> files = sources();
    REQUIRE(files.size() >= 10);
    std::vector<std::string> offences;
    for (const fs::path& file : files) {
        std::istringstream stream(read_bytes(file));
        std::string line;
        int number = 0;
        while (std::getline(stream, line)) {
            ++number;
            if (narrow_literal_has_non_ascii(line)) {
                offences.push_back(file.filename().string() + ":" +
                                   std::to_string(number));
            }
        }
    }
    for (const std::string& offence : offences) {
        UNSCOPED_INFO(offence);
    }
    CHECK(offences.empty());
}

TEST_CASE("sources carry no BOM and no double-encoded text", "[sources]")
{
    // The UTF-8 of a CP1252 round trip of an em-dash, and of a non-breaking
    // space: what a damaged file actually contains.
    const std::string dash = "\xC3\xA2\xE2\x82\xAC";
    const std::string nbsp = "\xC3\x82\xC2";
    std::vector<std::string> problems;
    for (const fs::path& file : sources()) {
        const std::string text = read_bytes(file);
        if (text.compare(0, 3, "\xEF\xBB\xBF") == 0) {
            problems.push_back(file.filename().string() + ": BOM");
        }
        // Line by line, so a line that shows the damage on purpose -- an
        // example in a comment -- can say so with "mojibake-ok", as in MD Boss.
        std::istringstream lines(text);
        std::string line;
        int number = 0;
        while (std::getline(lines, line)) {
            ++number;
            if (line.find("mojibake-ok") != std::string::npos) {
                continue;
            }
            if (line.find(dash) != std::string::npos ||
                line.find(nbsp) != std::string::npos) {
                problems.push_back(file.filename().string() + ":" +
                                   std::to_string(number) +
                                   ": double-encoded");
            }
        }
    }
    for (const std::string& problem : problems) {
        UNSCOPED_INFO(problem);
    }
    CHECK(problems.empty());
}
