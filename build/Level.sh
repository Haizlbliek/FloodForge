#!/bin/bash

clear

g++ src/*.cpp src/level/*.cpp src/font/*.cpp --std=c++17 -I"include/" -L"lib/GLFW/" -o FloodForge -lglfw3 -lgdi32 -lopengl32 -luser32 -lcomdlg32

if [ $? -eq 0 ]; then
    clear
    ./FloodForge
else
    echo "Compilation failed."
fi
