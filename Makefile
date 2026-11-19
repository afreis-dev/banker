CFLAGS ?= -std=c11 -Wall -Wextra -Wpedantic -O2
DEPFLAGS = -MMD -MP
SANITIZE_FLAGS = -std=c11 -Wall -Wextra -Wpedantic -g -O1 -fno-omit-frame-pointer -fsanitize=address,undefined

TARGET = banker
SOURCES = $(wildcard src/*.c)
OBJECTS = $(patsubst src/%.c,build/%.o,$(SOURCES))

.PHONY: all clean test sanitize

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CC) $(CFLAGS) $(OBJECTS) -o $@ $(LDFLAGS)

build/%.o: src/%.c | build
	$(CC) $(CFLAGS) $(DEPFLAGS) -c $< -o $@

build:
	mkdir -p build

test: $(TARGET)
	sh tests/run.sh

sanitize:
	$(MAKE) clean
	$(MAKE) CFLAGS="$(SANITIZE_FLAGS)" test
	$(MAKE) clean

clean:
	rm -rf build $(TARGET)

-include $(OBJECTS:.o=.d)
