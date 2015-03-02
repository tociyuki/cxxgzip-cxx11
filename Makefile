CC=c++
CFLAGS=-std=c++11 -O2
INCDIR=-I./
OBJECTS=bitinput.o bitoutput.o crc32.o decoder.o encoder.o gunzip.o \
	gzip.o huffcanonical.o huffsize.o hufftree.o lzss.o main.o

cxxgzip : deflate.hpp $(OBJECTS)
	$(CC) $(CFLAGS) -o cxxgzip $(OBJECTS)

bitinput.o : deflate.hpp bitinput.cpp
	$(CC) $(CFLAGS) -c bitinput.cpp

bitoutput.o : deflate.hpp bitoutput.cpp
	$(CC) $(CFLAGS) -c bitoutput.cpp

crc32.o : deflate.hpp crc32.cpp
	$(CC) $(CFLAGS) -c crc32.cpp

decoder.o : deflate.hpp decoder.cpp
	$(CC) $(CFLAGS) -c decoder.cpp

encoder.o : deflate.hpp encoder.cpp
	$(CC) $(CFLAGS) -c encoder.cpp

gunzip.o : deflate.hpp gunzip.cpp
	$(CC) $(CFLAGS) -c gunzip.cpp

gzip.o : deflate.hpp gzip.cpp
	$(CC) $(CFLAGS) -c gzip.cpp

huffcanonical.o : deflate.hpp huffcanonical.cpp
	$(CC) $(CFLAGS) -c huffcanonical.cpp

huffsize.o : deflate.hpp huffsize.cpp
	$(CC) $(CFLAGS) -c huffsize.cpp

hufftree.o : deflate.hpp hufftree.cpp
	$(CC) $(CFLAGS) -c hufftree.cpp

lzss.o : deflate.hpp lzss.cpp
	$(CC) $(CFLAGS) -c lzss.cpp

main.o : deflate.hpp main.cpp
	$(CC) $(CFLAGS) -c main.cpp

clean :
	rm -f $(OBJECTS) cxxgzip
