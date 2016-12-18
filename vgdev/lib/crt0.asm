        .z80
        .globl  _main
        .globl  s__INITIALIZER
        .globl  l__INITIALIZER
        .globl  s__INITIALIZED

        .area   _HEADER (ABS, OVR)
        .org    0x8080
        .dw     init

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
init:
        call    gsinit
        call    _main
done:
        halt
        jr      done

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
