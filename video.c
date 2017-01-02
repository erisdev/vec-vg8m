#include "config.h"
#include "internal.h"

#include <stdbool.h>
#include <memory.h>

static const uint32_t _colors[] = {
    /* dawnbringer's db32 palette <http://pixeljoint.com/forum/forum_posts.asp?TID=16247> */
    0xFF000000, // [00] Black
    0xFF222034, // [01] Valhalla
    0xFF45283C, // [02] Loulou
    0xFF663931, // [03] Oiled cedar
    0xFF8F563B, // [04] Rope
    0xFFDF7126, // [05] Tahiti gold
    0xFFD9A066, // [06] Twine
    0xFFEEC39A, // [07] Pancho
    0xFFFBF236, // [08] Golden fizz
    0xFF99E550, // [09] Atlantis
    0xFF6ABE30, // [0A] Christi
    0xFF37946E, // [0B] Elf green
    0xFF4B692F, // [0C] Dell
    0xFF524B24, // [0D] Verdigris
    0xFF323C39, // [0E] Opal
    0xFF3F3F74, // [0F] Deep koamaru
    0xFF306082, // [10] Venice blue
    0xFF5B6EE1, // [11] Royal blue
    0xFF639BFF, // [12] Cornflower
    0xFF5FCDE4, // [13] Viking
    0xFFCBDBFC, // [14] Light steel blue
    0xFFFFFFFF, // [15] White
    0xFF9BADB7, // [16] Heather
    0xFF847E87, // [17] Topaz
    0xFF696A6A, // [18] Dim gray
    0xFF595652, // [19] Smokey ash
    0xFF76428A, // [1A] Clairvoyant
    0xFFAC3232, // [1B] Brown
    0xFFD95763, // [1C] Mandy
    0xFFD77BBA, // [1D] Plum
    0xFF8F974A, // [1E] Rain forest
    0xFF8A6F30  // [1F] Stinger
};

const uint32_t *ORIGIN_HWPALETTE = _colors;

static const int SPRITE_LIMIT = 8;

struct sprite {
    uint8_t  vpos;
    uint8_t  hpos;
    uint8_t  vsize;
    uint8_t  hsize;
    uint16_t pat_name;
};

static inline uint8_t sample_1bpp(Origin *emu, uint16_t addr, bool flip_x, bool flip_y, uint8_t x, uint8_t y) {
    if (!flip_x) x = 7 - x;
    if ( flip_y) y = 7 - y;

    uint8_t b0 = origin_read8(emu, addr + y);

    return b0 >> x & 1;
}

static inline uint8_t sample_2bpp(Origin *emu, uint16_t addr, bool flip_x, bool flip_y, uint8_t x, uint8_t y) {
    if (!flip_x) x = 7 - x;
    if ( flip_y) y = 7 - y;

    uint8_t b0 = origin_read8(emu, addr + y * 2);
    uint8_t b1 = origin_read8(emu, addr + y * 2 + 1);

    return (b0 >> x & 1) | (b1 << 1 >> x & 2);
}

static inline uint8_t sample_3bpp(Origin *emu, uint16_t addr, bool flip_x, bool flip_y, uint8_t x, uint8_t y) {
    if (!flip_x) x = 7 - x;
    if ( flip_y) y = 7 - y;

    uint8_t b0 = origin_read8(emu, addr + y * 2);
    uint8_t b1 = origin_read8(emu, addr + y * 2 + 1);
    uint8_t b2 = origin_read8(emu, addr + y     + 16);

    return (b0 >> x & 1) | (b1 << 1 >> x & 2) | (b2 << 2 >> x & 4);
}

static inline void scan_map(Origin *emu, int y, uint32_t *pixels) {
    OriginRegisters *regs = &emu->hwregs;
    uint16_t sy = y + regs->map_vscroll;
    uint16_t ty = (sy >> 3) % regs->map_vsize;

    uint16_t name_base = regs->map_name_addr + ty * 2 * regs->map_hsize;
    uint16_t pat_base = emu->hwregs.map_pat_addr;

    for (int x = 0; x < ORIGIN_DISP_WIDTH; ++x) {
        uint16_t sx = x + regs->map_hscroll;
        uint16_t tx = (sx>> 3) % regs->map_hsize;
        uint16_t name = origin_read16(emu, name_base + tx * 2);

        bool flip_x   = name & 0x0800;
        bool flip_y   = name & 0x1000;
        uint16_t addr = pat_base + (name & 0x3FF) * 16;

        uint8_t p = name >> 13;
        uint8_t c = sample_2bpp(emu, addr, flip_x, flip_y, sx & 7, sy & 7);

        // pixels[x] = regs->palette[p][c];
        pixels[x] = ORIGIN_HWPALETTE[regs->palette[p][c] & 0x1F];
    }
}

