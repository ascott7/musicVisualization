CC = clang
CXX = clang++
CFLAGS = -g -std=c11 -Wall -Wextra -pedantic
CXXFLAGS = -g -std=c++11 -Wall -Wextra -pedantic

wav_reader: wav_reader.cpp
	$(CXX) $(CXXFLAGS) -o $@ $^

fft.o: fft.c fft.h
	$(CC) $(CFLAGS) -c $< -o $@

fft_test: fft_test.c fft.o
	$(CC) $(CFLAGS) -o $@ $^ -lm

test: fft_test
	./fft_test
