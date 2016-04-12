VERSION = $(shell git describe)
CPPFLAGS += -DVERSION=\"$(VERSION)\" -D_POSIX_C_SOURCE=199309L -DYA_INTERNAL -DYA_DYN_COL \
			-DYA_ENV_VARS -DYA_INTERNAL_EWMH -DYA_ICON
CFLAGS += -std=c99 -Iinclude -pedantic -Wall -Os `pkg-config --cflags pango pangocairo libconfig gdk-pixbuf-2.0`
LDLIBS := -lxcb -lpthread -lxcb-randr -lxcb-ewmh `pkg-config --libs pango pangocairo libconfig gdk-pixbuf-2.0`
PROGRAM := yabar
PREFIX ?= /usr
BINPREFIX ?= $(PREFIX)/bin
MANPREFIX ?= $(PREFIX)/share/man

OBJS := $(wildcard src/*.c) $(wildcard src/intern_blks/*.c)
OBJS := $(OBJS:.c=.o)

%.o: %.c %.h
	$(CC) $(CFLAGS) -c -o $@ $<
all: $(PROGRAM)
$(PROGRAM): $(OBJS)
	$(CC) -o $@ $^ $(LDLIBS)
install:
	mkdir -p "$(DESTDIR)$(BINPREFIX)"
	cp -pf $(PROGRAM) "$(DESTDIR)$(BINPREFIX)"
	mkdir -p "$(DESTDIR)$(MANPREFIX)"/man1
	cp -pf doc/yabar.1 "$(DESTDIR)$(MANPREFIX)"/man1
uninstall:
	rm -f "$(DESTDIR)$(BINPREFIX)/$(PROGRAM)"
	rm -f "$(DESTDIR)$(MANPREFIX)"/man1/yabar.1
clean:
	rm -f src/*.o $(PROGRAM)

.PHONY: all install uninstall clean
