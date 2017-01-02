#include "config.h"
#include "internal.h"

#include <stdbool.h>
#include <stdlib.h>

static const char JUNK[] = "DUMPSTERDIVEMOTHER666";

// COMMON

void origin_mem_fin(OriginMemSlot *slot) {
    if (slot->is_banked) {
        slot->banked.bank = NULL;
    }
    else if (slot->fixed.free) {
        slot->fixed.free(slot->fixed.bytes);
        slot->fixed.bytes = NULL;
    }
}

uint8_t *origin_mem_bytes(OriginMemSlot *slot) {
    if (slot->is_banked) {
        if (slot->banked.bank)
            return slot->banked.bank->bytes;
    }
    else {
        if (slot->fixed.bytes)
            return slot->fixed.bytes;
    }
    return 0;
}

uint16_t origin_mem_size(OriginMemSlot *slot) {
    if (slot->is_banked) {
        if (slot->banked.bank)
            return slot->banked.bank->size;
    }
    else {
        if (slot->fixed.bytes)
            return slot->fixed.size;
    }
    return 0;
}

uint8_t origin_mem_read(OriginMemSlot *slot, uint16_t addr) {
    return slot->read(slot, addr);
}

void origin_mem_write(OriginMemSlot *slot, uint16_t addr, uint8_t data) {
    if (slot->write)
        slot->write(slot, addr, data);
}

// FIXED

static uint8_t _read_fixed(OriginMemSlot *slot, uint16_t addr) {
    return slot->fixed.bytes[addr - slot->begin];
}

static void _write_fixed(OriginMemSlot *slot, uint16_t addr, uint8_t data) {
    slot->fixed.bytes[addr - slot->begin] = data;
}

void origin_mem_init(OriginMemSlot *slot, uint16_t addr, uint16_t size, void *data, OriginMemFree free_func) {
    slot->read  = _read_fixed;
    slot->write = _write_fixed;
    slot->begin = addr;
    slot->end   = addr + size - 1;

    slot->is_banked   = false;
    slot->fixed.free  = free_func;
    slot->fixed.bytes = data;
    slot->fixed.size  = size;
}

void origin_mem_init_ram(OriginMemSlot *slot, uint16_t addr, uint16_t size) {
    uint8_t *data = calloc(size, 1);
    origin_mem_init(slot, addr, size, data, free);
}

void origin_mem_init_rom(OriginMemSlot *slot, uint16_t addr, uint16_t size) {
    uint8_t *data = calloc(size, 1);
    origin_mem_init(slot, addr, size, data, free);
    slot->write = NULL;
}

// BANKED

static uint8_t _read_banked(OriginMemSlot *slot, uint16_t addr) {
    if (slot->banked.bank) {
        uint16_t offset = addr - slot->begin;
        if (offset < slot->banked.bank->size)
            return slot->banked.bank->bytes[offset];
    }
    return JUNK[addr & sizeof(JUNK)];
}

static void _write_banked(OriginMemSlot *slot, uint16_t addr, uint8_t data) {
    if (slot->banked.bank && slot->banked.bank->writable) {
        uint16_t offset = addr - slot->begin;
        if (offset < slot->banked.bank->size)
            slot->banked.bank->bytes[offset] = data;
    }
}

void origin_mem_init_banked(OriginMemSlot *slot, uint16_t addr, uint16_t size) {
    slot->read  = _read_banked;
    slot->write = _write_banked;
    slot->begin = addr;
    slot->end   = addr + size - 1;

    slot->is_banked   = true;
    slot->banked.bank = NULL;
}

bool origin_mem_set_bank(OriginMemSlot *slot, OriginMemBank *bank) {
    if (slot->is_banked) {
        slot->banked.bank = bank;
        return true;
    }
    else
        return false;
}

// MEMORY BANKS

void origin_bank_init(OriginMemBank *bank, uint16_t size, bool writable) {
    bank->size     = size;
    bank->bytes    = calloc(size, 1);
    bank->writable = writable;
}

void origin_bank_fin(OriginMemBank *bank) {
    if (bank->bytes) free(bank->bytes);
    bank->bytes = NULL;
    bank->size  = 0;
}

// MEMORY MAP

static inline bool _inrange(uint16_t addr, OriginMemSlot *slot) {
    return addr >= slot->begin && addr <= slot->end;
}

static inline OriginMemSlot *_find_slot(Origin *emu, uint16_t addr) {
    int len = sizeof(emu->memory.slots) / sizeof(OriginMemSlot);
    for (int i = 0; i < len; ++i) {
        OriginMemSlot *slot = &emu->memory.slots[i];
        if (_inrange(addr, slot))
            return slot;
    }
    return NULL;
}

uint8_t origin_read8(Origin *emu, uint16_t addr) {
    OriginMemSlot *slot = _find_slot(emu, addr);
    if (slot)
        return slot->read(slot, addr);
    else
        return JUNK[addr & sizeof(JUNK)];
}

void origin_write8(Origin *emu, uint16_t addr, uint8_t data) {
    OriginMemSlot *slot = _find_slot(emu, addr);
    if (slot && slot->write)
        slot->write(slot, addr, data);
}

uint16_t origin_read16(Origin *emu, uint16_t addr) {
    OriginMemSlot *slot;
    uint8_t h, l;

    slot = _find_slot(emu, addr);
    if (slot)
        l = slot->read(slot, addr);
    else
        l = JUNK[addr & sizeof(JUNK)];

    ++addr;
    if (!_inrange(addr, slot))
        slot = _find_slot(emu, addr);

    if (slot)
        h = slot->read(slot, addr);
    else
        h = JUNK[addr & sizeof(JUNK)];

    return (h << 8) | l;
}

void origin_write16(Origin *emu, uint16_t addr, uint16_t data) {
    OriginMemSlot *slot;

    slot = _find_slot(emu, addr);
    if (slot && slot->write)
        slot->write(slot, addr, data >> 8);

    ++addr;
    if (!_inrange(addr, slot))
        slot = _find_slot(emu, addr);

    if (slot && slot->write)
        slot->write(slot, addr, data);
}
