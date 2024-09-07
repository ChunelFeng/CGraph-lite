/***************************
@Author: Chunel
@Contact: chunel@foxmail.com
@File: pipeline.h
@Time: 2024/9/6 20:31
@Desc:
***************************/

#ifndef CGRAPH_LITE_PIPELINE_H
#define CGRAPH_LITE_PIPELINE_H

#include <set>
#include <vector>
#include <atomic>
#include <algorithm>
#include <mutex>
#include <memory>

#include "element.h"
#include "status.h"
#include "schedule.h"
#include "param_manager.h"

class GPipeline {
public:
    /**
     * init() + run(n) + destroy()
     * @param times
     * @return
     */
    CStatus process(size_t times = 1) {
        auto status = init();
        while (times-- && status.isOK()) {
            status += run();
        }
        status += destroy();
        return status;
    }

    /**
     * register element(node) into pipeline
     * @tparam T
     * @param elementRef
     * @param depends
     * @param name
     * @return
     */
    template<typename T,
            std::enable_if_t<std::is_base_of<GElement, T>::value, int> = 0>
    CStatus registerGElement(GElement** elementRef,
                             const std::set<GElement *> &depends,
                             const std::string &name) {
        if (std::any_of(depends.begin(), depends.end(), [](GElement* ptr) {
            return !ptr;
        })) {
            // no allow empty input
            return CStatus("input is null");
        }

        (*elementRef) = new(std::nothrow) T();
        (*elementRef)->addElementInfo(depends, name, &param_manager_);
        elements_.emplace_back(*elementRef);
        return CStatus();
    }

protected:
    CStatus init() {
        CStatus status;
        for (auto* element : elements_) {
            status += element->init();
        }
        schedule_ = std::make_unique<Schedule>();
        return status;
    }

    CStatus run() {
        setup();
        executeAll();
        return fetchResult();
    }

    CStatus destroy() {
        CStatus status;
        for (auto* element : elements_) {
            status += element->destroy();
        }
        return status;
    }

    void executeAll() {
        for (auto* element : elements_) {
            if (element->dependence_.empty()) {
                schedule_->commit([this, element] {
                    execute(element);
                });
            }
        }
    }

    void execute(GElement* element) {
        if (!status_.isOK()) {
            return;
        }

        status_ += element->run();
        for (auto* cur : element->run_before_) {
            if (--cur->left_depend_ <= 0) {
                schedule_->commit([this, cur] {
                    execute(cur);
                });
            }
        }

        std::unique_lock<std::mutex> lk(execute_mutex_);
        if (++finished_size_ >= elements_.size() || !status_.isOK()) {
            execute_cv_.notify_one();
        }
    }

    CStatus fetchResult() {
        {
            std::unique_lock<std::mutex> lk(execute_mutex_);
            execute_cv_.wait(lk, [this] {
                return finished_size_ >= elements_.size() || !status_.isOK();
            });
        }

        param_manager_.reset(status_);
        return status_;
    }

    void setup() {
        status_ = CStatus();
        finished_size_ = 0;
        for (auto* element : elements_) {
            element->left_depend_ = element->dependence_.size();
        }

        status_ += param_manager_.setup();
    }


private:
    std::vector<GElement *> elements_ {};
    std::unique_ptr<Schedule> schedule_ = nullptr;
    size_t finished_size_ {0};
    std::mutex execute_mutex_ {};
    std::condition_variable execute_cv_ {};
    CStatus status_;
    GParamManager param_manager_;
};

#endif //CGRAPH_LITE_PIPELINE_H
