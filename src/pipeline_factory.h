/***************************
@Author: Chunel
@Contact: chunel@foxmail.com
@File: pipeline_factory.h
@Time: 2024/9/6 23:29
@Desc: 
***************************/

#ifndef CGRAPH_LITE_PIPELINE_FACTORY_H
#define CGRAPH_LITE_PIPELINE_FACTORY_H

#include "status.h"
#include "pipeline.h"

class GPipelineFactory {
public:
    static GPipeline* create() {
        auto pipeline = new GPipeline();
        return pipeline;
    }

    static CStatus remove(GPipeline* pipeline) {
        if (pipeline) {
            delete pipeline;
            pipeline = nullptr;
        }

        return CStatus();
    }
};

#endif //CGRAPH_LITE_PIPELINE_FACTORY_H
