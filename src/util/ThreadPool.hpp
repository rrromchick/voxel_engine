#pragma once

#include <vector>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <future>
#include <functional>
#include <memory>
#include <type_traits>
#include <stdexcept>

struct ThreadPool {
    explicit ThreadPool(std::size_t threads = std::thread::hardware_concurrency()) 
        : stop(false) {
        if (threads == 0) threads = 1;

        for (std::size_t i = 0; i < threads; i++) {
            workers.emplace_back([this]() {
                while (true) {
                    std::function<void()> task;
                    {
                        std::unique_lock<std::mutex> lock(queue_mutex);
                        condition.wait(lock, [this]() {
                            return stop || tasks.empty();
                        });

                        if (stop && tasks.empty()) {
                            return;
                        }

                        task = std::move(tasks.front());
                        tasks.pop();
                    }

                    task();
                }
            });
        }
    }

    ThreadPool(const ThreadPool &other) = delete;
    ThreadPool(ThreadPool &&other) = default;
    ThreadPool &operator=(const ThreadPool &other) = delete;
    ThreadPool &operator=(ThreadPool &&other) = default;

    template <typename F, typename ...Args>
    auto enqueue(F &&f, Args&& ...args) 
        -> std::future<typename std::invoke_result_t<F, Args...>> {
        using return_type = typename std::invoke_result_t<F, Args...>;

        auto task = std::make_shared<std::packaged_task<return_type()>>(
            std::bind(std::forward<F>(f), std::forward<Args>(args)...));

        std::future<return_type> res = task->get_future();
        {
            std::unique_lock<std::mutex> lock(queue_mutex);

            if (stop) {
                throw std::runtime_error("enqueue called on stopped ThreadPool");
            }

            tasks.emplace([task]() { 
                (*task)(); 
            });
        }

        condition.notify_one();
        return res;
    }

    ~ThreadPool() {
        {
            std::unique_lock<std::mutex> lock(queue_mutex);
            stop = true;
        }

        condition.notify_all();
        for (std::thread &worker : workers) {
            if (worker.joinable()) {
                worker.join();
            }
        }
    }

    std::size_t size() const { 
        return workers.size();
    }

    std::size_t pending_tasks() {
        std::unique_lock<std::mutex> lock(queue_mutex);
        return tasks.size();
    }
    
private:
    std::vector<std::thread> workers;
    std::queue<std::function<void()>> tasks;

    std::mutex queue_mutex;
    std::condition_variable condition;
    bool stop;
};