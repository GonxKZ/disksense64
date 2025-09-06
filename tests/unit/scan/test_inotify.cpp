#include <cassert>
#include <atomic>
#include <thread>
#include <chrono>
#include <string>
#include <filesystem>
#include <cstdio>
#include "core/scan/monitor.h"

#if defined(__linux__)
int main(){
    std::string dir = (std::filesystem::temp_directory_path() / "ds_inotify_test").string();
    std::error_code ec; std::filesystem::create_directories(dir, ec);
    FsMonitor mon;
    std::atomic<int> added{0};
    bool ok = mon.start(dir, [&](const ScanEvent& ev){ if (ev.type==ScanEventType::FileAdded) added++; });
    assert(ok);
    // create a file
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    std::string f = dir+"/x.txt"; FILE* fp=fopen(f.c_str(),"wb"); assert(fp); fputs("x",fp); fclose(fp);
    // wait up to 2s
    for (int i=0;i<20 && added.load()==0;i++) std::this_thread::sleep_for(std::chrono::milliseconds(100));
    mon.stop();
    std::filesystem::remove_all(dir, ec);
    assert(added.load()>=1);
    return 0;
}
#else
int main(){ return 0; }
#endif

