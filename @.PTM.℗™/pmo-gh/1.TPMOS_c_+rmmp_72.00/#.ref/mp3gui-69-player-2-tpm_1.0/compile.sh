#!/bin/bash

CFLAGS="-Wall -O2 -pthread -I/usr/include/freetype2"
LIBS="-lm -lssl -lcrypto -lGL -lGLU -lglut -lfreetype -lavcodec -lavformat -lavutil -lswscale -lswresample -lpulse -lpulse-simple -lX11"

echo "Compiling yingyang-find..."
gcc $CFLAGS yingyang-find.c -o yingyang-find $LIBS

echo "Compiling yingyang-cli..."
gcc $CFLAGS yingyang-cli.c -o yingyang-cli $LIBS

echo "Compiling yingyang-gui..."
gcc $CFLAGS yingyang-gui.c -o yingyang-gui $LIBS

echo "Compilation complete."
chmod +x yingyang-find yingyang-cli yingyang-gui
