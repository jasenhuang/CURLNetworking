//
//  tthread.h
//  lite
//
//  Created by jasenhuang on 15/7/8.
//  Copyright (c) 2015年 tencent. All rights reserved.
//

#ifndef __lite__tthread__
#define __lite__tthread__

#include <iostream>
#include <thread>
#include "tbasictypes.h"
#include "tmessage_loop.h"

#ifdef USE_NAMESPACE
namespace Lite {
#endif

class Thread {
    
public:
    
    explicit Thread(const char* name);
    virtual ~Thread();
    
    bool Start(MessageLoop::Type type = MessageLoop::TYPE_DEFAULT);

    void Stop();
    
    void StopSoon();
    
    MessageLoop* message_loop() const { return message_loop_; }
    
private:
    
    void ThreadMain();
    
    MessageLoop* message_loop_;
    
    MessageLoop::Type message_loop_type_;
    
    // Thread name
    std::string name_;
    
#ifdef USE_PTHREAD
    
    friend void* ThreadFunc(void* context);
    
    pid_t thread_id_;
    pthread_t thread_;
#else
    std::thread::id thread_id_;
    std::unique_ptr<std::thread> thread_;
#endif
    
    // Whether we successfully started the thread.
    bool started_;
    
    // If true, we're in the middle of stopping, and shouldn't access
    // |message_loop_|. It may non-NULL and invalid.
    bool stopping_;
    
    // True while inside of Run().
    bool running_;
    
    std::mutex mutex_;
    std::condition_variable condition_;
};

#ifdef USE_NAMESPACE
};
#endif


#endif /* defined(dispatchlitetthread__) */
