CC = gcc
CFLAGS = -Wall -Wextra -O2
LIBS = -lraylib -lGL -lm -lpthread -ldl -lrt -lX11
TARGET = build/emergence
SRC = src/main.c src/game.c src/ui.c

all: $(TARGET)

$(TARGET): $(SRC) src/game.h src/ui.h
	mkdir -p build
	$(CC) $(CFLAGS) $(SRC) -o $(TARGET) $(LIBS)

clean:
	rm -f $(TARGET)

.PHONY: all clean
