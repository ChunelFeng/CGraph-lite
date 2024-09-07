/***************************
@Author: Chunel
@Contact: chunel@foxmail.com
@File: param_manager.h
@Time: 2024/9/7 16:38
@Desc: 
***************************/

#ifndef CGRAPH_LITE_PARAM_MANAGER_H
#define CGRAPH_LITE_PARAM_MANAGER_H

#include <unordered_map>
#include <mutex>

#include "param.h"

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
        if (iter == params_.end()) {
            return nullptr;
        }

        return dynamic_cast<T *>(iter->second);
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

#endif //CGRAPH_LITE_PARAM_MANAGER_H
