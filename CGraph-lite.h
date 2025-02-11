/***************************
@Author: Chunel
@Contact: chunel@foxmail.com
@File: CGraph-lite.h
@Time: 2024/9/6 20:33
@Desc:
***************************/

#ifndef CGRAPH_LITE_CGRAPH_LITE_H
#define CGRAPH_LITE_CGRAPH_LITE_H

#include <string>
#include <thread>
#include <mutex>
#include <unordered_map>
#include <set>
#include <atomic>
#include <vector>
#include <queue>
#include <memory>
#include <condition_variable>
#include <functional>

namespace CGraphLite {

class CStatus;
class GParam;
class GParamManager;
class Schedule;
class GElement;
class GPipeline;
class GPipelineFactory;
using GNode = GElement;

class CStatus {
public:
    explicit CStatus() = default;

    explicit CStatus(const std::string &errorInfo) {
        this->error_code_ = -1;
        this->error_info_ = errorInfo;
    }

    CStatus& operator+=(const CStatus& cur) {
        if (this->isOK() && !cur.isOK()) {
            this->error_code_ = cur.error_code_;
            this->error_info_ = cur.error_info_;
        }

        return (*this);
    }

    bool isOK() const {
        return 0 == error_code_;
    }

    int getCode() const {
        return error_code_;
    }

    std::string getInfo() const {
        return error_info_;
    }

private:
    int error_code_ { 0 };
    std::string error_info_;
};


class GParam {
public:
    std::mutex _param_shared_lock_;

protected:
    virtual CStatus setup() {
        return CStatus();
    }

    virtual void reset(const CStatus& curStatus) {}

    friend class GParamManager;
};


class GParamManager {
protected:
    template<typename T, std::enable_if_t<std::is_base_of<GParam, T>::value, int> = 0>
    CStatus create(const std::string& key) {
        std::lock_guard<std::mutex> lk(mutex_);
        auto iter = params_.find(key);
        if (iter != params_.end()) {
            auto p = iter->second;
            return (typeid(*p).name() == typeid(T).name()) ?
                   CStatus() : CStatus("create [" + key + "] param duplicate");
        }

        T* param = new(std::nothrow) T();
        params_.insert(std::pair<std::string, T*>(key, param));
        return CStatus();
    }

    template<typename T, std::enable_if_t<std::is_base_of<GParam, T>::value, int> = 0>
    T* get(const std::string& key) {
        auto iter = params_.find(key);
        return iter != params_.end() ? dynamic_cast<T *>(iter->second) : nullptr;
    }

    CStatus setup() {
        CStatus status;
        for (auto& param : params_) {
            status += param.second->setup();
        }
        return status;
    }

    void reset(const CStatus& curStatus) {
        for (auto& param : params_) {
            param.second->reset(curStatus);
        }
    }

    ~GParamManager() {
        for (auto& iter : params_) {
            delete iter.second;
        }
        params_.clear();
    }

private:
    std::unordered_map<std::string, GParam*> params_ {};
    std::mutex mutex_ {};

    friend class GParam;
    friend class GElement;
    friend class GPipeline;
};


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

    void commit(const std::function<void()>& task) {
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


class GElement {
protected:
    virtual CStatus init() {
        return CStatus();
    }

    virtual CStatus run() = 0;

    virtual CStatus destroy() {
        return CStatus();
    }

    std::string getName() const {
        return name_;
    }

    template<typename T, std::enable_if_t<std::is_base_of<GParam, T>::value, int> = 0>
    CStatus createGParam(const std::string& key) {
        return param_manager_->create<T>(key);
    }

    template<typename T, std::enable_if_t<std::is_base_of<GParam, T>::value, int> = 0>
    T* getGParam(const std::string& key) {
        return param_manager_->get<T>(key);
    }

    explicit GElement() = default;
    virtual ~GElement() = default;

private:
    void addElementInfo(const std::set<GElement *>& depends,
                        const std::string& name,
                        GParamManager* pm) {
        for (const auto& depend : depends) {
            dependence_.insert(depend);
            depend->run_before_.insert(this);
        }
        left_depend_ = dependence_.size();
        name_ = name;
        param_manager_ = pm;
    }

private:
    std::set<GElement *> run_before_ {};
    std::set<GElement *> dependence_ {};
    std::atomic<size_t> left_depend_ { 0 };
    std::string name_ {};
    GParamManager* param_manager_ = nullptr;

    friend class GPipeline;
    friend class Schedule;
};


class GPipeline {
public:
    CStatus process(size_t times = 1) {
        init();
        while (times-- && status_.isOK()) {
            run();
        }
        destroy();
        return status_;
    }

    template<typename T, std::enable_if_t<std::is_base_of<GElement, T>::value, int> = 0>
    CStatus registerGElement(GElement** elementRef,
                             const std::set<GElement *> &depends,
                             const std::string &name) {
        (*elementRef) = new(std::nothrow) T();
        (*elementRef)->addElementInfo(depends, name, &param_manager_);
        elements_.emplace_back(*elementRef);
        return CStatus();
    }

protected:
    void init() {
        status_ = CStatus();
        for (auto* element : elements_) {
            status_ += element->init();
        }
        schedule_ = std::make_unique<Schedule>();
    }

    void run() {
        setup();
        executeAll();
        reset();
    }

    void destroy() {
        for (auto* element : elements_) {
            status_ += element->destroy();
        }
        schedule_.reset();
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

    void setup() {
        finished_size_ = 0;
        for (auto* element : elements_) {
            element->left_depend_ = element->dependence_.size();
        }

        status_ += param_manager_.setup();
    }

    void reset() {
        {
            std::unique_lock<std::mutex> lk(execute_mutex_);
            execute_cv_.wait(lk, [this] {
                return finished_size_ >= elements_.size() || !status_.isOK();
            });
        }

        param_manager_.reset(status_);
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


class GPipelineFactory {
public:
    static GPipeline* create() {
        return new GPipeline();
    }

    static CStatus remove(GPipeline* pipeline) {
        delete pipeline;
        return CStatus();
    }
};

}

#endif //CGRAPH_LITE_CGRAPH_LITE_H
