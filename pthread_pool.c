#include <stdlib.h>

#include "pthread_pool.h"

void * worker_thread(void*);

unmgk_pool_t * pool_start(unsigned int nthreads){
	unmgk_pool_t* pool = (unmgk_pool_t*)malloc(sizeof(unmgk_pool_t));
	pool->threads = (pthread_t*)malloc(sizeof(pthread_t) * nthreads);
	pool->shutdown = 0;
	pool->nthreads = nthreads;
	pool->queue_head = pool->queue_tail = NULL;
	pool->last_id = 0;
	pool->remaining = 0;

	pthread_mutex_init(&pool->q_mtx, NULL);
	pthread_cond_init(&pool->q_cnd, NULL);
	for (unsigned int i = 0; i < nthreads; i++){
		pthread_create(&pool->threads[i], NULL, worker_thread, pool);
	}
	return pool;
}

unsigned int pool_enqueue(unmgk_pool_t* pool, unmgk_task_t* task){
	unmgk_pool_queue_t* new = (unmgk_pool_queue_t*)malloc(sizeof(unmgk_pool_queue_t));
	new->task = task;
	new->next = NULL;
	new->done = 0;

	pthread_mutex_lock(&pool->q_mtx);
	new->id = ++(pool->last_id);

	if (pool->queue_head != NULL) pool->queue_head->next = new;
	if (pool->queue_tail == NULL) pool->queue_tail = new;
	pool->queue_head = new;
	pool->remaining++;

	pthread_cond_signal(&pool->q_cnd);
	pthread_mutex_unlock(&pool->q_mtx);
	return new->id;
}

void pool_wait(unmgk_pool_t* pool, unsigned int* pids){
	for (unsigned int *p = pids; *p != 0; ++p) { 
		unsigned int id = *p;

		pthread_mutex_lock(&pool->q_mtx);
		unmgk_pool_queue_t* q = pool->queue_tail;
		while (q && q->id != id) {
			q = q->next;
		}

		if (!q) {
			//invalid
			pthread_mutex_unlock(&pool->q_mtx);
			continue;
		}

		while (!q->done) {
			pthread_cond_wait(&q->done_cnd, &pool->q_mtx);
		}
		pthread_mutex_unlock(&pool->q_mtx);
	}
}

void pool_end(unmgk_pool_t* pool){
	// doesnt matter mutex here since im the only one using shutdown
	// and its better since im eval this in worker loop i can cancel the pool
	pool->shutdown = 1; 
	pthread_mutex_lock(&pool->q_mtx);
	pthread_cond_broadcast(&pool->q_cnd);
	pthread_mutex_unlock(&pool->q_mtx);

	for (unsigned int i = 0; i < pool->nthreads; i++) {
		pthread_join(pool->threads[i], NULL);
	}

	unmgk_pool_queue_t *q;
	while (pool->queue_head != NULL) {
		q = pool->queue_head;
		pool->queue_head = q->next;

		free(q);
	}
	free(pool);
}

void *worker_thread(void *arg) {
    unmgk_pool_t *pool = (unmgk_pool_t *)arg;
    while (1) {
        pthread_mutex_lock(&pool->q_mtx);
        while (!pool->shutdown && pool->queue_tail == NULL) {
            pthread_cond_wait(&pool->q_cnd, &pool->q_mtx);
        }

        if (pool->shutdown) {
            pthread_mutex_unlock(&pool->q_mtx);
            break;
        }

        unmgk_pool_queue_t *task = pool->queue_tail;
        pool->queue_tail = task->next;
        if (pool->queue_tail == NULL) {
            pool->queue_head = NULL; // dont need to free now, use pool_end
        }

        pthread_mutex_unlock(&pool->q_mtx);

        unmgk_task_t *t = (unmgk_task_t *)task->task;
        if (t && t->fn) {
            t->fn(t->arg);
        }

        pthread_mutex_lock(&pool->q_mtx);
        task->done = 1;
        pthread_cond_signal(&task->done_cnd);
        pool->remaining--;
        pthread_mutex_unlock(&pool->q_mtx);
    }
    return NULL;
}
