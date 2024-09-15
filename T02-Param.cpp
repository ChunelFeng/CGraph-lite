/***************************
@Author: Chunel
@Contact: chunel@foxmail.com
@File: T02-param.cpp
@Time: 2024/9/7 17:32
@Desc: 
***************************/

#include <iostream>

#include "./src/CGraph-lite.h"

const std::string& kParamKey = "param_key";

struct MyParam : public GParam {
    void reset(const CStatus& curStatus) override {
        val_ = 0;
    }

    int val_ { 0 };
    int loop_ { 0 };
};


class MyReadParamNode : public GNode {
public:
    CStatus init() override {
        auto status = createGParam<MyParam>(kParamKey);
        return status;
    }

    CStatus run() override {
        auto param = getGParam<MyParam>(kParamKey);
        std::cout << getName() << ": val = " << param->val_ << ", loop = " << param->loop_ << std::endl;
        return CStatus();
    }
};


class MyWriteParamNode : public GNode {
public:
    CStatus run() override {
        auto param = getGParam<MyParam>(kParamKey);
        param->val_++;
        param->loop_++;
        std::cout << getName() << ": val = " << param->val_ << ", loop = " << param->loop_ << std::endl;
        return CStatus();
    }
};


void tutorial_param() {
    GElement *w, *r = nullptr;

    auto pipeline = GPipelineFactory::create();
    pipeline->registerGElement<MyWriteParamNode>(&w, {}, "WriteNode");
    pipeline->registerGElement<MyReadParamNode>(&r, {w}, "ReadNode");
    pipeline->process(5);
    GPipelineFactory::remove(pipeline);
}


int main() {
    tutorial_param();
    return 0;
}
