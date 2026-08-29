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

*(Add a screenshot here if you like.)*

## Build requirements

- A C compiler (`gcc` recommended)
- [GTK+ 3](https://www.gtk.org/)
- [libxml2](http://xmlsoft.org/)
- `pkg-config`

On Debian/Ubuntu:

```sh
sudo apt install build-essential pkg-config libgtk-3-dev libxml2-dev
```

## Build & install

```sh
make            # compile the binary
sudo make install   # install to /usr/local
```

`make install` installs:

- `/usr/local/bin/wii-meta-editor` (the binary)
- `/usr/local/share/applications/io.homebrew.WiiMetaEditor.desktop`
- `/usr/local/share/icons/hicolor/scalable/apps/io.homebrew.WiiMetaEditor.svg`

### Other make targets

| Command              | Description                                      |
|----------------------|--------------------------------------------------|
| `make` / `make all`  | Compile the binary from `src/`                   |
| `make install`       | Install binary, desktop entry and icon (root)    |
| `make uninstall`     | Remove installed files (root)                    |
| `make clean`         | Remove build artifacts and the binary            |

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
