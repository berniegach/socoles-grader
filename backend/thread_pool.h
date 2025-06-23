#ifndef THREAD_POOL_H
#define THREAD_POOL_H

#include <condition_variable>
#include <functional>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>
#include <atomic>

class ThreadPool {
public:
    ThreadPool(size_t num_threads = std::thread::hardware_concurrency());
    ~ThreadPool();
    void enqueue(std::function<void()> task);
    void wait_until_empty(); // Optional: To wait for all tasks to complete

private:
    std::vector<std::thread> threads_;
    std::queue<std::function<void()> > tasks_;
    std::mutex queue_mutex_;
    std::condition_variable cv_;
    bool stop_ = false;
    std::atomic<size_t> active_tasks_{0}; // For tracking active tasks
};

#endif // THREAD_POOL_H
