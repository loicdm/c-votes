CC=gcc
CCFLAGS=-Wall -Wextra -O2 -ggdb -g

build: src/main.c builddir targetdir
	cd build && $(CC) $(CCFLAGS) -o ../target/main ../src/main.c

builddir:
	mkdir -p build

targetdir:
	mkdir -p target

clean:
	rm -rf build target
