#include "config.h"

#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>

#include "vg8m.h"
#include "internal.h"

static const char MAGIC[] = "VGDMP100";

typedef struct s_dump_header DumpHeader;
struct s_dump_header {
    char magic[8];
    uint16_t flags;
    uint16_t prog_offset;
    uint16_t prog_size;
    uint16_t pat_2bpp_offset;
    uint16_t pat_2bpp_size;
    uint16_t pat_3bpp_offset;
    uint16_t pat_3bpp_size;
};

typedef struct s_cart_header CartHeader;
struct s_cart_header {
    char magic[8];
    char title[0x20];
    char developer[0x20];
    char lang[2];
    uint16_t flags;
    uint8_t version;
};

bool vg8m_load_cart(VG8M *emu, const char *filename) {
    DumpHeader header;
    int len;

    int fd = open(filename, O_RDONLY);
    if (fd == -1) goto error;

    if (read(fd, &header, sizeof(header)) == -1)
        goto error;

    len = _min(header.prog_size, CART_ROM_SIZE);
    if (len > 0) {
        if (lseek(fd, header.prog_offset, SEEK_SET) == -1
        ||  read(fd, emu->cart_prog_rom, len) == -1)
            goto error;
    }

    len = _min(header.pat_2bpp_size, CART_ROM_SIZE);
    if (len > 0) {
        if (lseek(fd, header.pat_2bpp_offset, SEEK_SET) == -1
        ||  read(fd, emu->cart_2bpp_rom, header.pat_2bpp_size) == -1)
            goto error;
    }

    len = _min(header.pat_3bpp_size, CART_ROM_SIZE);
    if (len > 0) {
        if (lseek(fd, header.pat_3bpp_offset, SEEK_SET) == -1
        ||  read(fd, emu->cart_3bpp_rom, header.pat_3bpp_size) == -1)
            goto error;
    }

    close(fd);
    return true;
error:
    if (fd != -1) close(fd);
    return false;
}
