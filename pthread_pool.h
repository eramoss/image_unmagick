#pragma once
#include <pthread.h>

typedef void (*task_fn)(void *);
typedef struct {
    task_fn fn;
    void *arg;
} unmgk_task_t;

typedef struct unmgk_pool_queue_t{
	unmgk_task_t *task;
	unsigned int id;
	int done;
	pthread_cond_t done_cnd;
	struct unmgk_pool_queue_t *next;
} unmgk_pool_queue_t;

typedef struct {
	unsigned int remaining;
	unsigned int nthreads;
	unmgk_pool_queue_t *queue_head;
	unmgk_pool_queue_t *queue_tail;
	pthread_mutex_t q_mtx;
	pthread_cond_t q_cnd;
	pthread_t * threads;
	unsigned int last_id;
	int shutdown;
} unmgk_pool_t;

unmgk_pool_t *pool_start(unsigned int nthreads);
unsigned int pool_enqueue(unmgk_pool_t* pool, unmgk_task_t* task);
void pool_wait(unmgk_pool_t * pool, unsigned int * pids);
void pool_end(unmgk_pool_t* pool);
