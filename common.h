#pragma once

#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <semaphore.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#define SHM_UNMGK_QUEUE "/image_unmagick_shm_queue"
#define SEM_UNMGK_MUTEX_QUEUE "/image_unmagick_mutex_sem_queue" // mutex for senders if multiples, why not multiples? :)
#define SEM_UNMGK_ITEMS_QUEUE "/image_unmagick_items_sem_queue" // signals to workers
#define FIFO_ACK_UNMGCK_QUEUE "/tmp/unmgk_ack_fifo"

#define QUEUE_SIZE 10
#define MAX_IMAGE_BYTES (8 * 1024 * 1024) 

typedef struct {
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

int fd_ack;
unmgk_shm_queue *queue;
sem_t *mutex, *items;

static inline void init_shared_resources_sender() {
	int created = 0;
	int shm_fd = shm_open(SHM_UNMGK_QUEUE, O_CREAT | O_EXCL | O_RDWR, 0666);
	if (shm_fd >= 0) {
		created = 1;
		ftruncate(shm_fd, sizeof(unmgk_shm_queue));
	} else {
		shm_fd = shm_open(SHM_UNMGK_QUEUE, O_RDWR, 0666);
		if (shm_fd < 0) {
			perror("shm_open");
			exit(1);
		}
	}
	queue = (unmgk_shm_queue*)mmap(NULL, sizeof(unmgk_shm_queue), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
	if (queue == MAP_FAILED) {
		perror("mmap");
		exit(1);
	}

	if (created) {
		queue->head = 0;
		queue->tail = 0;
	}

	if (created) {
		sem_unlink(SEM_UNMGK_MUTEX_QUEUE);
		sem_unlink(SEM_UNMGK_ITEMS_QUEUE);
	}

	mutex = sem_open(SEM_UNMGK_MUTEX_QUEUE, O_CREAT, 0666, 1);
	if (mutex == SEM_FAILED) { perror("sem_open mutex"); exit(1); }
	items = sem_open(SEM_UNMGK_ITEMS_QUEUE, O_CREAT, 0666, 0);
	if (items == SEM_FAILED) { perror("sem_open items"); exit(1); }

	if (mkfifo(FIFO_ACK_UNMGCK_QUEUE, 0666) < 0 && errno != EEXIST) {
		perror("mkfifo");
		exit(1);
	}
	fd_ack = open(FIFO_ACK_UNMGCK_QUEUE, O_RDONLY);
	if (fd_ack < 0) {
		perror("open ack fifo (producer)");
	}
}


static inline void init_shared_resources_worker() {
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

	mkfifo(FIFO_ACK_UNMGCK_QUEUE, 0666);
	fd_ack = open(FIFO_ACK_UNMGCK_QUEUE, O_WRONLY);
	if (fd_ack < 0) perror("open ack fifo (receiver)");
}

static inline void reset_and_exit() {
    sem_wait(mutex);

    queue->head = 0;
    queue->tail = 0;

    sem_post(mutex);

    sem_close(mutex);
    sem_close(items);
    sem_unlink(SEM_UNMGK_MUTEX_QUEUE);
    sem_unlink(SEM_UNMGK_ITEMS_QUEUE);

    munmap(queue, sizeof(unmgk_shm_queue));
    shm_unlink(SHM_UNMGK_QUEUE);

    close(fd_ack);
    unlink(FIFO_ACK_UNMGCK_QUEUE);

    printf("Fila resetada e recursos limpos. Saindo...\n");
    exit(0);
}

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"
