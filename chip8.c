// Complete CPU draw flag and display handling


#include "chip8.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>

// Standard CHIP-8 4x5 font (0-F)
static const uint8_t font_small[16 * 5] = {
    0xF0, 0x90, 0x90, 0x90, 0xF0, // 0
    0x20, 0x60, 0x20, 0x20, 0x70, // 1
    0xF0, 0x10, 0xF0, 0x80, 0xF0, // 2
    0xF0, 0x10, 0xF0, 0x10, 0xF0, // 3
    0x90, 0x90, 0xF0, 0x10, 0x10, // 4
    0xF0, 0x80, 0xF0, 0x10, 0xF0, // 5
    0xF0, 0x80, 0xF0, 0x90, 0xF0, // 6
    0xF0, 0x10, 0x20, 0x40, 0x40, // 7
    0xF0, 0x90, 0xF0, 0x90, 0xF0, // 8
    0xF0, 0x90, 0xF0, 0x10, 0xF0, // 9
    0xF0, 0x90, 0xF0, 0x90, 0x90, // A
    0xE0, 0x90, 0xE0, 0x90, 0xE0, // B
    0xF0, 0x80, 0x80, 0x80, 0xF0, // C
    0xE0, 0x90, 0x90, 0x90, 0xE0, // D
    0xF0, 0x80, 0xF0, 0x80, 0xF0, // E
    0xF0, 0x80, 0xF0, 0x80, 0x80  // F
};

// Super CHIP-8 8x10 font for 0-9 (big font)
static const uint8_t font_big[10 * 10] = {
    // 0
    0x3C, 0x42, 0x81, 0x81, 0x81, 0x81, 0x81, 0x81, 0x42, 0x3C,
    // 1
    0x18, 0x38, 0x58, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x7E,
    // 2
    0x3C, 0x42, 0x81, 0x01, 0x02, 0x0C, 0x30, 0x40, 0x80, 0xFF,
    // 3
    0xFF, 0x02, 0x0C, 0x10, 0x3C, 0x02, 0x01, 0x81, 0x42, 0x3C,
    // 4
    0x04, 0x0C, 0x14, 0x24, 0x44, 0x84, 0xFF, 0x04, 0x04, 0x04,
    // 5
    0xFF, 0x80, 0x80, 0xFC, 0x02, 0x01, 0x01, 0x81, 0x42, 0x3C,
    // 6
    0x3C, 0x42, 0x81, 0x80, 0xFC, 0x82, 0x81, 0x81, 0x42, 0x3C,
    // 7
    0xFF, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x40, 0x40, 0x40,
    // 8
    0x3C, 0x42, 0x81, 0x42, 0x3C, 0x42, 0x81, 0x81, 0x42, 0x3C,
    // 9
    0x3C, 0x42, 0x81, 0x81, 0x43, 0x3D, 0x01, 0x81, 0x42, 0x3C
};

static void clear_display(Chip8* c8) {
    memset(c8->display, 0, sizeof(c8->display));
    c8->draw_flag = true;
}

void chip8_init(Chip8* c8) {
    memset(c8, 0, sizeof(Chip8));

    c8->pc = 0x200;           // Programs start at 0x200
    c8->I = 0;
    c8->sp = 0;
    c8->high_res = false;          // Start in low resolution
    c8->running = true;

    // Load small font at 0x000
    memcpy(&c8->memory[0x000], font_small, sizeof(font_small));

    // Load big font at 0x050
    memcpy(&c8->memory[0x050], font_big, sizeof(font_big));

    srand((unsigned int)time(NULL));
}

bool chip8_load_rom(Chip8* c8, const char* path) {
    FILE* f = NULL;
#ifdef _MSC_VER
    fopen_s(&f, path, "rb");
#else
    f = fopen(path, "rb");
#endif
    if (!f) {
        fprintf(stderr, "Failed to open ROM: %s\n", path);
        return false;
    }

    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);

    if (size <= 0 || (size + 0x200) > CHIP8_MEMORY_SIZE) {
        fprintf(stderr, "ROM too big or invalid size\n");
        fclose(f);
        return false;
    }

    size_t read = fread(&c8->memory[0x200], 1, (size_t)size, f);
    fclose(f);

    if (read != (size_t)size) {
        fprintf(stderr, "ROM read mismatch\n");
        return false;
    }

    return true;
}

