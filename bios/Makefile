include ../vgdev/make/sdcc-vars.mk

CRT0 =
CODESEG = 0x0100
DATASEG = 0x0800

.PHONY : default
default : system.vg8 bios.bin charset.1bpp

bios.bin : boot.rel loader.rel

system.vg8 : bios.bin charset.1bpp tiles.2bpp
	$(PACKROM) $@ \
		--bank bios bios.bin \
		--bank txt  charset.1bpp \
		--bank bg   tiles.2bpp

charset.1bpp : charset.png
	$(TILECONV) -f 1bpp $@ $^

tiles.2bpp : logo.png
	$(TILECONV) -0 -f 2bpp $@ $^

include ../vgdev/make/sdcc-rules.mk
