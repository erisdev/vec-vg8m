#pragma once
#include "vg8m.h"

enum {
    VG8M_DISP_WIDTH = 256,
    VG8M_DISP_HEIGHT = 256,
};

const uint32_t *VG8M_HWPALETTE;
static const int VG8M_HWPALETTE_SIZE = 32;

void vg8m_scanline(VG8M *emu, uint32_t *pixels);