void chip8_key_down(Chip8* c8, uint8_t key) {
    if (key < CHIP8_KEY_COUNT) {
        c8->keys[key] = true;
    }
}

void chip8_key_up(Chip8* c8, uint8_t key) {
    if (key < CHIP8_KEY_COUNT) {
        c8->keys[key] = false;
    }
}
// Get display buffer and current logical resolution
const bool* chip8_get_display(const Chip8* c8, int* width, int* height) {
    if (c8->high_res) {
        *width = CHIP8_HIGH_RES_WIDTH;
        *height = CHIP8_HIGH_RES_HEIGHT;
    }
    else {
        *width = CHIP8_LOW_RES_WIDTH;
        *height = CHIP8_LOW_RES_HEIGHT;
    }
    return c8->display;
}

// Scroll display down by n pixels
static void scroll_down(Chip8* c8, uint8_t n) {
    int w = c8->high_res ? CHIP8_HIGH_RES_WIDTH : CHIP8_LOW_RES_WIDTH;
    int h = c8->high_res ? CHIP8_HIGH_RES_HEIGHT : CHIP8_LOW_RES_HEIGHT;

    if (n == 0) return;
    if (n > h) n = (uint8_t)h;

    for (int y = h - 1; y >= 0; --y) {
        int src_y = y - n;
        for (int x = 0; x < w; ++x) {
            bool val = false;
            if (src_y >= 0)
                val = c8->display[src_y * CHIP8_HIGH_RES_WIDTH + x];
            c8->display[y * CHIP8_HIGH_RES_WIDTH + x] = val;
        }
    }
    c8->draw_flag = true;
}
// Scroll display right by 4 pixels
static void scroll_right(Chip8* c8) {
    int w = c8->high_res ? CHIP8_HIGH_RES_WIDTH : CHIP8_LOW_RES_WIDTH;
    int h = c8->high_res ? CHIP8_HIGH_RES_HEIGHT : CHIP8_LOW_RES_HEIGHT;

    for (int y = 0; y < h; ++y) {
        for (int x = w - 1; x >= 0; --x) {
            int src_x = x - 4;
            bool val = false;
            if (src_x >= 0)
                val = c8->display[y * CHIP8_HIGH_RES_WIDTH + src_x];
            c8->display[y * CHIP8_HIGH_RES_WIDTH + x] = val;
        }
    }
    c8->draw_flag = true;
}
// Scroll display left by 4 pixels
static void scroll_left(Chip8* c8) {
    int w = c8->high_res ? CHIP8_HIGH_RES_WIDTH : CHIP8_LOW_RES_WIDTH;
    int h = c8->high_res ? CHIP8_HIGH_RES_HEIGHT : CHIP8_LOW_RES_HEIGHT;

    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            int src_x = x + 4;
            bool val = false;
            if (src_x < w)
                val = c8->display[y * CHIP8_HIGH_RES_WIDTH + src_x];
            c8->display[y * CHIP8_HIGH_RES_WIDTH + x] = val;
        }
    }
    c8->draw_flag = true;
}

