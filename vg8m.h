#pragma once
#include "z80.h"

typedef struct s_vg8m VG8M;
typedef struct s_gpu_registers GPURegisters;

typedef void (*VG8MCallback)(VG8M *emu, void *param);

struct s_gpu_registers {
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
};

struct s_vg8m {
    Z80Context *cpu;

    uint8_t *system_rom;
    uint8_t *system_ram;
    uint8_t *system_charset;
    uint8_t *user_ram;
    uint8_t *cartridge_rom;
    GPURegisters gpu_registers;

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

void vg8m_init(VG8M *emu);
void vg8m_fin(VG8M *emu);

bool vg8m_load_system(VG8M *emu, const char *rom_filename, const char *charset_filename);
bool vg8m_load_cart(VG8M *emu, const char *filename);

uint8_t  vg8m_read8(VG8M *emu, uint16_t addr);
void     vg8m_write8(VG8M *emu, uint16_t addr, uint8_t data);
uint16_t vg8m_read16(VG8M *emu, uint16_t addr);
void     vg8m_write16(VG8M *emu, uint16_t addr, uint16_t data);

void vg8m_step_frame(VG8M *emu);
void vg8m_step_instruction(VG8M *emu);
void vg8m_vblank(VG8M *emu);
void vg8m_hblank(VG8M *emu);
