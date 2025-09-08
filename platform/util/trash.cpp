#include "trash.h"
#include <filesystem>
#include <string>

#ifdef _WIN32
#  include <windows.h>
#  include <shlobj.h>
#  include <shobjidl.h>
#  include <shellapi.h>
#  include <knownfolders.h>
#  include <propsys.h>
#  include <propkey.h>
#  pragma comment(lib, "ole32.lib")
#  pragma comment(lib, "shell32.lib")
#else
#  include <fstream>
#  include <time.h>
#  include <pwd.h>
#  include <unistd.h>
#endif

namespace platform {

#ifdef _WIN32
static std::wstring to_wstring(const std::string& s) {
    return std::wstring(s.begin(), s.end());
}

static bool move_to_recycle_bin_ifileop(const std::wstring& wpath) {
    HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
    if (FAILED(hr)) return false;
    IFileOperation* pfo = nullptr;
    hr = CoCreateInstance(CLSID_FileOperation, NULL, CLSCTX_ALL, IID_PPV_ARGS(&pfo));
    if (FAILED(hr)) { CoUninitialize(); return false; }
    pfo->SetOperationFlags(FOF_ALLOWUNDO | FOF_NOCONFIRMMKDIR | FOF_SILENT | FOFX_NOCOPYSECURITYATTRIBS);
    IShellItem* item = nullptr;
    hr = SHCreateItemFromParsingName(wpath.c_str(), NULL, IID_PPV_ARGS(&item));
    if (SUCCEEDED(hr)) {
        pfo->DeleteItem(item, NULL);
        item->Release();
    }
    hr = pfo->PerformOperations();
    pfo->Release();
    CoUninitialize();
    return SUCCEEDED(hr);
}
#endif

bool move_to_trash(const std::string& path, std::string* trashed_path) {
#ifdef _WIN32
    std::wstring wpath = to_wstring(path);
    bool ok = move_to_recycle_bin_ifileop(wpath);
    if (ok && trashed_path) *trashed_path = std::string();
    return ok;
#else
    // XDG Trash spec: ~/.local/share/Trash/{files,info}
    const char* home = getenv("HOME");
    if (!home) {
        struct passwd* pw = getpwuid(getuid());
        if (!pw) return false;
        home = pw->pw_dir;
    }
    std::filesystem::path trash = std::filesystem::path(home) / ".local/share/Trash";
    std::filesystem::path filesDir = trash / "files";
    std::filesystem::path infoDir = trash / "info";
    std::error_code ec; std::filesystem::create_directories(filesDir, ec); ec.clear();
    std::filesystem::create_directories(infoDir, ec); ec.clear();
    std::filesystem::path src(path);
    if (!std::filesystem::exists(src, ec)) return false;
    std::string base = src.filename().string();
    std::filesystem::path dst = filesDir / base;
    int suffix = 1;
    while (std::filesystem::exists(dst, ec)) {
        dst = filesDir / (base + "." + std::to_string(suffix++));
    }
    ec.clear();
    std::filesystem::rename(src, dst, ec);
    if (ec) return false;
    // Create .trashinfo
    std::filesystem::path infoPath = infoDir / (dst.filename().string() + ".trashinfo");
    std::ofstream info(infoPath);
    if (info.is_open()) {
        // ISO 8601 date-time
        time_t now = time(nullptr); struct tm tmv; localtime_r(&now, &tmv);
        char buf[64]; strftime(buf, sizeof(buf), "%Y-%m-%dT%H:%M:%S", &tmv);
        info << "[Trash Info]\nPath=" << src.string() << "\nDeletionDate=" << buf << "\n";
        info.close();
    }
    if (trashed_path) *trashed_path = dst.string();
    return true;
#endif
}

std::vector<TrashEntry> list_trash() {
#ifdef _WIN32
    std::vector<TrashEntry> out;
    HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
    if (FAILED(hr)) return out;
    PIDLIST_ABSOLUTE pidlRB = nullptr;
    hr = SHGetKnownFolderIDList(FOLDERID_RecycleBinFolder, 0, NULL, &pidlRB);
    if (FAILED(hr)) { CoUninitialize(); return out; }
    IShellFolder* pFolder = nullptr;
    hr = SHBindToObject(NULL, pidlRB, NULL, IID_PPV_ARGS(&pFolder));
    if (FAILED(hr)) { CoTaskMemFree(pidlRB); CoUninitialize(); return out; }
    IEnumIDList* penum = nullptr;
    hr = pFolder->EnumObjects(NULL, SHCONTF_FOLDERS|SHCONTF_NONFOLDERS, &penum);
    if (SUCCEEDED(hr) && penum) {
        LPITEMIDLIST pidlRel = nullptr; ULONG fetched=0;
        while (penum->Next(1, &pidlRel, &fetched) == S_OK) {
            PIDLIST_ABSOLUTE pidlAbs = ILCombine(pidlRB, pidlRel);
            IShellItem2* item = nullptr;
            if (SUCCEEDED(SHCreateItemFromIDList(pidlAbs, IID_PPV_ARGS(&item)))) {
                // Absolute parsing path
                LPWSTR psz = nullptr; if (SUCCEEDED(item->GetDisplayName(SIGDN_DESKTOPABSOLUTEPARSING, &psz))) {
                    TrashEntry e; e.trashed_path = to_utf8(psz); CoTaskMemFree(psz);
                    // Properties: original location + item name → original path; deleted date
                    IPropertyStore* ps = nullptr;
                    if (SUCCEEDED(item->GetPropertyStore(GPS_DEFAULT, IID_PPV_ARGS(&ps)))) {
                        PROPVARIANT v; PropVariantInit(&v);
                        // Original location folder
                        std::wstring origLoc; if (SUCCEEDED(ps->GetValue(PKEY_OriginalLocation, &v)) && v.vt == VT_LPWSTR) { origLoc = v.pwszVal; }
                        PropVariantClear(&v);
                        // Item display name
                        std::wstring name; if (SUCCEEDED(ps->GetValue(PKEY_ItemNameDisplay, &v)) && v.vt == VT_LPWSTR) { name = v.pwszVal; }
                        PropVariantClear(&v);
                        if (!origLoc.empty()) {
                            if (!name.empty()) e.original_path = to_utf8(origLoc + L"\\" + name);
                            else e.original_path = to_utf8(origLoc);
                        }
                        // Date deleted
                        if (SUCCEEDED(ps->GetValue(PKEY_DateDeleted, &v)) && v.vt == VT_FILETIME) {
                            SYSTEMTIME st; FileTimeToSystemTime(&v.filetime, &st);
                            wchar_t buf[64]; swprintf(buf, 64, L"%04d-%02d-%02d %02d:%02d", st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute);
                            e.deletion_date = to_utf8(buf);
                        }
                        PropVariantClear(&v);
                        ps->Release();
                    }
                    out.push_back(std::move(e));
                }
                item->Release();
            }
            CoTaskMemFree(pidlAbs);
            CoTaskMemFree(pidlRel);
        }
        penum->Release();
    }
    pFolder->Release();
    CoTaskMemFree(pidlRB);
    CoUninitialize();
    return out;
#else
    std::vector<TrashEntry> out;
    const char* home = getenv("HOME");
    if (!home) { struct passwd* pw = getpwuid(getuid()); if (!pw) return out; home = pw->pw_dir; }
    std::filesystem::path trash = std::filesystem::path(home) / ".local/share/Trash";
    std::filesystem::path filesDir = trash / "files";
    std::filesystem::path infoDir = trash / "info";
    std::error_code ec; if (!std::filesystem::exists(infoDir, ec)) return out;
    for (auto it = std::filesystem::directory_iterator(infoDir, ec); !ec && it != std::filesystem::end(it); ++it) {
        if (!it->is_regular_file()) continue;
        auto p = it->path(); if (p.extension() != ".trashinfo") continue;
        std::ifstream f(p); if (!f.is_open()) continue;
        std::string line, orig, date;
        while (std::getline(f, line)) {
            if (line.rfind("Path=",0)==0) orig = line.substr(5);
            else if (line.rfind("DeletionDate=",0)==0) date = line.substr(13);
        }
        f.close();
        TrashEntry e; e.original_path = orig; e.deletion_date = date;
        e.trashed_path = (filesDir / p.stem()).string();
        out.push_back(std::move(e));
    }
    return out;
#endif
}

bool restore_from_trash(const TrashEntry& entry, std::string* restored_path) {
#ifdef _WIN32
    bool ok = false;
    HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
    if (FAILED(hr)) return false;
    PIDLIST_ABSOLUTE pidl = nullptr;
    ULONG eaten=0; DWORD attrs=0;
    std::wstring wabs = to_wstr(entry.trashed_path);
    hr = SHParseDisplayName(wabs.c_str(), NULL, &pidl, 0, &attrs);
    if (SUCCEEDED(hr) && pidl) {
        IShellFolder* pParent = nullptr; PCUITEMID_CHILD pidlChild = nullptr;
        if (SUCCEEDED(SHBindToParent(pidl, IID_PPV_ARGS(&pParent), &pidlChild)) && pParent) {
            IContextMenu* pcm = nullptr;
            HRESULT hr2 = pParent->GetUIObjectOf(NULL, 1, &pidlChild, IID_IContextMenu, NULL, (void**)&pcm);
            if (SUCCEEDED(hr2) && pcm) {
                CMINVOKECOMMANDINFOEX ci = {0};
                ci.cbSize = sizeof(ci);
                ci.fMask = CMIC_MASK_UNICODE;
                ci.hwnd = NULL;
                ci.lpVerb = "undelete";
                ci.lpVerbW = L"undelete";
                ci.nShow = SW_SHOWNORMAL;
                if (SUCCEEDED(pcm->InvokeCommand((CMINVOKECOMMANDINFO*)&ci))) ok = true;
                pcm->Release();
            }
            pParent->Release();
        }
        CoTaskMemFree(pidl);
    }
    CoUninitialize();
    return ok;
#else
    std::error_code ec;
    std::filesystem::path src(entry.trashed_path);
    std::filesystem::path dst(entry.original_path);
    std::filesystem::create_directories(dst.parent_path(), ec); ec.clear();
    std::filesystem::rename(src, dst, ec);
    if (ec) return false;
    if (restored_path) *restored_path = dst.string();
    // Remove .trashinfo
    std::filesystem::path info = std::filesystem::path(getenv("HOME")) / ".local/share/Trash/info" / (std::filesystem::path(entry.trashed_path).filename().string() + ".trashinfo");
    std::filesystem::remove(info, ec);
    return true;
#endif
}

} // namespace platform
