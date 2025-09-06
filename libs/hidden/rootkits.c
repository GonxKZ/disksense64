#include "rootkits.h"

// Known rootkits database
static const rootkit_info_t g_known_rootkits[] = {
    {
        "Adore",
        "Linux rootkit that hides files, processes, and network connections",
        "Check for /dev/.adore directory and adore module in kernel"
    },
    {
        "Beast",
        "Windows rootkit that hides processes and files",
        "Check for Beast registry entries and loaded drivers"
    },
    {
        "Diamorphine",
        "Linux LKM rootkit that hides processes, files, and network activity",
        "Check for diamorphine module and /proc/diamorphine_hidden_inodes"
    },
    {
        "Duckroot",
        "Windows rootkit that hides files and processes",
        "Check for Duckroot registry keys and loaded modules"
    },
    {
        "Flea",
        "Linux rootkit that hides files and processes",
        "Check for /etc/ld.so.preload modifications and hidden files"
    },
    {
        "Knark",
        "Linux rootkit that hides processes and files",
        "Check for knark module and /proc/knark directory"
    },
    {
        "Moodnt",
        "Windows rootkit that hides network connections",
        "Check for Moodnt registry entries and loaded drivers"
    },
    {
        "Ni4",
        "Linux rootkit that hides files and processes",
        "Check for ni4 module and /dev/.ni4 directory"
    },
    {
        "Orcus",
        "Windows rootkit and RAT",
        "Check for Orcus registry keys and processes"
    },
    {
        "Reptile",
        "Linux rootkit that hides files, processes, and network connections",
        "Check for reptile module and /reptile directory"
    },
    {
        "Suckit",
        "Linux rootkit that replaces system binaries",
        "Check for modified system binaries and /dev/.sk directory"
    },
    {
        "Torn",
        "Linux rootkit that hides files and processes",
        "Check for /dev/.torn directory and torn module"
    },
    {
        "Xzibit",
        "Windows rootkit that hides files and processes",
        "Check for Xzibit registry entries and loaded modules"
    }
};

// Function to get known rootkits
const rootkit_info_t* hidden_get_known_rootkits(size_t* count) {
    if (count) {
        *count = sizeof(g_known_rootkits) / sizeof(rootkit_info_t);
    }
    return g_known_rootkits;
}