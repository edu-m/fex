PREFIX ?= /usr
BINDIR ?= $(PREFIX)/bin
SYSCONFDIR ?= /etc

CC ?= cc
CFLAGS ?= -O2 -pipe -Wall -Wextra -pedantic -std=gnu99
LDFLAGS ?=

# Set PLATFORM=posix or PLATFORM=windows to pick the platform-specific
# implementation and link flags
PLATFORM ?= posix

ifeq ($(PLATFORM),posix)
PLATFORM_SRC := src/platform_posix.c src/xdg.c
PLATFORM_LDLIBS := -lncurses -lpanel -lm
else ifeq ($(PLATFORM),windows)
PLATFORM_SRC := src/platform_windows.c
PLATFORM_LDLIBS := -lncurses -lpanel -lm -lshell32
else
$(error Unsupported platform '$(PLATFORM)'; available: posix or windows)
endif

LDLIBS ?= $(PLATFORM_LDLIBS)

TARGET := fex_exec
WRAPPER := fex
SETUP := fex-setup

SRC := src/main.c src/trie.c $(PLATFORM_SRC)
HDR := src/xdg.h src/trie.h src/platform.h

all: $(TARGET)

$(TARGET): $(SRC) $(HDR)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $(SRC) $(LDLIBS)

clean:
	rm -f $(TARGET)

install: all
	install -Dm755 $(TARGET) "$(DESTDIR)$(BINDIR)/$(TARGET)"
	install -Dm755 $(WRAPPER) "$(DESTDIR)$(BINDIR)/$(WRAPPER)"
	install -Dm755 contrib/setup/$(SETUP) "$(DESTDIR)$(BINDIR)/$(SETUP)"
	install -Dm644 contrib/shell/fex.sh "$(DESTDIR)$(SYSCONFDIR)/profile.d/fex.sh"
	install -Dm644 contrib/shell/fex.zsh "$(DESTDIR)$(SYSCONFDIR)/profile.d/fex.zsh"
	install -Dm644 LICENSE "$(DESTDIR)$(PREFIX)/share/licenses/fex/LICENSE"
	install -Dm644 LICENSE.BOOST-1.0 "$(DESTDIR)$(PREFIX)/share/licenses/fex/LICENSE.BOOST-1.0"
	install -Dm644 README.md "$(DESTDIR)$(PREFIX)/share/doc/fex/README.md"

uninstall:
	rm -f "$(DESTDIR)$(BINDIR)/$(TARGET)" "$(DESTDIR)$(BINDIR)/$(WRAPPER)" "$(DESTDIR)$(BINDIR)/$(SETUP)"
	rm -f "$(DESTDIR)$(SYSCONFDIR)/profile.d/fex.sh" "$(DESTDIR)$(SYSCONFDIR)/profile.d/fex.zsh"
	rm -rf "$(DESTDIR)$(PREFIX)/share/licenses/fex" "$(DESTDIR)$(PREFIX)/share/doc/fex"

.PHONY: all clean install uninstall
