#!/bin/bash

SDK_PATH="/home/phantom/Android/Sdk"

CLANG_PATH=$(find $SDK_PATH/ndk -name "aarch64-linux-android30-clang++" | head -n 1)

if [ -z "$CLANG_PATH" ]; then
    echo "BŁĄD: Nie znaleziono kompilatorów NDK w $SDK_PATH/ndk"
    exit 1
fi

TOOLCHAIN=$(dirname "$CLANG_PATH")
echo "Znaleziono toolchain: $TOOLCHAIN"
echo "Kompiluję hardware_grabbed_64..."
$TOOLCHAIN/aarch64-linux-android30-clang++ -O3 -static-libstdc++ android_grabbed.cpp -o android_grabbed_64
echo "Kompiluję hardware_grabbed_32..."
$TOOLCHAIN/armv7a-linux-androideabi30-clang++ -O3 -static-libstdc++ android_grabbed.cpp -o android_grabbed_32
echo "Gotowe."
