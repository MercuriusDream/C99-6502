#include "display.h"
#include "bus.h"
#include "memory.h"
#include <SDL.h>
#include <stdio.h>
#include <string.h>

// Apple II display constants
#define SCREEN_WIDTH 560
#define SCREEN_HEIGHT 384
#define TEXT_COLS 40
#define TEXT_ROWS 24
#define CHAR_WIDTH 14
#define CHAR_HEIGHT 16

// Apple II memory addresses
#define TEXT_PAGE1 0x0400
#define HIRES_PAGE1 0x2000

// Display state
static SDL_Window* window = NULL;
static SDL_Renderer* renderer = NULL;
static SDL_Texture* texture = NULL;
static uint32_t* pixels = NULL;

// Keyboard state
static MEM_WORD keyboard_data = 0x00;
static int keyboard_ready = 0;

// Apple II display mode state
static int text_mode = 1;      // 1 = text, 0 = graphics
static int mixed_mode = 0;     // 1 = mixed (graphics + 4 lines text)
static int hires_mode = 0;     // 1 = hi-res, 0 = lo-res
static int page2 = 0;          // 1 = page 2, 0 = page 1

// Simple 5x8 character ROM (subset of Apple II font)
static const uint8_t char_rom[128][8] = {
    // 0x00-0x1F: Control characters (shown as spaces)
    [0x00 ... 0x1F] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},

    // 0x20: Space
    [0x20] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
    [0x21] = {0x08, 0x08, 0x08, 0x08, 0x08, 0x00, 0x08, 0x00}, // !
    [0x22] = {0x14, 0x14, 0x14, 0x00, 0x00, 0x00, 0x00, 0x00}, // "
    [0x23] = {0x14, 0x3E, 0x14, 0x14, 0x3E, 0x14, 0x00, 0x00}, // #
    [0x24] = {0x08, 0x1E, 0x28, 0x1C, 0x0A, 0x3C, 0x08, 0x00}, // $
    [0x25] = {0x30, 0x32, 0x04, 0x08, 0x10, 0x26, 0x06, 0x00}, // %
    [0x26] = {0x10, 0x28, 0x28, 0x10, 0x2A, 0x24, 0x1A, 0x00}, // &
    [0x27] = {0x08, 0x08, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00}, // '
    [0x28] = {0x04, 0x08, 0x10, 0x10, 0x10, 0x08, 0x04, 0x00}, // (
    [0x29] = {0x10, 0x08, 0x04, 0x04, 0x04, 0x08, 0x10, 0x00}, // )
    [0x2A] = {0x00, 0x08, 0x2A, 0x1C, 0x2A, 0x08, 0x00, 0x00}, // *
    [0x2B] = {0x00, 0x08, 0x08, 0x3E, 0x08, 0x08, 0x00, 0x00}, // +
    [0x2C] = {0x00, 0x00, 0x00, 0x00, 0x08, 0x08, 0x10, 0x00}, // ,
    [0x2D] = {0x00, 0x00, 0x00, 0x3E, 0x00, 0x00, 0x00, 0x00}, // -
    [0x2E] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x08, 0x00}, // .
    [0x2F] = {0x00, 0x02, 0x04, 0x08, 0x10, 0x20, 0x00, 0x00}, // /

    // 0x30-0x39: Numbers
    [0x30] = {0x1C, 0x22, 0x26, 0x2A, 0x32, 0x22, 0x1C, 0x00}, // 0
    [0x31] = {0x08, 0x18, 0x08, 0x08, 0x08, 0x08, 0x1C, 0x00}, // 1
    [0x32] = {0x1C, 0x22, 0x02, 0x0C, 0x10, 0x20, 0x3E, 0x00}, // 2
    [0x33] = {0x3E, 0x02, 0x04, 0x0C, 0x02, 0x22, 0x1C, 0x00}, // 3
    [0x34] = {0x04, 0x0C, 0x14, 0x24, 0x3E, 0x04, 0x04, 0x00}, // 4
    [0x35] = {0x3E, 0x20, 0x3C, 0x02, 0x02, 0x22, 0x1C, 0x00}, // 5
    [0x36] = {0x0C, 0x10, 0x20, 0x3C, 0x22, 0x22, 0x1C, 0x00}, // 6
    [0x37] = {0x3E, 0x02, 0x04, 0x08, 0x10, 0x10, 0x10, 0x00}, // 7
    [0x38] = {0x1C, 0x22, 0x22, 0x1C, 0x22, 0x22, 0x1C, 0x00}, // 8
    [0x39] = {0x1C, 0x22, 0x22, 0x1E, 0x02, 0x04, 0x18, 0x00}, // 9

    [0x3A] = {0x00, 0x00, 0x08, 0x00, 0x00, 0x08, 0x00, 0x00}, // :
    [0x3B] = {0x00, 0x00, 0x08, 0x00, 0x08, 0x08, 0x10, 0x00}, // ;
    [0x3C] = {0x02, 0x04, 0x08, 0x10, 0x08, 0x04, 0x02, 0x00}, // <
    [0x3D] = {0x00, 0x00, 0x3E, 0x00, 0x3E, 0x00, 0x00, 0x00}, // =
    [0x3E] = {0x10, 0x08, 0x04, 0x02, 0x04, 0x08, 0x10, 0x00}, // >
    [0x3F] = {0x1C, 0x22, 0x04, 0x08, 0x08, 0x00, 0x08, 0x00}, // ?
    [0x40] = {0x1C, 0x22, 0x2A, 0x2E, 0x2C, 0x20, 0x1E, 0x00}, // @

    // 0x41-0x5A: Uppercase letters
    [0x41] = {0x08, 0x14, 0x22, 0x22, 0x3E, 0x22, 0x22, 0x00}, // A
    [0x42] = {0x3C, 0x22, 0x22, 0x3C, 0x22, 0x22, 0x3C, 0x00}, // B
    [0x43] = {0x1C, 0x22, 0x20, 0x20, 0x20, 0x22, 0x1C, 0x00}, // C
    [0x44] = {0x38, 0x24, 0x22, 0x22, 0x22, 0x24, 0x38, 0x00}, // D
    [0x45] = {0x3E, 0x20, 0x20, 0x3C, 0x20, 0x20, 0x3E, 0x00}, // E
    [0x46] = {0x3E, 0x20, 0x20, 0x3C, 0x20, 0x20, 0x20, 0x00}, // F
    [0x47] = {0x1C, 0x22, 0x20, 0x2E, 0x22, 0x22, 0x1E, 0x00}, // G
    [0x48] = {0x22, 0x22, 0x22, 0x3E, 0x22, 0x22, 0x22, 0x00}, // H
    [0x49] = {0x1C, 0x08, 0x08, 0x08, 0x08, 0x08, 0x1C, 0x00}, // I
    [0x4A] = {0x0E, 0x04, 0x04, 0x04, 0x04, 0x24, 0x18, 0x00}, // J
    [0x4B] = {0x22, 0x24, 0x28, 0x30, 0x28, 0x24, 0x22, 0x00}, // K
    [0x4C] = {0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x3E, 0x00}, // L
    [0x4D] = {0x22, 0x36, 0x2A, 0x2A, 0x22, 0x22, 0x22, 0x00}, // M
    [0x4E] = {0x22, 0x22, 0x32, 0x2A, 0x26, 0x22, 0x22, 0x00}, // N
    [0x4F] = {0x1C, 0x22, 0x22, 0x22, 0x22, 0x22, 0x1C, 0x00}, // O
    [0x50] = {0x3C, 0x22, 0x22, 0x3C, 0x20, 0x20, 0x20, 0x00}, // P
    [0x51] = {0x1C, 0x22, 0x22, 0x22, 0x2A, 0x24, 0x1A, 0x00}, // Q
    [0x52] = {0x3C, 0x22, 0x22, 0x3C, 0x28, 0x24, 0x22, 0x00}, // R
    [0x53] = {0x1C, 0x22, 0x20, 0x1C, 0x02, 0x22, 0x1C, 0x00}, // S
    [0x54] = {0x3E, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x00}, // T
    [0x55] = {0x22, 0x22, 0x22, 0x22, 0x22, 0x22, 0x1C, 0x00}, // U
    [0x56] = {0x22, 0x22, 0x22, 0x22, 0x22, 0x14, 0x08, 0x00}, // V
    [0x57] = {0x22, 0x22, 0x22, 0x2A, 0x2A, 0x36, 0x22, 0x00}, // W
    [0x58] = {0x22, 0x22, 0x14, 0x08, 0x14, 0x22, 0x22, 0x00}, // X
    [0x59] = {0x22, 0x22, 0x14, 0x08, 0x08, 0x08, 0x08, 0x00}, // Y
    [0x5A] = {0x3E, 0x02, 0x04, 0x08, 0x10, 0x20, 0x3E, 0x00}, // Z

    [0x5B] = {0x1C, 0x10, 0x10, 0x10, 0x10, 0x10, 0x1C, 0x00}, // [
    [0x5C] = {0x00, 0x20, 0x10, 0x08, 0x04, 0x02, 0x00, 0x00}, // backslash
    [0x5D] = {0x1C, 0x04, 0x04, 0x04, 0x04, 0x04, 0x1C, 0x00}, // ]
    [0x5E] = {0x08, 0x14, 0x22, 0x00, 0x00, 0x00, 0x00, 0x00}, // ^
    [0x5F] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x3E, 0x00}, // _
    [0x60] = {0x10, 0x08, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00}, // `

    // 0x61-0x7A: Lowercase letters (same as uppercase for simplicity)
    [0x61] = {0x00, 0x00, 0x1C, 0x02, 0x1E, 0x22, 0x1E, 0x00}, // a
    [0x62] = {0x20, 0x20, 0x2C, 0x32, 0x22, 0x22, 0x3C, 0x00}, // b
    [0x63] = {0x00, 0x00, 0x1C, 0x22, 0x20, 0x22, 0x1C, 0x00}, // c
    [0x64] = {0x02, 0x02, 0x1A, 0x26, 0x22, 0x22, 0x1E, 0x00}, // d
    [0x65] = {0x00, 0x00, 0x1C, 0x22, 0x3E, 0x20, 0x1C, 0x00}, // e
    [0x66] = {0x0C, 0x12, 0x10, 0x38, 0x10, 0x10, 0x10, 0x00}, // f
    [0x67] = {0x00, 0x00, 0x1E, 0x22, 0x22, 0x1E, 0x02, 0x1C}, // g
    [0x68] = {0x20, 0x20, 0x2C, 0x32, 0x22, 0x22, 0x22, 0x00}, // h
    [0x69] = {0x08, 0x00, 0x18, 0x08, 0x08, 0x08, 0x1C, 0x00}, // i
    [0x6A] = {0x04, 0x00, 0x0C, 0x04, 0x04, 0x04, 0x24, 0x18}, // j
    [0x6B] = {0x20, 0x20, 0x22, 0x24, 0x38, 0x24, 0x22, 0x00}, // k
    [0x6C] = {0x18, 0x08, 0x08, 0x08, 0x08, 0x08, 0x1C, 0x00}, // l
    [0x6D] = {0x00, 0x00, 0x34, 0x2A, 0x2A, 0x2A, 0x2A, 0x00}, // m
    [0x6E] = {0x00, 0x00, 0x2C, 0x32, 0x22, 0x22, 0x22, 0x00}, // n
    [0x6F] = {0x00, 0x00, 0x1C, 0x22, 0x22, 0x22, 0x1C, 0x00}, // o
    [0x70] = {0x00, 0x00, 0x3C, 0x22, 0x22, 0x3C, 0x20, 0x20}, // p
    [0x71] = {0x00, 0x00, 0x1E, 0x22, 0x22, 0x1E, 0x02, 0x02}, // q
    [0x72] = {0x00, 0x00, 0x2C, 0x32, 0x20, 0x20, 0x20, 0x00}, // r
    [0x73] = {0x00, 0x00, 0x1E, 0x20, 0x1C, 0x02, 0x3C, 0x00}, // s
    [0x74] = {0x10, 0x10, 0x38, 0x10, 0x10, 0x12, 0x0C, 0x00}, // t
    [0x75] = {0x00, 0x00, 0x22, 0x22, 0x22, 0x26, 0x1A, 0x00}, // u
    [0x76] = {0x00, 0x00, 0x22, 0x22, 0x22, 0x14, 0x08, 0x00}, // v
    [0x77] = {0x00, 0x00, 0x22, 0x22, 0x2A, 0x2A, 0x14, 0x00}, // w
    [0x78] = {0x00, 0x00, 0x22, 0x14, 0x08, 0x14, 0x22, 0x00}, // x
    [0x79] = {0x00, 0x00, 0x22, 0x22, 0x22, 0x1E, 0x02, 0x1C}, // y
    [0x7A] = {0x00, 0x00, 0x3E, 0x04, 0x08, 0x10, 0x3E, 0x00}, // z

    [0x7B] = {0x06, 0x08, 0x08, 0x10, 0x08, 0x08, 0x06, 0x00}, // {
    [0x7C] = {0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x00}, // |
    [0x7D] = {0x30, 0x08, 0x08, 0x04, 0x08, 0x08, 0x30, 0x00}, // }
    [0x7E] = {0x00, 0x00, 0x00, 0x18, 0x24, 0x00, 0x00, 0x00}, // ~
    [0x7F] = {0x3E, 0x3E, 0x3E, 0x3E, 0x3E, 0x3E, 0x3E, 0x00}, // DEL (block)
};

