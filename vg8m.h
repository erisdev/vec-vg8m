#pragma once
#include <stdbool.h>
#include <stdint.h>

#include "z80.h"

typedef struct s_origin Origin;
typedef struct s_origin_hwregs OriginRegisters;
typedef struct s_origin_membank OriginMemBank;
typedef struct s_origin_memslot OriginMemSlot;
typedef struct s_origin_cartridge OriginCart;

typedef void (*OriginCallback)(Origin *emu, void *param);

typedef uint8_t (*OriginMemRead)(OriginMemSlot *bank, uint16_t addr);
typedef void    (*OriginMemWrite)(OriginMemSlot *bank, uint16_t addr, uint8_t value);
typedef void    (*OriginMemFree)(void *data);

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

struct s_origin_membank {
    uint16_t size;
    uint8_t *bytes;
    bool writable;
};

struct s_origin_memslot {
    OriginMemRead read;
    OriginMemWrite write;

    uint16_t begin;
    uint16_t end;
    bool is_banked;

    union {
        struct {
            OriginMemBank *bank;
        } banked;

        struct {
            OriginMemFree free;

            uint16_t size;
            uint8_t *bytes;
        } fixed;
    };
};

struct s_origin_cartridge {
    OriginMemBank prog;
    OriginMemBank ext;
    OriginMemBank bg;
    OriginMemBank spr;
};

struct s_origin {
    Z80Context *cpu;

    union {
        OriginMemSlot slots[7];
        struct {
            OriginMemSlot system_rom;
            OriginMemSlot system_ram;
            OriginMemSlot hwregs;
            OriginMemSlot user_ram;
            OriginMemSlot cart_prog;
            OriginMemSlot cart_ext;
        };
    } memory;

    OriginMemSlot pat_txt;
    OriginMemSlot pat_bg;
    OriginMemSlot pat_spr;

    OriginCart *cart;
    OriginCart *system;

    OriginRegisters hwregs;

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

void origin_mem_fin(OriginMemSlot *slot);
uint8_t *origin_mem_bytes(OriginMemSlot *slot);
uint16_t origin_mem_size(OriginMemSlot *slot);

void origin_mem_init(OriginMemSlot *slot, uint16_t addr, uint16_t size, void *data, OriginMemFree free_func);
void origin_mem_init_ram(OriginMemSlot *slot, uint16_t addr, uint16_t size);
void origin_mem_init_rom(OriginMemSlot *slot, uint16_t addr, uint16_t size);
void origin_mem_init_banked(OriginMemSlot *slot, uint16_t addr, uint16_t size);

bool origin_mem_set_bank(OriginMemSlot *slot, OriginMemBank *bank);

void origin_bank_init(OriginMemBank *bank, uint16_t size, bool writable);
void origin_bank_fin(OriginMemBank *bank);

// CARTRIDGE

void origin_cart_init(OriginCart *cart);
void origin_cart_fin(OriginCart *cart);
bool origin_cart_load(OriginCart *cart, const char *filename);

bool origin_insert_cart(Origin *emu, OriginCart *cart);
bool origin_remove_cart(Origin *emu);

// ERROR REPORTING

const char *origin_error();
