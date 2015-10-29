//
//  tdispatch.h
//  lite
//
//  Created by jasenhuang on 15/7/22.
//  Copyright (c) 2015年 tencent. All rights reserved.
//

#ifndef __lite__tdispatch__
#define __lite__tdispatch__

#include "tthread.h"

#ifdef USE_NAMESPACE
namespace Lite {
#endif

#ifdef USE_NAMESPACE
    
#define dispatch_init() Lite::Dispatch::Init()
#define dispatch_stop() Lite::Dispatch::Stop()
#define dispatch_logic_async(x) Lite::Dispatch::PostTask(Lite::Dispatch::LOGIC, x)
#define dispatch_io_async(x) Lite::Dispatch::PostTask(Lite::Dispatch::IO, x)
#define dispatch_db_async(x) Lite::Dispatch::PostTask(Lite::Dispatch::DB, x)
#define dispatch_thread_async(thread, x) thread->message_loop()->PostTask(x)
#define dispatch_current_after(milisecond, x) Lite::MessageLoop::current()->PostDelayedTask(x, Lite::TimeDelta::FromMilliseconds(milisecond))

#define dispatch_get_global() Lite::Dispatch::GlobalThread()
    
#else

#define dispatch_init() Dispatch::Init()
#define dispatch_stop() Dispatch::Stop()
#define dispatch_logic_async(x) Dispatch::PostTask(Dispatch::LOGIC, x)
#define dispatch_io_async(x) Dispatch::PostTask(Dispatch::IO, x)
#define dispatch_db_async(x) Dispatch::PostTask(Dispatch::DB, x)
#define dispatch_thread_async(thread, x) thread->message_loop()->PostTask(x)
#define dispatch_current_after(milisecond, x) MessageLoop::current()->PostDelayedTask(x, TimeDelta::FromMilliseconds(milisecond))

#define dispatch_get_global() Dispatch::GlobalThread()
    
#endif

class Dispatch {
public:
    
    enum ID {
        // The main thread.
        UI = 0,
        // This is the thread that interacts with the io.
        IO,
        // This is the thread that interacts with the db.
        DB,
        // This is the thread that process logic.
        LOGIC,
        // count
        ID_COUNT
    };
    
    static void Init();
    static void Stop();

    static Thread* CreateThread(const char* name);
    static Thread* GlobalThread();
    
    static bool PostTask(ID identifier, const std::function<void()>& task);
    static bool PostDelayedTask(ID identifier, const std::function<void()>& task, TimeDelta delay);
    static bool PostNonNestableTask(ID identifier, const std::function<void()>& task);
    static bool PostNonNestableDelayedTask(ID identifier, const std::function<void()>& task, TimeDelta delay);
    
    static bool PostTask(Thread* thread, const std::function<void()>& task);
    static bool PostDelayedTask(Thread* thread, const std::function<void()>& task, TimeDelta delay);
    static bool PostNonNestableTask(Thread* thread, const std::function<void()>& task);
    static bool PostNonNestableDelayedTask(Thread* thread, const std::function<void()>& task, TimeDelta delay);
    
private:
    
    static Thread* CreateThread(const char* name, ID identifier);
    
    static bool PostTaskHelper(ID identifier, const std::function<void()>& task, TimeDelta delay, bool nestable);
    static bool PostTaskHelper(Thread* thread, const std::function<void()>& task, TimeDelta delay, bool nestable);
    
    DISALLOW_COPY_AND_ASSIGN(Dispatch);
};

#ifdef USE_NAMESPACE
};
#endif

#endif /* defined(dispatchlitetdispatch__) */
