/***************************
@Author: Chunel
@Contact: chunel@foxmail.com
@File: param.h
@Time: 2024/9/6 20:32
@Desc: 
***************************/

#ifndef CGRAPH_LITE_PARAM_H
#define CGRAPH_LITE_PARAM_H

#include "status.h"

class GParam {
protected:
    virtual CStatus setup() {
        return CStatus();
    }

    virtual void reset(const CStatus& curStatus) {
    }
};

#endif //CGRAPH_LITE_PARAM_H
