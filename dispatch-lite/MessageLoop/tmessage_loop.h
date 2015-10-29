//
//  message_loop.h
//  lite
//
//  Created by jasenhuang on 15/5/21.
//  Copyright (c) 2015年 jasenhuang. All rights reserved.
//

#ifndef __lite__message_loop__
#define __lite__message_loop__

#include <iostream>
#include <queue>
#include "tbasictypes.h"
#include "ttime.h"
#include "tmessage_pump.h"

#ifdef USE_NAMESPACE
namespace Lite {
#endif

class RunLoop;
    
struct PendingTask
{
    PendingTask() = delete;
    PendingTask(const std::function<void()>& task);
    PendingTask(const std::function<void()>& task, TimeTicks delayed_run_time,bool nestable);
    ~PendingTask();
    
    bool operator<(const PendingTask& other) const;
    
    std::function<void()> task;
    
    TimeTicks delayed_run_time;
    
    int sequence_num;
    
    bool nestable;
};

class TaskQueue : public std::queue<PendingTask> {
public:
    void Swap(TaskQueue* queue);
};

class MessageLoop;
class IncomingTaskQueue {
public:
    explicit IncomingTaskQueue(MessageLoop* message_loop);
    bool AddToIncomingQueue(const std::function<void()>& task,TimeDelta delay,bool nestable);
    void ReloadWorkQueue(TaskQueue* work_queue);
    
private:
    
    bool PostPendingTask(PendingTask* pending_task);
    
    std::mutex incoming_queue_lock_;
    TaskQueue incoming_queue_;
    MessageLoop* message_loop_;
    
    int next_sequence_num_;
};
class MessagePumpLibevent;
class MessageLoop : public MessagePump::Delegate {
public:
    
    enum Type {
        TYPE_DEFAULT,
        TYPE_UI,
        TYPE_DB,
        TYPE_IO
    };
    
    explicit MessageLoop(Type type = TYPE_DEFAULT);
    ~ MessageLoop();
    
    // Returns the MessageLoop object for the current thread, or null if none.
    static MessageLoop* current();
    
    void PostTask(const std::function<void()>& task);
    
    void PostDelayedTask(const std::function<void()>& task, TimeDelta delay);
    
    void PostNonNestableTask(const std::function<void()>& task);
    
    void PostNonNestableDelayedTask(const std::function<void()>& task, TimeDelta delay);
    
    void Run();
    
    void RunUntilIdle();
    
    void Quit();
    
    void QuitWhenIdle();
    
#ifdef USE_EVENT
    MessagePumpLibevent* PumpIO();
#endif
    
private:
    friend class IncomingTaskQueue;
    friend class RunLoop;
    
    void Init();
    
    void RunHandler();
    
    bool ProcessNextDelayedNonNestableTask();
    
    void RunTask(const PendingTask& pending_task);
    
    bool DeferOrRunPendingTask(const PendingTask& pending_task);
    
    void AddToDelayedWorkQueue(const PendingTask& pending_task);
    
    bool DeletePendingTasks();
    
    void ReloadWorkQueue();
    
    void ScheduleWork(bool was_empty);
    
    virtual bool DoWork();
    virtual bool DoDelayedWork(TimeTicks* next_delayed_work_time);
    virtual bool DoIdleWork();
    
    const Type type_;
    
    TaskQueue work_queue_;
    
    TaskQueue deferred_non_nestable_work_queue_;
    
    //its first element is always the greatest of the elements it contains
    std::priority_queue<PendingTask> delayed_work_queue_;
    
    TimeTicks recent_time_;
    
    std::shared_ptr<IncomingTaskQueue> incoming_task_queue_;
    
    std::shared_ptr<MessagePump> pump_;
    
    RunLoop* run_loop_;
    
    DISALLOW_COPY_AND_ASSIGN(MessageLoop);
    
};

#ifdef USE_NAMESPACE
};
#endif
#endif /* defined(dispatchlitemessage_loop__) */
