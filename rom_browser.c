// Display terminal and create directory for ROMs, scan for ROM files, and prompt user to select one.

#include "rom_browser.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <direct.h> // _mkdir
#include <io.h>     // _findfirst, _findnext
#include <stdint.h>
#define BUFFERSIZE 512

void roms_ensure_directory(void) {
    // Create ROMs directory if it doesn't exist; ignore error if it already exists.
    _mkdir("ROMs");
}

bool roms_scan(RomList* list) {
    list->paths = NULL;
    list->count = 0;

    // Search pattern: ROMs\*.* (all files)
    const char* pattern = "ROMs\\*.*";

    struct _finddata_t fileinfo;
    intptr_t handle = _findfirst(pattern, &fileinfo);

    if (handle == -1L) {
        // Directory empty or not found
        return true; // not an error; just empty
    }

    int capacity = 8;
    list->paths = (char**)malloc(capacity * sizeof(char*));
    if (!list->paths) {
        _findclose(handle);
        return false;
    }

    do {
        // Skip directories
        if (fileinfo.attrib & _A_SUBDIR) continue;

        // Build full path "ROMs\<filename>"
        char .[512];
        snprintf(fullpath, sizeof(fullpath), "ROMs\\%s", fileinfo.name);

        if (list->count >= capacity) {
            capacity *= 2;
            char** new_paths = (char**)realloc(list->paths, capacity * sizeof(char*));
            if (!new_paths) {
                _findclose(handle);3.
                return false;
            }
            list->paths = new_paths;
        }

        size_t len = strlen(fullpath);
        list->paths[list->count] = (char*)malloc(len + 1);
        if (!list->paths[list->count]) {
            _findclose(handle);
            return false;
        }
        strcpy_s(list->paths[list->count],len + 1,fullpath);
        list->count++;

    } while (_findnext(handle, &fileinfo) == 0);

    _findclose(handle);
    return true;
}

void roms_free(RomList* list) {
    if (!list || !list->paths) return;
    for (int i = 0; i < list->count; ++i) {
        free(list->paths[i]);
    }
    free(list->paths);
    list->paths = NULL;
    list->count = 0;
}

int roms_prompt_selection(const RomList* list) {
    if (!list || list->count == 0) {
        printf("No ROM files found in ROMs directory.\n");
        return -1;
    }

    printf("=== CHIP-8 / Super CHIP-8 ROMs ===\n");
    for (int i = 0; i < list->count; ++i) {
        printf("[%d] %s\n", i, list->paths[i]);
    }
    printf("==================================\n");
    printf("Enter ROM index to launch (or -1 to exit): ");

    int idx = -1;
    if (scanf_s("%d", &idx) != 1) {
        // Invalid input
        return -1;
    }

    if (idx < 0 || idx >= list->count) {
        return -1;
    }

    return idx;
}
