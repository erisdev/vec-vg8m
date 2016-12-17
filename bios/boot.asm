        .z80
        .globl  _system_main
        .globl  _int_ignore
        .globl  s__INITIALIZER
        .globl  l__INITIALIZER
        .globl  s__INITIALIZED

        .area   _HEADER (ABS)
        .org    0x0000

        jp boot

        .org 0x08
        reti

        .org 0x10
        reti

        .org 0x18
        reti

        .org 0x20
        reti

        .org 0x28
        reti

        .org 0x30
        reti

        .org 0x38
        reti

        .strz "VEC-VG8ME SYS.ROM v1.2"
        .even
boot:
        ; set up interrupts with jump table at the end of RAM
        im      2               ; mode 2 is more versatile & fun
        ld      A,#0x0E         ; load high bits of interrupt table address
        ld      I,A

        ; fill jump table with default "ignore interrupt" handler
        ld      HL,#_int_ignore
        ld      (0x0E00),HL
        ld      HL,#0xFF00
        ld      DE,#0xFF02
        ld      BC,#0xFE
        ldir

        ; stack grows down from the top of user RAM
        ld      SP,#0x7FFF

        ; finally, copy initializers & jump into the actual system rom
        call    gsinit
        jp      _system_main

        ; nmi jumps to 0x66, make it reboot
        .org    0x66
        jp      boot

        ; declare sections in the order they're meant to appear
        .area   _HOME
        .area   _CODE
        .area   _INITIALIZER
        .area   _GSINIT
        .area   _GSFINAL

        .area   _DATA
        .area   _INITIALIZED
        .area   _BSEG
        .area   _BSS
        .area   _HEAP


        .area   _CODE

_int_ignore:
        ei
        reti

        .area   _GSINIT
gsinit::
	ld	bc, #l__INITIALIZER
	ld	a, b
	or	a, c
	jr	Z, gsinit_next
	ld	de, #s__INITIALIZED
	ld	hl, #s__INITIALIZER
	ldir
gsinit_next:

	.area   _GSFINAL
	ret