static void draw_sprite(Chip8* c8, uint8_t x, uint8_t y, uint8_t n) {
    int w = c8->high_res ? CHIP8_HIGH_RES_WIDTH : CHIP8_LOW_RES_WIDTH;
    int h = c8->high_res ? CHIP8_HIGH_RES_HEIGHT : CHIP8_LOW_RES_HEIGHT;

    c8->V[0xF] = 0;

    if (n == 0 && c8->high_res) {
        // Super CHIP-8 16x16 sprite
        for (int row = 0; row < 16; ++row) {
            uint16_t spr_row = (uint16_t)c8->memory[c8->I + row * 2] << 8 |
                (uint16_t)c8->memory[c8->I + row * 2 + 1];

            for (int col = 0; col < 16; ++col) {
                if ((spr_row & (0x8000 >> col)) != 0) {
                    int px = (x + col) % w;
                    int py = (y + row) % h;
                    int idx = py * CHIP8_HIGH_RES_WIDTH + px;
                    if (c8->display[idx])
                        c8->V[0xF] = 1;
                    c8->display[idx] = !c8->display[idx];
                }
            }
        }
    }
    else {
        // Standard 8xN sprite
        for (int row = 0; row < n; ++row) {
            uint8_t spr_row = c8->memory[c8->I + row];
            for (int col = 0; col < 8; ++col) {
                if ((spr_row & (0x80 >> col)) != 0) {
                    int px = (x + col) % w;
                    int py = (y + row) % h;
                    int idx = py * CHIP8_HIGH_RES_WIDTH + px;
                    if (c8->display[idx])
                        c8->V[0xF] = 1;
                    c8->display[idx] = !c8->display[idx];
                }
            }
        }
    }

    c8->draw_flag = true;
}