// Soft switch I/O handlers
static MEM_WORD display_softswitch_read(MEM_TWO_WORDS addr, void* ctx) {
    (void)ctx;

    switch (addr) {
        case 0xC050:
            if (text_mode != 0) {
                printf("[DISPLAY] Switched to GRAPHICS mode\n");
                text_mode = 0;
            }
            break;
        case 0xC051:
            if (text_mode != 1) {
                printf("[DISPLAY] Switched to TEXT mode\n");
                text_mode = 1;
            }
            break;
        case 0xC052:
            if (mixed_mode != 1) {
                printf("[DISPLAY] MIXED mode ON\n");
                mixed_mode = 1;
            }
            break;
        case 0xC053:
            if (mixed_mode != 0) {
                printf("[DISPLAY] MIXED mode OFF\n");
                mixed_mode = 0;
            }
            break;
        case 0xC054: page2 = 0; break;              // PAGE1
        case 0xC055: page2 = 1; break;              // PAGE2
        case 0xC056:
            if (hires_mode != 0) {
                printf("[DISPLAY] Switched to LORES\n");
                hires_mode = 0;
            }
            break;
        case 0xC057:
            if (hires_mode != 1) {
                printf("[DISPLAY] Switched to HIRES\n");
                hires_mode = 1;
            }
            break;
    }

    return 0x00;
}

