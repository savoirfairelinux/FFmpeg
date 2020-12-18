#!/bin/bash
#
# Script to build ffmpeg as needed

set -e

export MAKEFLAGS="-j$[$(nproc) + 1]"

./configure --extra-libs=-ldl \
        --enable-gpl --enable-libx264 \
        --disable-ffplay --disable-ffprobe \
        --enable-small --disable-stripping --disable-debug

make
