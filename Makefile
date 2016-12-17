LIBS = sdl2

CFLAGS += -iquote gen -iquote vendor/libz80 -g \
	$(shell pkg-config --cflags $(LIBS))

LDFLAGS += -Lvendor/libz80 -g \
	$(shell pkg-config --libs $(LIBS))

.PHONY : all
all : vg8m bios demo

vg8m : sdl_driver.o vg8m.o video.o vendor/libz80/z80.o

vg8m.o : vg8m.c vg8m.h
video.o : video.c video.h vg8m.h
sdl_driver.o : sdl_driver.c vg8m.h video.h

gen/%.h : bios/%
	xxd -i $< $@

vendor/libz80/z80.o :
	$(MAKE) -C vendor/libz80 z80.o

.PHONY : clean
clean : bios-clean launchpad-clean demo-clean
	$(RM) gen/*.h
	$(RM) *.o
	$(RM) vg8m

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
