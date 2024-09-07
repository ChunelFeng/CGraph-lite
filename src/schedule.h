/***************************
@Author: Chunel
@Contact: chunel@foxmail.com
@File: schedule.h
@Time: 2024/9/6 20:54
@Desc:
***************************/

#ifndef CGRAPH_LITE_SCHEDULE_H
#define CGRAPH_LITE_SCHEDULE_H

#include <vector>
#include <queue>
#include <memory>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <functional>


class Schedule {
public:
    explicit Schedule(size_t num = 8) {
        for (size_t i = 0; i < num; i++) {
            workers_.emplace_back([this]{
                for(;;) {
                    std::function<void()> task;
                    {
                        std::unique_lock<std::mutex> lock(this->queue_mutex_);
                        this->cv_.wait(lock, [this]{
                            return this->stop_ || !this->tasks_.empty();
                        });
                        if(this->stop_ && this->tasks_.empty()) {
                            return;
                        }
                        task = std::move(this->tasks_.front());
                        this->tasks_.pop();
                    }

                    task();
                }
            });
        }
    }

    void push(const std::function<void()>& task) {
        std::unique_lock<std::mutex> lock(queue_mutex_);
        if (stop_) {
            return;
        }
        tasks_.emplace(task);
        cv_.notify_one();
    }

    ~Schedule() {
        {
            std::unique_lock<std::mutex> lock(queue_mutex_);
            stop_ = true;
        }
        cv_.notify_all();
        for(std::thread &worker: workers_) {
            worker.join();
        }
    }

private:
    std::vector<std::thread> workers_;
    std::queue<std::function<void()>> tasks_;
    std::mutex queue_mutex_;
    std::condition_variable cv_;
    bool stop_ = false;
};

#endif //CGRAPH_LITE_SCHEDULE_H
