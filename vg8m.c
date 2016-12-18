#include "config.h"

#include <fcntl.h>
#include <memory.h>
#include <stdbool.h>
#include <stdlib.h>

#ifdef USE_MMAP
#include <sys/mman.h>
#else
#include <unistd.h>
#endif

#include "vg8m.h"
#include "video.h"

static uint8_t *_load_file(uint16_t size, const char *filename);
static void _unload_file(uint8_t *buffer, uint16_t size);

static uint8_t _readio(VG8M *emu, uint16_t addr);
static void _writeio(VG8M *emu, uint16_t addr, uint8_t data);

static uint8_t _readmem(VG8M *emu, uint16_t addr);
static void _writemem(VG8M *emu, uint16_t addr, uint8_t data);

enum {
    SYS_ROM_SIZE  = 0x0800,
    SYS_RAM_SIZE  = 0x0700,
    HWREGS_SIZE   = sizeof(VG8MRegisters),
    CHAR_ROM_SIZE = 0x1000,
    USER_RAM_SIZE = 0x4000,
    CART_ROM_SIZE = 0x8000,
};


enum {
    SYS_ROM_ADDR  = 0x0000, SYS_ROM_END  = 0x07FF,
    SYS_RAM_ADDR  = 0x0800, SYS_RAM_END  = 0x0EFF,
    HWREGS_ADDR   = 0x0F00, HWREGS_END   = HWREGS_ADDR + HWREGS_SIZE - 1,
    CHAR_ROM_ADDR = 0x1000, CHAR_ROM_END = 0x1FFF,
    USER_RAM_ADDR = 0x4000, USER_RAM_END = 0x7FFF,
    CART_ROM_ADDR = 0x8000, CART_ROM_END = 0xFFFF,
};

enum {
    INT_VBLANK = 0x20,
    INT_HBLANK = 0x22,
};

enum {
    MODE_HBLANK,
    MODE_VBLANK,
    MODE_DRAW,
};

enum {
    CYCLES_HBLANK = 200,
    CYCLES_VBLANK = 500,
    CYCLES_DRAW   = 200,
};

static inline uint16_t _min(uint16_t a, uint16_t b) {
    return a < b ? a : b;
}

void vg8m_init(VG8M *emu) {
    emu->system_ram     = calloc(SYS_RAM_SIZE, sizeof(uint8_t));
    emu->user_ram       = calloc(USER_RAM_SIZE, sizeof(uint8_t));

    // memcpy(emu->system_rom,     bios_system_bin,   bios_system_bin_len);
    // memcpy(emu->system_charset, bios_charset_1bpp, bios_charset_1bpp_len);
    // load_system_rom(emu, "bios/system.bin");
    // load_system_charset(emu, "bios/charset.dat");

    emu->cpu = calloc(1, sizeof(Z80Context));
    emu->cpu->ioParam = emu;
    emu->cpu->ioRead = (Z80DataIn)_readio;
    emu->cpu->ioWrite = (Z80DataOut)_writeio;

    emu->cpu->memParam = emu;
    emu->cpu->memRead = (Z80DataIn)_readmem;
    emu->cpu->memWrite = (Z80DataOut)_writemem;

    emu->mode = MODE_HBLANK;
    emu->modeclock = 0;
    emu->line = 0;
}

void vg8m_fin(VG8M *emu) {
    // free(emu->system_rom);
    free(emu->system_ram);
    free(emu->user_ram);
    free(emu->cpu);

    _unload_file(emu->system_rom, SYS_ROM_SIZE);
    _unload_file(emu->system_charset, CHAR_ROM_SIZE);
    _unload_file(emu->cartridge_rom, CART_ROM_SIZE);
}

static uint8_t *_load_file(uint16_t size, const char *filename) {
    uint8_t *buffer = NULL;

    int fd = open(filename, O_RDONLY);
    if (fd == -1) goto error;

#ifdef USE_MMAP
    buffer = mmap(NULL, size, PROT_READ, MAP_FILE | MAP_PRIVATE, fd, 0);
    if (!buffer) goto error;
#else
    buffer = calloc(size, 1);
    if (!buffer)
        goto error;
    if (read(fd, buffer, size) == -1)
        goto error;
#endif

    return buffer;
error:
    if (buffer) _unload_file(buffer, size);
    return NULL;
}

static void _unload_file(uint8_t *buffer, uint16_t size) {
#ifdef USE_MMAP
    if (buffer) munmap(buffer, size);
#else
    if (buffer) free(buffer);
#endif
}

bool vg8m_load_system(VG8M *emu, const char *rom_filename, const char *charset_filename) {
    uint8_t *rom_buffer = NULL, *charset_buffer = NULL;

    rom_buffer = _load_file(SYS_ROM_SIZE, rom_filename);
    if (!rom_buffer)
        goto error;

    charset_buffer = _load_file(CHAR_ROM_SIZE, charset_filename);
    if (!charset_buffer)
        goto error;

    emu->system_rom = rom_buffer;
    emu->system_charset = charset_buffer;

    return true;
error:
    _unload_file(rom_buffer, SYS_ROM_SIZE);
    _unload_file(charset_buffer, CHAR_ROM_SIZE);
    return false;
}