static inline void scan_spr(Origin *emu, int y, uint32_t *pixels) {
    OriginRegisters *regs = &emu->hwregs;

    uint16_t pat_base = emu->hwregs.spr_pat_addr;

    struct sprite sprites[SPRITE_LIMIT];
    int count = 0;
    for (int i = 0; i < regs->spr_count; ++i) {
        uint16_t spr_base = regs->spr_addr + i * 4;
        uint8_t  vpos     = origin_read8(emu, spr_base);
        uint8_t  hpos     = origin_read8(emu, spr_base + 1);
        uint16_t pat_name = origin_read16(emu, spr_base + 2);

        bool tall = (pat_name & 0x0400);
        bool wide = (pat_name & 0x0200);

        uint8_t vsize = tall ? 16 : 8;
        if (y < vpos || y >= vpos + vsize) continue;

        sprites[count].vpos = vpos;
        sprites[count].hpos = hpos;
        sprites[count].vsize = vsize;
        sprites[count].hsize = wide ? 16 : 8;
        sprites[count].pat_name = pat_name;
        ++count;

        if (count == SPRITE_LIMIT) break;
    }

    if (count == 0)
        return;

    for (int x = 0; x < ORIGIN_DISP_WIDTH; ++x) {
        struct sprite *spr;
        for (int i = 0; i < count; ++i) {
            spr  = sprites + i;
            if (x >= spr->hpos && x < spr->hpos + spr->hsize) {
                bool flip_x   = spr->pat_name & 0x0800;
                bool flip_y   = spr->pat_name & 0x1000;
                uint16_t name = spr->pat_name & 0x1FF;

                uint16_t sy = y - spr->vpos;
                uint16_t sx = x - spr->hpos;

                if (!flip_y) sy = spr->vsize - sy;
                if (!flip_x) sx = spr->hsize - sx;

                uint16_t ty = (sy >> 3) % (spr->vsize >> 3);
                uint16_t tx = (sx >> 3) % (spr->hsize >> 3);

                uint16_t addr = pat_base + (name + ty * 16 + tx) * 24;

                uint8_t p = spr->pat_name >> 13;
                uint8_t c = sample_3bpp(emu, addr, flip_x, flip_y, sx & 7, sy & 7);

                if (!(sy & 7))
                    (void)0;

                pixels[x] = ORIGIN_HWPALETTE[regs->palette[p][c]];
            }
        }
    }
}

void scan_txt(Origin *emu, int y, uint32_t *pixels) {
    OriginRegisters *regs = &emu->hwregs;

    if (y <  regs->txt_vscroll) return;
    if (y >= regs->txt_vscroll + regs->txt_vsize * 8) return;

    uint16_t sy = y - regs->txt_vscroll;
    uint16_t ty = sy >> 3;

    uint16_t name_base = regs->txt_addr + ty * regs->txt_hsize;
    uint16_t pat_base = 0x1000;

    for (int x = 0; x < ORIGIN_DISP_WIDTH; ++x) {
        if (x <  regs->txt_hscroll) continue;
        if (x >= regs->txt_hscroll + regs->txt_hsize * 8) continue;

        uint16_t sx = x - regs->txt_hscroll;
        uint16_t tx = sx >> 3;
        uint16_t name = origin_read8(emu, name_base + tx);

        uint16_t addr = pat_base + name * 8;

        uint8_t c = sample_1bpp(emu, addr, false, false, sx & 7, sy & 7);

        // pixels[x] = regs->palette[p][c];
        if (c) pixels[x] = ORIGIN_HWPALETTE[regs->palette[0][c] & 0x1F];
    }
}

void origin_scanline(Origin *emu, uint32_t *pixels) {
    OriginRegisters *regs = &emu->hwregs;

    int y = emu->line;
    for (int x = 0; x < ORIGIN_DISP_HEIGHT; ++x)
        pixels[x] = ORIGIN_HWPALETTE[regs->palette[0][0]];

    if (regs->map_name_addr && regs->map_vsize && regs->map_hsize)
        scan_map(emu, y, pixels);
    if (regs->spr_addr && regs->spr_count)
        scan_spr(emu, y, pixels);
    if (regs->txt_addr && regs->txt_vsize && regs->txt_hsize)
        scan_txt(emu, y, pixels);
}
