/***************************
@Author: Chunel
@Contact: chunel@foxmail.com
@File: T01-simple.cpp
@Time: 2024/9/6 20:24
@Desc: 
***************************/

#include <iostream>
#include <chrono>

#include "./src/CGraph-lite.h"


class MyNode1 : public GNode {
    CStatus run() override {
        std::cout << getName() << ": sleep 1s.\n";
        std::this_thread::sleep_for(std::chrono::seconds(1));
        return CStatus();
    }
};

class MyNode2 : public GNode {
    CStatus run() override {
        std::cout << getName() << ": sleep 2s.\n";
        std::this_thread::sleep_for(std::chrono::seconds(2));
        return CStatus();
    }
};


void tutorial_simple() {
    GElement *a, *b, *c, *d = nullptr;

    auto pipeline = GPipelineFactory::create();
    pipeline->registerGElement<MyNode1>(&a, {}, "nodeA");
    pipeline->registerGElement<MyNode2>(&b, {a}, "nodeB");
    pipeline->registerGElement<MyNode1>(&c, {a}, "nodeC");
    pipeline->registerGElement<MyNode2>(&d, {b, c}, "nodeD");

    pipeline->process();
    GPipelineFactory::remove(pipeline);
}


int main() {
    tutorial_simple();
    return 0;
}
