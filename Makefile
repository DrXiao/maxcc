CC := gcc
CFLAGS := -g -O0 -std=c99
TARGET := maxcc
TEST_DIR := tests
TEST := main.c

$(TARGET): $(TARGET).c
	$(CC) $(CFLAGS) -o $@ $^

test:
	./$(TARGET) --dump-ir $(TEST_DIR)/$(TEST)

clean:
	rm $(TARGET)
