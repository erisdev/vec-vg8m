#include "config.h"
#include "internal.h"

#include <stdbool.h>
#include <stdlib.h>

void origin_memory_init(OriginMemory *bank, uint16_t addr, uint16_t size, void *data, OriginMemRead read_func, OriginMemWrite write_func, OriginMemFree free_func) {
    bank->begin = addr;
    bank->end   = addr + size - 1;
    bank->size  = size;
    bank->data  = data;
    bank->read  = read_func;
    bank->write = write_func;
    bank->free  = free_func;
}

void origin_memory_fin(OriginMemory *bank) {
    if (bank->free)
        bank->free(bank);
}

void origin_ram_init(OriginMemory *bank, uint16_t addr, uint16_t size) {
    origin_memory_init(bank, addr, size, calloc(size, 1),
        ORIGIN_MEM_READ_DEFAULT, ORIGIN_MEM_WRITE_DEFAULT, ORIGIN_MEM_FREE_DEFAULT);
}

void origin_rom_init(OriginMemory *bank, uint16_t addr, uint16_t size) {
    origin_memory_init(bank, addr, size, calloc(size, 1),
        ORIGIN_MEM_READ_DEFAULT, NULL, ORIGIN_MEM_FREE_DEFAULT);
}

static inline bool _inrange(uint16_t addr, uint16_t begin, uint16_t end) {
    return addr >= begin && addr <= end;
}

uint8_t origin_read8(Origin *emu, uint16_t addr) {
    int len = sizeof(emu->memory.banks) / sizeof(OriginMemory);
    for (int i = 0; i < len; ++i) {
        OriginMemory *bank = &emu->memory.banks[i];
        if (_inrange(addr, bank->begin, bank->end)) {
            if (bank->read)
                return bank->read(bank, addr);
            else
                break;
        }
    }
    return 0xFF;
}

void origin_write8(Origin *emu, uint16_t addr, uint8_t data) {
    int len = sizeof(emu->memory.banks) / sizeof(OriginMemory);
    for (int i = 0; i < len; ++i) {
        OriginMemory *bank = &emu->memory.banks[i];
        if (_inrange(addr, bank->begin, bank->end)) {
            if (bank->write)
                bank->write(bank, addr, data);
            return;
        }
    }
}

uint16_t origin_read16(Origin *emu, uint16_t addr) {
    return (origin_read8(emu, addr + 1) << 8)
          | origin_read8(emu, addr);
}

void origin_write16(Origin *emu, uint16_t addr, uint16_t data) {
    origin_write8(emu, addr,    data);
    origin_write8(emu, addr+ 1, data >> 8);
}

uint8_t ORIGIN_MEM_READ_DEFAULT(OriginMemory *bank, uint16_t addr) {
    uint8_t *bytes = bank->data;
    return bytes[addr - bank->begin];
}

void ORIGIN_MEM_WRITE_DEFAULT(OriginMemory *bank, uint16_t addr, uint8_t data) {
    uint8_t *bytes = bank->data;
    bytes[addr - bank->begin] = data;
}

static void ORIGIN_MEM_FREE_DEFAULT(OriginMemory *bank) {
    if(bank->data) free(bank->data);
}
