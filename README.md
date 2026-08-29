# Wii Meta Editor

A lightweight GTK3 editor for the `meta.xml` file of the **Wii Homebrew Channel**.

It lets you open a `meta.xml`, edit the important tags (name, version, release
date, coder, short/long description, `ahb_access`) and save it back as clean,
properly formatted XML.

## Features

- Open & edit Wii Homebrew Channel `meta.xml` files
- Edit: app name, version, coder, release date, short & long description,
  and the `ahb_access` flag
- Flexible date input: `YYYY-MM-DD`, `DD.MM.YYYY`, `DD.MM.YY` or the native
  Wii format `YYYYMMDDHHMMSS`
- Blank/new files are offered as an empty template so you can build a
  `meta.xml` from scratch
- Clean, formatted UTF-8 XML output
- Open a file directly from the file manager (opens via `argv[1]`)

## Screenshot

![Wii Meta Editor](screenshots/screenshot1.png)

## Download

Prebuilt releases are available on the [Releases](https://github.com/lfisbeck650-cloud/wii-meta-editor/releases) page
with three installation options:

| Asset                                         | For who                                              |
|-----------------------------------------------|------------------------------------------------------|
| `wii-meta-editor_<version>_amd64.deb`         | Debian/Ubuntu users (`.deb` package install)         |
| `wii-meta-editor-<version>-amd64.AppImage`    | Any Linux distro (portable, bundles its own libs)    |
| `wii-meta-editor` (raw binary)                | Any Linux with GTK3 + libxml2 already installed      |

### Install the .deb

```sh
sudo apt install ./wii-meta-editor_<version>_amd64.deb
```

This installs the binary, a desktop entry and an icon. Afterwards launch
**Wii Meta Editor** from your app menu, or run `wii-meta-editor`.

### Run the AppImage

AppImages are portable and don't need installation. Make it executable and run:

```sh
chmod +x wii-meta-editor-<version>-amd64.AppImage
./wii-meta-editor-<version>-amd64.AppImage
```

If you use a file manager, simply double-click the AppImage. To open a
specific file, pass it as an argument:

```sh
./wii-meta-editor-<version>-amd64.AppImage path/to/meta.xml
```

### Run the raw binary

You need GTK3 and libxml2 installed, then:

```sh
chmod +x wii-meta-editor
./wii-meta-editor
```

## Build from source

### Requirements (Debian/Ubuntu)

Install the build dependencies with:

```sh
sudo apt install git build-essential pkg-config libgtk-3-dev libxml2-dev
```

### 1. Clone & build

```sh
git clone https://github.com/lfisbeck650-cloud/wii-meta-editor.git
cd wii-meta-editor
make               # compiles the binary into ./wii-meta-editor
```

### 2. Install

```sh
sudo make install
```

`make install` installs:

- `/usr/local/bin/wii-meta-editor` (the binary)
- `/usr/local/share/applications/io.homebrew.WiiMetaEditor.desktop`
- `/usr/local/share/icons/hicolor/scalable/apps/io.homebrew.WiiMetaEditor.svg`

### 3. Run

```sh
wii-meta-editor                    # open the editor
wii-meta-editor path/to/meta.xml   # open a specific file
```

### Uninstall

```sh
sudo make uninstall
```

### Building packages from source

Build a Debian package:

```sh
make deb          # produces wii-meta-editor_<version>_amd64.deb
```

Build a portable AppImage (requires `linuxdeploy` on your `PATH`):

```sh
make appimage     # produces wii-meta-editor-<version>-amd64.AppImage
```

The standalone binary is produced by the default `make` target.

The version is derived from the latest git tag; override it with `make deb
VERSION=1.2.3` (or `make appimage VERSION=1.2.3`) if needed.

### Make targets

| Command              | Description                                            |
|----------------------|--------------------------------------------------------|
| `make` / `make all`  | Compile the binary from `src/`                         |
| `make install`       | Install binary, desktop entry and icon (root)          |
| `make uninstall`     | Remove installed files (root)                          |
| `make deb`           | Build a `.deb` package                                 |
| `make appimage`      | Build a portable AppImage (needs `linuxdeploy`)        |
| `make binary`        | Alias for the default compile (`wii-meta-editor`)      |
| `make clean`         | Remove build artifacts and the package outputs         |

## Project layout

```
.
├── Makefile          # build rules (outputs into build/)
├── build/            # compiler artifacts (*.o, *.d)
├── src/
│   ├── main.c        # entry point
│   ├── app.h         # shared structs & interfaces
│   ├── app.c         # controller: dialogs & file handling
│   ├── gui.c         # pure GTK GUI (form rendering)
│   └── meta.c        # pure XML logic (libxml2, date handling)
├── io.homebrew.WiiMetaEditor.desktop
├── io.homebrew.WiiMetaEditor.svg
├── debian/
│   └── control       # metadata for the .deb package
├── screenshots/
└── README.md
```

## Usage

Launch from the app menu, or open a file directly:

```sh
wii-meta-editor path/to/meta.xml
```

## Keyboard shortcuts

| Shortcut        | Action          |
|-----------------|-----------------|
| `Ctrl+O`        | Open            |
| `Ctrl+S`        | Save            |
| `Ctrl+Shift+S`  | Save As         |

## License

*Add the license of your choice here.*
