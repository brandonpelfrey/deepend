
#include <chrono>
#include <cstdio>
#include "ParallelWorkQueue.h"

int main(int argc, char **argv)
{
  const int n_tasks = 1000;
  printf("Running test for %d tasks\n", n_tasks);

  ParallelWorkQueue work(32);
  for (int i = 0; i < n_tasks; ++i)
    work.Enqueue([i] {
      auto wait_time = rand() % 1000;

      long sum = 0;
      for (int j = 0; j < wait_time * 1000; ++j)
        sum += rand() * sum;

      printf(".");
      fflush(stdout);
      std::this_thread::sleep_for(std::chrono::milliseconds(wait_time));
    });

  work.AwaitCompletion();
  work.Shutdown();
  fflush(stdout);
  printf("\nCompleted.\n");

  return 0;
}