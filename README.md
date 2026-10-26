# FukeBoard

FukeBoard is a Qt Widgets whiteboard application for drawing, text, images,
and MileGrid boards.

## Requirements

- CMake 3.21 or newer
- A C++20-capable compiler
- Qt 6.3 or newer with the Widgets and Test development components
- Ninja (recommended)

## Build

From the repository root, configure and build an out-of-source build:

```sh
cmake -S . -B build -G Ninja
cmake --build build
```

Run the application:

```sh
./build/FukeBoard
```

If CMake cannot find your Qt installation, pass its installation prefix while
configuring. For example, with a Qt SDK kit installed under `~/Qt`:

```sh
cmake -S . -B build -G Ninja \
  -DCMAKE_PREFIX_PATH="$HOME/Qt/6.11.2/gcc_64"
cmake --build build
```

Qt Creator can also open `CMakeLists.txt` directly. Select a Qt 6 desktop kit
when prompted, then configure and build from the IDE.

## Tests

Tests are enabled by default. After building, run them with CTest:

```sh
ctest --test-dir build --output-on-failure
```

## Continuous builds

Every push and pull request runs the GitHub Actions workflow in
`.github/workflows/appimage.yml`. It builds and tests FukeBoard on Ubuntu,
then uploads a `FukeBoard-x86_64.AppImage` artifact on the workflow run's
**Summary** page. Artifacts are retained for 30 days.

## Quick user guide

- Double-click the canvas and start typing to add text.
- Press the middle mouse button to pan.
- Use the mouse wheel to zoom in or out.
- Tool modes use the shortcuts `Ctrl+1`, `Ctrl+2`, and `Ctrl+3`.
