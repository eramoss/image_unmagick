#pragma once

#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <semaphore.h>
#include <sys/stat.h>

#define SHM_UNMGK_QUEUE "/image_unmagick_shm_queue"
#define SEM_UNMGK_MUTEX_QUEUE "/image_unmagick_mutex_sem_queue" // mutex for senders if multiples, why not multiples? :)
#define SEM_UNMGK_ITEMS_QUEUE "/image_unmagick_items_sem_queue" // signals to workers
#define FIFO_ACK_UNMGCK_QUEUE "/tmp/unmgk_ack_fifo"

#define QUEUE_SIZE 10
#define MAX_IMAGE_BYTES (8 * 1024 * 1024) 

typedef enum {
	UNMGK_NEGATIVE,
	UNMGK_THRESHOLD,
	UNMGK_UNKNOWN_OP
} unmgk_img_op;

typedef struct {
	int slice_t1_thr;
	int slice_t2_thr;
} args_op_t;

typedef struct {
	unmgk_img_op op;
	args_op_t args_op;
	int size;
	int width;
	int height;
	int channels;
	unsigned char data[];
} unmgk_shm_image;

typedef struct {
	int head;
	int tail; 
	char buffer[QUEUE_SIZE][sizeof(unmgk_shm_image) + MAX_IMAGE_BYTES];
} unmgk_shm_queue;

extern int fd_ack;
extern unmgk_shm_queue *queue;
extern sem_t *mutex, *items;

void init_shared_resources_sender();
void init_shared_resources_worker();
void reset_and_exit();
