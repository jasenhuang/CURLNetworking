//
//  trequest.cpp
//  mail
//
//  Created by jasenhuang on 15/7/29.
//  Copyright (c) 2015年 tencent. All rights reserved.
//

#include "trequest.h"

#ifdef USE_NAMESPACE
namespace Network {
#endif

Request::Request()
:pos_(0),total_(0),fd_(NULL),
connect_timeout_ms_(20000L),read_timeout_ms_(60000L)
{
    
}

Request::~Request(){
    buf_.clear();
    pos_ = 0;
    total_ = 0;
    if (fd_) fclose(fd_),fd_ = NULL;
}

size_t Request::Read(char* buf, size_t size)
{
    if (!fd_ && !file_path_.empty()) {
        fd_ = fopen(file_path_.c_str(), "rb");
    }
    size_t len = 0;
    if (fd_) {
        len = fread(buf, size, 1, fd_);
        
    }else if(pos_ < buf_.size()) {
        size_t len = std::min(size, (buf_.size() - pos_));
        if(len > 0) {
            memcpy(buf, &(buf_[pos_]), len);
            pos_ += len; /* advance pointer */
        }
    }
    return len;
}

void Request::SetHeader(const std::string& key, const std::string& value)
{
    headers_.push_back(std::make_pair(key, value));
}

std::string Request::GetHeader(const std::string& key)
{
    std::vector<std::pair<std::string , std::string> >::iterator iter = headers_.begin();
    for (; iter != headers_.end(); ++iter) {
        if (iter->first == key) {
            return iter->second;
            break;
        }
    }
    return "";
}

#ifdef USE_NAMESPACE
};
#endif