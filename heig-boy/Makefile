EMUDIR = .
COREDIR = core
LINUXDIR = linux
INTER = interface
TARGET = MainFrame
CC = gcc
CC2 = g++
CFLAGS = -I$(EMUDIR) -I$(LINUXDIR) -g
LIBS = -lasound
CXX = $(shell wx-config --cxx)

OBJS = $(LINUXDIR)/os_specific.o \
	$(LINUXDIR)/sound-driver.o \
	$(COREDIR)/color_it.o $(COREDIR)/cpu.o $(COREDIR)/dasm.o \
	$(COREDIR)/debug.o $(COREDIR)/emu.o $(COREDIR)/io.o \
	$(COREDIR)/lcd.o $(COREDIR)/lcd_status.o $(COREDIR)/mbc.o \
	$(COREDIR)/mem.o $(COREDIR)/save.o $(COREDIR)/sound.o \
	$(COREDIR)/timer.o

OBJS2 = $(INTER)/MainFrame.o $(INTER)/KeysMap.o $(INTER)/ConfigDialog.o $(INTER)/RefreshManager.o

# Compile les fichiers
%.o: %.c
	$(CC) -c -o $@ $< $(CFLAGS)

# Fabrique l'exécutable. $@: à gauche du :, $^: à droite.
$(TARGET): $(OBJS)
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)

# Compile les fichiers
%.o: %.cpp
	$(CXX) -c `wx-config --cxxflags` -o $@ $< $(CFLAGS)

# Fabrique l'exécutable. $@: à gauche du :, $^: à droite.
$(TARGET): $(OBJS2)
	$(CXX) -o $@ $^ $(CFLAGS) $(LIBS) `wx-config --libs`
# Clean-up
clean:
	rm -f $(TARGET) $(OBJS)

