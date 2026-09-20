# tplay

A work-in-progress terminal video viewer. `tplay` receives an H.265
MPEG-TS stream over UDP, decodes it with the NVIDIA `nvh265dec` GStreamer
decoder, and displays it in a full-screen ncurses interface (video window
plus status bar).

## Build status

Passing.

Native (x86_64) and arm64 cross builds both configure, compile, and link
successfully in the devcontainer, and `make install` assembles a runnable
bundle for each. The CI pipeline builds both architectures and uploads the
bundles from `dist/`.

Current working tree was verified with:

- `make build ARCH=native` / `make install ARCH=native`
- `cmake --preset arm64` + `make build ARCH=arm64 BUILD_DIR=build/arm64`

Note: the project is a work in progress. See [Status](#status) for what is
and is not implemented yet.

## Pipeline

```
udpsrc → tsdemux → h265parse → nvh265dec → videoconvert → capsfilter → appsink
```

The video source is selected from the command line:

```sh
tplay udp://localhost:5000        # or udp://<ip>:<port>
```

## Building

The devcontainer (`make configure` requires CMake + Ninja, GStreamer dev
packages, and ncurses headers) provides everything needed. The arm64 side
is cross-compiled with an aarch64 toolchain and a GStreamer built from
source into `/opt/gst/arm64` inside the devcontainer.

The Makefile is the build driver and wraps the CMake presets:

```sh
make                     # configure + build native (Release) into build/
make configure ARCH=arm64
make build ARCH=arm64
make install ARCH=arm64  # assemble the bundle
```

Make variables (overridable on the command line):

| Variable        | Default    | Purpose                         |
|-----------------|------------|---------------------------------|
| `ARCH`          | `native`   | `native` or `arm64`             |
| `BUILD_DIR`     | `build/`   | CMake build directory           |
| `INSTALL_PREFIX`| `dist/`    | Bundle output directory         |
| `BUILD_TYPE`    | `Release`  | CMake build type                |

The defaults are arch-agnostic: a single build always lands in `build/`
and the bundle in `dist/`. To keep multiple architectures in the same
workspace, give each its own names, e.g. for the CI workflow:

```sh
make build   ARCH=native BUILD_DIR=build/native
make install ARCH=native BUILD_DIR=build/native INSTALL_PREFIX=dist/tplay-native
make build   ARCH=arm64  BUILD_DIR=build/arm64
make install ARCH=arm64  BUILD_DIR=build/arm64  INSTALL_PREFIX=dist/tplay-arm64
```

Clean up with `make clean` (removes `BUILD_DIR` and the bundle).

## Running

Send a test stream (requires GStreamer tools with `x265enc`, e.g. the
`gstreamer1.0-plugins-bad` package):

```sh
scripts/start_test_video.sh --port 5000 --resolution 720
```

Then run the bundled viewer:

```sh
dist/run.sh udp://localhost:5000
```

The bundle is a self-contained directory copyable onto a target machine:

```
<prefix>/bin/tplay
<prefix>/lib/*.so*                dependency closure (NEEDED)
<prefix>/lib/gstreamer-1.0/*.so   pipeline plugins
<prefix>/run.sh                   launcher (sets LD_LIBRARY_PATH etc.)
```

`run.sh` sets `LD_LIBRARY_PATH`, `GST_PLUGIN_PATH`, and execs
`<prefix>/bin/tplay`.

## Status

Implemented so far:

- UDP MPEG-TS ingest, H.265 parse, NVH265 decode, colorspace conversion
- app sink frame delivery (resolution/stride/format logged)
- ncurses UI with video and status windows (resize handling, ESC to quit)
- spdlog logging (GStreamer debug redirected into spdlog)
- Native + arm64 cross builds and dependency bundling

Not yet implemented:

- Drawing the decoded video into the video window (pixel mapping is stubbed)
- Runtime config beyond the `udp://` source argument
- The pipeline currently runs for a fixed interval before tearing down

## Layout

```
Makefile                       build driver
CMakePresets.json              native / arm64 configure+build presets
cmake/toolchains/              aarch64 cross toolchain
scripts/bundle.sh              assemble the dependency bundle
scripts/start_test_video.sh    generate a test H.265/UDP stream
src/                           application source
extern/spdlog                  vendored spdlog
.devcontainer/                 dev container (includes arm64 GStreamer build)
.github/workflows/build.yml    CI: build + bundle native & arm64
```