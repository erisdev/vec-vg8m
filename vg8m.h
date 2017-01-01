#pragma once
#include <stdbool.h>
#include <stdint.h>

#include "z80.h"

typedef struct s_vg8m VG8M;
typedef struct s_vg8m_hwregs VG8MRegisters;

typedef void (*VG8MCallback)(VG8M *emu, void *param);

struct s_vg8m_hwregs {
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

struct s_vg8m {
    Z80Context *cpu;

    uint8_t *system_rom;
    uint8_t *system_ram;
    uint8_t *system_charset;
    uint8_t *user_ram;
    uint8_t *cart_prog_rom;
    uint8_t *cart_2bpp_rom;
    uint8_t *cart_3bpp_rom;
    VG8MRegisters hwregs;

    VG8MCallback scanline_callback;
    void *scanline_param;

    VG8MCallback display_callback;
    void *display_param;

    VG8MCallback input_callback;
    void *input_param;

    int mode;
    int modeclock;
    int line;
};

typedef uint16_t VG8MButtonMask;
enum e_vg8m_button {
    VG8M_BUTTON_ALPHA = 0x0001,
    VG8M_BUTTON_BETA  = 0x0002,
    VG8M_BUTTON_GAMMA = 0x0004,
    VG8M_BUTTON_DELTA = 0x0008,

    VG8M_BUTTON_LT    = 0x0010,
    VG8M_BUTTON_RT    = 0x0020,

    VG8M_BUTTON_UP    = 0x0100,
    VG8M_BUTTON_DOWN  = 0x0200,
    VG8M_BUTTON_LEFT  = 0x0400,
    VG8M_BUTTON_RIGHT = 0x0800,
};

enum {
    VG8M_DISP_WIDTH = 256,
    VG8M_DISP_HEIGHT = 256,
};

const uint32_t *VG8M_HWPALETTE;
static const int VG8M_HWPALETTE_SIZE = 32;

void vg8m_init(VG8M *emu);
void vg8m_fin(VG8M *emu);

void vg8m_reset(VG8M *emu);

bool vg8m_load_system(VG8M *emu, const char *rom_filename, const char *charset_filename);
bool vg8m_load_cart(VG8M *emu, const char *filename);

void vg8m_set_buttons(VG8M *emu, VG8MButtonMask buttons, bool pressed);

uint8_t  vg8m_read8(VG8M *emu, uint16_t addr);
void     vg8m_write8(VG8M *emu, uint16_t addr, uint8_t data);
uint16_t vg8m_read16(VG8M *emu, uint16_t addr);
void     vg8m_write16(VG8M *emu, uint16_t addr, uint16_t data);

void vg8m_step_frame(VG8M *emu);
void vg8m_step_instruction(VG8M *emu);

void vg8m_dump_instruction(VG8M* emu, FILE *file);
void vg8m_dump_registers(VG8M *emu, FILE *file);

void vg8m_vblank(VG8M *emu);
void vg8m_hblank(VG8M *emu);

void vg8m_scanline(VG8M *emu, uint32_t *pixels);
