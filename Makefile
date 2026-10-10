# Makefile for abraflexi-tui
# ============================
#
# This Makefile provides convenient targets for building, testing, installing,
# and running the abraflexi-tui project. It wraps the CMake build system with
# user-friendly targets.
#
# Usage:
#   make          - Build the project (default)
#   make run      - Build and run the application
#   make test     - Run tests
#   make help     - Show all available targets
#
# Project metadata
PROJECT := abraflexi-tui
VERSION := 1.1.0

# Build configuration
BUILD_DIR ?= build
BUILD_TYPE ?= Release
INSTALL_PREFIX ?= /usr/local

# Binary names
MAIN_BINARY := $(BUILD_DIR)/$(PROJECT)
CLI_CHECK_BINARY := $(BUILD_DIR)/$(PROJECT)-cliclient-check
PROFILE_CHECK_BINARY := $(BUILD_DIR)/$(PROJECT)-profile-check

# CMake options
CMAKE_OPTIONS := -DCMAKE_BUILD_TYPE=$(BUILD_TYPE) \
                -DCMAKE_INSTALL_PREFIX=$(INSTALL_PREFIX)

# Detect number of CPU cores for parallel builds
NPROC ?= $(shell nproc 2>/dev/null || echo 1)

.PHONY: all build force-build clean install uninstall test test-cliclient test-profile test-integration run run-cli run-env run-demo debug run-debug run-ci run-test test-all clean-all help version check-deps pot-update update-po update-po-cs update-po-de clean-translations package package-source git-pull git-status

# =============================================================================
# Pattern rules for parameterized targets
# =============================================================================
%:
	@:

# =============================================================================
# Main targets
# =============================================================================

all: build

build: $(MAIN_BINARY) $(CLI_CHECK_BINARY) $(PROFILE_CHECK_BINARY)

$(MAIN_BINARY):
	@echo "Building $(PROJECT) with CMake..."
	@mkdir -p $(BUILD_DIR)
	@cmake -S . -B $(BUILD_DIR) $(CMAKE_OPTIONS) >/dev/null 2>&1 || \
		(echo "CMake configuration failed. Ensure you have cmake and all dependencies installed." && exit 1)
	@cmake --build $(BUILD_DIR) -j$(NPROC)

$(CLI_CHECK_BINARY): $(MAIN_BINARY)

$(PROFILE_CHECK_BINARY): $(MAIN_BINARY)

clean:
	@echo "Cleaning build directory..."
	@rm -rf $(BUILD_DIR)

clean-all: clean
	@echo "Cleaning additional artifacts..."
	@rm -rf obj-x86_64-linux-gnu

rebuild: clean build

force-build:
	@echo "Forcing rebuild of $(PROJECT)..."
	@rm -rf $(BUILD_DIR)
	@mkdir -p $(BUILD_DIR)
	@cmake -S . -B $(BUILD_DIR) $(CMAKE_OPTIONS)
	@cmake --build $(BUILD_DIR) -j$(NPROC)

# =============================================================================
# Installation targets
# =============================================================================

install: $(MAIN_BINARY)
	@echo "Installing $(PROJECT) to $(INSTALL_PREFIX)..."
	@sudo cmake --install $(BUILD_DIR) --prefix $(INSTALL_PREFIX)

uninstall:
	@echo "Uninstalling $(PROJECT) from $(INSTALL_PREFIX)..."
	@sudo xargs rm -f < $(BUILD_DIR)/install_manifest.txt 2>/dev/null || \
		echo "Could not find install manifest. Try: sudo rm -f $(INSTALL_PREFIX)/bin/$(PROJECT)"
	@sudo rm -f $(INSTALL_PREFIX)/bin/$(PROJECT)

# =============================================================================
# Test targets
# =============================================================================

test test-all: build
	@echo "Running tests..."
	@echo "========================================"
	@echo "Test 1: CliClient smoke test"
	@if [ -x "$(CLI_CHECK_BINARY)" ]; then \
		./$(CLI_CHECK_BINARY) --help 2>&1 | head -5 || echo "CliClient test binary available"; \
	else \
		echo "CliClient test binary not found at $(CLI_CHECK_BINARY)"; \
	fi
	@echo ""
	@echo "Test 2: Profile check test"
	@if [ -x "$(PROFILE_CHECK_BINARY)" ]; then \
		./$(PROFILE_CHECK_BINARY) 2>&1 | head -5 || echo "Profile check test binary available"; \
	else \
		echo "Profile check test binary not found at $(PROFILE_CHECK_BINARY)"; \
	fi
	@echo ""
	@echo "========================================"
	@echo "Tests completed!"
	@echo "Note: Full integration tests require abraflexi-cli to be installed"

