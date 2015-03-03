PROGRAM=cxxgzip
DEPS=deflate.hpp
OBJS=bitinput.o bitoutput.o crc32.o decoder.o encoder.o gunzip.o\
 gzip.o huffcanonical.o huffsize.o hufftree.o lzss.o main.o

CXX=c++
CPPFLAGS=-I.
CXXFLAGS=-std=c++11 -Wall -O2
LDFLAGS=-std=c++11

all : $(PROGRAM)

$(PROGRAM) : $(OBJS)
	$(CXX) $(LDFLAGS) -o $(PROGRAM) $(OBJS)

#%.o : %.cpp $(DEPS)
#	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $<

bitinput.o : bitinput.cpp $(DEPS)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c bitinput.cpp

bitoutput.o : bitoutput.cpp $(DEPS)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c bitoutput.cpp

crc32.o : crc32.cpp $(DEPS)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c crc32.cpp

decoder.o : decoder.cpp $(DEPS)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c decoder.cpp

encoder.o : encoder.cpp $(DEPS)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c encoder.cpp

gunzip.o : gunzip.cpp $(DEPS)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c gunzip.cpp

gzip.o : gzip.cpp $(DEPS)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c gzip.cpp

huffcanonical.o : huffcanonical.cpp $(DEPS)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c huffcanonical.cpp

huffsize.o : huffsize.cpp $(DEPS)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c huffsize.cpp

hufftree.o : hufftree.cpp $(DEPS)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c hufftree.cpp

lzss.o : lzss.cpp $(DEPS)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c lzss.cpp

main.o : main.cpp $(DEPS)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c main.cpp

clean :
	rm -f $(PROGRAM) $(OBJS)

.PHONY: clean
