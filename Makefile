CC = clang
CFLAGS = -Wall -Wextra -I./library
LDFLAGS = -lncurses

SRC = src/main.c \
      src/vehicule.c \
      src/configuration.c

OBJ = $(SRC:.c=.o)

TARGET = parking

.PHONY: all clean run

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(OBJ) $(LDFLAGS) -o $(TARGET)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

run: $(TARGET)
	./$(TARGET)

clean:
	rm -f $(OBJ) $(TARGET)
