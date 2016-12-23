include ../vgdev/make/sdcc-vars.mk

CRT0 =
CODESEG = 0x0100
DATASEG = 0x0800

.PHONY : default
default : system.bin charset.1bpp

system.bin : boot.rel system.rel

charset.1bpp : charset.png
	$(TILECONV) -f 1bpp $@ $^

include ../vgdev/make/sdcc-rules.mk
