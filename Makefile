# tplay build driver.
#
# Targets:
#   make help                this help
#   make [ARCH=..]           configure + build (default target)
#   make configure           configure the CMake preset for ARCH into BUILD_DIR
#   make build               build via CMake into BUILD_DIR
#   make install             assemble the bundle with all linked deps
#            [INSTALL_PREFIX=]  (default: dist/)
#   make clean               remove BUILD_DIR and the bundle
#
# Variables:
#   ARCH           native | arm64    (default: native)
#   BUILD_DIR      build dir (default: build/)
#   BUILD_TYPE     debug | release (default: release)
#   INSTALL_PREFIX bundle output dir (default: dist/)
#
# The defaults are arch-agnostic: a single build goes into build/ and the
# bundle into dist/, whatever ARCH it is. When several ARCHs need to coexist
# (e.g. native + arm64), give each a distinct name, e.g.
# BUILD_DIR=build/native INSTALL_PREFIX=dist/tplay-native.
#
# Example:
#   make configure ARCH=arm64
#   make build ARCH=arm64
#   make install ARCH=arm64 INSTALL_PREFIX=/opt/tplay-arm64

ARCH ?= native
BUILD_DIR ?= $(CURDIR)/build
INSTALL_PREFIX ?= $(CURDIR)/dist
BUILD_TYPE = Release

BUILD_FILE = $(BUILD_DIR)/build.ninja

.PHONY: help all configure build bundle install clean

all: build

help:
	@echo "tplay build driver"
	@echo
	@echo "Targets:"
	@echo "  make [ARCH=..]                  configure + build (default)"
	@echo "  make configure                  configure CMake preset into BUILD_DIR"
	@echo "  make build                      build via CMake into BUILD_DIR"
	@echo "  make install [INSTALL_PREFIX=]  bundle binary + all linked deps"
	@echo "  make clean                      remove BUILD_DIR and the bundle"
	@echo
	@echo "Variables:"
	@echo "  ARCH           native | arm64   (default: native)"
	@echo "  BUILD_DIR      build dir (default: build/)"
	@echo "  BUILD_TYPE     debug | release (default: release)"
	@echo "  INSTALL_PREFIX bundle output dir (default: dist/)"
	@echo
	@echo "Example:"
	@echo "  make install ARCH=arm64 INSTALL_PREFIX=/opt/tplay-arm64"
	@echo "  make build ARCH=arm64 BUILD_DIR=build/arm64"

configure:
	cmake --preset $(ARCH) -B $(BUILD_DIR) -DCMAKE_BUILD_TYPE=$(BUILD_TYPE)

$(BUILD_FILE): configure
	@true

build: $(BUILD_FILE)
	cmake --build $(BUILD_DIR)

bundle: install

install: build
	scripts/bundle.sh $(ARCH) $(INSTALL_PREFIX) $(BUILD_DIR)

clean:
	rm -rf $(BUILD_DIR) $(INSTALL_PREFIX)