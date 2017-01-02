Z80_DIR = vendor/libz80
PACKAGES = sdl2

CFLAGS += -iquote gen -iquote $(Z80_DIR) -g \
	$(shell pkg-config --cflags $(PACKAGES))

LDFLAGS += -L$(Z80_DIR) -lz80 -g \
	$(shell pkg-config --libs $(PACKAGES))

.PHONY : all
all : vg8m bios demo

vg8m : sdl_driver.o vg8m.o cart.o error.o memory.o video.o | libz80

cart.o : cart.c vg8m.h
error.o : error.c vg8m.h
memory.o : memory.c vg8m.h
vg8m.o : vg8m.c vg8m.h
video.o : video.c vg8m.h
sdl_driver.o : sdl_driver.c vg8m.h

.PHONY : clean
clean :
	$(RM) *.o
	$(RM) vg8m

.PHONY : deepclean
deepclean : clean libz80-clean bios-clean demo-clean

.PHONY : libz80
libz80 :
	$(MAKE) -C vendor/libz80

.PHONY : libz80-clean
libz80-clean :
	$(MAKE) -C vendor/libz80 clean

.PHONY : bios
bios :
	$(MAKE) -C bios

.PHONY : bios-clean
bios-clean :
	$(MAKE) -C bios clean

.PHONY : launchpad
launchpad :
	$(MAKE) -C roms/lanchpad

.PHONY : launchpad-clean
launchpad-clean :
	$(MAKE) -C roms/launchpad clean

.PHONY : demo
demo :
	$(MAKE) -C roms/demo

.PHONY : demo-clean
demo-clean :
	$(MAKE) -C roms/demo clean
