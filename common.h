#pragma once

#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <semaphore.h>

#define SHM_UNMGK_QUEUE "/image_unmagick_shm_queue"
#define SEM_UNMGK_MUTEX_QUEUE "/image_unmagick_mutex_sem_queue" // mutex for senders if multiples, why not multiples? :)
#define SEM_UNMGK_ITEMS_QUEUE "/image_unmagick_items_sem_queue" // signals to workers

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

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"
