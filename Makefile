all:
	gcc -ggdb -Wall chip-8.c -o chip-8 -I /usr/include/SDL/ `sdl-config --cflags --libs` -std=c99

clean:
	rm -rf chip-8
