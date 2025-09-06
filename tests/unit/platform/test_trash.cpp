#include <cassert>
#include <cstdio>
#include <string>
#include <filesystem>
#include "platform/util/trash.h"

static std::string write_temp_file(const char* name) {
    auto tmp = std::filesystem::temp_directory_path() / name;
    FILE* f = std::fopen(tmp.string().c_str(), "wb"); assert(f);
    const char* msg = "trash-test"; std::fwrite(msg, 1, 10, f); std::fclose(f);
    return tmp.string();
}

int main() {
    // Create temp file
    std::string f = write_temp_file("ds_trash_test.tmp");
    assert(std::filesystem::exists(f));
    std::string trashed;
    bool mv = platform::move_to_trash(f, &trashed);
    assert(mv);
    assert(!std::filesystem::exists(f));

    // Find in trash and restore
    auto items = platform::list_trash();
    bool found = false; platform::TrashEntry entry;
    for (const auto& e : items) {
        if (!e.trashed_path.empty() && (e.trashed_path.find("ds_trash_test.tmp") != std::string::npos)) { found = true; entry = e; break; }
        if (!e.original_path.empty() && (e.original_path.find("ds_trash_test.tmp") != std::string::npos)) { found = true; entry = e; break; }
    }
    assert(found);
    bool ok = platform::restore_from_trash(entry);
    assert(ok);
    // Either restored to original or into a collision-resolved path; check original first
    bool exists = std::filesystem::exists(entry.original_path);
    if (!exists) {
        // Try temp dir as fallback (some desktops restore differently)
        exists = std::filesystem::exists(f);
    }
    assert(exists);
    return 0;
}

