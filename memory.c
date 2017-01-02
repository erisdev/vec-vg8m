#include "config.h"
#include "internal.h"

#include <stdbool.h>
#include <stdlib.h>

void vg8m_memory_init(VG8MMemory *bank, uint16_t addr, uint16_t size, void *data, VG8MMemRead read_func, VG8MMemWrite write_func, VG8MMemFree free_func) {
    bank->begin = addr;
    bank->end   = addr + size - 1;
    bank->size  = size;
    bank->data  = data;
    bank->read  = read_func;
    bank->write = write_func;
    bank->free  = free_func;
}

void vg8m_memory_fin(VG8MMemory *bank) {
    if (bank->free)
        bank->free(bank);
}

void vg8m_ram_init(VG8MMemory *bank, uint16_t addr, uint16_t size) {
    vg8m_memory_init(bank, addr, size, calloc(size, 1),
        VG8M_MEM_READ_DEFAULT, VG8M_MEM_WRITE_DEFAULT, VG8M_MEM_FREE_DEFAULT);
}

void vg8m_rom_init(VG8MMemory *bank, uint16_t addr, uint16_t size) {
    vg8m_memory_init(bank, addr, size, calloc(size, 1),
        VG8M_MEM_READ_DEFAULT, NULL, VG8M_MEM_FREE_DEFAULT);
}

static inline bool _inrange(uint16_t addr, uint16_t begin, uint16_t end) {
    return addr >= begin && addr <= end;
}

uint8_t vg8m_read8(VG8M *emu, uint16_t addr) {
    int len = sizeof(emu->memory.banks) / sizeof(VG8MMemory);
    for (int i = 0; i < len; ++i) {
        VG8MMemory *bank = &emu->memory.banks[i];
        if (_inrange(addr, bank->begin, bank->end)) {
            if (bank->read)
                return bank->read(bank, addr);
            else
                break;
        }
    }
    return 0xFF;
}

void vg8m_write8(VG8M *emu, uint16_t addr, uint8_t data) {
    int len = sizeof(emu->memory.banks) / sizeof(VG8MMemory);
    for (int i = 0; i < len; ++i) {
        VG8MMemory *bank = &emu->memory.banks[i];
        if (_inrange(addr, bank->begin, bank->end)) {
            if (bank->write)
                bank->write(bank, addr, data);
            return;
        }
    }
}

uint16_t vg8m_read16(VG8M *emu, uint16_t addr) {
    return (vg8m_read8(emu, addr + 1) << 8)
          | vg8m_read8(emu, addr);
}

void vg8m_write16(VG8M *emu, uint16_t addr, uint16_t data) {
    vg8m_write8(emu, addr,    data);
    vg8m_write8(emu, addr+ 1, data >> 8);
}

uint8_t VG8M_MEM_READ_DEFAULT(VG8MMemory *bank, uint16_t addr) {
    uint8_t *bytes = bank->data;
    return bytes[addr - bank->begin];
}

void VG8M_MEM_WRITE_DEFAULT(VG8MMemory *bank, uint16_t addr, uint8_t data) {
    uint8_t *bytes = bank->data;
    bytes[addr - bank->begin] = data;
}

static void VG8M_MEM_FREE_DEFAULT(VG8MMemory *bank) {
    if(bank->data) free(bank->data);
}
