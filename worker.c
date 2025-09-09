#include <stdio.h>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#include "pthread_pool.h"
#define NTHREADS 16
#define NTASKS 32
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
} slice_task_t;

void negative_sliced(void* args){
	slice_task_t *task = (slice_task_t *)args;
	int width = task->img->width;
	int channels = task->img->channels;
	long start_pixel_idx = (long)task->start_row * width * channels;
	long end_pixel_idx = (long)task->end_row * width * channels;

	unsigned char *src = task->img->data;
	unsigned char *dest = task->output_buffer;
	for (long i = start_pixel_idx; i < end_pixel_idx; i += channels) {
		dest[i] = 255 - src[i];
		if (channels > 1) {
			dest[i+1] = 255 - src[i+1];
			dest[i+2] = 255 - src[i+2];
		}
		if (channels == 4) dest[i+3] = src[i+3];
	}
}
void threshold_sliced(void* args){
	slice_task_t *task = (slice_task_t *)args;
	
	int width = task->img->width;
	int channels = task->img->channels;
	long start_pixel_idx = (long)task->start_row * width * channels;
	long end_pixel_idx = (long)task->end_row * width * channels;

	unsigned char *src = task->img->data;
	unsigned char *dest = task->output_buffer;
	args_op_t args_thr = task->img->args_op;
	for (long i = start_pixel_idx; i < end_pixel_idx; i += channels) {
		int gray_value = (channels == 1) ? src[i] : (src[i] + src[i+1] + src[i+2]) / 3;

		if (gray_value <= args_thr.slice_t1_thr || gray_value >= args_thr.slice_t2_thr) {
			dest[i] = 255;
			if (channels > 1) { dest[i+1] = 255; dest[i+2] = 255; }
		} else {
			dest[i] = src[i];
			if (channels > 1) { dest[i+1] = src[i+1]; dest[i+2] = src[i+2]; }
		}
		if (channels == 4) dest[i+3] = src[i+3];
	}
}

typedef struct {
	unmgk_shm_image *img;
	int img_id;
	unmgk_pool_t* pool;
} task_t;

void process_img_thread(void* args){
	task_t *task = (task_t*)args;
	unmgk_shm_image * img = task->img;
	printf("Thread de processamento iniciada para a imagem %d...\n", task->img_id);

	unsigned char *output_pixels = malloc(img->size);
	if (!output_pixels) {
		perror("malloc");
		return;
	}

	void* fn;
	switch(img->op){
		case UNMGK_NEGATIVE: {
			fn = negative_sliced;
			break;
		}
		case UNMGK_THRESHOLD: {
			fn = threshold_sliced;
			break;
		}
		case UNMGK_UNKNOWN_OP:
		default:
			printf("Unknown operation: %d for image %d\n",task->img->op, task->img_id);
	}

	unsigned int tasks_pool_ids[NTASKS + 1] = {0};
	slice_task_t slice_tasks[NTASKS];
	int rows_per_thread = img->height / NTASKS;

	for (int i = 0; i < NTASKS; i++) {
		unmgk_task_t *pt = malloc(sizeof(*pt));
    pt->fn = fn;
    pt->arg = &slice_tasks[i];
		slice_tasks[i].img = img;
		slice_tasks[i].output_buffer = output_pixels;
		slice_tasks[i].start_row = i * rows_per_thread;
		slice_tasks[i].end_row = (i == NTASKS - 1) ? img->height : (i + 1) * rows_per_thread;

		unsigned int id = pool_enqueue(task->pool, pt);
		tasks_pool_ids[i] = id;
	}
	pool_wait(task->pool, tasks_pool_ids);
	
	char output_filename[256];
	sprintf(output_filename, "processed_%d.png", task->img_id);
	stbi_write_png(output_filename, img->width, img->height, img->channels, output_pixels, img->width * img->channels);

	printf("Thread de processamento terminada para a imagem %d, enviando ack\n", task->img_id);
	write(fd_ack, &task->img_id, sizeof(task->img_id));
}

int main() {
	init_shared_resources_worker();
	unmgk_pool_t* pool = pool_start(NTHREADS);

	while (1) {
		sem_wait(items);
		sem_wait(mutex);

		int img_position = queue->head % QUEUE_SIZE;
		unmgk_shm_image *img = (unmgk_shm_image *) &queue->buffer[img_position];

		printf("Recebido imagem: %dx%d, %d canais, %d bytes (pos=%d) para task: %d\n",
				 img->width, img->height, img->channels, img->size, img_position, img->op);

		int img_id = queue->head;
		queue->head++;
		task_t *arg = malloc(sizeof(*arg));
		arg->img = img;
		arg->img_id = img_id;
		arg->pool = pool;

		unmgk_task_t *t = malloc(sizeof(*t));
		t->fn = process_img_thread;
		t->arg = arg;
		sem_post(mutex);
		pool_enqueue(pool, t);
	}

	return 0;
}
