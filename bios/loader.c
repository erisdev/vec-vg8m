#include <stdint.h>
#include <string.h>
#include <hwregs.h>

#define EI __asm ei __endasm
#define DI __asm di __endasm
#define HALT __asm halt __endasm

extern void int_ignore(void);

typedef void (*int_handler_t)(void);
static int_handler_t __at(0x0E20) int_vblank;
static int_handler_t __at(0x0E22) int_hblank;

__sfr __at(0x60) map_cart_prog;
__sfr __at(0x61) map_cart_vrom;

struct cart_header {
    char  magic[8];
    char  title[0x20];
    char  developer[0x20];
    char  language[2];
    short flags;
    char  version;
    char  reserved0[0x33];
    short entry_point;
    char  reserved1[0x7E];
};

static void wait_frames(char n);
static void slow_print(char *dst, const char *src);

static void VBLANK(void) __interrupt(0x20);
// static void HBLANK(void) __interrupt(0x22);

static struct cart_header __at(0x8000) cart;

static char msg_buffer[96];

static const char msg_blank[]        = "                                ";
static const char msg_copyright[]    = "(C) 2187 VIDIUS ELECTRIC CORP   ";
static const char msg_loading[]      = "LOADING...                      ";
static const char msg_unrecognized[] = "UNRECOGNIZED CARTRIDGE          ";
static const char msg_bad_entry[]    = "BAD ENTRY POINT                 ";
static const char msg_bad_checksum[] = "BAD CHECKSUM                    ";

static const short logo[] = {
    0x0001, 0x0002, 0x0003, 0x0004, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
    0x000B, 0x0000, 0x0000, 0x000C, 0x0005, 0x0006, 0x0007, 0x0008, 0x0009, 0x000A,
    0x0000, 0x0000, 0x0000, 0x000D, 0x000F, 0x0010, 0x0011, 0x0012, 0x0013, 0x0014,
    0x0000, 0x0000, 0x000E, 0x1801, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
};

const char palette[] = {0x00, 0x13, 0x15};

const char fade_to_red[]   = {0x11, 0x0F, 0x02, 0x1A, 0x1C};
const char fade_to_black[] = {0x11, 0x0F, 0x02, 0x01, 0x00};

void system_main(void) __naked {
    int i;

    memcpy(r_palettes[0], palette, sizeof(palette));

    r_map_name_addr = logo;
    r_map_vsize     =  4;
    r_map_hsize     = 10;
    r_map_vscroll   =  0;
    r_map_hscroll   =  0;

    r_txt_addr    = msg_buffer;
    r_txt_vsize   =   3;
    r_txt_hsize   =  32;
    r_txt_vscroll = 240;
    r_txt_hscroll =   8;

    int_vblank = VBLANK;
    EI;

    slow_print(msg_buffer, msg_copyright);
    wait_frames(20);

    slow_print(msg_buffer, msg_loading);
    wait_frames(20);

    map_cart_prog = 1;

    if (strncmp(cart.magic, "VEC-VG8M", sizeof(cart.magic)) != 0) {
        slow_print(msg_buffer, msg_unrecognized);
        goto failure;
    }

    if (cart.entry_point < 0x8000) {
        slow_print(msg_buffer, msg_bad_entry);
        goto failure;
    }

    r_txt_vscroll -= 8;
    slow_print(msg_buffer + 32, cart.title);

    r_txt_vscroll -= 8;
    slow_print(msg_buffer + 64, cart.developer);

    slow_print(msg_buffer, msg_blank);
    wait_frames(60);

    for (i = 0; i < sizeof(fade_to_red); ++i) {
        r_palettes[0][1] = fade_to_black[i];
        wait_frames(4);
    }

    DI;
    int_vblank = int_ignore;
    map_cart_vrom = 1;

    __asm
        ld      hl,(0x8080)
        jp      (hl)
    __endasm;

failure:
    for (i = 0; i < sizeof(fade_to_red); ++i) {
        r_palettes[0][1] = fade_to_red[i];
        wait_frames(4);
    }

    for (;;) HALT;

}

volatile char frames_wait;
static void wait_frames(char n) {
    frames_wait = n;
    while (frames_wait) HALT;
}

static void slow_print(char *dst, const char *src) {
    int i;
    for (i = 0; i < 32; ++i) {
        dst[i] = src[i];
        wait_frames(1);
    }
}

static void VBLANK(void) __interrupt(0x20) {
    if (frames_wait) --frames_wait;
}
