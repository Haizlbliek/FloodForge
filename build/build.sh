#!/bin/bash

# read arguments
app=world
while test $# -gt 0
do
    case "$1" in
        -d) export buildmode=debug
            ;;
        --debug) export buildmode=debug
            ;;
        --dev) export buildmode=debug
            ;;
        -r) export buildmode=release
            ;;
        --release) export buildmode=release
            ;;
        --fresh) fresh=1 # clean obj directories before building
            ;;
        world) app=world
            ;;
        level) app=level
            ;;
        clean) app=clean
            ;;
        --*) echo "bad option $1"
            ;;
        *) echo "argument $1"
            ;;
    esac
    shift
done

function clean {
    rm -rf build/obj 2> /dev/null
    rm build/.depend 2> /dev/null
}

if [[ "$app" == "clean" ]]; then
    clean
    exit 0
fi

# clean obj directory when build mode/app changes
if [[ "$fresh" == 1 || ! -f build/.buildmode || "$app/$buildmode" != "$(cat build/.buildmode)" ]]; then
    echo "$app/$buildmode" > build/.buildmode
    clean
fi

if [[ "$app" == "world" ]]; then
    make -f build/world.mk
elif [[ "$app" == "level" ]]; then
    make -f build/level.mk
else
    exit 1
fi

if [ $? -eq 0 ]; then
    build/FloodForge
else
    echo "Compilation failed."
fi