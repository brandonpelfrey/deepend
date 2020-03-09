#pragma once

#include <queue>
#include <functional>
#include <mutex>
#include <thread>
#include <chrono>

class ParallelWorkQueue
{
private:
  using WorkFunction = std::function<void()>;
  std::queue<WorkFunction> work_queue;
  std::mutex work_queue_mutex;

  std::condition_variable work_queue_exhausted;

  // State of the queue
  enum QueueState
  {
    STATE_READY,
    STATE_SHUTTING_DOWN
  };
  QueueState state = STATE_READY;

  bool nextWorkItem(WorkFunction &function)
  {
    const std::lock_guard<std::mutex> queue_lock(work_queue_mutex);
    if (!work_queue.empty())
    {
      function = work_queue.front();
      work_queue.pop();

      if (work_queue.empty())
        work_queue_exhausted.notify_all();

      return true;
    }
    else
    {
      return false;
    }
  }

  std::condition_variable wakeup_cv;
  std::mutex wakeup_mutex;

  std::vector<std::thread> threads;
  void worker_thread_body(int worker_number)
  {
    WorkFunction work_item;

    //printf("[worker-%d] : starting\n", worker_number);
    while (true)
    {

      // Are we still alive?
      if (state == STATE_SHUTTING_DOWN)
      {
        //printf("[worker-%d] : detected queue in shutting-down state, exiting.\n", worker_number);
        break;
      }

      // Get the next item, or wait for a signal.
      if (!nextWorkItem(work_item))
      {
        // Wait for a signal to wake-up.
        std::unique_lock<std::mutex> lk(wakeup_mutex);
        wakeup_cv.wait(lk, [this] { return !work_queue.empty(); });
        //printf("[worker-%d] : woke up\n", worker_number);
        continue;
      }

      // If the queue is in shutdown, then just break out
      //printf("[worker-%d] : doing work item\n", worker_number);
      work_item();
    }
  }

public:
  ParallelWorkQueue(unsigned int n_workers = -1)
  {
    if (n_workers == -1)
      n_workers = std::thread::hardware_concurrency();

    // Start n threads, all waiting
    for (int i = 0; i < n_workers; ++i)
      threads.push_back(std::thread(&ParallelWorkQueue::worker_thread_body, this, i));
  }

  size_t n_workers() { return threads.size(); }

  void AwaitCompletion()
  {
    std::mutex wait_mu;
    std::unique_lock<std::mutex> wait_lk(wait_mu);
    work_queue_exhausted.wait(wait_lk, [this] { return work_queue.empty(); });
  }

  void Enqueue(const WorkFunction &work_item)
  {
    std::lock_guard<std::mutex> queue_guard(work_queue_mutex);
    work_queue.emplace(work_item);
    wakeup_cv.notify_one();
  }

  void Shutdown()
  {
    state = STATE_SHUTTING_DOWN;
    wakeup_cv.notify_all();

    for (auto &t : threads)
      t.join();
  }
};