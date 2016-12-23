include ../../vgdev/make/sdcc-vars.mk

demo.vg8 : demo.bin
	$(PACKROM) $@ $<

demo.bin : header.rel demo.rel

include ../../vgdev/make/sdcc-rules.mk
