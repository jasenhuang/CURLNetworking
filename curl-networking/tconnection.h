//
//  tconnection.h
//  curl_networking
//
//  Created by jasenhuang on 15/7/24.
//  Copyright (c) 2015年 tencent. All rights reserved.
//

#ifndef __curl_networking__tconnection__
#define __curl_networking__tconnection__

#include "curl.h"
#include "tbasictypes.h"

#ifdef USE_NAMESPACE
namespace Network {
#endif

#ifdef USE_EVENT
class ConnectionWatcher;
#endif

class Connection{
public:
    
    class Delegate{
    public:
        virtual void OnSuccess() = 0;
        virtual void OnError(CURLcode code) = 0;
    };
    
    Connection();
    virtual ~ Connection();

    void Initialize(Delegate* delegate);
    void UnInitialize();
    
    void Start();
    
private:
    friend class ConnectionOperationQueue;
    friend class ConnectionOperation;
    friend class ConnectionRunner;
    
    void OnSuccess();
    void OnError(CURLcode code);
    
#ifdef USE_EVENT
    friend class ConnectionWatcher;
    ConnectionWatcher*      watcher_;
#endif
    
    Delegate*               delegate_;
    
    int                     fd_;            //socket fd
    CURL*                   handler_;
    curl_slist*             headers_;
    
};

#ifdef USE_NAMESPACE
};
#endif

#endif /* defined(__curl_networking__tconnection__) */
