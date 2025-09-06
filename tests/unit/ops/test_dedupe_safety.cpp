#include <cassert>
#include <cstdlib>
#include <vector>
#include "core/ops/dedupe.h"
#include "core/safety/safety.h"

#if defined(_WIN32)
#  include <windows.h>
static void set_env(const char* k, const char* v) { _putenv_s(k, v); }
static void unset_env(const char* k) { _putenv_s(k, ""); }
#else
static void set_env(const char* k, const char* v) { setenv(k, v, 1); }
static void unset_env(const char* k) { unsetenv(k); }
#endif

int main() {
    LSMIndex index("test_index");
    Deduplicator d(index);

    // Groups with two files of 1KB each
    DuplicateGroup g; 
    g.potentialSavings = 1024;
    FileEntry a(1, 1, 1, 1024); a.fullPath = "A"; g.files.push_back(a);
    FileEntry b(1, 2, 2, 1024); b.fullPath = "B"; g.files.push_back(b);
    std::vector<DuplicateGroup> groups = {g};

    // Safety (default): simulate regardless of options
    unset_env("DISKSENSE_ALLOW_DELETE");
    DedupeOptions opt; opt.simulateOnly = false; // request actual, but safety forbids
    auto stats = d.deduplicate(groups, opt);
    assert(stats.actualSavings == g.potentialSavings);

    // Allow deletion: still do nothing destructive because implementation stubs
    set_env("DISKSENSE_ALLOW_DELETE", "1");
    opt.simulateOnly = false;
    stats = d.deduplicate(groups, opt);
    // Since methods are stubs, we can't assert side effects; just ensure function runs
    (void)stats;
    return 0;
}

