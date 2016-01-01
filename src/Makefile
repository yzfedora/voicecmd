CC	= gcc
CFLAGS	= -Wall -g -lasound -lcurl -ljansson -lm
OBJS	= google_sprec.o sndcap.o voice_command.o voice_action.o google_key.o
PROGS	= main

ALL: $(OBJS) $(PROGS)

%.o: %.c %.h
	$(CC) -c $^ $(CFLAGS)

%: %.c $(OBJS)
	$(CC) -o $@ $^ $(CFLAGS)


.PHONY: clean
clean:
	$(RM) $(OBJS) $(PROGS) $(wildcard *.h.gch)