static void display_softswitch_write(MEM_TWO_WORDS addr, MEM_WORD data, void* ctx) {
    (void)data;
    display_softswitch_read(addr, ctx);  // Same behavior for writes
}

int display_init(void) {
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        fprintf(stderr, "SDL_Init failed: %s\n", SDL_GetError());
        return -1;
    }

    window = SDL_CreateWindow(
        "Apple II - C99-6502 Emulator",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        SCREEN_WIDTH,
        SCREEN_HEIGHT,
        SDL_WINDOW_SHOWN
    );

    if (!window) {
        fprintf(stderr, "SDL_CreateWindow failed: %s\n", SDL_GetError());
        SDL_Quit();
        return -1;
    }

    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (!renderer) {
        fprintf(stderr, "SDL_CreateRenderer failed: %s\n", SDL_GetError());
        SDL_DestroyWindow(window);
        SDL_Quit();
        return -1;
    }

    texture = SDL_CreateTexture(
        renderer,
        SDL_PIXELFORMAT_ARGB8888,
        SDL_TEXTUREACCESS_STREAMING,
        SCREEN_WIDTH,
        SCREEN_HEIGHT
    );

    if (!texture) {
        fprintf(stderr, "SDL_CreateTexture failed: %s\n", SDL_GetError());
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return -1;
    }

    pixels = malloc(SCREEN_WIDTH * SCREEN_HEIGHT * sizeof(uint32_t));
    if (!pixels) {
        fprintf(stderr, "Failed to allocate pixel buffer\n");
        SDL_DestroyTexture(texture);
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return -1;
    }

    // Register soft switches for display mode control
    mem_region_add_io(0xC050, 8, display_softswitch_read, display_softswitch_write, NULL);

    printf("SDL display initialized (%dx%d)\n", SCREEN_WIDTH, SCREEN_HEIGHT);
    return 0;
}

