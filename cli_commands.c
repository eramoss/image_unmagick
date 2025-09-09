#include <stddef.h>
#include <string.h>
#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>

#include "cli_commands.h"
#include "utils.h"
#include "shared_res.h"

command_entry_t commands[] = {
	{"neg_img", cmd_neg, "neg_img <imagem>"},
	{"threshold_img", cmd_threshold, "threshold_img <imagem>"},
	{"status", cmd_status, "status"},
	{"exit", cmd_exit, "exit"},
	{"clean_exit", cmd_clean_exit, "clean_exit"},
	{NULL, NULL, NULL} // sentinel
};

extern void *load_image_thread(void *arg);
void cmd_neg(const char *args) {
	char *path = trim((char *)args);
	load_task_t *task = malloc(sizeof(load_task_t));
	strncpy(task->path, path, sizeof(task->path) - 1);
	task->path[sizeof(task->path) - 1] = '\0';
	task->op = UNMGK_NEGATIVE;

	pthread_t tid;
	pthread_create(&tid, NULL, load_image_thread, task);
}
void cmd_threshold(const char *args) {
	char *path = trim((char *)args);
	load_task_t *task = malloc(sizeof(load_task_t));
	strncpy(task->path, path, sizeof(task->path) - 1);
	task->path[sizeof(task->path) - 1] = '\0';
	task->op = UNMGK_THRESHOLD;

	pthread_t tid;
	pthread_create(&tid, NULL, load_image_thread, task);
}

void cmd_status(const char *args) {
	(void)args;
	printf("Fila: head=%d, tail=%d, ocupados=%d\n",
				queue->head, queue->tail, (queue->tail - queue->head));
}

void cmd_exit(const char *args) {
	(void)args;
	exit(0);
}

void cmd_clean_exit(const char *args) {
	(void)args;
	reset_and_exit();
}
