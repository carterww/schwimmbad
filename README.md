# Schwimmbad Thread Pool
Portable thread pool library built on top of the POSIX Threads API. Eventually, this library will be able to run on any system with a POSIX Threads implementation, but for now it is only tested on Linux.
## Todos
This library is still a work in progress and is not ready for any use. These must be completed prior to the library's first release:
1. A feature which allows the thread pool and job queue to dynamically grow and shrink.
2. Performance tests.
3. Linked list implementations of the FIFO and priority job queues.
    - Right now, the queues are contiguously allocated arrays. A linked list impl of both would allow the queue to be dynamically sized. It may result in a slow down, but that's why I would like to test it.
## Design Goals
1. **Safety**: The library should prevent race conditions and have well defined behavior.
2. **Portability**: The library should be able to run on any system with a POSIX Threads implementation.
3. **Simplicity**: The library should provide a simple interface with minimal user interaction. A user should be able to set it and forget it.
4. **Speed**: The library should minimize pool management overhead.
## Nomenclature
There are many terms used throughout the source code. Many people use these terms in different ways, so it is best to define them here.
- **Thread**: A POSIX thread which executes concurrently alongside the main thread. It can be executing a job or waiting.
- **Thread Pool**: A group of threads that pull jobs from the job queue to execute them. The threads are already initialized and waiting for a job to execute.
- **Job**: A user defined procedure which is executed by a thread in the pool. Jobs have an id, function pointer, argument, and priority (ignored in FIFO implementations).
- **Job Queue**: A queue of jobs which are waiting to be executed by a thread in the pool. Jobs are added to the queue by the user and removed by a thread in the pool.
