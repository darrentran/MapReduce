# CMPUT379-Assignment2 dtran1 - 1495312

#How the intermediate key/value pairs are stored:
The intermediate key/value pairs are stored in a vector of Partition items. Partition items are a struct composed 
of a mutex lock, a multimap, a multimap iterator and a reduce function. The mutex lock within each partition is a 
fine grained locking mechanism. This allows for mappers to write to each individual partition safely without locking
the entire partition vector. The multimap is the data structure that stores key value pairs, it is sorted based on the 
comparator function provided in the template. The multimap iterator is used to compare the current value of the iterator
with the key being reduced. 

# Time complexity of MR_Emit and MR_GetNext functions:
*MR_Emit*: The time complexity is `O(logN)`. We get the partition and we insert it into the multimap. 
Multimap time complexity is `O(logN)`
*MR_GetNext*: The time complexity `O(1)`. We compare the key with the current current value pointed to by the iterator.
These comparisons take constant time, so overall the time complexity is `O(1)`.
 
# Implementation of thread pool library:
The data structure used to implement the task queue was a c++ queue. A queue is used to push work items to the back 
and pop work items from the front so that they can be worked on in the order in which they were passed in. This is 
important because we sort these work items by size before passing them in.

The threadpool library contains a vector of all the p_threads, the work queue, one mutex lock, one condition variable,
one boolean flag to let threads know they need to stop running and also an integer to count the number of live threads.
The mutex is used to lock the work queue when threads are attempting to add and remove work items. The condition 
variable is to let the threads wait until they are signalled work is available. When running in their loop, they first 
check to see if there are an items in the work queue. If there are no items in the work queue, they will wait on this 
condition. When work is added to the work queue, the condition will be signal to the waiting threads and the first
thread to wake up and take the work item will do the work. If there are already work items in the queue, the threads 
will not wait and automatically pick up the work items in the queue. The boolean flag is used to let threads know when
to stop execution. When the boolean flag is set to true, the threads will exit out of their main loop. when a thread
exits out of their main loop, a the live_threads counter will also decrease. 

# How I tested my code:
To test my code, I would first add elements into my partitions and print out the results in each partition. I would do 
this immediately after the map step finishes. For debugging and testing, I would put print statements in different 
sections of the code to see where I could see which parts of my code will get a segmentation fault. I also used the  
debugger to step through my code to figure out which sections would give me segmentation faults. I also used valgrind
to check for memory leaks and erorrs in my code. To test the correctness, I checked against the test cases provided
on eclass and also the example test cases posted in the eclass forums.


Links used:
[Threadpools](https://stackoverflow.com/questions/22030027/c11-dynamic-threadpool)
[Threadpools](https://nachtimwald.com/2019/04/12/thread-pool-in-c/)
[Threadpools](https://stackoverflow.com/questions/10865865/using-pthreads-as-a-thread-pool-with-queue)
[pthread_join](https://stackoverflow.com/questions/22427007/difference-between-pthread-exit-pthread-join-and-pthread-detach)
[pthread_join](http://man7.org/linux/man-pages/man3/pthread_join.3.html)
[pthread_create](http://man7.org/linux/man-pages/man3/pthread_create.3.html)
[pthread_mutex_init](https://linux.die.net/man/3/pthread_mutex_init)
[pthread_cond_init](https://linux.die.net/man/3/pthread_cond_init)
[pointers](https://stackoverflow.com/questions/56938324/malloc-error-for-object-pointer-being-freed-was-not-allocated)
[sort](https://en.cppreference.com/w/cpp/algorithm/sort)