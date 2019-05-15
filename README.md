# thread_safe_hash

### Objectives
- Multithreaded programming with Pthreads (POSIX threads)
- Practicing synchronization, use of lock variables.
- Designing and performing experiments; applying probability and statistics knowledge.

The hash table has N buckets (buckets 0 through N-1), i.e., N entries. A key i and an associate value (data), i.e., a key-value pair, is inserted into bucket j = hash(i), where j is in range [0, N-1]. The hash function is a simple hash function, i.e., hash (i) = i mod N. 
The key type is integer and valid keys are positive. The value type will be void*. Multiple key-value pairs mapping to the same bucket is added to a linked list (chaining). In this way collisions are resolved. Hence, for each bucket of the hash table there is an initially empty linked list.

The hash table is protected by multiple locks to reduce lock contention while accessing the hash table from multiple threads. There is one lock per M consecutive buckets in the hash table. M consecutive buckets is called a region. There is be N/M = K regions, hence K locks. The first M consecutive buckets is the region 0 and is protected by lock 0; the next M buckets is region 1 and protected by lock 1, and so on. While doing an operation on the hash table (like insert, delete, get) and accessing a bucket, the corresponding lock is acquired.