void display_cleanup(void) {
    if (pixels) {
        free(pixels);
        pixels = NULL;
    }
    if (texture) {
        SDL_DestroyTexture(texture);
        texture = NULL;
    }
    if (renderer) {
        SDL_DestroyRenderer(renderer);
        renderer = NULL;
    }
    if (window) {
        SDL_DestroyWindow(window);
        window = NULL;
    }
    SDL_Quit();
}

// Render Apple II text mode (40x24 characters)
static void render_text_mode(void) {
    uint32_t color_white = 0xFF00FF00;  // Green phosphor (authentic Apple II)
    uint32_t color_black = 0xFF000000;

    // Clear screen
    memset(pixels, 0, SCREEN_WIDTH * SCREEN_HEIGHT * sizeof(uint32_t));

    for (int row = 0; row < TEXT_ROWS; row++) {
        for (int col = 0; col < TEXT_COLS; col++) {
            // Apple II text screen has unusual row addressing
            int base = (row / 8) * 0x28 + (row % 8) * 0x80;
            MEM_WORD ch = bus_read(TEXT_PAGE1 + base + col);

            // Handle Apple II character encoding
            // $00-$3F: Inverse
            // $40-$7F: Flash (treat as normal for now)
            // $80-$FF: Normal
            int inverse = (ch < 0x80);

            // Convert to ASCII
            if (ch >= 0xC0) {
                // $C0-$FF: Normal uppercase + symbols
                ch = ch & 0x7F;
            } else if (ch >= 0xA0) {
                // $A0-$BF: Normal punctuation/numbers
                ch = ch & 0x7F;
            } else if (ch >= 0x80) {
                // $80-$9F: Normal uppercase
                ch = ch & 0x7F;
            } else if (ch >= 0x40) {
                // $40-$7F: Flash (treat as normal)
                ch = ch & 0x7F;
                inverse = 0;
            } else {
                // $00-$3F: Inverse
                ch = ch + 0x40;
            }

            // Get character bitmap
            const uint8_t* glyph = char_rom[ch & 0x7F];

            // Render character (doubled in size: 5x8 -> 14x16)
            for (int cy = 0; cy < 8; cy++) {
                uint8_t row_data = glyph[cy];
                for (int cx = 0; cx < 7; cx++) {
                    int pixel_on = (row_data & (1 << (6 - cx))) != 0;
                    if (inverse) pixel_on = !pixel_on;

                    uint32_t color = pixel_on ? color_white : color_black;

                    // Double the pixels for proper sizing
                    int screen_x = col * CHAR_WIDTH + cx * 2;
                    int screen_y = row * CHAR_HEIGHT + cy * 2;

                    if (screen_x < SCREEN_WIDTH - 1 && screen_y < SCREEN_HEIGHT - 1) {
                        pixels[screen_y * SCREEN_WIDTH + screen_x] = color;
                        pixels[screen_y * SCREEN_WIDTH + screen_x + 1] = color;
                        pixels[(screen_y + 1) * SCREEN_WIDTH + screen_x] = color;
                        pixels[(screen_y + 1) * SCREEN_WIDTH + screen_x + 1] = color;
                    }
                }
            }
        }
    }
}

