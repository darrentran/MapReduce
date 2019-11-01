#include "mapreduce.h"
#include "threadpool.h"
#include <pthread.h>
#include <sys/stat.h>
#include <map>
#include <stdlib.h>
#include <string.h>

struct cmp {
    bool operator() (const char* a, const char* b) const {
        return strcmp(a,b) < 0;
    }
};

struct Partition{
    std::multimap<char*,char*, cmp> partition_map;
    std::multimap<char*, char*, cmp>::iterator partition_iterator;
    pthread_mutex_t partition_mutex;
    Reducer reduce;
};

std::vector<Partition> partitionVector;
int PARTITIONS;

int fileSizeCompare(const void * a, const void* b) {

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

/*
 * Main run function for map reduce library. Creates specified number of mappers and lets them map the input
 * files into partitions. Once the mappers are finished, the function creates the reducers which take a single
 * partition and write their outputs to the specified file. If the partition assigned to a reducer thread is
 * empty, the thread exits and does not crate an output file.
 */
void MR_Run(int num_files, char *filenames[], Mapper map, int num_mappers, Reducer concate, int num_reducers) {
    // Create thread pool
    ThreadPool_t *threadPool = ThreadPool_create(num_mappers);
    PARTITIONS = num_reducers;

    // Create partitions object with its properties and put them into the partition vector
    partitionVector = std::vector<Partition>();
    for(int i = 0; i < PARTITIONS; i++) {
        // Create partition object
        Partition p;
        p.reduce = concate;
        p.partition_map = std::multimap<char *, char*, cmp>();
        pthread_mutex_init(&p.partition_mutex, NULL);

        // Add partition object to vector
        partitionVector.push_back(p);
    }

    // Sort file names by size and put them into work queue
    qsort(filenames, num_files, sizeof(char*), fileSizeCompare);
    for(int i = 0; i < num_files; i++) {
        ThreadPool_add_work(threadPool, (thread_func_t) map, filenames[i]);
    }

    // Add NULL work item to let threads know they have reached the last work item
    ThreadPool_add_work(threadPool, NULL, NULL);

    //  Wait for mappers to finish
    for(int i = 0; i < num_mappers; i++) {
        pthread_join(threadPool->pool.at(i), NULL);
    }

    // Destroy the mapper threadpool
    ThreadPool_destroy(threadPool);

    // Create reducer threads
    pthread_t reducer_thread[PARTITIONS];
    for(int i = 0; i < PARTITIONS; i++){
        pthread_create(&reducer_thread[i], NULL, (void *(*)(void *)) &MR_ProcessPartition, (void *) (intptr_t) i);
    }

    //  Wait for reducers to finish
    for(int i = 0; i < PARTITIONS; i++) {
        pthread_join(reducer_thread[i], NULL);
    }

    // Free duplicated keys in multimap.
    for(int i = 0; i < PARTITIONS; i++) {
        Partition currentPartition = partitionVector.at(i);
        std::multimap<char*,char*,cmp>::iterator iter;
        for(iter = currentPartition.partition_map.begin(); iter != currentPartition.partition_map.end(); iter++) {
            free(iter->first);
        }
    }
}

/*
 * Partition function given from assignment description
 */
unsigned long MR_Partition(char *key, int num_partitions){
    unsigned long hash = 5381;
    int c;
    while ((c = *key++) != '\0')
        hash = hash * 33 + c;
    return hash % num_partitions;
}

/*
 * Take a key value pair and saves them to a partition. We use fine grained
 * mutex locks in this part to make sure that other mappers do not write to
 * the same partition at the same time.
 */
void MR_Emit(char *key, char *value){
    // Finds the partition
    unsigned long partition = MR_Partition(key, PARTITIONS);

    // Lock mutex
    pthread_mutex_lock(&(partitionVector.at(partition).partition_mutex));

    // Make copy of key because it is freed from memory in map function
    char* keyCopy = strdup(key);

    // Put key-values pairs in partition map and keys in set
    partitionVector.at(partition).partition_map.insert(std::pair<char*, char*>(keyCopy, value));

    //  Unlock mutex
    pthread_mutex_unlock(&(partitionVector.at(partition).partition_mutex));
}

/*
 * This is the function that reducer threads run to reduce the
 * partitions their appropriate result text files
 */
void MR_ProcessPartition(int partition_number){

    //  Get the appropriate partition
    Partition *currentPartition = &partitionVector.at(partition_number);

    //  If there are no values in the partition, return immediately
    if(currentPartition->partition_map.size() == 0) {
        pthread_exit(0);
    }

    //  Store an iterator pointing to the start of the partition for comparing
    //  keys in the MR_GetNext function
    currentPartition->partition_iterator = currentPartition->partition_map.begin();

    //  Create iterator to iterate through all unique keys of this partition
    std::multimap<char*, char*, cmp>::iterator keyIterator;

    // Iterate through each unique key and apply the reduce function to each key
    while(currentPartition->partition_iterator != currentPartition->partition_map.end()) {
        currentPartition->reduce((char *)(currentPartition->partition_iterator)->first, partition_number);
    }

    pthread_exit(0);
}

/*
 * Get the next value associated with the key or return null otherwise
 */
char *MR_GetNext(char *key, int partition_number){
    //  Get the appropriate partition
    Partition *currentPartition = &partitionVector.at(partition_number);

    //  Set the return value
    char *value = NULL;

    // Return NULL if we have reached the end of our partition
    if(currentPartition->partition_iterator == currentPartition->partition_map.end()) {
        return NULL;
    }

    //  If we haven't reached the end of our list of key, pass return the value
    if(strcmp(currentPartition->partition_iterator->first,key) == 0) {
        value = (currentPartition->partition_iterator++)->second;
    }

    return value;
}
