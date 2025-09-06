#include <cassert>
#include <string>
#include <atomic>
#include "core/scan/scanner.h"
#ifdef _WIN32
#include "core/scan/win_mft.h"
#endif

int main() {
#ifdef _WIN32
    std::atomic<int> count{0};
    bool ok = winfs::enumerate_mft("C:\\", [&](const ScanEvent& ev){ if (ev.type==ScanEventType::FileAdded) count++; });
    // On CI without privileges, ok may be false; ensure no crash
    if (ok) { assert(count.load() >= 1); }
    else { assert(count.load() >= 0); }
#endif
    return 0;
}

