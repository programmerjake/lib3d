#!/bin/bash
opt_arg="-O3"
if [ "$1" == "--debug" -o "$1" == "-d" ]; then
    opt_arg="-g"
fi
args="-s DISABLE_EXCEPTION_CATCHING=0 -s ALLOW_MEMORY_GROWTH=1 $opt_arg -Wall -Ilibpng -Ilibz"
libs=""
mkdir -p obj/emscripten/libpng
mkdir -p obj/emscripten/libz
for i in *.cpp; do
    echo em++ -c $args -std=c++11 "$i" -o "obj/emscripten/$i.o"
    em++ -c $args -std=c++11 "$i" -o "obj/emscripten/$i.o" || exit 1
    libs="$libs obj/emscripten/$i.o"
done
for i in libpng/*.c libz/*.c; do
    if [ "`echo "$i" | grep "/gz"`" == "" ]; then
        echo emcc -c $args "$i" -o "obj/emscripten/$i.o"
        emcc -c $args "$i" -o "obj/emscripten/$i.o" || exit 1
        libs="$libs obj/emscripten/$i.o"
    fi
done
echo em++ $args -o bin/lib3d.html $libs --embed-file test2.png
em++ $args -o bin/lib3d.html $libs --embed-file test2.png

