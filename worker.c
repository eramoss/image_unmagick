#include <stdio.h>
#include <stdlib.h>

#include "common.h"

unmgk_shm_queue *queue;
sem_t *mutex, *items;

void init_shared_resources() {
	int shm_fd = shm_open(SHM_UNMGK_QUEUE, O_RDWR, 0666);
	if (shm_fd < 0) { perror("shm_open"); exit(1); }

	queue = (unmgk_shm_queue *) mmap(NULL, sizeof(unmgk_shm_queue), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
	if (queue == MAP_FAILED) {
		perror("mmap");
		exit(1);
	}

	mutex = sem_open(SEM_UNMGK_MUTEX_QUEUE, 0);
	if (!mutex) { perror("sem_open mutex"); exit(1); }

	items = sem_open(SEM_UNMGK_ITEMS_QUEUE, 0);
	if (!items) { perror("sem_open items"); exit(1); }
}

int main() {
	init_shared_resources();

	while (1) {
		sem_wait(items);
		sem_wait(mutex);

		int img_position = queue->head % QUEUE_SIZE;
		unmgk_shm_image *img = (unmgk_shm_image *) &queue->buffer[img_position];

		printf("Recebido imagem: %dx%d, %d canais, %d bytes (pos=%d)\n",
				 img->width, img->height, img->channels, img->size, img_position);

		queue->head++;
		sem_post(mutex);
	}

	return 0;
}