// Get hi-res screen address (handles interleaved scanlines)
static uint16_t hires_addr(int y) {
    int block = y / 64;          // 0, 1, or 2
    int line = y % 64;
    int group = line / 8;
    int row = line % 8;

    return HIRES_PAGE1 + (block * 0x28) + (row * 0x400) + (group * 0x80);
}

// Render Apple II hi-res mode (280x192, 7 pixels per byte)
static void render_hires_mode(void) {
    uint32_t color_white = 0xFF00FF00;  // Green phosphor
    uint32_t color_black = 0xFF000000;

    // Clear screen
    memset(pixels, 0, SCREEN_WIDTH * SCREEN_HEIGHT * sizeof(uint32_t));

    for (int y = 0; y < 192; y++) {
        uint16_t addr = hires_addr(y);

        for (int x_byte = 0; x_byte < 40; x_byte++) {
            MEM_WORD byte = bus_read(addr + x_byte);

            // Each byte represents 7 pixels (bit 7 is used for color on real Apple II)
            for (int bit = 0; bit < 7; bit++) {
                int pixel_on = (byte & (1 << bit)) != 0;
                uint32_t color = pixel_on ? color_white : color_black;

                // Each pixel is doubled (280 -> 560, 192 -> 384)
                int screen_x = (x_byte * 7 + bit) * 2;
                int screen_y = y * 2;

                pixels[screen_y * SCREEN_WIDTH + screen_x] = color;
                pixels[screen_y * SCREEN_WIDTH + screen_x + 1] = color;
                pixels[(screen_y + 1) * SCREEN_WIDTH + screen_x] = color;
                pixels[(screen_y + 1) * SCREEN_WIDTH + screen_x + 1] = color;
            }
        }
    }

    // If mixed mode, render bottom 4 lines as text
    if (mixed_mode) {
        uint32_t color_white = 0xFF00FF00;
        uint32_t color_black = 0xFF000000;

        for (int row = 20; row < TEXT_ROWS; row++) {
            for (int col = 0; col < TEXT_COLS; col++) {
                int base = (row / 8) * 0x28 + (row % 8) * 0x80;
                MEM_WORD ch = bus_read(TEXT_PAGE1 + base + col);

                int inverse = (ch & 0x80) == 0;
                ch = ch & 0x7F;
                if (ch < 0x20) ch += 0x40;

                const uint8_t* glyph = char_rom[ch];

                for (int cy = 0; cy < 8; cy++) {
                    uint8_t row_data = glyph[cy];
                    for (int cx = 0; cx < 7; cx++) {
                        int pixel_on = (row_data & (1 << (6 - cx))) != 0;
                        if (inverse) pixel_on = !pixel_on;

                        uint32_t color = pixel_on ? color_white : color_black;

                        int screen_x = col * CHAR_WIDTH + cx * 2;
                        int screen_y = row * CHAR_HEIGHT + cy * 2;

                        if (screen_x < SCREEN_WIDTH - 1 && screen_y < SCREEN_HEIGHT - 1) {
                            pixels[screen_y * SCREEN_WIDTH + screen_x] = color;
                            pixels[screen_y * SCREEN_WIDTH + screen_x + 1] = color;
                            pixels[(screen_y + 1) * SCREEN_WIDTH + screen_x] = color;
                            pixels[(screen_y + 1) * SCREEN_WIDTH + screen_x + 1] = color;
                        }
                    }
                }
            }
        }
    }
}

