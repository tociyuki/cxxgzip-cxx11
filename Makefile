PROGRAM=cxxgzip
DEPS=deflate.hpp
OBJS=bitinput.o bitoutput.o crc32.o decoder.o encoder.o gunzip.o\
 gzip.o huffcanonical.o huffsize.o hufftree.o lzss.o main.o

CXX=c++
CXXFLAGS=-std=c++11 -Wall -O2
CPPFLAGS=-I.
LDFLAGS=-std=c++11

.PHONY: all clean

all : $(PROGRAM)

$(PROGRAM) : $(OBJS)
	$(CXX) $(LDFLAGS) -o $(PROGRAM) $(OBJS)

#%.o : %.cpp $(DEPS)
#	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $<

bitinput.o : bitinput.cpp $(DEPS)
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) -c bitinput.cpp

bitoutput.o : bitoutput.cpp $(DEPS)
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) -c bitoutput.cpp

crc32.o : crc32.cpp $(DEPS)
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) -c crc32.cpp

decoder.o : decoder.cpp $(DEPS)
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) -c decoder.cpp

encoder.o : encoder.cpp $(DEPS)
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) -c encoder.cpp

gunzip.o : gunzip.cpp $(DEPS)
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) -c gunzip.cpp

gzip.o : gzip.cpp $(DEPS)
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) -c gzip.cpp

huffcanonical.o : huffcanonical.cpp $(DEPS)
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) -c huffcanonical.cpp

huffsize.o : huffsize.cpp $(DEPS)
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) -c huffsize.cpp

hufftree.o : hufftree.cpp $(DEPS)
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) -c hufftree.cpp

lzss.o : lzss.cpp $(DEPS)
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) -c lzss.cpp

main.o : main.cpp $(DEPS)
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) -c main.cpp

clean :
	rm -f $(PROGRAM) $(OBJS)

