#include "config.h"
#include "internal.h"

#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <sys/errno.h>
#include <unistd.h>


static const char MAGIC[] = "VGDMP100";

enum {
    BANK_SLOT_PROG = 0x00,
    BANK_SLOT_EXT  = 0x01,
    BANK_SLOT_BG   = 0x02,
    BANK_SLOT_SPR  = 0x03,
};

typedef struct s_dump_header DumpHeader;
struct s_dump_header {
    char magic[8];
    uint16_t flags;
    uint16_t num_banks;
};

typedef struct s_bank_header BankHeader;
struct s_bank_header {
    uint16_t size;
    uint16_t flags;
    uint8_t  slot;
    uint8_t  id;

    uint16_t _pad;
};

void origin_cart_init(OriginCart *cart) {
    origin_bank_init(&cart->prog, CART_ROM_SIZE, false);
    origin_bank_init(&cart->ext,  CART_EXT_SIZE, false);
    origin_bank_init(&cart->bg,   PAT_BG_SIZE,   false);
    origin_bank_init(&cart->spr,  PAT_SPR_SIZE,  false);
}

void origin_cart_fin(OriginCart *cart) {
    origin_bank_fin(&cart->prog);
    origin_bank_fin(&cart->ext);
    origin_bank_fin(&cart->bg);
    origin_bank_fin(&cart->spr);
}

bool origin_cart_load(OriginCart *cart, const char *filename) {
    bool status = true;

    int fd = open(filename, O_RDONLY);
    if (fd == -1) goto libc_error;

    // read the file header first
    DumpHeader header;
    if (read(fd, &header, sizeof(header)) == -1)
        goto libc_error;

    // check the magic number
    if (strncmp(header.magic, MAGIC, 8) != 0) {
        origin_set_error("bad magic number %s", header.magic);
        goto format_error;
    }

    while (header.num_banks--) {
        BankHeader bank_header;
        if (read(fd, &bank_header, sizeof(bank_header)) == -1)
            goto libc_error;

        // check the bank slot
        OriginMemBank *bank;
        switch (bank_header.slot) {
        case BANK_SLOT_PROG: bank = &cart->prog; break;
        case BANK_SLOT_EXT:  bank = &cart->ext;  break;
        case BANK_SLOT_BG:   bank = &cart->bg;   break;
        case BANK_SLOT_SPR:  bank = &cart->spr;  break;
        default:
            origin_set_error("bad bank slot %02x", bank_header.slot);
            goto format_error;
        }

        // bank switching isn't actually supported yet
        if (bank_header.id != 0) {
            origin_set_error("bank switching is not supported");
            goto format_error;
        }

        // neither are flags lol
        if (bank_header.flags != 0) {
            origin_set_error("bad flags %04x", bank_header.flags);
            goto format_error;
        }

        // read bank data if everything looks ok
        int len = _min(bank->size, bank_header.size);
        if (read(fd, bank->bytes, len) == -1)
            goto libc_error;
    }

cleanup:
    if (fd != -1) close(fd);
    return status;

libc_error:
    origin_set_error("%s", strerror(errno));
    status = false;
    goto cleanup;

format_error:
    status = false;
    goto cleanup;
}

bool origin_insert_cart(Origin *emu, OriginCart *cart) {
    if (!emu->cart) {
        emu->cart = cart;
        origin_mem_set_bank(&emu->memory.cart_prog, &emu->cart->prog);
        origin_mem_set_bank(&emu->memory.cart_ext,  &emu->cart->ext);
        return true;
    }
    return false;
}

bool origin_remove_cart(Origin *emu) {
    if (emu->cart) {
        origin_mem_set_bank(&emu->memory.cart_prog, NULL);
        origin_mem_set_bank(&emu->memory.cart_ext,  NULL);
        origin_mem_set_bank(&emu->pat_bg,  &emu->system->bg);
        origin_mem_set_bank(&emu->pat_spr, &emu->system->spr);
        emu->cart = NULL;
        return true;
    }
    return false;
}
