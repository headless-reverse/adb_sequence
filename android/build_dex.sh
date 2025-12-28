#!/bin/bash

# Konfiguracja ścieżek
SDK_PATH="/home/phantom/Android/Sdk"
ANDROID_JAR="$SDK_PATH/platforms/android-33/android.jar"
D8_PATH="$SDK_PATH/build-tools/30.0.3/d8"
PACKAGE_PATH="dev/headless/sequence"
JAR_NAME="sequence.jar"

echo "--- Przygotowanie struktury ---"
mkdir -p build/$PACKAGE_PATH

echo "--- Kompilacja Java (wszystkie moduły) ---"
javac -source 8 -target 8 \
      -cp "$ANDROID_JAR" \
      -d build \
      *.java

if [ $? -ne 0 ]; then
    echo "Błąd kompilacji javac!"
    exit 1
fi

echo "--- Konwersja D8 (DEX) ---"
CLASS_FILES=$(find build -name "*.class")

$D8_PATH $CLASS_FILES \
    --lib "$ANDROID_JAR" \
    --output $JAR_NAME \
    --min-api 26

if [ $? -eq 0 ]; then
    echo "--------------------------------------------"
    echo "--- SUKCES! Plik: $JAR_NAME gotowy ---"
    echo "--------------------------------------------"
    echo "1. Prześlij na telefon:"
    echo "   adb push $JAR_NAME /data/local/tmp/"
    echo ""
    echo "2. Uruchom serwer (pamiętaj o CLASSPATH):"
    echo "   adb shell \"CLASSPATH=/data/local/tmp/$JAR_NAME app_process / dev.headless.sequence.Server\""
else
    echo "Błąd d8!"
    exit 1
fi
