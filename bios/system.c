#include <stdint.h>
#include <string.h>
#include <hwregs.h>

uint8_t reboot_mode;

extern void int_ignore(void);
extern uint8_t sin(uint8_t);

typedef void (*int_handler_t)(void);

struct cart_header {
    uint8_t  magic[8];
    uint8_t  title[0x20];
    uint8_t  developer[0x20];
    uint8_t  language[2];
    uint16_t flags;
    uint8_t  version;
    uint8_t  reserved[0xB3];
};

static void int_set(uint8_t intid, int_handler_t handler);
static void int_clear(uint8_t intid);

static void VBLANK(void) __interrupt(0x20);
static void HBLANK(void) __interrupt(0x22);

void system_main(void) __naked {
    // static struct cart_header __at(0x8000) cart;

    __asm
        ei
        jp      0x8100  ; jump into program
    __endasm;
}

/* setting & clearing interrupt handlers */

static int_handler_t __at(0x0E00) int_handlers[0x80];

static void int_set(uint8_t intid, int_handler_t handler) {
    int_handlers[intid >> 1] = handler;
}

static void int_clear(uint8_t intid) {
    int_handlers[intid >> 1] = int_ignore;
}
