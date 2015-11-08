CC = clang
CFLAGS = -g -std=c11 -Wall -Wextra -pedantic

fft.o: fft.c fft.h
	$(CC) $(CFLAGS) -c $< -o $@

fft_test: fft_test.c fft.o
	$(CC) $(CFLAGS) -o $@ $^ -lm

test: fft_test
	./fft_test
