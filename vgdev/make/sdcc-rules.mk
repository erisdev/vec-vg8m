.SUFFIXES :
.INTERMEDIATE :

%.bin : | $(CRT0)
	$(CC) $(CFLAGS) --no-std-crt0 \
		--code-loc $(CODESEG) \
		--data-loc $(DATASEG) \
		-o $(@:.bin=.ihx) $(CRT0) $^ $(LIBS)
	$(OBJCOPY) -Iihex -Obinary $(@:.bin=.ihx) $@

%.S : %.c
	$(CC) $(CFLAGS) -S -o $@ $<

%.rel : %.asm
	$(AS) -o $@ $<

%.rel : %.S
	$(AS) -o $@ $<

$(VGDEV)/lib/crt0.rel : $(VGDEV)/lib/crt0.asm

.PHONY : clean
clean :
	$(RM) *.bin *.ihx *.rel *.map *.noi
