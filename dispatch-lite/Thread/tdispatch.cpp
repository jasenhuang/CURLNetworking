//
//  tdispatch.cpp
//  lite
//
//  Created by jasenhuang on 15/7/22.
//  Copyright (c) 2015年 tencent. All rights reserved.
//

#include "tdispatch.h"
#include <map>
#include <assert.h>
#include "tthread.h"
#include "tlazy_instance.h"

#ifdef USE_NAMESPACE
namespace Lite {
#endif

static const char* g_thread_names[Dispatch::ID_COUNT] = {
    "DISPATCH_UI",  // UI (name assembled in browser_main.cc).
    "DISPATCH_IO",  // IO
    "DISPATCH_DB",  // DB
    "DISPATCH_LOGIC"  // LOGIC
};

struct ThreadGlobals{
    
    ThreadGlobals()
    :initialized(false){
        memset(threads, 0, sizeof(Thread*) * Dispatch::ID_COUNT);
    }
    bool initialized;
    std::mutex lock;
    Thread* threads[Dispatch::ID_COUNT];
    std::map<int, Thread*> user_threads;
    
};

LazyInstance<ThreadGlobals> g_globals = {0};

/*static function*/

void Dispatch::Init()
{
    ThreadGlobals& globals = g_globals.Get();
    if (globals.initialized) return;
    globals.initialized = true;
    
    Thread* thread = NULL;
    for (size_t thread_id = UI + 1; thread_id < ID_COUNT; ++thread_id) {
        MessageLoop::Type type = MessageLoop::TYPE_DEFAULT;
        switch (thread_id) {
            case IO:
                thread = CreateThread(g_thread_names[IO], IO);
                type = MessageLoop::TYPE_IO;
                break;
            case DB:
                thread = CreateThread(g_thread_names[DB], DB);
                type = MessageLoop::TYPE_DB;
                break;
            case LOGIC:
                thread = CreateThread(g_thread_names[LOGIC], LOGIC);
                type = MessageLoop::TYPE_DEFAULT;
                break;
            case UI:
            case ID_COUNT:
            default:
                break;
        }
        if (thread) {
            thread->Start(type);
        }
    }
}
void Dispatch::Stop()
{
    ThreadGlobals& globals = g_globals.Get();
    std::unique_lock<std::mutex> lock;
    
    Thread* thread = NULL;
    for (size_t thread_id = UI + 1; thread_id < ID_COUNT; ++thread_id) {
        thread = globals.threads[thread_id];
        if (thread != NULL) {
            thread->StopSoon();
        }
    }
    std::map<int, Thread*>::iterator iter = globals.user_threads.begin();
    for (; iter != globals.user_threads.end(); ++iter) {
        iter->second->StopSoon();
    }
    
}
Thread* Dispatch::CreateThread(const char* name)
{
    Thread* thread = new Thread(name);
    ThreadGlobals& globals = g_globals.Get();
    std::unique_lock<std::mutex> lock;
    int identifier = (int)globals.user_threads.size();
    globals.user_threads[identifier] = thread;
    return thread;
}

Thread* Dispatch::CreateThread(const char* name, ID identifier)
{
    Thread* thread = new Thread(name);
    ThreadGlobals& globals = g_globals.Get();
    std::unique_lock<std::mutex> lock;
    globals.threads[identifier] = thread;
    return thread;
}
    
Thread* Dispatch::GlobalThread()
{
    return NULL;
}

bool Dispatch::PostTask(ID identifier, const std::function<void()>& task)
{
    return PostTaskHelper(identifier, task, TimeDelta(), true);
}
bool Dispatch::PostDelayedTask(ID identifier, const std::function<void()>& task, TimeDelta delay)
{
    return PostTaskHelper(identifier, task, delay, true);
}
bool Dispatch::PostNonNestableTask(ID identifier, const std::function<void()>& task)
{
    return PostTaskHelper(identifier, task, TimeDelta(), false);
}
bool Dispatch::PostNonNestableDelayedTask(ID identifier, const std::function<void()>& task, TimeDelta delay)
{
    return PostTaskHelper(identifier, task, delay, false);
}

bool Dispatch::PostTask(Thread* thread, const std::function<void()>& task)
{
    return PostTaskHelper(thread, task, TimeDelta(), true);
}
bool Dispatch::PostDelayedTask(Thread* thread, const std::function<void()>& task, TimeDelta delay)
{
    return PostTaskHelper(thread, task, delay, true);
}
bool Dispatch::PostNonNestableTask(Thread* thread, const std::function<void()>& task)
{
    return PostTaskHelper(thread, task, TimeDelta(), false);
}
bool Dispatch::PostNonNestableDelayedTask(Thread* thread, const std::function<void()>& task, TimeDelta delay)
{
    return PostTaskHelper(thread, task, delay, false);
}

bool Dispatch::PostTaskHelper(ID identifier, const std::function<void()>& task, TimeDelta delay, bool nestable)
{
    ThreadGlobals& globals = g_globals.Get();
    assert(identifier >= 0 && identifier < ID_COUNT);
    MessageLoop* message_loop = globals.threads[identifier] ? globals.threads[identifier]->message_loop():NULL;
    if (message_loop) {
        if (nestable) {
            message_loop->PostDelayedTask(task, delay);
        } else {
            message_loop->PostNonNestableDelayedTask(task, delay);
        }
        return true;
    }
    return false;
}
bool Dispatch::PostTaskHelper(Thread* thread, const std::function<void()>& task, TimeDelta delay, bool nestable)
{
    assert(thread);
    MessageLoop* message_loop = thread->message_loop();
    if (message_loop) {
        if (nestable) {
            message_loop->PostDelayedTask(task, delay);
        } else {
            message_loop->PostNonNestableDelayedTask(task, delay);
        }
        return true;
    }
    return false;
}

#ifdef USE_NAMESPACE
};
#endif
