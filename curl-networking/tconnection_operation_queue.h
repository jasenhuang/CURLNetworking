//
//  tconnection_operation_queue.h
//  curl_networking
//
//  Created by jasenhuang on 15/7/24.
//  Copyright (c) 2015年 tencent. All rights reserved.
//

#ifndef __curl_networking__tconnection_operation_queue__
#define __curl_networking__tconnection_operation_queue__

#include <iostream>
#include <queue>
#include <vector>
#include "tconnection_operation_delegate.h"

#ifdef USE_NAMESPACE
namespace Network {
#endif

class Connection;
class ConnectionOperation;
class ConnectionOperationDelegate;

typedef std::priority_queue<std::shared_ptr<ConnectionOperation> > CONNECTION_PRIORITY_QUEUE;

class ConnectionOperationQueue : public ConnectionOperationDelegate{
    
public:
    /* connection count, default is 0, no limit */
    explicit ConnectionOperationQueue(int max_count = 0);
    ~ ConnectionOperationQueue();
    
    /* add operation to queue and start automatically*/
    void AddOperation(std::shared_ptr<ConnectionOperation> oper);
    
protected:
    /* delegate */
    virtual void OnStart(ConnectionOperation* oper);
    virtual void OnFinish(ConnectionOperation* oper);
    
private:
    
    void Schedule();
    
    int max_count_;
    
    CONNECTION_PRIORITY_QUEUE priority_queue_;
    
    std::vector<std::shared_ptr<ConnectionOperation> > running_queue_;
    
//    std::vector<std::shared_ptr<Connection> > connections_;
    
};

#ifdef USE_NAMESPACE
};
#endif

#endif /* defined(__curl_networking__tconnection_operation_queue__) */
