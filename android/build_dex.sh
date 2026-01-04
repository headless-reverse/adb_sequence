#!/bin/bash

SDK_PATH="/home/phantom/Android/Sdk"
ANDROID_JAR="$SDK_PATH/platforms/android-33/android.jar"
D8_PATH="$SDK_PATH/build-tools/30.0.3/d8"
JAR_NAME="sequence.jar"

rm -rf build
mkdir -p build

echo "--- Kompilacja Java ---"

javac -source 8 -target 8 \
      -cp "$ANDROID_JAR" \
      -d build \
      *.java

if [ $? -ne 0 ]; then
    echo "Błąd kompilacji javac!"
    exit 1
fi

echo "--- Konwersja D8 (DEX) ---"
$D8_PATH build/dev/headless/sequence/*.class \
    --lib "$ANDROID_JAR" \
    --output $JAR_NAME \
    --min-api 26

if [ $? -eq 0 ]; then
    echo "--------------------------------------------"
    echo "--- SUKCES! Plik: $JAR_NAME gotowy ---"
    echo "--------------------------------------------"
    echo "    adb push $JAR_NAME /data/local/tmp/"
    echo "--------------------------------------------"
    echo "    adb shell \"CLASSPATH=/data/local/tmp/$JAR_NAME app_process /data/local/tmp dev.headless.sequence.Server\""
else
    echo "Błąd d8!"
    exit 1
fi
