#pragma once
#include <stdbool.h>
#include <stdint.h>

#include "z80.h"

typedef struct s_origin Origin;
typedef struct s_origin_hwregs OriginRegisters;
typedef struct s_origin_memory OriginMemory;

typedef void (*OriginCallback)(Origin *emu, void *param);

typedef uint8_t (*OriginMemRead)(OriginMemory *bank, uint16_t addr);
typedef void    (*OriginMemWrite)(OriginMemory *bank, uint16_t addr, uint8_t value);
typedef void    (*OriginMemFree)(OriginMemory *bank);

struct s_origin_hwregs {
    /* gpu */
    uint8_t  palette[8][8];
    uint16_t map_name_addr;
    uint16_t map_pat_addr;
    uint8_t  map_vsize;
    uint8_t  map_hsize;
    uint16_t map_vscroll;
    uint16_t map_hscroll;
    uint16_t txt_addr;
    uint8_t  txt_vsize;
    uint8_t  txt_hsize;
    uint16_t txt_vscroll;
    uint16_t txt_hscroll;
    uint16_t spr_addr;
    uint16_t spr_pat_addr;
    uint8_t  spr_count;
    uint8_t  _empty0;

    /* inputs */
    uint16_t buttons;
};

struct s_origin_memory {
    uint16_t begin;
    uint16_t end;
    uint16_t size;

    void *data;

    OriginMemRead read;
    OriginMemWrite write;
    OriginMemFree free;
};

struct s_origin {
    Z80Context *cpu;

    union {
        OriginMemory banks[6];
        struct {
            OriginMemory system_rom;
            OriginMemory system_ram;
            OriginMemory system_charset;
            OriginMemory hwregs;
            OriginMemory user_ram;
            OriginMemory cart_prog;
        };
    } memory;

    OriginRegisters hwregs;
    OriginMemory cart_2bpp_rom;
    OriginMemory cart_3bpp_rom;

    OriginCallback scanline_callback;
    void *scanline_param;

    OriginCallback display_callback;
    void *display_param;

    OriginCallback input_callback;
    void *input_param;

    int mode;
    int modeclock;
    int line;
};

typedef uint16_t OriginButtonMask;
enum e_vg8m_button {
    ORIGIN_BUTTON_ALPHA = 0x0001,
    ORIGIN_BUTTON_BETA  = 0x0002,
    ORIGIN_BUTTON_GAMMA = 0x0004,
    ORIGIN_BUTTON_DELTA = 0x0008,

    ORIGIN_BUTTON_LT    = 0x0010,
    ORIGIN_BUTTON_RT    = 0x0020,

    ORIGIN_BUTTON_UP    = 0x0100,
    ORIGIN_BUTTON_DOWN  = 0x0200,
    ORIGIN_BUTTON_LEFT  = 0x0400,
    ORIGIN_BUTTON_RIGHT = 0x0800,
};

enum {
    ORIGIN_DISP_WIDTH = 256,
    ORIGIN_DISP_HEIGHT = 256,
};

const uint32_t *ORIGIN_HWPALETTE;
static const int ORIGIN_HWPALETTE_SIZE = 32;

void origin_init(Origin *emu);
void origin_fin(Origin *emu);

void origin_reset(Origin *emu);

bool origin_load_system(Origin *emu, const char *rom_filename, const char *charset_filename);
bool origin_load_cart(Origin *emu, const char *filename);

void origin_set_buttons(Origin *emu, OriginButtonMask buttons, bool pressed);

uint8_t  origin_read8(Origin *emu, uint16_t addr);
void     origin_write8(Origin *emu, uint16_t addr, uint8_t data);
uint16_t origin_read16(Origin *emu, uint16_t addr);
void     origin_write16(Origin *emu, uint16_t addr, uint16_t data);

void origin_step_frame(Origin *emu);
void origin_step_instruction(Origin *emu);

void origin_dump_instruction(Origin* emu, FILE *file);
void origin_dump_registers(Origin *emu, FILE *file);

void origin_vblank(Origin *emu);
void origin_hblank(Origin *emu);

void origin_scanline(Origin *emu, uint32_t *pixels);

// MEMORY

void origin_memory_init(OriginMemory *bank, uint16_t addr, uint16_t size, void *data, OriginMemRead read_func, OriginMemWrite write_func, OriginMemFree free_func);
void origin_memory_fin(OriginMemory *bank);
void origin_ram_init(OriginMemory *bank, uint16_t begin, uint16_t size);
void origin_rom_init(OriginMemory *bank, uint16_t begin, uint16_t size);

uint8_t ORIGIN_MEM_READ_DEFAULT(OriginMemory *bank, uint16_t addr);
void ORIGIN_MEM_WRITE_DEFAULT(OriginMemory *bank, uint16_t addr, uint8_t data);
static void ORIGIN_MEM_FREE_DEFAULT(OriginMemory *bank);
