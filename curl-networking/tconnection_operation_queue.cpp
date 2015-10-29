//
//  tconnection_operation_queue.cpp
//  curl_networking
//
//  Created by jasenhuang on 15/7/24.
//  Copyright (c) 2015年 tencent. All rights reserved.
//

#include "tconnection_operation_queue.h"
#include "tconnection_operation.h"
#include "tconnection.h"
#include "tdispatch.h"

#ifdef USE_NAMESPACE
namespace Network {
#endif

ConnectionOperationQueue::ConnectionOperationQueue(int count)
:max_count_(count)
{
    dispatch_init();
}
ConnectionOperationQueue::~ConnectionOperationQueue()
{
}

void ConnectionOperationQueue::AddOperation(std::shared_ptr<ConnectionOperation> oper)
{
    dispatch_io_async([=]{
        priority_queue_.push(oper);
        oper->SetDelegate(this);
        Schedule();
    });
}

void ConnectionOperationQueue::Schedule()
{
    std::shared_ptr<Connection> conn = NULL;
    if (!max_count_ || running_queue_.size() < max_count_) {
        conn = std::make_shared<Connection>();
    }
    if (conn.get() != NULL) {
        std::shared_ptr<ConnectionOperation> oper = priority_queue_.top();
        oper->connection_ = conn;
        priority_queue_.pop();
        running_queue_.push_back(oper);
        oper->Start();
    }
}

/* virtual function delegate*/

void ConnectionOperationQueue::OnStart(ConnectionOperation* oper)
{
    //empty
}
void ConnectionOperationQueue::OnFinish(ConnectionOperation* oper)
{
    //Run in IO Thread

    /* remove from running queue*/
    std::vector<std::shared_ptr<ConnectionOperation> >::iterator iter = running_queue_.begin();
    for (; iter != running_queue_.end(); ++iter) {
        if (iter->get() == oper) break;
    }
    running_queue_.erase(iter);
    
    /* schedule next operation*/
    if (max_count_) {
        Schedule();
    }
}

#ifdef USE_NAMESPACE
};
#endif

