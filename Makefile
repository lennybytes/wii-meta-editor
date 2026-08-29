CC      ?= gcc
PKGS    := gtk+-3.0 libxml-2.0
CFLAGS  += $(shell pkg-config --cflags $(PKGS)) -Wall -Wextra -O2 -MMD
LDLIBS  += $(shell pkg-config --libs $(PKGS))

APP_ID   := io.homebrew.WiiMetaEditor
TARGET   := wii-meta-editor
BUILD    := build
SRC      := src

APP_ID_FILE := $(wildcard $(APP_ID).desktop)
ICON_FILE   := $(wildcard $(APP_ID).svg)

PREFIX  ?= /usr/local
BINDIR  := $(PREFIX)/bin
APPSDIR := $(PREFIX)/share/applications
ICONDIR := $(PREFIX)/share/icons/hicolor/scalable/apps

SOURCES := $(wildcard $(SRC)/*.c)
OBJECTS := $(patsubst $(SRC)/%.c,$(BUILD)/%.o,$(SOURCES))
DEPS    := $(OBJECTS:.o=.d)

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDLIBS)

$(BUILD)/%.o: $(SRC)/%.c | $(BUILD)
	$(CC) $(CFLAGS) -c -o $@ $<

$(BUILD):
	mkdir -p $(BUILD)

install: $(TARGET)
	install -Dm755 $(TARGET) $(DESTDIR)$(BINDIR)/$(TARGET)
	install -Dm644 $(APP_ID_FILE) $(DESTDIR)$(APPSDIR)/$(APP_ID).desktop
	install -Dm644 $(ICON_FILE) $(DESTDIR)$(ICONDIR)/$(APP_ID).svg
	@-update-desktop-database $(DESTDIR)$(APPSDIR) 2>/dev/null || true

uninstall:
	rm -f $(DESTDIR)$(BINDIR)/$(TARGET)
	rm -f $(DESTDIR)$(APPSDIR)/$(APP_ID).desktop
	rm -f $(DESTDIR)$(ICONDIR)/$(APP_ID).svg

clean:
	rm -rf $(BUILD) $(TARGET)

-include $(DEPS)

.PHONY: all install uninstall clean
