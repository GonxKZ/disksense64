#ifndef PLATFORM_UTIL_TRASH_H
#define PLATFORM_UTIL_TRASH_H

#include <string>
#include <vector>

namespace platform {

// Move a file or directory to the system Trash/Recycle Bin when possible.
// Returns true on success. Optionally returns the final trashed path.
bool move_to_trash(const std::string& path, std::string* trashed_path = nullptr);

struct TrashEntry { std::string trashed_path; std::string original_path; std::string deletion_date; };

// List trash items (Linux XDG Trash). On Windows returns empty list (not implemented).
std::vector<TrashEntry> list_trash();

// Restore a trashed item (Linux XDG Trash). On Windows returns false.
bool restore_from_trash(const TrashEntry& entry, std::string* restored_path = nullptr);

}

#endif // PLATFORM_UTIL_TRASH_H
