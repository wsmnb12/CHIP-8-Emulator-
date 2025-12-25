// Initialize and run a CHIP-8 / Super CHIP-8 emulator with SDL for graphics and input handling.

#include <stdio.h>
#include <stdbool.h>
#include <windows.h> // For Beep()
#define SDL_MAIN_HANDLED
#include <SDL.h>

#include "chip8.h"
#include "platform.h"
#include "rom_browser.h"

// Desired speeds
#define CPU_HZ   700
#define TIMER_HZ 60

int main(int argc, char* argv[]) {
    (void)argc;
    (void)argv;

    // Console: ROM browser
    roms_ensure_directory();

    RomList roms;
    if (!roms_scan(&roms)) {
        printf("Error scanning ROMs directory.\n");
        printf("Press Enter to exit...\n");
        getchar();
        return 1;
    }

    if (roms.count == 0) {
        printf("No ROMs found. Place .ch8 / Super CHIP-8 ROM files into the ROMs folder.\n");
        printf("Press Enter to exit...\n");
        getchar();
        roms_free(&roms);
        return 0;
    }

    int idx = roms_prompt_selection(&roms);
    if (idx < 0) {
        roms_free(&roms);
        return 0;
    }

    const char* rom_path = roms.paths[idx];
    printf("Loading ROM: %s\n", rom_path);

    // Initialize CHIP-8 machine
    Chip8 chip8;
    chip8_init(&chip8);

    if (!chip8_load_rom(&chip8, rom_path)) {
        printf("Failed to load ROM.\n");
        printf("Press Enter to exit...\n");
        getchar();
        roms_free(&roms);
        return 1;
    }

    roms_free(&roms);

    // Initialize SDL platform
    // Use high-res logical size; SDL will scale low-res as needed.
    int logical_w = CHIP8_HIGH_RES_WIDTH;
    int logical_h = CHIP8_HIGH_RES_HEIGHT;

    // Window scaling factor
    int scale = 8; // 128*8 = 1024, 64*8 = 512
    int window_w = logical_w * scale;
    int window_h = logical_h * scale;

    if (!platform_init("CHIP-8 / Super CHIP-8 Emulator",
        window_w, window_h,
        logical_w, logical_h)) {
        printf("Failed to initialize SDL.\n");
        printf("Press Enter to exit...\n");
        getchar();
        return 1;
    }

    bool quit = false;
    Uint32 last_timer_tick = SDL_GetTicks();
    Uint32 last_cycle_tick = SDL_GetTicks();

    // Main emulation loop
    while (!quit && chip8.running) {
        Uint32 now = SDL_GetTicks();
        Uint32 elapsed_ms = now - last_cycle_tick;

        // Run enough cycles to approximate CPU_HZ
        double cycles_to_run_f = (double)elapsed_ms * CPU_HZ / 1000.0;
        int cycles_to_run = (int)cycles_to_run_f;
        if (cycles_to_run <= 0) {
            cycles_to_run = 1;
        }

        for (int i = 0; i < cycles_to_run && chip8.running; ++i) {
            chip8_cycle(&chip8);
        }
        last_cycle_tick = now;

        // Timers at 60Hz
        Uint32 timer_now = SDL_GetTicks();
        if (timer_now - last_timer_tick >= (1000 / TIMER_HZ)) {
            if (chip8.delay_timer > 0) {
                chip8.delay_timer--;
            }
            if (chip8.sound_timer > 0) {
                // Simple square beep; you can enhance with SDL audio if desired
                Beep(800, 10); // frequency 800Hz, duration 10ms
                chip8.sound_timer--;
            }
            last_timer_tick = timer_now;
        }

        // Handle input (ESC or window close should quit)
        platform_handle_input(&chip8, &quit);

        // Redraw if needed
        if (chip8.draw_flag) {
            platform_draw(&chip8);
            chip8.draw_flag = false;
        }

        SDL_Delay(1); // Small delay to avoid 100% CPU usage
    }

    platform_cleanup();

    // When emulator exits (ESC or window close), console will also terminate because the process ends.
    return 0;
}
