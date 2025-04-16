.PHONY: all clean dist

CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -g
LDFLAGS = -lm

OBJDIR = obj
BINDIR = bin

# Sources
JDIS_SRC = jdis/jdis.c
MAIN_SRC = main.c

# Librairies externes
HASHTABLE_DIR = hashtable
HOLDALL_DIR = holdall

# Objets
OBJS = $(OBJDIR)/jdis.o $(OBJDIR)/main.o $(OBJDIR)/hashtable.o $(OBJDIR)/holdall.o

# Exécutable
TARGET = $(BINDIR)/jdis

all: directories $(TARGET)

directories:
	mkdir -p $(OBJDIR)
	mkdir -p $(BINDIR)

$(OBJDIR)/jdis.o: $(JDIS_SRC)
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJDIR)/main.o: $(MAIN_SRC)
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJDIR)/hashtable.o: $(HASHTABLE_DIR)/hashtable.c $(HASHTABLE_DIR)/hashtable.h
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJDIR)/holdall.o: $(HOLDALL_DIR)/holdall.c $(HOLDALL_DIR)/holdall.h
	$(CC) $(CFLAGS) -c $< -o $@

$(TARGET): $(OBJS)
	$(CC) $(OBJS) -o $@ $(LDFLAGS)

clean:
	rm -rf $(OBJDIR) $(BINDIR)

dist: clean
	tar -hzcf "$(CURDIR).tar.gz" hashtable/* holdall/* jdis/* main.c makefile