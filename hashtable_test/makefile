hashtable_dir = ../hashtable/
holdall_dir = ../holdall/
CC = gcc
CFLAGS = -std=c2x \
  -Wall -Wconversion -Werror -Wextra -Wpedantic -Wwrite-strings \
  -O2 \
  -I$(hashtable_dir) -I$(holdall_dir) \
  -DxWANT_HASHTABLE_EXT
vpath %.c $(hashtable_dir) $(holdall_dir)
vpath %.h $(hashtable_dir) $(holdall_dir)
objects = main.o hashtable.o holdall.o
executable = test

.PHONY: all clean

all: $(executable)

clean:
	$(RM) $(objects) $(executable)

$(executable): $(objects)
	$(CC) $(objects) -o $(executable)

main.o: main.c hashtable.h hashtable_ip.h holdall.h holdall_ip.h
hashtable.o: hashtable.c hashtable.h hashtable_ip.h
holdall.o: holdall.c holdall.h holdall_ip.h

# Force rebuild when makefile changes
main.o hashtable.o holdall.o: makefile 