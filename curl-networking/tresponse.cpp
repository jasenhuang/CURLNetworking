//
//  tresponse.cpp
//  curl_networking
//
//  Created by jasenhuang on 15/7/29.
//  Copyright (c) 2015年 tencent. All rights reserved.
//

#include "tresponse.h"

#ifdef USE_NAMESPACE
namespace Network {
#endif

Response::Response()
:pos_(0),total_(0),fd_(NULL) {
    
}

Response::~Response(){
    buf_.clear();
    pos_ = 0;
    total_ = 0;
    if (fd_) fclose(fd_),fd_ = NULL;
}

void Response::Write(const char* buf, size_t size)
{
    if (!fd_ && !file_path_.empty()) {
        fd_ = fopen(file_path_.c_str(), "wb");
    }
    if (fd_ && buf){
        fwrite(buf, size, 1, fd_);
        fflush(fd_);
    } else {
        buf_.append(buf, size);
    }
    pos_ += size;
}

#ifdef USE_NAMESPACE
};
#endif