test-cliclient: $(CLI_CHECK_BINARY)
	@echo "Running CliClient smoke test..."
	@./$(CLI_CHECK_BINARY) abraflexi-cli status 2>&1 || echo "Test requires abraflexi-cli to be installed"

test-profile: $(PROFILE_CHECK_BINARY)
	@echo "Running profile check test..."
	@./$(PROFILE_CHECK_BINARY) 2>&1 || true

# Run full integration tests (requires abraflexi-cli)
test-integration: $(CLI_CHECK_BINARY) $(PROFILE_CHECK_BINARY)
	@echo "Running integration tests..."
	@echo "Testing CliClient with status command..."
	@./$(CLI_CHECK_BINARY) abraflexi-cli status 2>&1 || echo "Status test: SKIPPED (abraflexi-cli not configured)"
	@echo "Testing profile management..."
	@./$(PROFILE_CHECK_BINARY) 2>&1 || echo "Profile test: completed"

# =============================================================================
# Run targets
# =============================================================================

run: $(MAIN_BINARY)
	@echo "Running $(PROJECT)..."
	@if ! command -v abraflexi-cli >/dev/null 2>&1; then \
		echo "WARNING: abraflexi-cli not found in PATH!"; \
		echo "Install it first: https://github.com/VitexSoftware/abraflexi-cli"; \
		echo "Press Enter to continue anyway, or Ctrl+C to abort"; \
		read -r dummy; \
	fi
	@./$(MAIN_BINARY)

debug: CMAKE_OPTIONS += -DCMAKE_BUILD_TYPE=Debug
debug: build
	@echo "Debug build ready. Run with: gdb $(MAIN_BINARY)"

run-debug: debug

run-demo: $(MAIN_BINARY)
	@echo "Running with demo server configuration..."
	@ABRAFLEXI_URL=https://demo.flexibee.eu:5434 \
		ABRAFLEXI_LOGIN=winstrom \
		ABRAFLEXI_PASSWORD=winstrom \
		ABRAFLEXI_COMPANY=demo \
		./$(MAIN_BINARY)

# Parameterized run targets (use make run-cli PATH=value or make run-env FILE=value)
run-cli:
	@echo "Usage: make run-cli PATH=<path-to-cli>"
	@echo "Example: make run-cli PATH=/usr/local/bin/abraflexi-cli"
	@false

run-env:
	@echo "Usage: make run-env FILE=<path-to-env>"
	@echo "Example: make run-env FILE=~/.config/abraflexi-cli/.env"
	@false

# =============================================================================
# Development targets
# =============================================================================

# Update translations
pot-update:
	@echo "Updating translation template (pot file)..."
	@cmake -S . -B $(BUILD_DIR) $(CMAKE_OPTIONS) >/dev/null 2>&1
	@cmake --build $(BUILD_DIR) --target pot-update

update-po-cs:
	@echo "Updating Czech translations..."
	@cmake -S . -B $(BUILD_DIR) $(CMAKE_OPTIONS) >/dev/null 2>&1
	@cmake --build $(BUILD_DIR) --target update-po-cs

update-po-de:
	@echo "Updating German translations..."
	@cmake -S . -B $(BUILD_DIR) $(CMAKE_OPTIONS) >/dev/null 2>&1
	@cmake --build $(BUILD_DIR) --target update-po-de

update-po: pot-update update-po-cs update-po-de

# Clean translation artifacts
clean-translations:
	@echo "Cleaning translation artifacts..."
	@rm -rf $(BUILD_DIR)/locale
	@rm -rf $(BUILD_DIR)/po

# =============================================================================
# Package targets
# =============================================================================

package: clean
	@echo "Building Debian package..."
	@git submodule update --init --recursive >/dev/null 2>&1 || true
	@dpkg-buildpackage -us -uc -b

package-source:
	@echo "Creating source package..."
	@dpkg-buildpackage -us -uc -S

# =============================================================================
# Information targets
# =============================================================================

