#pragma once

typedef struct {
	char path[256];
} load_task_t;

typedef void (*command_func_t)(const char *args);

typedef struct {
	const char *name;
	command_func_t func;
	const char *help;
} command_entry_t;

extern command_entry_t commands[];

void cmd_load(const char *args);
void cmd_status(const char *args);
void cmd_exit(const char *args);
void cmd_clean_exit(const char *args);
