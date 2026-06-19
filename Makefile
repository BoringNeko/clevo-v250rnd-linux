CC      := gcc
CFLAGS  := -Wall -Wextra -O2
TARGET  := perf_mode

.PHONY: all clean install

all: $(TARGET)

$(TARGET): perf_mode.c
	$(CC) $(CFLAGS) -o $@ $<

clean:
	rm -f $(TARGET)

install: $(TARGET)
	install -m 755 $(TARGET) /usr/local/bin/
