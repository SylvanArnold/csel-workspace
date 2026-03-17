EXE=app
SRCS=$(wildcard *.c)

ifneq ($(DEBUG),)
EXTRA_CFLAGS+=-DDEBUG
endif

# Include the standard application Makefile for the CSEL1 labs
include ../appl.mk
