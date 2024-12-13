CC = gcc
CFLAGS = -Wall -Wextra -O2
TARGET = download
SRC = protocol.c

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC)

clean:
	rm -f $(TARGET)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC)

.PHONY: all clean