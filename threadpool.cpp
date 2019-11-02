#include "threadpool.h"
#include <pthread.h>

ThreadPool_t *ThreadPool_create(int num){

    // Create work queue object
    ThreadPool_work_queue_t workQueue;
    workQueue.queue = std::queue<ThreadPool_work_t>();

    //  Create threadpool object
    ThreadPool_t *tp = new ThreadPool_t;

    tp->work_queue = workQueue;
    tp->stop_running = false;
    tp->pool = std::vector<pthread_t>(num);
    tp->live_threads = num;
    tp->work_mutex = PTHREAD_MUTEX_INITIALIZER;
    tp->work_available_cond = PTHREAD_COND_INITIALIZER;

    //  Create mapper threads
    for(int i = 0; i < num; i++) {
        pthread_create(&tp->pool.at(i), NULL, (void *(*)(void *))Thread_run, tp);
    }

    return tp;
}

void ThreadPool_destroy(ThreadPool_t *tp){

    //  Wait for all live threads to finish
    while(tp->live_threads != 0);

    // Lock the mutex
    pthread_mutex_lock(&tp->work_mutex);
    tp->stop_running = true;

    // Unlock mutex
    pthread_mutex_unlock(&tp->work_mutex);

    // Destroy mutex and conditions before destroying threadpool object
    pthread_cond_destroy(&tp->work_available_cond);
    pthread_mutex_destroy(&tp->work_mutex);

    delete(tp);
}

bool ThreadPool_add_work(ThreadPool_t *tp, thread_func_t func, void *arg) {

    // Create the work item
    ThreadPool_work_t work;
    work.func = func;
    work.arg = arg;

    // lock mutex
    pthread_mutex_lock(&(tp->work_mutex));
    // add item to queue
    tp->work_queue.queue.push(work);

    // let waiting threads know a new item has been added to the queue
    pthread_cond_signal(&(tp->work_available_cond));

    // unlock mutex
    pthread_mutex_unlock(&(tp->work_mutex));
    return true;
}

ThreadPool_work_t *ThreadPool_get_work(ThreadPool_t *tp) {

    // get the item at front of queue and decrease number of items on queue by 1
    ThreadPool_work_t *nextWorkItem = &(tp->work_queue.queue.front());
    tp->work_queue.queue.pop();

    return nextWorkItem->func == NULL && nextWorkItem->arg == NULL ? NULL : nextWorkItem;
}

void *Thread_run(ThreadPool_t *tp) {

    ThreadPool_work_t *work = NULL;

    // thread will continually run this function until stop is signaled
    while(true) {

        // lock mutex to access work queue exclusively
        pthread_mutex_lock(&(tp->work_mutex));

        // break loop is stop condition has been notified
        if (tp->stop_running) {
            break;
        }

        // If there is no work in the queue, wait until there is work available
        while (tp->work_queue.queue.size() == 0 && !tp->stop_running) {
            pthread_cond_wait(&(tp->work_available_cond), &(tp->work_mutex));
        }

        // Get the next work item
        if(!tp->stop_running) {
            work = ThreadPool_get_work(tp);
        }

        //  If the work item is NULL, set termination flag to true
        //  and wake up all sleeping threads.
        if(work == NULL) {
            tp->stop_running = true;
            pthread_cond_broadcast(&(tp->work_available_cond));
            break;
        }

        //  Unlock mutex
        pthread_mutex_unlock(&(tp->work_mutex));

        // Do the work
        work->func(work->arg);
    }

    // Decrease number of live threads, unlock mutex and terminate thread.
    tp->live_threads--;
    pthread_mutex_unlock(&(tp->work_mutex));
    pthread_exit(0);
}
