#include <stdio.h>
#include <stdlib.h>

#include "common.h"

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
		
    int fd = open(FIFO_ACK_UNMGCK_QUEUE, O_WRONLY);
    if (fd >= 0) {
        write(fd, &img_id, sizeof(img_id));
        close(fd);
    } else {
        perror("open fifo ack");
    }
	}

	return 0;
}
