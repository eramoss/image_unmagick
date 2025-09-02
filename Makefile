CC = gcc
CFLAGS = -Wall -Wextra

LIBS = -lpthread

SRC = worker.c sender.c

BUILD_DIR = build

WORKER = $(BUILD_DIR)/worker
SENDER = $(BUILD_DIR)/sender

all: $(BUILD_DIR) $(WORKER) $(SENDER)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(WORKER): worker.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -o $@ worker.c $(LIBS)

$(SENDER): sender.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -o $@ sender.c $(LIBS)

clean:
	rm -rf $(BUILD_DIR)

.PHONY: all clean
