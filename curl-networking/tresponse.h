//
//  tresponse.h
//  curl_networking
//
//  Created by jasenhuang on 15/7/29.
//  Copyright (c) 2015年 tencent. All rights reserved.
//

#ifndef __curl_networking__tresponse__
#define __curl_networking__tresponse__

#include <iostream>
#include <vector>
#include "tbasictypes.h"

#ifdef USE_NAMESPACE
namespace Network {
#endif

class ConnectionOperation;

class Response {
public:
    
    Response();
    virtual ~ Response();
    
    void Write(const char* buf, size_t size);
    
    std::string ResponseText(){return buf_;}
    
    std::vector<std::pair<std::string , std::string> >
    ResponseHeaders(){return headers_;}
    
    void SetFilePath(const std::string& path){file_path_ = path;};
    std::string GetFilePath(){return file_path_;}
    
protected:
    friend class ConnectionOperation;
    
    std::string     file_path_;         //下行的file path
    std::vector<std::pair<std::string , std::string> > headers_;
    
    std::string     buf_;               //下行的数据
    size_t          pos_;
    size_t          total_;

private:
    FILE*            fd_;
};

class Error{
public:
    int code;
    std::string msg;
};

#ifdef USE_NAMESPACE
};
#endif

#endif /* defined(__curl_networking__tresponse__) */
