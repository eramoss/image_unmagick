#include <stdio.h>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#include "pthread_pool.h"
#include "shared_res.h"
int fd_ack;
unmgk_shm_queue *queue;
sem_t *mutex, *items;

typedef struct {
    int start_row;
    int end_row;
    unmgk_shm_image *img;
    unsigned char *output_buffer;
    int filter_mode;
    int slice_t1;
    int slice_t2;
} slice_task_t;

void *worker_thread_slice(void *arg) {
    slice_task_t *task = (slice_task_t *)arg;

    int width = task->img->width;
    int channels = task->img->channels;
    long start_pixel_idx = (long)task->start_row * width * channels;
    long end_pixel_idx = (long)task->end_row * width * channels;

    unsigned char *src = task->img->data;
    unsigned char *dest = task->output_buffer;

    if (task->filter_mode == 0) {
        for (long i = start_pixel_idx; i < end_pixel_idx; i += channels) {
            dest[i] = 255 - src[i];
            if (channels > 1) {
                dest[i+1] = 255 - src[i+1];
                dest[i+2] = 255 - src[i+2];
            }
            if (channels == 4) dest[i+3] = src[i+3];
        }
    } else if (task->filter_mode == 1) {
        for (long i = start_pixel_idx; i < end_pixel_idx; i += channels) {
            int gray_value = (channels == 1) ? src[i] : (src[i] + src[i+1] + src[i+2]) / 3;

            if (gray_value <= task->slice_t1 || gray_value >= task->slice_t2) {
                dest[i] = 255;
                if (channels > 1) { dest[i+1] = 255; dest[i+2] = 255; }
            } else {
                dest[i] = src[i];
                if (channels > 1) { dest[i+1] = src[i+1]; dest[i+2] = src[i+2]; }
            }
            if (channels == 4) dest[i+3] = src[i+3];
        }
    }
    return NULL;
}

typedef struct {
	unmgk_shm_image *img;
	int img_id;
} task_t;

void process_img_thread(void* args){
	task_t *task = (task_t*)args;
	write(fd_ack, &task->img_id, sizeof(task->img_id));
}

int main() {
	init_shared_resources_worker();
	unmgk_pool_t* pool = pool_start(8);

	while (1) {
		sem_wait(items);
		sem_wait(mutex);

		int img_position = queue->head % QUEUE_SIZE;
		unmgk_shm_image *img = (unmgk_shm_image *) &queue->buffer[img_position];

		printf("Recebido imagem: %dx%d, %d canais, %d bytes (pos=%d) para task: %d\n",
				 img->width, img->height, img->channels, img->size, img_position, img->op);

		int img_id = queue->head;
		queue->head++;
		sem_post(mutex);

		task_t arg = {.img = img, .img_id = img_id};
		unmgk_task_t t = {.fn = &process_img_thread, .arg=&arg};
		pool_enqueue(pool, &t);
	}

	return 0;
}
