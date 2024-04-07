.PHONY: default deb rel build format tidy clean

MAKE := $(MAKE)
MAKEFLAGS += --no-print-directory
# Adjust parallel build jobs based on your available cores. 
# Try linux environment first then applex86 or M1, then give up and just do one
JOBS ?= $(shell (command -v nproc > /dev/null 2>&1 && echo "-j$$(nproc)") || (command -v sysctl -n hw.ncpu > /dev/null 2>&1 && echo "-j$$(sysctl -n hw.ncpu)") || echo "")

default: build

build:
	cmake --build build/ $(JOBS)

rel:
	cmake --preset=rel
	$(MAKE) build

deb:
	cmake --preset=deb
	$(MAKE) build

format:
	cmake --build build/ --target format

tidy:
	cmake --build build/ --target tidy $(JOBS)

clean:
	rm -rf build/
