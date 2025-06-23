#include "thread_pool.h"

ThreadPool::ThreadPool(size_t num_threads)
{
    //if we use all the threads the pc becomes slow 
    //so we remove 1 thread
    for (size_t i = 0; i < num_threads; ++i) {
        threads_.emplace_back([this] {
            while (true) {
                std::function<void()> task;
                {
                    std::unique_lock<std::mutex> lock(queue_mutex_);
                    cv_.wait(lock, [this] {
                        return !tasks_.empty() || stop_;
                    });

                    if (stop_ && tasks_.empty()) {
                        return;
                    }

                    task = std::move(tasks_.front());
                    tasks_.pop();
                    active_tasks_++; // Increment active tasks
                }

                task(); // Execute the task

                {
                    std::unique_lock<std::mutex> lock(queue_mutex_);
                    active_tasks_--; // Decrement active tasks
                    if (tasks_.empty() && active_tasks_ == 0) {
                        cv_.notify_all(); // Notify waiting threads
                    }
                }
            }
        });
    }
}

ThreadPool::~ThreadPool()
{
    {
        std::unique_lock<std::mutex> lock(queue_mutex_);
        stop_ = true;
    }

    cv_.notify_all();

    for (auto& thread : threads_) {
        thread.join();
    }
}

void ThreadPool::enqueue(std::function<void()> task)
{
    {
        std::unique_lock<std::mutex> lock(queue_mutex_);
        tasks_.emplace(std::move(task));
    }
    cv_.notify_one();
}

void ThreadPool::wait_until_empty()
{
    std::unique_lock<std::mutex> lock(queue_mutex_);
    cv_.wait(lock, [this] {
        return tasks_.empty() && (active_tasks_ == 0);
    });
}
