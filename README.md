# Schwimmbad Thread Pool
Portable thread pool library build on top of the POSIX Threads API.
## Roadmap
This library is still a work in progress and is not ready for any use. These must be completed prior to the library having its first release:
1. Integration tests.
2. Enhance the communication to the user after a thread completes by optionally signalling the user's main thread.
3. A feature which allows all threads to be waited on.
4. A feature which allows a job to be canceled.
5. A feature which allows users to see all running jobs.
6. A feature which allows the thread pool and job queue to dynamically grow and shrink.
7. Performance tests.

After these are completed, the library will be in a good enough spot for my personal use.
