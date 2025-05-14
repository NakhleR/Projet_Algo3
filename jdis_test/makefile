jdis_dir = ../jdis/
hashtable_dir = ../hashtable/
holdall_dir = ../holdall/
CC = gcc
CFLAGS = -std=c2x \
  -Wall -Wconversion -Werror -Wextra -Wpedantic -Wwrite-strings \
  -O2 \
  -I$(jdis_dir) -I$(hashtable_dir) -I$(holdall_dir) \
  -DHASHTABLE_STATS=0 -DWANT_HOLDALL_EXT -DHOLDALL_EXT
vpath %.c $(jdis_dir) $(hashtable_dir) $(holdall_dir)
vpath %.h $(jdis_dir) $(hashtable_dir) $(holdall_dir)
objects = main.o jdis.o hashtable.o holdall.o
executable = jdis
makefile_indicator = .\#makefile\#

.PHONY: all clean

all: $(executable)

clean:
	$(RM) $(objects) $(executable)
	@$(RM) $(makefile_indicator)

$(executable): $(objects)
	$(CC) $(objects) -o $(executable)

main.o: main.c jdis.h hashtable.h holdall.h
jdis.o: jdis.c jdis.h
hashtable.o: hashtable.c hashtable.h
holdall.o: holdall.c holdall.h

include $(makefile_indicator)

$(makefile_indicator): makefile
	@touch $@
	@$(RM) $(objects) $(executable)
