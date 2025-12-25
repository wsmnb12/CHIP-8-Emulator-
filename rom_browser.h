// Creates a simple ROM browser for a Chip-8 emulator via console.

#ifndef ROM_BROWSER_H
#define ROM_BROWSER_H

#include <stdbool.h>

typedef struct RomList {
    char** paths;
    int    count;
} RomList;

// Ensure ROMs directory exists
void roms_ensure_directory(void);

// Scan ROMs directory for files; fills RomList.
// Returns false on error or no directory.
bool roms_scan(RomList* list);

// Free paths allocated in RomList
void roms_free(RomList* list);

// Print ROM list to console and ask user to select one.
// Returns index in [0, count) or -1 on cancel.
int roms_prompt_selection(const RomList* list);

#endif 
