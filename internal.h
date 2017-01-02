#pragma once
#include "vg8m.h"

enum {
    SYS_ROM_ADDR  = 0x0000, SYS_ROM_SIZE  = 0x0800,
    SYS_RAM_ADDR  = 0x0800, SYS_RAM_SIZE  = 0x0700,
    HWREGS_ADDR   = 0x0F00, HWREGS_SIZE   = sizeof(OriginRegisters),
    CHAR_ROM_ADDR = 0x1000, CHAR_ROM_SIZE = 0x1000,
    USER_RAM_ADDR = 0x4000, USER_RAM_SIZE = 0x4000,
    CART_ROM_ADDR = 0x8000, CART_ROM_SIZE = 0x4000,
    CART_EXT_ADDR = 0xC000, CART_EXT_SIZE = 0x4000,

    CART_BG_SIZE  = 0x4000,
    CART_SPR_SIZE = 0x3000,
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

void origin_set_error(const char *fmt, ...) __attribute__ ((format (printf, 1, 2)));

static inline uint16_t _min(uint16_t a, uint16_t b) {
    return a < b ? a : b;
}
