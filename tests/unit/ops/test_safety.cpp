#include <cassert>
#include <cstdlib>
#include <cstdio>
#include <string>
#include <filesystem>
#include "core/safety/safety.h"
#include "core/ops/cleanup.h"
#include "core/ops/secure_delete.h"

#if defined(_WIN32)
#  include <windows.h>
static void set_env(const char* k, const char* v) { _putenv_s(k, v); }
static void unset_env(const char* k) { _putenv_s(k, ""); }
#else
static void set_env(const char* k, const char* v) { setenv(k, v, 1); }
static void unset_env(const char* k) { unsetenv(k); }
#endif

int main() {
    // Safety mode defaults to disabled deletions
    unset_env("DISKSENSE_ALLOW_DELETE");
    assert(!safety::deletion_allowed());

    // Cleanup analysis produces candidates and cleanup applies to quarantine
    std::filesystem::path tmp = std::filesystem::temp_directory_path() / "ds_test_safety";
    std::filesystem::create_directories(tmp);
    std::filesystem::path f = tmp / "a.tmp";
    {
        FILE* fp = std::fopen(f.string().c_str(), "wb");
        assert(fp);
        const char data[] = "1234";
        std::fwrite(data, 1, sizeof(data), fp);
        std::fclose(fp);
    }

    CleanupOptions copts; 
    copts.simulateOnly = false; 
    copts.extensions = {".tmp"};
    copts.removeEmptyDirs = true; 
    copts.useQuarantine = true; 
    copts.quarantineDir = (tmp / "quarantine").string();
    auto rep = cleanup_analyze(tmp.string(), copts);
    assert(!rep.candidates.empty());
    size_t affected = cleanup_apply(rep, copts);
    assert(affected >= 1);
    // Original must not exist and should be in quarantine
    assert(!std::filesystem::exists(f));
    assert(std::filesystem::exists(std::filesystem::path(copts.quarantineDir)));

    // secure_delete should be blocked by Safety Mode by default
    std::filesystem::path f2 = tmp / "b.bin";
    {
        FILE* fp = std::fopen(f2.string().c_str(), "wb");
        assert(fp);
        std::string blob(1024, '\xAB');
        std::fwrite(blob.data(), 1, blob.size(), fp);
        std::fclose(fp);
    }
    std::string err;
    SecureDeleteOptions sopts; sopts.passes = 1; sopts.useRandom = false; sopts.verify = false;
    bool ok = secure_delete_file(f2.string(), sopts, &err);
    assert(!ok);
    assert(!err.empty());
    assert(std::filesystem::exists(f2));

    // Enable deletion via env and verify secure_delete works
    set_env("DISKSENSE_ALLOW_DELETE", "1");
    assert(safety::deletion_allowed());
    err.clear();
    ok = secure_delete_file(f2.string(), sopts, &err);
    assert(ok);
    assert(!std::filesystem::exists(f2));

    // Cleanup: remove temp dir
    std::error_code ec; std::filesystem::remove_all(tmp, ec);
    return 0;
}

