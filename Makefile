.PHONY: all build run clean

BUILD_DIR := build
BINARY := TimetableCodex2
UNAME_S := $(shell uname -s)

all: build

build:
	cmake -S . -B $(BUILD_DIR)
	cmake --build $(BUILD_DIR)

run: build
	@if [ "$(UNAME_S)" = "Darwin" ]; then \
		open "$(BUILD_DIR)/$(BINARY).app"; \
	else \
		"./$(BUILD_DIR)/$(BINARY)"; \
	fi

clean:
	rm -rf $(BUILD_DIR)
