CC = gcc
CFLAGS = -Wall -Wextra

LIBS = -lpthread -lm -lreadline
INCLUDES = -Ithirdparty

WORKER_SRC = worker.c shared_res.c pthread_pool.c
SENDER_SRC = sender.c shared_res.c cli.c cli_commands.c

BUILD_DIR = build

WORKER = $(BUILD_DIR)/worker
SENDER = $(BUILD_DIR)/sender

all: $(BUILD_DIR) $(WORKER) $(SENDER)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(WORKER): $(WORKER_SRC) | $(BUILD_DIR)
	$(CC) $(CFLAGS) -o $@ $(WORKER_SRC) $(LIBS) $(INCLUDES)

$(SENDER): $(SENDER_SRC) | $(BUILD_DIR)
	$(CC) $(CFLAGS) -o $@ $(SENDER_SRC) $(LIBS) $(INCLUDES)

clean:
	rm -rf $(BUILD_DIR)

clang_cmd:
	bear --output $(BUILD_DIR)/compile_commands.json -- make -B

tmux_run:	$(BUILD_DIR) $(WORKER) $(SENDER)
	tmux split-window -hd "build/worker" & build/sender

.PHONY: all clean
