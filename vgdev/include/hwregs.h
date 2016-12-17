#pragma once
static char        __at(0x0F00) r_palettes[8][8];
static const void* __at(0x0F40) r_map_name_addr;
static const void* __at(0x0F42) r_map_pat_addr;
static char        __at(0x0F44) r_map_vsize;
static char        __at(0x0F45) r_map_hsize;
static short       __at(0x0F46) r_map_vscroll;
static short       __at(0x0F48) r_map_hscroll;
static const void* __at(0x0F4A) r_txt_addr;
static char        __at(0x0F4C) r_txt_vsize;
static char        __at(0x0F4D) r_txt_hsize;
static short       __at(0x0F4E) r_txt_vscroll;
static short       __at(0x0F50) r_txt_hscroll;
static const void* __at(0x0F52) r_spr_def_addr;
static const void* __at(0x0F54) r_spr_pat_addr;
static char        __at(0x0F56) r_spr_count;
