//
//  tthread.cpp
//  lite
//
//  Created by jasenhuang on 15/7/8.
//  Copyright (c) 2015年 tencent. All rights reserved.
//

#include "tthread.h"
#include "tlog.h"
#include <assert.h>
#include "tlazy_instance.h"

#ifdef USE_NAMESPACE
namespace Lite {
#endif

// This is used to trigger the message loop to exit.
void ThreadQuitHelper() {
    MessageLoop::current()->QuitWhenIdle();
}
#if defined(USE_PTHREAD)
void* ThreadFunc(void* context)
{
    if (context != NULL) {
        Thread* thread = static_cast<Thread*>(context);
        thread->ThreadMain();
    }
    return NULL;
}

static bool CreateThread(Thread* thread, size_t stack_size, bool joinable) {
    bool success = false;
    
    pthread_attr_t attributes;
    pthread_attr_init(&attributes);
    if (!joinable) {
        pthread_attr_setdetachstate(&attributes, PTHREAD_CREATE_DETACHED);
    }
    if (stack_size > 0)
        pthread_attr_setstacksize(&attributes, stack_size);
    
    pthread_t handle = 0;
    int err = pthread_create(&handle, &attributes, ThreadFunc, thread);
    
    success = !err;
    if (!success) {
        errno = err;
    }
    pthread_attr_destroy(&attributes);
    return success;
}
#endif

Thread::Thread(const char* name)
    :started_(false),stopping_(false),running_(false),name_(name)
{
}

Thread::~Thread()
{
    Stop();
}

/* class function */
bool Thread::Start(MessageLoop::Type type)
{
    message_loop_type_ = type;
#if defined(USE_PTHREAD)
    if (!CreateThread(this, 0, true)) {
        TLOGE("%s","failed to create thread");
        return false;
    }
#else
    thread_.reset(new std::thread( std::bind(&Thread::ThreadMain, this) ));

#endif
    std::unique_lock<std::mutex> lock(mutex_);
    condition_.wait(lock);
    
    started_ = true;
    return true;
}
void Thread::Stop()
{
    if (!started_) {
        return;
    }
    
    StopSoon();
    
#if defined(USE_PTHREAD)
    // Wait for the thread to exit.
    pthread_join(thread_, NULL);
#else
    thread_->join();
#endif
    
    started_ = false;
    
    stopping_ = false;
}

void Thread::StopSoon()
{
    if (stopping_ || !message_loop_) {
        return;
    }
    
    stopping_ = true;
    
    message_loop_->PostTask(std::bind(&ThreadQuitHelper));
}

/* thread name function */
void Thread::ThreadMain()
{
#if defined(USE_PTHREAD)
    //pthread
#if defined(OS_MACOSX)
    thread_id_ = pthread_mach_thread_np(pthread_self());
#elif defined(OS_ANDROID)
    thread_id_ = gettid();
#endif
    thread_ = pthread_self();
    
#if defined(OS_MACOSX)
    pthread_setname_np(name_.c_str());
#elif defined(OS_ANDROID)
    pthread_setname_np(thread_, name_.c_str());
#endif
    
#else
    thread_id_ = std::this_thread::get_id();//std thread
#endif
    
    message_loop_ = new MessageLoop(message_loop_type_);
    
    running_ = true;
    
    {
        std::unique_lock<std::mutex> lock(mutex_);
        condition_.notify_all();
    }
    
    message_loop_->Run();
    
    running_ = false;
    
    delete message_loop_;
    message_loop_ = NULL;
}


#ifdef USE_NAMESPACE
};
#endif

