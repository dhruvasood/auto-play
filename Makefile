CC=g++ 
CFLAGS=-g -c -Wall -std=c++11 -I.
CXXFLAGS=-std=c++11 -static

all: animation

animation: main.o bmp.o
	$(CC)  main.o bmp.o -o animation

main.o: main.cpp 
	$(CC) $(CFLAGS) $(CXXFLAGS) main.cpp
bmp.o: bmp.cpp
	$(CC) $(CFLAGS) $(CXXFLAGS) bmp.cpp 

install:
	cp ./animation /usr/local/bin/
clean:
	rm -rf *o animation
