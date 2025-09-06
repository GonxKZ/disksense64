#include <cassert>
#include <cstdio>
#include <string>
#include <filesystem>
#include <vector>
#include "core/ops/dedupe.h"

static std::string write_temp_file(const char* name, const std::string& content) {
    auto tmp = std::filesystem::temp_directory_path() / name;
    FILE* f = std::fopen(tmp.string().c_str(), "wb"); assert(f);
    std::fwrite(content.data(), 1, content.size(), f);
    std::fclose(f);
    return tmp.string();
}

int main() {
    // Prepare two identical files and one different
    std::string p1 = write_temp_file("ds_hash_a.bin", std::string(1024, 'A'));
    std::string p2 = write_temp_file("ds_hash_b.bin", std::string(1024, 'A'));
    std::string p3 = write_temp_file("ds_hash_c.bin", std::string(1024, 'C'));

    FileEntry e1; e1.fullPath = p1; e1.sizeLogical = 1024;
    FileEntry e2; e2.fullPath = p2; e2.sizeLogical = 1024;
    FileEntry e3; e3.fullPath = p3; e3.sizeLogical = 1024;

    std::vector<FileEntry> cand = {e1,e2,e3};
    LSMIndex dummy("test_index");
    Deduplicator d(dummy);
    auto withHashes = d.computeHashesForTesting(cand);
    assert(withHashes.size() == 3);
    // Find digests
    auto findHash = [&](const std::string& path)->std::vector<uint8_t> {
        for (const auto& fe: withHashes) {
            if (fe.fullPath == path) { assert(fe.sha256.has_value()); return *fe.sha256; }
        }
        assert(false && "path not found");
        return {};
    };
    auto ha = findHash(p1);
    auto hb = findHash(p2);
    auto hc = findHash(p3);
    assert(ha.size() > 0 && hb.size() > 0 && hc.size() > 0);
    // two identical contents must match
    assert(ha == hb);
    // different content must differ
    assert(ha != hc);
    // cleanup
    std::error_code ec; std::filesystem::remove(p1, ec); std::filesystem::remove(p2, ec); std::filesystem::remove(p3, ec);
    return 0;
}
