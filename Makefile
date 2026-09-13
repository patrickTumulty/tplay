# tplay build driver.
#
# Targets:
#   make help                this help
#   make [ARCH=..]           configure + build (default target)
#   make configure           run the CMake preset configure for ARCH
#   make build               build via the CMake preset
#   make install             assemble the bundle with all linked deps
#            [INSTALL_PREFIX=]  (default: dist/tplay-$$(ARCH))
#   make clean               remove build/<ARCH> and the bundle for ARCH
#
# Variables:
#   ARCH           native | arm64    (default: native)
#   INSTALL_PREFIX bundle output dir (default: dist/tplay-$(ARCH))
#
# Example:
#   make configure ARCH=arm64
#   make build ARCH=arm64
#   make install ARCH=arm64 INSTALL_PREFIX=/opt/tplay-arm64

ARCH ?= native
INSTALL_PREFIX ?= $(CURDIR)/dist/tplay-$(ARCH)

BUILD_DIR = build/$(ARCH)
BUILD_FILE = $(BUILD_DIR)/build.ninja

.PHONY: help all configure build bundle install clean

all: build

help:
	@echo "tplay build driver"
	@echo
	@echo "Targets:"
	@echo "  make [ARCH=..]                  configure + build (default)"
	@echo "  make configure                  run CMake preset configure"
	@echo "  make build                      build via CMake preset"
	@echo "  make install [INSTALL_PREFIX=]  bundle binary + all linked deps"
	@echo "  make clean                      remove build/<ARCH> and its bundle"
	@echo
	@echo "Variables:"
	@echo "  ARCH           native | arm64   (default: native)"
	@echo "  INSTALL_PREFIX bundle output dir (default: dist/tplay-$(ARCH))"
	@echo
	@echo "Example:"
	@echo "  make install ARCH=arm64 INSTALL_PREFIX=/opt/tplay-arm64"

all: build

configure:
	cmake --preset $(ARCH)

$(BUILD_FILE): configure
	@true

build: $(BUILD_FILE)
	cmake --build --preset $(ARCH)

bundle: install

install: build
	scripts/bundle.sh $(ARCH) $(INSTALL_PREFIX)

clean:
	rm -rf $(BUILD_DIR) dist/tplay-$(ARCH)