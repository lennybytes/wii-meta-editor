CC      ?= gcc
PKGS    := gtk+-3.0 libxml-2.0
CFLAGS  += $(shell pkg-config --cflags $(PKGS)) -Wall -Wextra -O2 -MMD
LDLIBS  += $(shell pkg-config --libs $(PKGS))

APP_ID   := io.homebrew.WiiMetaEditor
TARGET   := wii-meta-editor
BUILD    := build
SRC      := src
PKGNAME  := wii-meta-editor
PKGDIR   := pkg
APPDIR   := AppDir
DEB_ARCH := $(shell dpkg-architecture -qDEB_HOST_ARCH 2>/dev/null || echo amd64)

# Version taken from git describe, or fall back to 0.1.0
VERSION  ?= $(shell git describe --tags --always --dirty 2>/dev/null | sed 's/^v//; s/-/+/g' || echo 0.1.0)

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

deb: $(TARGET)
	rm -rf $(PKGDIR)
	mkdir -p $(PKGDIR)/DEBIAN $(PKGDIR)$(BINDIR) $(PKGDIR)$(APPSDIR) $(PKGDIR)$(ICONDIR)
	install -Dm755 $(TARGET) $(PKGDIR)$(BINDIR)/$(TARGET)
	install -Dm644 $(APP_ID_FILE) $(PKGDIR)$(APPSDIR)/$(APP_ID).desktop
	install -Dm644 $(ICON_FILE) $(PKGDIR)$(ICONDIR)/$(APP_ID).svg
	install -m644 debian/control $(PKGDIR)/DEBIAN/control
	sed -i 's/^Architecture: .*/Architecture: $(DEB_ARCH)/' $(PKGDIR)/DEBIAN/control
	sed -i 's/^Version: .*/Version: $(VERSION)/' $(PKGDIR)/DEBIAN/control
	dpkg-deb --build --root-owner-group $(PKGDIR) $(PKGNAME)_$(VERSION)_$(DEB_ARCH).deb
	rm -rf $(PKGDIR)

# Standalone binary (requires GTK3 + libxml2 installed on the target system)
binary: $(TARGET)

# AppImage: bundles the app and its libraries into a single portable file.
appimage: $(TARGET)
	rm -rf $(APPDIR)
	mkdir -p $(APPDIR)/usr/bin $(APPDIR)/usr/share/applications \
		$(APPDIR)/usr/share/icons/hicolor/scalable/apps
	cp $(TARGET) $(APPDIR)/usr/bin/$(TARGET)
	cp $(APP_ID_FILE) $(APPDIR)/usr/share/applications/$(APP_ID).desktop
	cp $(ICON_FILE) $(APPDIR)/usr/share/icons/hicolor/scalable/apps/$(APP_ID).svg
	cp $(ICON_FILE) $(APPDIR)/$(APP_ID).svg
	printf '[Desktop Entry]\nType=Application\nName=Wii Meta Editor\nExec=%s %%F\nIcon=%s\nCategories=Development;\n' \
		"$(TARGET)" "$(APP_ID)" > $(APPDIR)/$(APP_ID).desktop
	ln -sf usr/bin/$(TARGET) $(APPDIR)/AppRun
	export ARCH=$(DEB_ARCH) OUTPUT=$(PKGNAME)-$(VERSION)-$(DEB_ARCH).AppImage \
		&& linuxdeploy --appdir $(APPDIR) --output appimage

clean:
	rm -rf $(BUILD) $(TARGET) $(PKGDIR) $(APPDIR) $(PKGNAME)_*.deb $(PKGNAME)-*.AppImage

-include $(DEPS)

.PHONY: all install uninstall clean deb binary appimage