void display_refresh(void) {
    if (!pixels || !texture || !renderer) return;

    // Render based on current display mode
    if (text_mode) {
        render_text_mode();
    } else {
        render_hires_mode();
    }

    // Update texture and render
    SDL_UpdateTexture(texture, NULL, pixels, SCREEN_WIDTH * sizeof(uint32_t));
    SDL_RenderClear(renderer);
    SDL_RenderCopy(renderer, texture, NULL, NULL);
    SDL_RenderPresent(renderer);
}

int display_handle_events(void) {
    SDL_Event event;

    while (SDL_PollEvent(&event)) {
        switch (event.type) {
            case SDL_QUIT:
                return 1;  // Signal to quit

            case SDL_KEYDOWN:
                if (event.key.keysym.sym == SDLK_ESCAPE) {
                    return 1;  // ESC to quit
                }

                // Convert SDL key to Apple II key
                SDL_Keycode key = event.key.keysym.sym;

                if (key >= SDLK_a && key <= SDLK_z) {
                    // Convert to uppercase and set high bit
                    keyboard_data = (key - SDLK_a + 'A') | 0x80;
                    keyboard_ready = 1;
                    printf("[KEYBOARD] Key pressed: %c (code $%02X)\n", (key - SDLK_a + 'A'), keyboard_data);
                } else if (key >= SDLK_0 && key <= SDLK_9) {
                    keyboard_data = (key - SDLK_0 + '0') | 0x80;
                    keyboard_ready = 1;
                    printf("[KEYBOARD] Key pressed: %c (code $%02X)\n", (key - SDLK_0 + '0'), keyboard_data);
                } else if (key == SDLK_SPACE) {
                    keyboard_data = ' ' | 0x80;
                    keyboard_ready = 1;
                    printf("[KEYBOARD] Key pressed: SPACE (code $%02X)\n", keyboard_data);
                } else if (key == SDLK_RETURN) {
                    keyboard_data = 0x8D;  // CR with high bit
                    keyboard_ready = 1;
                    printf("[KEYBOARD] Key pressed: RETURN (code $%02X)\n", keyboard_data);
                } else if (key == SDLK_LEFT) {
                    keyboard_data = 0x88;  // Left arrow
                    keyboard_ready = 1;
                    printf("[KEYBOARD] Key pressed: LEFT (code $%02X)\n", keyboard_data);
                } else if (key == SDLK_RIGHT) {
                    keyboard_data = 0x95;  // Right arrow
                    keyboard_ready = 1;
                    printf("[KEYBOARD] Key pressed: RIGHT (code $%02X)\n", keyboard_data);
                }
                break;
        }
    }

    return 0;  // Continue running
}

MEM_WORD display_get_keyboard(void) {
    // Return keyboard data with bit 7 set if key ready
    // Reading this does NOT clear the strobe
    return keyboard_ready ? keyboard_data : 0x00;
}

void display_clear_keyboard_strobe(void) {
    // Clear the keyboard strobe
    keyboard_ready = 0;
    // Also clear bit 7 of keyboard_data to indicate key is no longer pressed
    keyboard_data &= 0x7F;
}
