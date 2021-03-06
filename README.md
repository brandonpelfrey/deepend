# deepend
This is a basic C++ Thread Pool implementation for use in other projects.

# Example
```cpp
#include <chrono>
#include <cstdio>
#include "ThreadPool.h"

int main(int argc, char **argv)
{
  using deepend::ThreadPool;

  const int n_workers = 16;
  ThreadPool work(n_workers);

  const int n_tasks = 10000;
  for (int i = 0; i < n_tasks; ++i)
    work.Enqueue([&] {
      // Do expensive calculation.
    });

  // Wait for all of the queue and currently running work to complete.
  // This is a blocking call. 
  work.AwaitCompletion();

  // Could potentially add more work at this point.

  // NOTE : Worker threads will shutdown on thread pool destruction
  return 0;
}
```

## todo
- [❌] Return values as future
  - *After reading, sounds like this might have a lot of overhead for the futures and messaging, but still needs investigation.*
- [ ] Basic testing
