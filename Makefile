.PHONY: default gcc-release gcc-debug arm-release arm-debug clang-release clang-debug build format tidy clean

MAKE := $(MAKE)
# Adjust parallel build jobs based on your available cores. 
# Try linux environment first then applex86 or M1, then give up and use default
# in case the build generator has some other way of managing default parallelism and specify nothing.
JOBS ?= $(shell (command -v nproc > /dev/null 2>&1 && echo -n "-j$$(nproc)") || (command -v sysctl -n hw.ncpu > /dev/null 2>&1 && echo -n "-j$$(sysctl -n hw.ncpu)") || echo -n "")

default: build

gcc-release:
	cmake --preset=gcc-release
	$(MAKE) --no-print-directory -C build/ $(JOBS)

gcc-debug:
	cmake --preset=gcc-debug
	$(MAKE) --no-print-directory -C build/ $(JOBS)

arm-release:
	cmake --preset=arm-release
	$(MAKE) --no-print-directory -C build/ $(JOBS)

arm-debug:
	cmake --preset=arm-debug
	$(MAKE) --no-print-directory -C build/ $(JOBS)

clang-release:
	cmake --preset=clang-release
	$(MAKE) --no-print-directory -C build/ $(JOBS)

clang-debug:
	cmake --preset=clang-debug
	$(MAKE) --no-print-directory -C build/ $(JOBS)

build:
	$(MAKE) --no-print-directory -C build/ $(JOBS)

format:
	$(MAKE) --no-print-directory -C build/ format

tidy:
	$(MAKE) --no-print-directory -C build/ tidy

clean:
	rm -rf build/
