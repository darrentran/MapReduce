#include "mapreduce.h"
#include "threadpool.h"
#include <pthread.h>
#include <sys/stat.h>
#include <iostream>
#include <map>
#include <set>

typedef struct Partition{
    std::multimap<std::string, std::string> partition_map;
    std::set<std::string> keys_set;
    std::multimap<std::string, std::string>::iterator partitionIter;
    pthread_mutex_t partition_mutex;
    Reducer func;              // Reducer function
} Partition;

std::vector<Partition> partitionVector;
int PARTITIONS;

int fileNameCompare(const void* a, const void* b) {
    struct stat sa;
    struct stat sb;
    stat((char*) a, &sa);
    stat((char*) b, &sb);

    if ( sa.st_size < sb.st_size ) {
        return -1;
    } else if(sa.st_size ==  sb.st_size) {
        return 0;
    } else {
        return 1;
    }
}

void printPartitionContents(){
    for (int i = 0; i < partitionVector.size(); i++)
    {
        printf("partition: %d\n", i);
        std::multimap<std::string, std::string>::iterator iter;
        for (iter = partitionVector.at(i).partition_map.begin(); iter != partitionVector.at(i).partition_map.end() ; ++iter)
        {
            printf("key: %s, value: %s\n", (*iter).first.c_str(), (*iter).second.c_str());
        }
    }
}

void MR_Run(int num_files, char *filenames[], Mapper map, int num_mappers, Reducer concate, int num_reducers) {
    // Create thread pool
    ThreadPool_t *threadPool = ThreadPool_create(num_mappers);
    PARTITIONS = num_reducers;

    // Sort file names by size and put them into work queue
    qsort(filenames, num_files, sizeof(char*), fileNameCompare);
    for(int i = 0; i < num_files; i++) {
        ThreadPool_add_work(threadPool, (thread_func_t) map, filenames[i]);
    }

    ThreadPool_add_work(threadPool, NULL, NULL);

    // Create partitions object with its properties and put them into the partition vector
    partitionVector = std::vector<Partition>();
    for(int i = 0; i < PARTITIONS; i++) {
        // Create partition object
        Partition p;
        p.func = concate;
        p.partition_map = std::multimap<std::string, std::string>();
        pthread_mutex_init(&p.partition_mutex, NULL);

        // Add partition object to vector
        partitionVector.push_back(p);
    }

    // Wait for all threads to finish working
    while (threadPool->working_threads != 0 || threadPool->work_queue.queue.size() != 0){
//        printf("working threads: %d, queue size: %lu\n", threadPool->working_threads, threadPool->work_queue.queue.size());
    }

//    printPartitionContents();

    ThreadPool_destroy(threadPool); // Not actually deleting the threads...

//    pthread_t p_threads[num_reducers];
//
//    for(int i = 0; i < PARTITIONS; i++){
//        printf("For loop iteration: %i\n", i);
//        pthread_create(&p_threads[i], NULL, (void *(*)(void *)) &MR_ProcessPartition, (void *)i);
//    }
//
//    for(int i = 0; i < PARTITIONS; i++) {
//        printf("Waiting for thread %i to finish execution\n", i);
//        pthread_join(p_threads[i], NULL);
//    }

    printf("Main Thread: Waited on %d mappers. Done.\n", num_mappers);
}

unsigned long MR_Partition(char *key, int num_partitions){
    unsigned long hash = 5381;
    int c;
    while ((c = *key++) != '\0')
        hash = hash * 33 + c;
    return hash % num_partitions;
}

void MR_Emit(char *key, char *value){

    unsigned long partition = MR_Partition(key, PARTITIONS-1);

    pthread_mutex_lock(&(partitionVector.at(partition).partition_mutex));
//    printf("Adding key: %s, with value: %s to partition: %lu\n", key, value, partition);
    partitionVector.at(partition).partition_map.insert(std::pair<std::string,std::string>(key, value));
    partitionVector.at(partition).keys_set.insert(key);
    pthread_mutex_unlock(&(partitionVector.at(partition).partition_mutex));
}

void MR_ProcessPartition(int partition_number){
//    int partitionNumber = *((int*) partition_number);
    printf("partition number: %i\n", partition_number);
    Partition currentPartition = partitionVector.at(partition_number);
    std::set<std::string>::iterator keyIterator = currentPartition.keys_set.begin();
    currentPartition.partitionIter = currentPartition.partition_map.begin();

    printf("Running reducer thread...");

    while(keyIterator != currentPartition.keys_set.end()) {

        pthread_mutex_lock(&(currentPartition.partition_mutex));

        char *nextKey = MR_GetNext((char *) (*keyIterator).c_str(), partition_number);
        if(nextKey == NULL) {
            keyIterator++;
            continue;
        }
        printf("Current Key: %s, next value: %c\n", (*keyIterator).c_str() , *nextKey);
        currentPartition.func(nextKey, partition_number);

        pthread_mutex_lock(&(currentPartition.partition_mutex));
    }

    printf("Exiting Reducer thread");
    pthread_exit(0);
}

char *MR_GetNext(char *key, int partition_number){

    /*
    The MR GetNext function returns the next value associated with the given
    key in the sorted partition or NULL when the key’s values have been processed completely
     */

    Partition currentPartition = partitionVector.at(partition_number);
    char *value = NULL;
    pthread_mutex_lock(&(currentPartition.partition_mutex));
    if((*currentPartition.partitionIter).first == key) {
        value = (char *) (*currentPartition.partitionIter).second.c_str();
    }

    currentPartition.partitionIter++;
    pthread_mutex_unlock(&(currentPartition.partition_mutex));

    return value;

}