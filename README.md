# synchronization_and_deadlocks

### Objectives
- Practice multi-threaded programming.
- Practice synchronization: mutex and condition variables; Pthreads API.
- Practice deadlock detection and avoidance methods.
- Practice designing and performing experiments.

This is a resource allocation library that simulates the behaviour of a kernel in terms of resource allocation and deadlock handling. Like a kernel, it allocates resources to multiple processes. It can do deadlock avoidance and deadlock detection. Multiple processes requesting resources is simulated with multiple threads. A multithreaded application using the library can create multiple threads and each thread will be like a process requesting resources whenever needed and releasing when finished. The library does resource access control and allocation. If resources are not available or it is not safe to allocate, the requesting thread is blocked by the library.
