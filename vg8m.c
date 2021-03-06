#include "config.h"
#include "internal.h"

#include <fcntl.h>
#include <memory.h>
#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h>

static uint8_t _readio(Origin *emu, uint16_t addr);
static void _writeio(Origin *emu, uint16_t addr, uint8_t data);

void origin_init(Origin *emu) {
    origin_mem_init_banked(&emu->memory.system_rom,  SYS_ROM_ADDR,  SYS_ROM_SIZE);
    origin_mem_init_ram(&emu->memory.system_ram,     SYS_RAM_ADDR,  SYS_RAM_SIZE);
    origin_mem_init(&emu->memory.hwregs,             HWREGS_ADDR,   HWREGS_SIZE, &emu->hwregs, NULL);
    origin_mem_init_ram(&emu->memory.user_ram,       USER_RAM_ADDR, USER_RAM_SIZE);
    origin_mem_init_banked(&emu->memory.cart_prog,   CART_ROM_ADDR, CART_ROM_SIZE);
    origin_mem_init_banked(&emu->memory.cart_prog,   CART_ROM_ADDR, CART_ROM_SIZE);

    origin_mem_init_banked(&emu->pat_txt, 0, PAT_TXT_SIZE);
    origin_mem_init_banked(&emu->pat_bg,  0, PAT_BG_SIZE);
    origin_mem_init_banked(&emu->pat_spr, 0, PAT_SPR_SIZE);

    emu->cpu = calloc(1, sizeof(Z80Context));
    emu->cpu->ioParam = emu;
    emu->cpu->ioRead = (Z80DataIn)_readio;
    emu->cpu->ioWrite = (Z80DataOut)_writeio;

    emu->cpu->memParam = emu;
    emu->cpu->memRead = (Z80DataIn)origin_read8;
    emu->cpu->memWrite = (Z80DataOut)origin_write8;

    emu->mode = MODE_HBLANK;
    emu->modeclock = 0;
    emu->line = 0;
}

void origin_fin(Origin *emu) {
    origin_mem_fin(&emu->memory.system_rom);
    origin_mem_fin(&emu->memory.system_ram);
    origin_mem_fin(&emu->memory.hwregs);
    origin_mem_fin(&emu->memory.user_ram);
    origin_mem_fin(&emu->memory.cart_prog);
    origin_mem_fin(&emu->memory.cart_ext);

    origin_mem_fin(&emu->pat_txt);
    origin_mem_fin(&emu->pat_bg);
    origin_mem_fin(&emu->pat_spr);

    free(emu->cpu);
}

void origin_reset(Origin *emu) {
    origin_map_system_prog(emu);
    origin_map_system_vrom(emu);
    Z80NMI(emu->cpu);
}

bool origin_load_system(Origin *emu, const char *filename) {
    emu->system = calloc(1, sizeof(OriginCart));
    origin_cart_init(emu->system);

    if (!origin_cart_load(emu->system, filename))
        return false;

    origin_mem_set_bank(&emu->memory.system_rom, origin_cart_bank(emu->system, BANK_SLOT_BIOS, 0));
    origin_mem_set_bank(&emu->pat_txt,           origin_cart_bank(emu->system, BANK_SLOT_TXT,  0));
    origin_map_system_prog(emu);
    origin_map_system_vrom(emu);

    return true;
}

void origin_map_system_prog(Origin *emu) {
    origin_mem_set_bank(&emu->memory.cart_prog, origin_cart_bank(emu->system, BANK_SLOT_PROG, 0));
    origin_mem_set_bank(&emu->memory.cart_ext,  origin_cart_bank(emu->system, BANK_SLOT_EXT,  0));
}

void origin_map_system_vrom(Origin *emu) {
    origin_mem_set_bank(&emu->pat_bg,  origin_cart_bank(emu->system, BANK_SLOT_BG,  0));
    origin_mem_set_bank(&emu->pat_spr, origin_cart_bank(emu->system, BANK_SLOT_SPR, 0));
}

void origin_set_buttons(Origin *emu, OriginButtonMask buttons, bool pressed) {
    if (pressed)
        emu->hwregs.buttons |= buttons;
    else
        emu->hwregs.buttons &= ~buttons;
}

void origin_step_frame(Origin *emu) {
    for (;;) {
        origin_step_instruction(emu);

        // this is a hacky check to see if the screen has been drawn
        if (emu->mode == MODE_VBLANK && emu->modeclock == 0)
            break;
    }
}

void origin_step_instruction(Origin *emu) {
    Z80Execute(emu->cpu);

    emu->modeclock += emu->cpu->tstates;
    emu->cpu->tstates = 0;

    switch (emu->mode) {
    case MODE_HBLANK:
        if (emu->modeclock > CYCLES_HBLANK) {
            emu->modeclock = 0;
            ++emu->line;

            if (emu->line == ORIGIN_DISP_HEIGHT) {
                emu->mode = MODE_VBLANK;

                if (emu->display_callback)
                    emu->display_callback(emu, emu->display_param);

                if (emu->input_callback)
                    emu->input_callback(emu, emu->input_param);

                origin_vblank(emu);
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

            if (emu->line == (int)(ORIGIN_DISP_HEIGHT * 3 / 2)) {
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

            origin_hblank(emu);
        }
    }
}

void origin_dump_instruction(Origin* emu, FILE *file) {
    char hexbuffer[256];
    char asmbuffer[256];
    Z80Debug(emu->cpu, hexbuffer, asmbuffer);
    fprintf(file, "%-8s; %s\n", hexbuffer, asmbuffer);
}

void origin_dump_registers(Origin *emu, FILE *file) {
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

void origin_vblank(Origin *emu) {
    Z80INT(emu->cpu, INT_VBLANK);
}

void origin_hblank(Origin *emu) {
    Z80INT(emu->cpu, INT_HBLANK);
}

static uint8_t _readio(Origin *emu, uint16_t addr) {
    return 0xFF;
}

static void _writeio(Origin *emu, uint16_t addr, uint8_t data) {
    addr &= 0xFF;
    if (addr == 0x60) {
        if (data)
            origin_map_cart_prog(emu);
        else
            origin_map_system_prog(emu);
    }
    else if (addr == 0x61) {
        if (data)
            origin_map_cart_vrom(emu);
        else
            origin_map_system_vrom(emu);
    }
    else if (addr == 0xFF) {
        putc(data, stderr);
    }
}
