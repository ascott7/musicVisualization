CXX = clang++
CXXFLAGS = -g -std=c++11 -Wall -Wextra -pedantic

TARGETS=wav_reader fft_test

all: $(TARGETS)

wav_reader: wav_reader.cpp
	$(CXX) $(CXXFLAGS) -o $@ $^

fft_test: fft_test.cpp fft.hpp
	$(CXX) $(CXXFLAGS) -O3 -o $@ $< -lm

test: fft_test
	./fft_test

clean:
	rm -f $(TARGETS)
