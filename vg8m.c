#include "config.h"
#include "internal.h"

#include <fcntl.h>
#include <memory.h>
#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h>

static uint8_t _readio(VG8M *emu, uint16_t addr);
static void _writeio(VG8M *emu, uint16_t addr, uint8_t data);

void vg8m_init(VG8M *emu) {
    vg8m_rom_init(&emu->memory.system_rom,     SYS_ROM_ADDR,  SYS_ROM_SIZE);
    vg8m_ram_init(&emu->memory.system_ram,     SYS_RAM_ADDR,  SYS_RAM_SIZE);
    vg8m_rom_init(&emu->memory.system_charset, CHAR_ROM_ADDR, CHAR_ROM_SIZE);
    vg8m_ram_init(&emu->memory.user_ram,       USER_RAM_ADDR, USER_RAM_SIZE);
    vg8m_rom_init(&emu->memory.cart_prog,      CART_ROM_ADDR, CART_ROM_SIZE);
    vg8m_rom_init(&emu->cart_2bpp_rom,         0,             CART_2BPP_SIZE);
    vg8m_rom_init(&emu->cart_3bpp_rom,         0,             CART_3BPP_SIZE);

    vg8m_memory_init(&emu->memory.hwregs, HWREGS_ADDR, HWREGS_SIZE,
        &emu->hwregs, VG8M_MEM_READ_DEFAULT, VG8M_MEM_WRITE_DEFAULT, NULL);

    emu->cpu = calloc(1, sizeof(Z80Context));
    emu->cpu->ioParam = emu;
    emu->cpu->ioRead = (Z80DataIn)_readio;
    emu->cpu->ioWrite = (Z80DataOut)_writeio;

    emu->cpu->memParam = emu;
    emu->cpu->memRead = (Z80DataIn)vg8m_read8;
    emu->cpu->memWrite = (Z80DataOut)vg8m_write8;

    emu->mode = MODE_HBLANK;
    emu->modeclock = 0;
    emu->line = 0;
}

void vg8m_fin(VG8M *emu) {
    vg8m_memory_fin(&emu->memory.system_rom);
    vg8m_memory_fin(&emu->memory.system_ram);
    vg8m_memory_fin(&emu->memory.user_ram);
    vg8m_memory_fin(&emu->memory.cart_prog);
    vg8m_memory_fin(&emu->cart_2bpp_rom);
    vg8m_memory_fin(&emu->cart_3bpp_rom);

    free(emu->cpu);
}

void vg8m_reset(VG8M *emu) {
    Z80NMI(emu->cpu);
}

static bool _load_file(uint8_t *buffer, uint16_t size, const char *filename) {
    int fd = open(filename, O_RDONLY);
    if (fd == -1)
        goto error;

    if (read(fd, buffer, size) == -1)
        goto error;

    close(fd);
    return true;
error:
    if (fd != -1) close(fd);
    return false;
}

bool vg8m_load_system(VG8M *emu, const char *rom_filename, const char *charset_filename) {
    if (!_load_file(emu->memory.system_rom.data, SYS_ROM_SIZE, rom_filename))
        return false;

    if (!_load_file(emu->memory.system_charset.data, CHAR_ROM_SIZE, charset_filename))
        return false;

    return true;
}

void vg8m_set_buttons(VG8M *emu, VG8MButtonMask buttons, bool pressed) {
    if (pressed)
        emu->hwregs.buttons |= buttons;
    else
        emu->hwregs.buttons &= ~buttons;
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

void vg8m_dump_instruction(VG8M* emu, FILE *file) {
    char hexbuffer[256];
    char asmbuffer[256];
    Z80Debug(emu->cpu, hexbuffer, asmbuffer);
    fprintf(file, "%-8s; %s\n", hexbuffer, asmbuffer);
}

void vg8m_dump_registers(VG8M *emu, FILE *file) {
    Z80Context *cpu = emu->cpu;
    Z80Regs regs = cpu->R1;
    uint8_t flags = regs.br.F;

    fprintf(file, "\tAF %04X   BC %04X   DE %04X\n",
        regs.wr.AF, regs.wr.BC, regs.wr.DE);
    fprintf(file, "\tHL %04X   IX %04X   IY %04X\n",
        regs.wr.HL, regs.wr.IX, regs.wr.IY);
    fprintf(file, "\tPC %04x   SP %04X\n",
        emu->cpu->PC, regs.wr.SP);
    fprintf(file, "\tFLAGS %c%c%c%c%c%c\n",
        flags & F_C  ? 'C'  : '-',
        flags & F_N  ? 'N'  : '-',
        flags & F_PV ? 'V'  : '-',
        flags & F_H  ? 'H'  : '-',
        flags & F_Z  ? 'Z'  : '-',
        flags & F_S  ? 'S'  : '-');
    fprintf(file, "\tIM %d (%s)\n",
        emu->cpu->IM, (emu->cpu->IFF1 ? "enabled" : "disabled"));
    fprintf(file, "\n");
}

void vg8m_vblank(VG8M *emu) {
    Z80INT(emu->cpu, INT_VBLANK);
}

void vg8m_hblank(VG8M *emu) {
    Z80INT(emu->cpu, INT_HBLANK);
}

static uint8_t _readio(VG8M *emu, uint16_t addr) {
    return 0xFF;
}

static void _writeio(VG8M *emu, uint16_t addr, uint8_t data) {
    if ((addr & 0xFF) == 0xFF) {
        putc(data, stderr);
    }
}
