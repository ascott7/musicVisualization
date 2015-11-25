CXX = clang++
CXXFLAGS = -g -std=c++11 -Wall -Wextra -pedantic -O3

TARGETS=wav_reader_test fft_test visualize fft_test2

#export MAKEFLAGS="-j 8

all: $(TARGETS)

wav_reader_test: wav_reader.o wav_reader_test.cpp
	$(CXX) $(CXXFLAGS) -o $@ $^

fft_test: fft_test.cpp fft.hpp
	$(CXX) $(CXXFLAGS) -o $@ $< -lm

fft_test2: wav_reader.o fft.hpp fft_test2.cpp
	$(CXX) $(CXXFLAGS) -o $@ $^

visualize: visualize.cpp frame.o wav_reader.o piHelpers.o
	$(CXX) $(CXXFLAGS) -o $@ $^

test: fft_test
	./fft_test

clean:
	rm -f $(TARGETS) *.o


wav_reader.o: wav_reader.hpp wav_reader.cpp
frame.o: frame.hpp frame.cpp
piHelpers.o: piHelpers.h piHelpers.c
#visualize.o: visualize.cpp
