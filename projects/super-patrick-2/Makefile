CC	= ../../bin/lcc -Wa-l -Wl-m -v
CFLAGS	= 

OBJS	= main.o \
	cailloux.o \
	game.o \
	perso.o \
	util.o \
	gfx/level-01/bglvl01.til.o \
	gfx/level-01/bglvl01.map.o
	
BINS	= super-patrick.gb

all:	$(BINS)

%.o:	%.c
	$(CC) $(CFLAGS) -c -o $@ $<

%.o:	%.s
	$(CC) $(CFLAGS) -c -o $@ $<

%.s:	%.c
	$(CC) $(CFLAGS) -S -o $@ $<

%.s:	%.asm
	../../bin/maccer -o $@ $<

%.o:	%.s
	$(CC) $(CFLAGS) -c -o $@ $<

%.gb:	%.o
	$(CC) $(CFLAGS) -o $@ $<

clean:
	rm -f *.o *.lst *.map *.gb

# Link file, and write 0x80 at position 0x143 in header
super-patrick.gb:	$(OBJS)
	$(CC) $(CFLAGS) -Wl-yp0x143=0x80 -o super-patrick.gb $(OBJS)
