CC = gcc
CFLAGS = -Wall -Wextra -O2
LIBS = -lraylib -lGL -lm -lpthread -ldl -lrt -lX11
TARGET = build/emergence

all: $(TARGET)

$(TARGET): src/main.c
	mkdir -p build
	$(CC) $(CFLAGS) src/main.c -o $(TARGET) $(LIBS)

clean:
	rm -f $(TARGET)

.PHONY: all clean
