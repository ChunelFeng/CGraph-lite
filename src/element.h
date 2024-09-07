/***************************
@Author: Chunel
@Contact: chunel@foxmail.com
@File: element.h
@Time: 2024/9/6 20:44
@Desc: 
***************************/

#ifndef CGRAPH_LITE_ELEMENT_H
#define CGRAPH_LITE_ELEMENT_H

#include <set>
#include <string>
#include <atomic>

#include "status.h"

class GElement {
protected:
    explicit GElement() = default;

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

    virtual ~GElement() = default;

    void addElementInfo(const std::set<GElement *>& depends,
                        const std::string& name) {
        for (const auto& depend : depends) {
            dependence_.insert(depend);
            depend->run_before_.insert(this);
        }
        left_depend_ = dependence_.size();
        name_ = name;
    }

private:
    std::set<GElement *> run_before_ {};
    std::set<GElement *> dependence_ {};
    std::atomic<size_t> left_depend_ { 0 };
    std::string name_ {};

    friend class GPipeline;
    friend class Schedule;
};


using GNode = GElement;

#endif //CGRAPH_LITE_ELEMENT_H
