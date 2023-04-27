#!/bin/sh
exec make -j`sysctl -n hw.logicalcpu` "$@"