bool vg8m_load_cart(VG8M *emu, const char *filename) {
    uint8_t *buffer = NULL;

    buffer = _load_file(CART_ROM_SIZE, filename);
    if (!buffer)
        goto error;

    emu->cartridge_rom = buffer;

    return true;
error:
    _unload_file(buffer, CART_ROM_SIZE);
    return false;
}

void vg8m_set_buttons(VG8M *emu, VG8MButtonMask buttons, bool pressed) {
    if (pressed)
        emu->hwregs.buttons |= buttons;
    else
        emu->hwregs.buttons &= ~buttons;
}

uint8_t vg8m_read8(VG8M *emu, uint16_t addr) {
    return _readmem(emu, addr);
}

void vg8m_write8(VG8M *emu, uint16_t addr, uint8_t data) {
    _writemem(emu, addr, data);
}

uint16_t vg8m_read16(VG8M *emu, uint16_t addr) {
    return (_readmem(emu, addr + 1) << 8)
          | _readmem(emu, addr);
}

void vg8m_write16(VG8M *emu, uint16_t addr, uint16_t data) {
    _writemem(emu, addr,    data);
    _writemem(emu, addr+ 1, data >> 8);
}

void vg8m_step_frame(VG8M *emu) {
    for (;;) {
        vg8m_step_instruction(emu);

        // this is a hacky check to see if the screen has been drawn
        if (emu->mode == MODE_VBLANK && emu->modeclock == 0)
            break;
    }
}

void vg8m_step_instruction(VG8M *emu) {
    Z80Execute(emu->cpu);

    emu->modeclock += emu->cpu->tstates;
    emu->cpu->tstates = 0;

    switch (emu->mode) {
    case MODE_HBLANK:
        if (emu->modeclock > CYCLES_HBLANK) {
            emu->modeclock = 0;
            ++emu->line;

            if (emu->line == VG8M_DISP_HEIGHT) {
                emu->mode = MODE_VBLANK;

                if (emu->display_callback)
                    emu->display_callback(emu, emu->display_param);

                if (emu->input_callback)
                    emu->input_callback(emu, emu->input_param);

                vg8m_vblank(emu);
            }
            else {
                emu->mode = MODE_DRAW;
            }
        }
        break;
    case MODE_VBLANK:
        if (emu->modeclock > CYCLES_VBLANK) {
            emu->modeclock = 0;
            ++emu->line;

            if (emu->line == (int)(VG8M_DISP_HEIGHT * 3 / 2)) {
                emu->mode = MODE_HBLANK;
                emu->line = 0;
            }
        }
        break;
    case MODE_DRAW:
        if(emu->modeclock > CYCLES_DRAW) {
            emu->modeclock = 0;
            emu->mode = MODE_HBLANK;

            if (emu->scanline_callback)
                emu->scanline_callback(emu, emu->scanline_param);

            vg8m_hblank(emu);
        }
    }
}

void vg8m_vblank(VG8M *emu) {
    Z80INT(emu->cpu, INT_VBLANK);
}

void vg8m_hblank(VG8M *emu) {
    Z80INT(emu->cpu, INT_HBLANK);
}

static inline bool inregion(uint16_t addr, uint16_t begin, uint16_t end) {
    return addr >= begin && addr <= end;
}

static uint8_t _readio(VG8M *emu, uint16_t addr) {
    return 0xFF;
}

static void _writeio(VG8M *emu, uint16_t addr, uint8_t data) {
    if ((addr & 0xFF) == 0xFF) {
        fprintf(stderr, "%04x %04x %02x\n", emu->cpu->PC, addr, data);
    }
}

static uint8_t _readmem(VG8M *emu, uint16_t addr) {
    if      (inregion(addr, SYS_ROM_ADDR, SYS_ROM_END) && emu->system_rom)
        return emu->system_rom[addr - SYS_ROM_ADDR];

    else if (inregion(addr, SYS_RAM_ADDR, SYS_RAM_END))
        return emu->system_ram[addr - SYS_RAM_ADDR];

    else if (inregion(addr, HWREGS_ADDR, HWREGS_END))
        return ((uint8_t*)&(emu->hwregs))[addr - HWREGS_ADDR];

    else if (inregion(addr, CHAR_ROM_ADDR, CHAR_ROM_END) && emu->system_charset)
        return emu->system_charset[addr - CHAR_ROM_ADDR];

    else if (inregion(addr, USER_RAM_ADDR, USER_RAM_END))
        return emu->user_ram[addr - USER_RAM_ADDR];

    else if (inregion(addr, CART_ROM_ADDR, CART_ROM_END) && emu->cartridge_rom)
        return emu->cartridge_rom[addr - CART_ROM_ADDR];

    return 0xFF;
}

static void _writemem(VG8M *emu, uint16_t addr, uint8_t data) {
    if      (inregion(addr, SYS_RAM_ADDR, SYS_RAM_END))
        emu->system_ram[addr - SYS_RAM_ADDR] = data;

    else if (inregion(addr, HWREGS_ADDR, HWREGS_END))
        ((uint8_t*)&(emu->hwregs))[addr - HWREGS_ADDR] = data;

    else if (inregion(addr, USER_RAM_ADDR, USER_RAM_END))
        emu->user_ram[addr - USER_RAM_ADDR] = data;
}
