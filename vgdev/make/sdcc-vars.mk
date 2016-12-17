MAKEFLAGS += --no-builtin-rules

VGDEV := $(patsubst %/make/,%,$(dir $(lastword $(MAKEFILE_LIST))))
SDCC_SHARE = /usr/local/share/sdcc

CODESEG    = 0x8100
DATASEG    = 0x4000

CRT0       = $(VGDEV)/lib/crt0.rel

CC         = sdcc -mz80
AS         = sdasz80
OBJCOPY    = sdobjcopy

CFLAGS     = -I$(VGDEV)/include
LIBS       =