void chip8_cycle(Chip8* c8) {
    if (!c8->running) return;

    uint16_t opcode = (uint16_t)c8->memory[c8->pc] << 8 |
        (uint16_t)c8->memory[c8->pc + 1];
    c8->pc += 2;

    uint8_t  x = (opcode & 0x0F00) >> 8;
    uint8_t  y = (opcode & 0x00F0) >> 4;
    uint8_t  n = (opcode & 0x000F);
    uint8_t  kk = (opcode & 0x00FF);
    uint16_t nnn = (opcode & 0x0FFF);

    switch (opcode & 0xF000) {
    case 0x0000:
        switch (opcode) {
        case 0x00E0: // CLS
            clear_display(c8);
            break;
        case 0x00EE: // RET
            if (c8->sp > 0) {
                c8->sp--;
                c8->pc = c8->stack[c8->sp];
            }
            break;
        case 0x00FE: // LOW RES (Super CHIP-8)
            c8->high_res = false;
            clear_display(c8);
            break;
        case 0x00FF: // HIGH RES (Super CHIP-8)
            c8->high_res = true;
            clear_display(c8);
            break;
        case 0x00FD: // EXIT (Super CHIP-8)
            c8->running = false;
            break;
        case 0x00FB: // SCROLL RIGHT 4
            scroll_right(c8);
            break;
        case 0x00FC: // SCROLL LEFT 4
            scroll_left(c8);
            break;
        default:
            if ((opcode & 0xFFF0) == 0x00C0) {
                // 00CN: scroll down N lines
                uint8_t lines = (uint8_t)(opcode & 0x000F);
                scroll_down(c8, lines);
            }
            else {
                // System call / ignored
            }
            break;
        }
        break;

    case 0x1000: // JP addr
        c8->pc = nnn;
        break;

    case 0x2000: // CALL addr
        if (c8->sp < CHIP8_STACK_SIZE) {
            c8->stack[c8->sp] = c8->pc;
            c8->sp++;
            c8->pc = nnn;
        }
        break;

    case 0x3000: // SE Vx, byte
        if (c8->V[x] == kk)
            c8->pc += 2;
        break;

    case 0x4000: // SNE Vx, byte
        if (c8->V[x] != kk)
            c8->pc += 2;
        break;

    case 0x5000: // SE Vx, Vy
        if ((opcode & 0x000F) == 0x0) {
            if (c8->V[x] == c8->V[y])
                c8->pc += 2;
        }
        break;

    case 0x6000: // LD Vx, byte
        c8->V[x] = kk;
        break;

    case 0x7000: // ADD Vx, byte
        c8->V[x] += kk;
        break;

    case 0x8000:
        switch (opcode & 0x000F) {
        case 0x0: // LD Vx, Vy
            c8->V[x] = c8->V[y];
            break;
        case 0x1: // OR Vx, Vy
            c8->V[x] |= c8->V[y];
            break;
        case 0x2: // AND Vx, Vy
            c8->V[x] &= c8->V[y];
            break;
        case 0x3: // XOR Vx, Vy
            c8->V[x] ^= c8->V[y];
            break;
        case 0x4: { // ADD Vx, Vy
            uint16_t sum = c8->V[x] + c8->V[y];
            c8->V[0xF] = sum > 0xFF;
            c8->V[x] = (uint8_t)(sum & 0xFF);
        } break;
        case 0x5: // SUB Vx, Vy
            c8->V[0xF] = c8->V[x] > c8->V[y];
            c8->V[x] = (uint8_t)(c8->V[x] - c8->V[y]);
            break;
        case 0x6: // SHR Vx {, Vy}
            c8->V[0xF] = c8->V[x] & 0x1;
            c8->V[x] >>= 1;
            break;
        case 0x7: // SUBN Vx, Vy
            c8->V[0xF] = c8->V[y] > c8->V[x];
            c8->V[x] = (uint8_t)(c8->V[y] - c8->V[x]);
            break;
        case 0xE: // SHL Vx {, Vy}
            c8->V[0xF] = (c8->V[x] & 0x80) >> 7;
            c8->V[x] <<= 1;
            break;
        default:
            break;
        }
        break;

    case 0x9000: // SNE Vx, Vy
        if ((opcode & 0x000F) == 0x0) {
            if (c8->V[x] != c8->V[y])
                c8->pc += 2;
        }
        break;

    case 0xA000: // LD I, addr
        c8->I = nnn;
        break;

    case 0xB000: // JP V0, addr
        c8->pc = nnn + c8->V[0];
        break;

    case 0xC000: // RND Vx, byte
        c8->V[x] = (uint8_t)((rand() % 256) & kk);
        break;

    case 0xD000: // DRW Vx, Vy, nibble
        draw_sprite(c8, c8->V[x], c8->V[y], n);
        break;

    case 0xE000:
        switch (opcode & 0x00FF) {
        case 0x9E: // SKP Vx
            if (c8->keys[c8->V[x]]) c8->pc += 2;
            break;
        case 0xA1: // SKNP Vx
            if (!c8->keys[c8->V[x]]) c8->pc += 2;
            break;
        default:
            break;
        }
        break;

    case 0xF000:
        switch (opcode & 0x00FF) {
        case 0x07: // LD Vx, DT
            c8->V[x] = c8->delay_timer;
            break;
        case 0x0A: { // LD Vx, K (wait for key)
            bool found = false;
            for (int i = 0; i < CHIP8_KEY_COUNT; ++i) {
                if (c8->keys[i]) {
                    c8->V[x] = (uint8_t)i;
                    found = true;
                    break;
                }
            }
            if (!found) {
                // Repeat this instruction
                c8->pc -= 2;
            }
        } break;
        case 0x15: // LD DT, Vx
            c8->delay_timer = c8->V[x];
            break;
        case 0x18: // LD ST, Vx
            c8->sound_timer = c8->V[x];
            break;
        case 0x1E: // ADD I, Vx
            c8->I += c8->V[x];
            break;
        case 0x29: // LD F, Vx (small font)
            c8->I = (uint16_t)(c8->V[x] * 5);
            break;
        case 0x30: // LD HF, Vx (big font digit)
            c8->I = (uint16_t)(0x50 + (c8->V[x] * 10));
            break;
        case 0x33: { // LD B, Vx (BCD)
            uint8_t v = c8->V[x];
            c8->memory[c8->I + 0] = (uint8_t)(v / 100);
            c8->memory[c8->I + 1] = (uint8_t)((v / 10) % 10);
            c8->memory[c8->I + 2] = (uint8_t)(v % 10);
        } break;
        case 0x55: // LD [I], V0..Vx
            for (uint8_t i = 0; i <= x; ++i) {
                c8->memory[c8->I + i] = c8->V[i];
            }
            break;
        case 0x65: // LD V0..Vx, [I]
            for (uint8_t i = 0; i <= x; ++i) {
                c8->V[i] = c8->memory[c8->I + i];
            }
            break;
        default:
            break;
        }
        break;

    default:
        break;
    }
}
