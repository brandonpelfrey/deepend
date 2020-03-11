#pragma once

#include <queue>
#include <functional>
#include <mutex>
#include <thread>
#include <chrono>

namespace deepend
{

/**
 * A simple thread pool which takes in tasks and works on them in FIFO
 * order. The threads are kept alive until the destruction of this ThreadPool.
 * 
 * When the pool is destroyed, any currently-running tasks will continue to completion,
 * but the remainder (if any) of the work queue will be ignored.
 */
class ThreadPool
{
private:
  using WorkFunction = std::function<void()>;

  // The set of tasks that remain to be worked on.
  std::queue<WorkFunction> work_queue;
  std::mutex work_queue_mutex;

  // State of the queue
  enum QueueState
  {
    // A nominal state state for the queue, ready to accept new tasks
    STATE_READY,

    // A state in which no more tasks are accepted, and the queue is shutting down.
    STATE_SHUTTING_DOWN
  };
  QueueState state = STATE_READY;

  /**
   * Atomically pull the next item from the work queue.
   * 
   * @param function a reference to to a work item to be retrieved from the queue
   * @return true if a valid work item was retrieved, else false
   */
  bool nextWorkItem(WorkFunction &function)
  {
    const std::lock_guard<std::mutex> queue_lock(work_queue_mutex);
    if (!work_queue.empty())
    {
      function = work_queue.front();
      work_queue.pop();
      return true;
    }

    return false;
  }

  // A CV for signaling that the threads currently waiting for available work (or shutdown)
  // should wake up.
  std::condition_variable wakeup_cv;
  std::mutex wakeup_mutex;

  // The set of worker threads.
  std::vector<std::thread> threads;

  // How many threads are currently sleeping, waiting for work to enter the queue.
  std::atomic_int sleeping_threads;

  void worker_thread_body(int worker_number)
  {
    WorkFunction work_item;

    while (state != STATE_SHUTTING_DOWN)
    {
      // Get the next item, or wait for a signal to try again.
      if (!nextWorkItem(work_item))
      {
        std::unique_lock<std::mutex> lk(wakeup_mutex);

        sleeping_threads++;
        wakeup_cv.wait(lk, [this] { return !work_queue.empty() || state == STATE_SHUTTING_DOWN; });
        sleeping_threads--;

        continue;
      }

      work_item();
    }
  }

public:
/**
 * Construct a new ThreadPool. 
 * @param n_workers the number of worker threads to spawn. Defaults to -1, meaning which 
 *                  launches (std::thread::hardware_concurrency) threads.
 */
  ThreadPool(unsigned int n_workers = -1)
  {
    if (n_workers == -1)
      n_workers = std::thread::hardware_concurrency();

    sleeping_threads = 0;

    for (int i = 0; i < n_workers; ++i)
      threads.push_back(std::thread(&ThreadPool::worker_thread_body, this, i));
  }

  // We don't handle replicating the state of a threadpool to another instance.
  ThreadPool(const ThreadPool &) = delete;
  ThreadPool &operator=(const ThreadPool &) = delete;

  /** @return the number of worker threads in this ThreadPool. */
  size_t n_workers() { return threads.size(); }

  /** Block execution until the work queue is empty and all workers have completed. Can be called multiple times. */
  void AwaitCompletion()
  {
    while (sleeping_threads < threads.size() && !work_queue.empty())
      std::this_thread::yield();
  }

  size_t RemainingTasks()
  {
    return threads.size() - sleeping_threads + work_queue.size();
  }

  /** Enqueue a new task for the ThreadPool to work on. Tasks are executed in FIFO order. */
  void Enqueue(const WorkFunction &work_item)
  {
    std::lock_guard<std::mutex> queue_guard(work_queue_mutex);
    work_queue.emplace(work_item);
    wakeup_cv.notify_one();
  }

  /** Destroy this threadpool. 
   * This will await completion of any currently running tasks. 
   * Any remaining queued tasks are ignored.
   */
  ~ThreadPool()
  {
    state = STATE_SHUTTING_DOWN;
    wakeup_cv.notify_all();

    for (auto &t : threads)
      t.join();
  }
};

}; // namespace deepend