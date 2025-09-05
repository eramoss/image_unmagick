#include <pthread.h>
#include <stdio.h>
#include <signal.h>

#include <readline/readline.h>
#include <readline/history.h>

#include "utils.h"
#include "common.h"

#define ARG_IS(cmd) (strncmp(input, cmd, strlen(cmd)) == 0)


typedef struct {
	char path[256];
} load_task_t;


void *load_image_thread(void *arg) {
	load_task_t *task = (load_task_t *) arg;

	int width, height, channels;
	unsigned char *pixels = stbi_load(task->path, &width, &height, &channels, 0);
	if (!pixels) {

		THREAD_PRINT("Erro: não foi possível carregar a imagem %s\n", task->path);
		THREAD_PRINT("stbi_failure_reason(): %s\n", stbi_failure_reason());
		free(task);
		return NULL;
	}

	int img_size = width * height * channels;

	sem_wait(mutex);
	int img_position = queue->tail % QUEUE_SIZE;
	unmgk_shm_image *img = (unmgk_shm_image *) queue->buffer[img_position];

	img->size = img_size;
	img->width = width;
	img->height = height;
	img->channels = channels;
	memcpy(img->data, pixels, img_size);

	int img_id = queue->tail; 
	queue->tail++;
	sem_post(mutex);
	sem_post(items);

	THREAD_PRINT("[OK] %s → %dx%d, %d canais, %d bytes (pos=%d)\n",
							task->path, width, height, channels, img_size, img_position);

	int ack_id;
	while (1) {
		read(fd_ack, &ack_id, sizeof(ack_id));
		if (ack_id == img_id) {
			THREAD_PRINT("Confirmação recebida para %s, gravado em processed_%d.png\n", task->path, ack_id);
			break;
		}
		usleep(1000);
	}

	stbi_image_free(pixels);
	free(task);
	return NULL;
}

char *command_generator(const char *text, int state) {
	static int list_index, len;
	static char *commands[] = { "load", "status", "exit", "clean_exit" , NULL };

	if (!state) {
		list_index = 0;
		len = strlen(text);
	}

	char *name;
	while ((name = commands[list_index++])) {
		if (strncmp(name, text, len) == 0) return strdup(name);
	}
	return NULL;
}

char **cli_completion(const char *text, int start, int _) {
	(void)_ ;// unused
	rl_attempted_completion_over = 1;

	if (start == 0) {
		return rl_completion_matches(text, command_generator);
	}

	char *line = rl_line_buffer;
	if (strncmp(line, "load ", 5) == 0) {
		return rl_completion_matches(text, rl_filename_completion_function);
	}

	return NULL;
}

void cli_loop() {
	rl_attempted_completion_function = cli_completion;

	char *input;
	while ((input = readline("unmgk> ")) != NULL) {
		if (*input) add_history(input);

		if (ARG_IS("exit")) {
			free(input);
			break;
		} 
		else if (ARG_IS("load ")) {
			char *path = trim(input + strlen("load "));
			load_task_t *task = malloc(sizeof(load_task_t));
			strncpy(task->path, path, sizeof(task->path) - 1);
			task->path[sizeof(task->path) - 1] = '\0';

			pthread_t tid;
			pthread_create(&tid, NULL, load_image_thread, task);
		} 
		else if (ARG_IS("status")) {
			printf("Fila: head=%d, tail=%d, ocupados=%d\n",
					queue->head, queue->tail,
					(queue->tail - queue->head));
		} 
		else if (ARG_IS("clean_exit")) {
			reset_and_exit();
		} 
		else {
			printf("Comandos:\n"
					">	load <imagem>\n"\
					">	status\n"\
					">	clean_exit\n"\
					">	exit\n");
		}

		free(input);
	}
}

void signal_handler(int signo) {
	(void)signo; 
	reset_and_exit();
}

int main() {
	// Yes i know that its too much for M1, i just want to be fancy :)
	struct sigaction sa;
	sa.sa_handler = signal_handler;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = 0;

	sigaction(SIGINT, &sa, NULL);
	sigaction(SIGTERM, &sa, NULL);
	sigaction(SIGHUP, &sa, NULL); 

	init_shared_resources_sender();
	cli_loop();
	return 0;
}
