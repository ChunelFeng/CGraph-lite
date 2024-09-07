/***************************
@Author: Chunel
@Contact: chunel@foxmail.com
@File: status.h
@Time: 2024/9/6 20:31
@Desc: 
***************************/

#ifndef CGRAPH_LITE_STATUS_H
#define CGRAPH_LITE_STATUS_H

#include <string>

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

#endif //CGRAPH_LITE_STATUS_H
