CC = gcc
CFLAGS = -g -fsanitize=address -Wall -Wextra
TARGET = Result
$(TARGET): Result.o
	$(CC) $(CFLAGS) -o $(TARGET) Result.o

Result.o: Result.c
	$(CC) $(CFLAGS) -c Result.c

clean:
	rm -f $(TARGET) Result.o
