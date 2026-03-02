CC ?= gcc
CFLAGS ?= -Wall -Wextra -Werror

TARGET := sysfs-editor

.PHONY: all clean

all: $(TARGET)

$(TARGET): sysfs-editor.c
	$(CC) $(CFLAGS) -o $(TARGET) sysfs-editor.c

clean:
	rm -f $(TARGET)