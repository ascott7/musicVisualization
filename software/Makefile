CXX = clang++
CXXFLAGS = -g -std=c++11 -Wall -Wextra -pedantic

TARGETS=wav_reader_test fft_test visualize

all: $(TARGETS)

wav_reader_test: wav_reader.o wav_reader_test.cpp
	$(CXX) $(CXXFLAGS) -o $@ $^

fft_test: fft_test.cpp fft.hpp
	$(CXX) $(CXXFLAGS) -O3 -o $@ $< -lm

visualize: visualize.o frame.o wav_reader.o
	$(CXX) $(CXXFLAGS) -O3 -o $@ $^

test: fft_test
	./fft_test

clean:
	rm -f $(TARGETS) *.o


wav_reader.o: wav_reader.hpp wav_reader.cpp
frame.o: frame.cpp frame.hpp
visualize.o: visualize.cpp
