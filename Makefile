CFLAGS += -std=c99 -Wall `pkg-config --cflags pango pangocairo libconfig`
INCLDS := -I.
LDLIBS := -lxcb -lpthread `pkg-config --libs pango pangocairo libconfig`
PROGRAM := yabar
PREFIX ?= /usr
BINPREFIX ?= $(PREFIX)/bin
MANPREFIX ?= $(PREFIX)/share/man

OBJS := $(wildcard src/*.c)
OBJS := $(OBJS:.c=.o)

%.o: %.c %.h
	$(CC) $(CFLAGS) -c -o $@ $<
all: $(PROGRAM)
$(PROGRAM): $(OBJS)
	$(CC) $(LDLIBS) -o $@ $^
install:
	mkdir -p "$(BINPREFIX)"
	cp -pf $(PROGRAM) "$(BINPREFIX)"
	mkdir -p "$(MANPREFIX)"/man1
	cp -pf doc/yabar.1 "$(MANPREFIX)"/man1
uninstall:
	rm -f "$(BINPREFIX)/$(PROGRAM)"
	rm -f "$(MANPREFIX)"/man1/yabar.1
clean:
	rm -f src/*.o $(PROGRAM)
