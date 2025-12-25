// Chip-8 emulator core header file for defining the Chip8 struct and function prototypes.

#ifndef CHIP8_H
#define CHIP8_H

#include <stdint.h>
#include <stdbool.h>

#define CHIP8_MEMORY_SIZE       4096
#define CHIP8_STACK_SIZE        16
#define CHIP8_REGISTER_COUNT    16
#define CHIP8_KEY_COUNT         16

#define CHIP8_LOW_RES_WIDTH     64
#define CHIP8_LOW_RES_HEIGHT    32
#define CHIP8_HIGH_RES_WIDTH    128
#define CHIP8_HIGH_RES_HEIGHT   64

typedef struct Chip8 {
    uint8_t  memory[CHIP8_MEMORY_SIZE];
    uint8_t  V[CHIP8_REGISTER_COUNT];  // General registers V0-VF
    uint16_t I;                        // Index register
    uint16_t pc;                       // Program counter
    uint16_t stack[CHIP8_STACK_SIZE];
    uint8_t  sp;                       // Stack pointer
    uint8_t  delay_timer;
    uint8_t  sound_timer;

    bool     display[CHIP8_HIGH_RES_WIDTH * CHIP8_HIGH_RES_HEIGHT];
    bool     keys[CHIP8_KEY_COUNT];

    bool     draw_flag;
    bool     high_res;   // false = 64x32, true = 128x64
    bool     running;    // false when 00FD (exit) or external quit

} Chip8;

// Initialize machine state and load fonts
void chip8_init(Chip8* c8);

// Load ROM into memory starting at 0x200
bool chip8_load_rom(Chip8* c8, const char* path);

// Execute a single instruction cycle
void chip8_cycle(Chip8* c8);

// Key press / release
void chip8_key_down(Chip8* c8, uint8_t key);
void chip8_key_up(Chip8* c8, uint8_t key);

// Get display buffer and current logical resolution
const bool* chip8_get_display(const Chip8* c8, int* width, int* height);

#endif 
