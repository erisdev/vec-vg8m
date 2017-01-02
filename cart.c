#include "config.h"
#include "internal.h"

#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <sys/errno.h>
#include <unistd.h>


static const char MAGIC[] = "VGDMP100";

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
    cart->capacity = 4; // a good guess lol
    cart->num_banks = 0;
    cart->banks = calloc(cart->capacity, sizeof(OriginCartBank));
}

void origin_cart_fin(OriginCart *cart) {
    if (cart->banks) {
        for (int i = 0; i < cart->num_banks; ++i)
            origin_bank_fin(&cart->banks[i].bank);
        free(cart->banks);
    }
}

int _cmp_bank(const void *ptr_a, const void *ptr_b) {
    const OriginCartBank *a = ptr_a;
    const OriginCartBank *b = ptr_b;
    return ((a->slot << 8) | a->id) - ((b->slot << 8) | b->id);
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

        // grow the cartridge bank list if needed...
        ++cart->num_banks;
        if (cart->num_banks > cart->capacity) {
            cart->capacity *= 2;
            cart->banks = realloc(cart->banks, cart->capacity * sizeof(OriginCartBank));
        }

        // initialize the bank & read the data in
        OriginCartBank *entry = &cart->banks[cart->num_banks - 1];
        origin_bank_init(&entry->bank, bank_header.size, false);
        entry->slot = bank_header.slot;
        entry->id   = bank_header.id;

        if (read(fd, entry->bank.bytes, bank_header.size) == -1)
            goto libc_error;
    }

cleanup:
    // make sure all the banks are sorted regardless of return status
    qsort(cart->banks, cart->num_banks, sizeof(OriginCartBank), _cmp_bank);

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

OriginMemBank *origin_cart_bank(OriginCart *cart, uint8_t slot, uint8_t id) {
    OriginCartBank key = {.slot = slot, .id = id};
    OriginCartBank *entry = bsearch(&key, cart->banks, cart->num_banks, sizeof(OriginCartBank), _cmp_bank);
    return entry ? &entry->bank : NULL;
}

bool origin_insert_cart(Origin *emu, OriginCart *cart) {
    if (!emu->cart) {
        emu->cart = cart;
        origin_mem_set_bank(&emu->memory.cart_prog, origin_cart_bank(cart, BANK_SLOT_PROG, 0));
        origin_mem_set_bank(&emu->memory.cart_ext,  origin_cart_bank(cart, BANK_SLOT_EXT,  0));
        return true;
    }
    return false;
}

bool origin_remove_cart(Origin *emu) {
    if (emu->cart) {
        origin_mem_set_bank(&emu->memory.cart_prog, NULL);
        origin_mem_set_bank(&emu->memory.cart_ext,  NULL);
        origin_mem_set_bank(&emu->pat_bg,  origin_cart_bank(emu->system, BANK_SLOT_BG,  0));
        origin_mem_set_bank(&emu->pat_spr, origin_cart_bank(emu->system, BANK_SLOT_SPR, 0));
        emu->cart = NULL;
        return true;
    }
    return false;
}
