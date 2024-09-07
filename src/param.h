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
    /**
     * exec before all node run.
     * @return
     */
    virtual CStatus setup() {
        return CStatus();
    }

    /**
     * exec after all node run finished.
     * @param curStatus
     */
    virtual void reset(const CStatus& curStatus) {
    }

    friend class GParamManager;
};

#endif //CGRAPH_LITE_PARAM_H
