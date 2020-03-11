
#include <chrono>
#include <cstdio>
#include "ThreadPool.h"

int main(int argc, char **argv)
{
  using deepend::ThreadPool;

  // Create a ThreadPool with specified number of threads.
  // (If you specify none, it's determined by the hardware.)
  ThreadPool work(32);

  // Enqueue a bunch of tasks to be worked on by the thread pool.
  for (int i = 0; i < 100; ++i)
    work.Enqueue([&] {
      // Do expensive calculation.
      std::this_thread::sleep_for(std::chrono::milliseconds(rand() % 500));
    });

  // Wait for all of the queue and currently running work to complete.
  // This is a blocking call. 
  work.AwaitCompletion();

  // Could potentially add more work at this point.

  // NOTE : Worker threads will shutdown on thread pool destruction
  return 0;
}