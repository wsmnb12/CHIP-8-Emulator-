---// defines the Chip8 structure used in platform.h

#ifndef PLATFORM_H
#define PLATFORM_H

#include <stdbool.h>
#include "chip8.h"

// Initialize SDL, create window/renderer/texture.
// window_width/height: actual window size on screen.
// logical_width/height: current CHIP-8 resolution (will be scaled).
bool platform_init(const char* title,
    int window_width, int window_height,
    int logical_width, int logical_height);

// Handle SDL input and map to CHIP-8 keys; set quit to true on ESC or window close.
void platform_handle_input(Chip8* c8, bool* quit);

// Draw current CHIP-8 display buffer to the window.
void platform_draw(const Chip8* c8);

// Clean up SDL resources.
void platform_cleanup(void);

#endif // PLATFORM_H
#pragma once
