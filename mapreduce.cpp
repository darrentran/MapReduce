#include "mapreduce.h"
#include "threadpool.h"
#include <pthread.h>
#include <sys/stat.h>
#include <iostream>
#include <map>
#include <set>

struct cmp {
    bool operator() (const char* a, const char* b) const {
        return strcmp(a,b) < 0;
    }
};

struct Partition{
    std::multimap<char*,char*, cmp> partition_map;
    std::set<char*, cmp> keys_set;
    std::multimap<char*, char*, cmp>::iterator partition_iterator;
    pthread_mutex_t partition_mutex;
    Reducer reduce;
};

std::vector<Partition> partitionVector;
int PARTITIONS;

int fileSizeCompare(const void* a, const void* b) {
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
        std::multimap<char*, char*>::iterator iter;
        for (iter = partitionVector.at(i).partition_map.begin(); iter != partitionVector.at(i).partition_map.end() ; ++iter)
        {
            printf("key: %s, value: %s\n", iter->first, iter->second);
        }
    }
}

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
        p.keys_set = std::set<char *, cmp>();
        pthread_mutex_init(&p.partition_mutex, NULL);

        // Add partition object to vector
        partitionVector.push_back(p);
    }

    // Sort file names by size and put them into work queue
    qsort(filenames, num_files, sizeof(char*), fileSizeCompare);
    for(int i = 0; i < num_files; i++) {
        ThreadPool_add_work(threadPool, (thread_func_t) map, filenames[i]);
    }

    ThreadPool_add_work(threadPool, NULL, NULL);

    for(int i = 0; i < num_mappers; i++) {
        pthread_join(threadPool->pool.at(i), NULL);
    }

//    printPartitionContents();

    ThreadPool_destroy(threadPool);

    pthread_t reducer_thread[num_reducers];

    for(int i = 0; i < num_reducers; i++){
        pthread_create(&reducer_thread[i], NULL, (void *(*)(void *)) &MR_ProcessPartition, (void *) (intptr_t) i);
    }

    for(int i = 0; i < num_reducers; i++) {
        pthread_join(reducer_thread[i], NULL);
    }

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

    unsigned long partition = MR_Partition(key, PARTITIONS);

    pthread_mutex_lock(&(partitionVector.at(partition).partition_mutex));
    char* keyCopy = strdup(key);
    partitionVector.at(partition).partition_map.insert(std::pair<char*, char*>(keyCopy, value));
    partitionVector.at(partition).keys_set.insert(keyCopy);
    pthread_mutex_unlock(&(partitionVector.at(partition).partition_mutex));
}

void MR_ProcessPartition(int partition_number){

    Partition currentPartition = partitionVector.at(partition_number);

    if(currentPartition.partition_map.size() == 0) {
        return;
    }

    currentPartition.partition_iterator = currentPartition.partition_map.begin();

    std::set<char*, cmp>::iterator keyIterator;
    for(keyIterator = currentPartition.keys_set.begin(); keyIterator != currentPartition.keys_set.end(); keyIterator++) {
      char *key = *keyIterator;
        currentPartition.reduce(key, partition_number);
    }
    pthread_exit(0);
}

char *MR_GetNext(char *key, int partition_number){

    Partition currentPartition = partitionVector.at(partition_number);

    if(currentPartition.partition_iterator == currentPartition.partition_map.end()) {
        return NULL;
    }

    char *value = NULL;
    if(currentPartition.partition_iterator->first == key) {
        value = (currentPartition.partition_iterator++)->second;
        printf("%s",value);
    }

    return value;
}