#include "mapreduce.h"
#include "threadpool.h"
#include <pthread.h>
#include <sys/stat.h>
#include <iostream>
#include <map>
#include <set>

struct alphabetical_order {
    bool operator()(char *const string, char *string1) {
        return string<string1;
    }

//    bool operator() (char& lhs, char& rhs)
//    {return lhs<rhs;}
};

struct Partition{
    std::multimap<char*,char*, alphabetical_order> partition_map;
    std::set<char*> keys_set;
    std::multimap<char*, char*, alphabetical_order>::iterator partition_iterator;
    pthread_mutex_t partition_mutex;
    Reducer func;
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
        p.func = concate;
        p.partition_map = std::multimap<char *, char*, alphabetical_order>();
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

    ThreadPool_destroy(threadPool); // Not actually deleting the threads...

    pthread_t p_threads[num_reducers];

    for(int i = 0; i < PARTITIONS; i++){
        pthread_create(&p_threads[i], NULL, (void *(*)(void *)) &MR_ProcessPartition, (void *) (intptr_t) i);
    }

    for(int i = 0; i < PARTITIONS; i++) {
        pthread_join(p_threads[i], NULL);
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

    std::set<char*>::iterator keyIterator;
    currentPartition.partition_iterator = currentPartition.partition_map.begin();

//    printf("partition: %d, iterator: %s\n", partition_number, currentPartition.partitionIter->first);

    for(keyIterator = currentPartition.keys_set.begin(); keyIterator != currentPartition.keys_set.end(); ++keyIterator) {
        currentPartition.func(*keyIterator, partition_number);
    }

    pthread_exit(0);
}

char *MR_GetNext(char *key, int partition_number){

    Partition currentPartition = partitionVector.at(partition_number);
    std::multimap<char*,char*, alphabetical_order>::key_compare cstr = currentPartition.partition_map.key_comp();

//    printf("%s",currentPartition.partition_iterator->first);
    char *value = NULL;

//    if(currentPartition.partition_iterator->first == key) {
//        value = (currentPartition.partition_iterator++)->second;
//    }

    if(cstr(currentPartition.partition_iterator->first, key)) {
        value = (currentPartition.partition_iterator++)->second;
    }
    return value;
}