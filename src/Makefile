# debug output is enbale by default. using DEBUG = n to disable it.
DEBUG	= y
CC	= gcc
MODDIR	= -DMODDIR=\"$(shell pkg-config --variable=modeldir pocketsphinx)\"
PSCFLAGS= $(shell pkg-config --cflags --libs pocketsphinx sphinxbase)
CFLAGS	= -Wall -g -lasound -lcurl -ljansson -lm $(PSCFLAGS) $(MODDIR)
OBJS	= google_sprec.o sndcap.o voice_command.o voice_action.o google_key.o \
	  sphinx_sprec.o
PROGS	= main

ifeq ($(DEBUG), y)
	CFLAGS += -D__DEBUG__
endif

ALL: $(OBJS) $(PROGS)

%.o: %.c %.h
	$(CC) -c $^ $(CFLAGS)

%: %.c $(OBJS)
	$(CC) -o $@ $^ $(CFLAGS)


.PHONY: clean
clean:
	$(RM) $(OBJS) $(PROGS) $(wildcard *.h.gch)
