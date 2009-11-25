# Petit makefile un peu pourri mais en même temps c'est pas moi le pro de Linux
EMUDIR = .
COREDIR = core
LINUXDIR = linux

TARGET = heig-boy
CC = gcc
CFLAGS = -Wall -I$(EMUDIR) -I$(LINUXDIR) -g
LIBS = -lasound
# Y a moyen d'inclure génériquement tout ce qu'il y a dans core et linux, mais je sais plus comment de tête
OBJS = $(LINUXDIR)/main.o $(LINUXDIR)/os_specific.o \
	$(LINUXDIR)/sound-driver.o \
	$(COREDIR)/color_it.o $(COREDIR)/cpu.o $(COREDIR)/dasm.o \
	$(COREDIR)/debug.o $(COREDIR)/emu.o $(COREDIR)/io.o \
	$(COREDIR)/lcd.o $(COREDIR)/lcd_status.o $(COREDIR)/mbc.o \
	$(COREDIR)/mem.o $(COREDIR)/save.o $(COREDIR)/sound.o \
	$(COREDIR)/timer.o

# Compile les fichiers
%.o: %.c
	$(CC) -c -o $@ $< $(CFLAGS)

# Fabrique l'exécutable. $@: à gauche du :, $^: à droite.
$(TARGET): $(OBJS)
	gcc -o $@ $^ $(CFLAGS) $(LIBS)

# Clean-up
clean:
	rm -f $(TARGET) $(OBJS)

