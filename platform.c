// Generate the implementation of the platform layer for the CHIP-8 emulator using SDL2.

#include "platform.h"
#include <SDL.h>
#include <string.h>

static SDL_Window* g_window = NULL;
static SDL_Renderer* g_renderer = NULL;
static SDL_Texture* g_texture = NULL;

static int g_window_width = 0;
static int g_window_height = 0;

bool platform_init(const char* title,
    int window_width, int window_height,
    int logical_width, int logical_height)
{
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_EVENTS) != 0) {
        SDL_Log("SDL_Init failed: %s", SDL_GetError());
        return false;
    }

    g_window_width = window_width;
    g_window_height = window_height;

    g_window = SDL_CreateWindow(
        title,
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        g_window_width, g_window_height,
        SDL_WINDOW_SHOWN
    );

    if (!g_window) {
        SDL_Log("SDL_CreateWindow failed: %s", SDL_GetError());
        SDL_Quit();
        return false;
    }

    g_renderer = SDL_CreateRenderer(
        g_window, -1,
        SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC
    );
    if (!g_renderer) {
        SDL_Log("SDL_CreateRenderer failed: %s", SDL_GetError());
        SDL_DestroyWindow(g_window);
        SDL_Quit();
        return false;
    }

    // Texture for full high-res buffer (128x64), we will scale it
    g_texture = SDL_CreateTexture(
        g_renderer,
        SDL_PIXELFORMAT_ARGB8888,
        SDL_TEXTUREACCESS_STREAMING,
        CHIP8_HIGH_RES_WIDTH,
        CHIP8_HIGH_RES_HEIGHT
    );

    if (!g_texture) {
        SDL_Log("SDL_CreateTexture failed: %s", SDL_GetError());
        SDL_DestroyRenderer(g_renderer);
        SDL_DestroyWindow(g_window);
        SDL_Quit();
        return false;
    }

    // Set logical size for auto scaling (keeps aspect ratio with letterboxing)
    SDL_RenderSetLogicalSize(g_renderer,
        logical_width == 0 ? CHIP8_HIGH_RES_WIDTH : logical_width,
        logical_height == 0 ? CHIP8_HIGH_RES_HEIGHT : logical_height);

    return true;
}

static int map_key(SDL_Keycode key) {
    // PC keymap to CHIP-8 hex keypad:
    // 1 2 3 4    -> 1 2 3 C
    // Q W E R    -> 4 5 6 D
    // A S D F    -> 7 8 9 E
    // Z X C V    -> A 0 B F

    switch (key) {
    case SDLK_1: return 0x1;
    case SDLK_2: return 0x2;
    case SDLK_3: return 0x3;
    case SDLK_4: return 0xC;
    case SDLK_q: return 0x4;
    case SDLK_w: return 0x5;
    case SDLK_e: return 0x6;
    case SDLK_r: return 0xD;
    case SDLK_a: return 0x7;
    case SDLK_s: return 0x8;
    case SDLK_d: return 0x9;
    case SDLK_f: return 0xE;
    case SDLK_z: return 0xA;
    case SDLK_x: return 0x0;
    case SDLK_c: return 0xB;
    case SDLK_v: return 0xF;
    default:     return -1;
    }
}

void platform_handle_input(Chip8* c8, bool* quit) {
    SDL_Event e;
    while (SDL_PollEvent(&e)) {
        switch (e.type) {
        case SDL_QUIT:
            *quit = true;
            c8->running = false;
            break;
        case SDL_KEYDOWN:
            if (e.key.keysym.sym == SDLK_ESCAPE) {
                *quit = true;
                c8->running = false;
                break;
            }
            else {
                int mapped = map_key(e.key.keysym.sym);
                if (mapped >= 0) {
                    chip8_key_down(c8, (uint8_t)mapped);
                }
            }
            break;
        case SDL_KEYUP: {
            int mapped = map_key(e.key.keysym.sym);
            if (mapped >= 0) {
                chip8_key_up(c8, (uint8_t)mapped);
            }
            break;
        }
        default:
            break;
        }
    }
}

void platform_draw(const Chip8* c8) {
    int w, h;
    const bool* disp = chip8_get_display(c8, &w, &h);

    // We always update a 128x64 texture, using only w x h area
    uint32_t* pixels = NULL;
    int pitch = 0;
    if (SDL_LockTexture(g_texture, NULL, (void**)&pixels, &pitch) != 0) {
        SDL_Log("SDL_LockTexture failed: %s", SDL_GetError());
        return;
    }

    int tex_width = CHIP8_HIGH_RES_WIDTH;
    int tex_height = CHIP8_HIGH_RES_HEIGHT;

    // Clear entire texture to black
    int total = tex_width * tex_height;
    for (int i = 0; i < total; ++i) {
        pixels[i] = 0xFF000000; // ARGB: opaque black
    }

    // Draw pixels
    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            int idx_disp = y * CHIP8_HIGH_RES_WIDTH + x;
            if (disp[idx_disp]) {
                int idx_tex = y * tex_width + x;
                // ARGB: opaque greenish color (for "color" look; change as desired)
                pixels[idx_tex] = 0xFF00FF00;
            }
        }
    }

    SDL_UnlockTexture(g_texture);

    SDL_RenderClear(g_renderer);
    SDL_RenderCopy(g_renderer, g_texture, NULL, NULL);
    SDL_RenderPresent(g_renderer);
}

void platform_cleanup(void) {
    if (g_texture) {
        SDL_DestroyTexture(g_texture);
        g_texture = NULL;
    }
    if (g_renderer) {
        SDL_DestroyRenderer(g_renderer);
        g_renderer = NULL;
    }
    if (g_window) {
        SDL_DestroyWindow(g_window);
        g_window = NULL;
    }
    SDL_Quit();
}
