# Schwimmbad Thread Pool
Portable thread pool library built on top of the POSIX Threads API. Eventually, this library will be able to run on any system with a POSIX Threads implementation, but for now it is only tested on Linux.
## Todos
This library is still a work in progress and is not ready for any use. These must be completed prior to the library's first release:
1. Integration tests.
2. A feature which allows all threads to be waited on.
3. A feature which allows the thread pool and job queue to dynamically grow and shrink.
4. Performance tests.
## Design Goals
1. **Safety**: The library should prevent any race conditions and define behavior for all edge cases.
2. **Portability**: The library should be able to run on any system with a POSIX Threads implementation.
3. **Simplicity**: The library should provide a simple interface with minimal user interaction. A user should be able to set it and forget it.
4. **Speed**: The library should minimize pool management overhead.
## Nomenclature
There are many terms used throughout the source code. Many people use these terms in different ways, so it is best to define them here.
- **Job**: A user defined procedure which is executed by a thread in the pool. Jobs have an id, function pointer, argument, and priority (ignored in FIFO implementations).
- **Thread**: A POSIX thread which executes concurrently with the main thread. Either running a job or waiting.
- **Job Queue**: A queue of jobs which are waiting to be executed by a thread in the pool. Jobs are added to the queue by the user and removed by a thread in the pool.
- **Thread Pool**: A group of threads that pull jobs from the job queue to execute them. The threads are already initialized and waiting for a job to execute.
## Usage
The library should not be used yet because I may change the API.
