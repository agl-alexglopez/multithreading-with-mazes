.PHONY: default gcc-rel gcc-deb arm-rel arm-deb clang-rel clang-deb build format tidy clean

MAKE := $(MAKE)
MAKEFLAGS += --no-print-directory
# Adjust parallel build jobs based on your available cores. 
# Try linux environment first then applex86 or M1, then give up and just do one
JOBS ?= $(shell (command -v nproc > /dev/null 2>&1 && echo "-j$$(nproc)") || (command -v sysctl -n hw.ncpu > /dev/null 2>&1 && echo "-j$$(sysctl -n hw.ncpu)") || echo "")

default: build

gcc-rel:
	cmake --preset=gcc-rel
	cmake --build build/ $(JOBS)

gcc-deb:
	cmake --preset=gcc-deb
	cmake --build build/ $(JOBS)

arm-rel:
	cmake --preset=arm-rel
	cmake --build build/ $(JOBS)

arm-deb:
	cmake --preset=arm-deb
	cmake --build build/ $(JOBS)

clang-rel:
	cmake --preset=clang-rel
	cmake --build build/ $(JOBS)

clang-deb:
	cmake --preset=clang-deb
	cmake --build build/ $(JOBS)

build:
	cmake --build build/ $(JOBS)

format:
	cmake --build build/ --target format

tidy:
	cmake --build build/ --target tidy $(JOBS)

clean:
	rm -rf build/