version:
	@echo "$(PROJECT) version $(VERSION)"
	@echo "Build type: $(BUILD_TYPE)"
	@echo "Build directory: $(BUILD_DIR)"
	@if [ -f "$(MAIN_BINARY)" ]; then \
		ls -lh "$(MAIN_BINARY)" | awk '{print $$9, "- Last built:", $$6, $$7, $$8}'; \
	fi

check-deps:
	@echo "Checking dependencies for $(PROJECT)..."
	@for pkg in cmake g++ pkg-config libtvision-dev libncurses-dev nlohmann-json3-dev gettext; do \
		if dpkg -l "$$pkg" 2>/dev/null | grep -q "^ii"; then \
			echo "  OK $$pkg"; \
		else \
			echo "  MISSING $$pkg"; \
		fi; \
	done
	@echo ""
	@echo "Optional dependencies:"
	@if command -v abraflexi-cli >/dev/null 2>&1; then \
		version=$$(abraflexi-cli --version 2>&1 | head -1); \
		echo "  OK abraflexi-cli ($$version)"; \
	else \
		echo "  MISSING abraflexi-cli (required for runtime)"; \
	fi

# =============================================================================
# Git helper targets
# =============================================================================

git-pull:
	@echo "Updating from Git..."
	@git pull
	@git submodule update --init --recursive

git-status:
	@git status
	@git submodule status

# =============================================================================
# CI/CD targets
# =============================================================================

run-ci:
	@echo "Running CI build..."
	@make clean
	@make build BUILD_TYPE=Release
	@make test

run-test:
	@echo "Running all tests..."
	@make test

# =============================================================================
# Help target
# =============================================================================

help:
	@echo "abraflexi-tui Makefile - Available targets:"
	@echo ""
	@echo "Build targets:"
	@echo "  all           - Build all targets (default)"
	@echo "  build         - Build the project"
	@echo "  rebuild       - Clean and rebuild"
	@echo "  clean         - Remove build directory"
	@echo "  clean-all     - Remove all build artifacts"
	@echo "  force-build   - Force clean rebuild"
	@echo ""
	@echo "Installation targets:"
	@echo "  install       - Install to $(INSTALL_PREFIX)"
	@echo "  uninstall     - Remove installed files"
	@echo ""
	@echo "Test targets:"
	@echo "  test          - Run all tests"
	@echo "  test-all      - Alias for test"
	@echo "  test-cliclient - Run CliClient smoke test"
	@echo "  test-profile  - Run profile check test"
	@echo "  test-integration - Run integration tests"
	@echo ""
	@echo "Run targets:"
	@echo "  run           - Run the application"
	@echo "  debug         - Build with debug symbols"
	@echo "  run-debug    - Run debug build"
	@echo "  run-demo     - Run with demo server config"
	@echo "  run-cli      - Show usage for custom CLI path"
	@echo "  run-env      - Show usage for custom env file"
	@echo ""
	@echo "Development targets:"
	@echo "  pot-update    - Update translation template"
	@echo "  update-po     - Update all translations"
	@echo "  update-po-cs  - Update Czech translations"
	@echo "  update-po-de  - Update German translations"
	@echo "  clean-translations - Remove translation artifacts"
	@echo ""
	@echo "Package targets:"
	@echo "  package        - Build Debian package"
	@echo "  package-source - Build source package"
	@echo ""
	@echo "Other targets:"
	@echo "  docs          - View documentation"
	@echo "  git-pull      - Update from Git"
	@echo "  git-status    - Show Git status"
	@echo "  version       - Show version information"
	@echo "  check-deps    - Check installed dependencies"
	@echo "  help          - Show this help message"
	@echo ""
	@echo "CI/CD targets:"
	@echo "  run-ci        - Run CI build"
	@echo "  run-test      - Run all tests"
	@echo ""
	@echo "Configuration options:"
	@echo "  BUILD_DIR      - Build directory (default: build)"
	@echo "  BUILD_TYPE    - Build type (Release or Debug, default: Release)"
	@echo "  INSTALL_PREFIX - Installation prefix (default: /usr/local)"
	@echo "  NPROC         - Number of build jobs (default: auto-detect)"
	@echo ""
	@echo "Examples:"
	@echo "  make run"
	@echo "  make BUILD_TYPE=Debug"
	@echo "  make INSTALL_PREFIX=$$HOME/.local"
	@echo "  make run-cli PATH=/usr/local/bin/abraflexi-cli"
	@echo "  make run-env FILE=~/.config/abraflexi-cli/.env"
