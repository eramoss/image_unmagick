#include <stdlib.h>

#include "pthread_pool.h"

void *worker_thread(void *arg);

unmgk_pool_t *pool_start(unsigned int nthreads) {
  unmgk_pool_t *pool = malloc(sizeof(unmgk_pool_t));
  pool->threads = malloc(sizeof(pthread_t) * nthreads);
  pool->shutdown = 0;
  pool->nthreads = nthreads;
  pool->queue_head = pool->queue_tail = NULL;
  pool->last_id = 0;
  pool->remaining = 0;

  pthread_mutex_init(&pool->q_mtx, NULL);
  pthread_cond_init(&pool->q_cnd, NULL);

  for (unsigned int i = 0; i < nthreads; i++) {
    pthread_create(&pool->threads[i], NULL, worker_thread, pool);
  }

  return pool;
}

unsigned int pool_enqueue(unmgk_pool_t *pool, unmgk_task_t *task) {
  unmgk_pool_queue_t *new = malloc(sizeof(unmgk_pool_queue_t));
  new->task = task;
  new->next = NULL;
  new->done = 0;
  pthread_cond_init(&new->done_cnd, NULL);

  pthread_mutex_lock(&pool->q_mtx);
  new->id = ++pool->last_id;

  if (pool->queue_tail == NULL) {
    pool->queue_tail = pool->queue_head = new;
  } else {
    pool->queue_head->next = new;
    pool->queue_head = new;
  }

  pool->remaining++;
  pthread_cond_signal(&pool->q_cnd);
  pthread_mutex_unlock(&pool->q_mtx);

  return new->id;
}

void pool_wait(unmgk_pool_t *pool, unsigned int *pids) {
  for (unsigned int *p = pids; *p != 0; ++p) {
    unsigned int id = *p;

    pthread_mutex_lock(&pool->q_mtx);
    unmgk_pool_queue_t *q = pool->queue_tail;
    while (q && q->id != id) {
      q = q->next;
    }
    if (!q) {
      pthread_mutex_unlock(&pool->q_mtx);
      continue; // invalid
    }

    while (!q->done) {
      pthread_cond_wait(&q->done_cnd, &pool->q_mtx);
    }
    pthread_mutex_unlock(&pool->q_mtx);
  }
}

void pool_end(unmgk_pool_t *pool) {
  pthread_mutex_lock(&pool->q_mtx);
  pool->shutdown = 1;
  pthread_cond_broadcast(&pool->q_cnd);
  pthread_mutex_unlock(&pool->q_mtx);

  for (unsigned int i = 0; i < pool->nthreads; i++) {
    pthread_join(pool->threads[i], NULL);
  }

  unmgk_pool_queue_t *q = pool->queue_tail;
  while (q) {
    unmgk_pool_queue_t *tmp = q;
    q = q->next;

    if (tmp->task)
      free(tmp->task);
    pthread_cond_destroy(&tmp->done_cnd);
    free(tmp);
  }

  free(pool->threads);
  pthread_mutex_destroy(&pool->q_mtx);
  pthread_cond_destroy(&pool->q_cnd);
  free(pool);
}

void *worker_thread(void *arg) {
  unmgk_pool_t *pool = (unmgk_pool_t *)arg;

  while (1) {
    pthread_mutex_lock(&pool->q_mtx);
    while (!pool->shutdown && pool->queue_tail == NULL) {
      pthread_cond_wait(&pool->q_cnd, &pool->q_mtx);
    }

    if (pool->shutdown && pool->queue_tail == NULL) {
      pthread_mutex_unlock(&pool->q_mtx);
      break;
    }

    unmgk_pool_queue_t *task_node = pool->queue_tail;
    pool->queue_tail = task_node->next;
    if (pool->queue_tail == NULL)
      pool->queue_head = NULL;
    pthread_mutex_unlock(&pool->q_mtx);

    if (task_node->task && task_node->task->fn) {
      task_node->task->fn(task_node->task->arg);
    }

    pthread_mutex_lock(&pool->q_mtx);
    task_node->done = 1;
    pthread_cond_signal(&task_node->done_cnd);
    pool->remaining--;
    pthread_mutex_unlock(&pool->q_mtx);
  }

  return NULL;
}
