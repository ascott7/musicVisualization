__FLAGS = -g -Wall -Wextra -pedantic -O3

CXX = clang++
CXXFLAGS = $(__FLAGS) -std=c++11
CC=clang
CFLAGS= $(__FLAGS) -std=c99

TARGETS=wav_reader_test fft_test scrolling_fft fft_test2 reset one_frame

export MAKEFLAGS="-j 4"

all: $(TARGETS)

wav_reader_test: wav_reader.o wav_reader_test.cpp
	$(CXX) $(CXXFLAGS) -o $@ $^

fft_test: fft_test.cpp fft.hpp util.hpp
	$(CXX) $(CXXFLAGS) -o $@ $< -lm

# clang doesn't want an hpp and a .o file both at once, but the hpp is a
# dependency so we can't use $^ (there's probably a cleaner way to do this)
fft_test2: fft_test2.cpp wav_reader.o fft.hpp
	$(CXX) $(CXXFLAGS) -o $@ fft_test2.cpp wav_reader.o

scrolling_fft: scrolling_fft.cpp frame.o wav_reader.o piHelpers.o
	$(CXX) $(CXXFLAGS) -o $@ $^

reset: reset.cpp piHelpers.o
	$(CXX) $(CXXFLAGS) -o $@ $^

one_frame: one_frame.cpp piHelpers.o frame.o wav_reader.o
	$(CXX) $(CXXFLAGS) -o $@ $^

test: fft_test
	./fft_test

clean:
	rm -f $(TARGETS) *.o

wav_reader.o: wav_reader.hpp wav_reader.cpp
frame.o: frame.hpp frame.cpp fft.hpp util.hpp
piHelpers.o: piHelpers.c piHelpers.h
