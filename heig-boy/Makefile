# Petit makefile un peu pourri mais en même temps c'est pas moi le pro de Linux
TARGET = heig-boy
CC = gcc
CFLAGS = -Wall
LIBS = -lglut
# Y a moyen d'inclure génériquement tout ce qu'il y a dans core et linux, mais je sais plus comment de tête
OBJS = linux/main.o

# Compile les fichiers
%.o: %.c
	$(CC) -c -o $@ $< $(CFLAGS)

# Fabrique l'exécutable. $@: à gauche du :, $^: à droite.
$(TARGET): $(OBJS)
	gcc -o $@ $^ $(CFLAGS) $(LIBS)

# Clean-up
clean:
	rm -f $(TARGET) $(OBJS)
