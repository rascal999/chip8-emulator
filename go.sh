#!/bin/bash

gcc chip-8.c -o chip-8 -I /usr/include/SDL/ `sdl-config --cflags --libs`
