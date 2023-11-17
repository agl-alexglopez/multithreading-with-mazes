.PHONY: default release debug arm-release arm-debug build format tidy clean

MAKE := $(MAKE)
# Adjust parallel build jobs based on your available cores. 
# Try linux environment first then applex86 or M1, then give up and just do one
JOBS ?= $(shell (command -v nproc > /dev/null 2>&1 && nproc) || (command -v sysctl -n hw.ncpu > /dev/null 2>&1 && sysctl -n hw.ncpu) || echo 1)

default: build

release:
	@cmake --preset=gcc-release
	@$(MAKE) --no-print-directory -C build/ -j$(JOBS)

debug:
	@cmake --preset=gcc-debug
	@$(MAKE) --no-print-directory -C build/ -j$(JOBS)

arm-release:
	@cmake --preset=arm-release
	@$(MAKE) --no-print-directory -C build/ -j$(JOBS)

arm-debug:
	@cmake --preset=arm-debug
	@$(MAKE) --no-print-directory -C build/ -j$(JOBS)

build:
	@$(MAKE) --no-print-directory -C build/ -j$(JOBS)

format:
	@$(MAKE) --no-print-directory -C build/ format

tidy:
	@$(MAKE) --no-print-directory -C build/ tidy

clean:
	rm -rf build/
