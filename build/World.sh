#!/bin/bash
make -f build/world.mk

if [ $? -eq 0 ]; then
    build/FloodForge
else
    echo "Compilation failed."
fi