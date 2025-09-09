#include <pthread.h>
#include <stdio.h>
#include <signal.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include "cli.h"
#include "cli_commands.h"
#include "utils.h"
#include "shared_res.h"
int fd_ack;
unmgk_shm_queue *queue;
sem_t *mutex, *items;

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
	img->op = task->op;
	img->args_op = task->args_op;
	memcpy(img->data, pixels, img_size);

	int img_id = queue->tail; 
	queue->tail++;
	sem_post(mutex);
	sem_post(items);

	THREAD_PRINT("[OK] %s → %dx%d, %d canais, %d bytes (pos=%d), op = %d\n",
							task->path, width, height, channels, img_size, img_position, task->op);

	int ack_id;
	while (1) {
		if (read(fd_ack, &ack_id, sizeof(ack_id)) < 0) {
			perror("read"); 
		}
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
