#include <stdio.h>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#include "shared_res.h"
int fd_ack;
unmgk_shm_queue *queue;
sem_t *mutex, *items;

int main() {
	init_shared_resources_worker();

	while (1) {
		sem_wait(items);
		sem_wait(mutex);

		int img_position = queue->head % QUEUE_SIZE;
		unmgk_shm_image *img = (unmgk_shm_image *) &queue->buffer[img_position];

		printf("Recebido imagem: %dx%d, %d canais, %d bytes (pos=%d)\n",
				 img->width, img->height, img->channels, img->size, img_position);

		int img_id = queue->head;
		queue->head++;
		sem_post(mutex);

		sleep(2);
		printf("Processed img_id: %d\n", img_id);

		write(fd_ack, &img_id, sizeof(img_id));
	}

	return 0;
}
