//
//  message_loop.cpp
//  lite
//
//  Created by jasenhuang on 15/5/21.
//  Copyright (c) 2015年 jasenhuang. All rights reserved.
//

#include "tmessage_loop.h"
#include "tlazy_instance.h"
#include "tmessage_pump_default.h"
#include "tmessage_loop_libevent.h"
#include "trun_loop.h"
#include <assert.h>

#ifdef USE_NAMESPACE
namespace Lite {
#endif

LazyInstance<ThreadLocalPointer<MessageLoop> > lazy_tls_pointer = {0};

/*Pending Task*/
PendingTask::PendingTask(const std::function<void()>& task)
:task(task), delayed_run_time(TimeTicks()) ,nestable(true),sequence_num(0)
{
}

PendingTask::PendingTask(const std::function<void()>& task, TimeTicks delayed_run_time, bool nestable)
:task(task), delayed_run_time(delayed_run_time) ,nestable(nestable),sequence_num(0)
{
}

PendingTask::~PendingTask() {
}

bool PendingTask::operator<(const PendingTask& other) const {
    
    if (delayed_run_time < other.delayed_run_time)
        return false;
    
    if (delayed_run_time > other.delayed_run_time)
        return true;
    
    return (sequence_num - other.sequence_num) > 0;
}

/*TaskQueue*/
void TaskQueue::Swap(TaskQueue* queue) {
    c.swap(queue->c);  // Calls std::deque::swap.
}

/*IncomingTaskQueue*/
IncomingTaskQueue::IncomingTaskQueue(MessageLoop* message_loop)
:next_sequence_num_(0)
{
    message_loop_ = message_loop;
}
bool IncomingTaskQueue::AddToIncomingQueue(const std::function<void ()> &task, TimeDelta delay, bool nestable)
{
    std::unique_lock<std::mutex> lock(incoming_queue_lock_);
    TimeTicks delayed_run_time;
    if (delay > TimeDelta()) {
        delayed_run_time = TimeTicks::Now() + delay;
    }
    PendingTask pending_task(task, delayed_run_time, nestable);
    PostPendingTask(&pending_task);
    return true;
}

bool IncomingTaskQueue::PostPendingTask(PendingTask* pending_task)
{
    if (!message_loop_) return false;
    
    //记录序列号
    pending_task->sequence_num = next_sequence_num_ ++;
    
    bool was_empty = incoming_queue_.empty();
    
    incoming_queue_.push(*pending_task);
    
    message_loop_->ScheduleWork(was_empty);
    
    return true;
}

void IncomingTaskQueue::ReloadWorkQueue(TaskQueue *work_queue)
{
    std::unique_lock<std::mutex> lock(incoming_queue_lock_);
    if (!incoming_queue_.empty())
        incoming_queue_.Swap(work_queue);  // Constant time
}


// MessageLoop ......
// Returns true if MessagePump::ScheduleWork() must be called one
// time for every task that is added to the MessageLoop incoming queue.
bool AlwaysNotifyPump(MessageLoop::Type type) {
#if defined(OS_ANDROID)
    return type == MessageLoop::TYPE_UI;
#else
    return false;
#endif
}

MessageLoop::MessageLoop(Type type)
:type_(type),run_loop_(NULL)
{
    Init();
    MessagePump* pump = NULL;
    if (type == MessageLoop::TYPE_UI) {
        assert(0);//UI MessageLoop is not implement yet;
    }
#ifdef USE_EVENT
    if (type == MessageLoop::TYPE_IO){
        pump = new MessagePumpLibevent();
        // messageloop for io use libevent
    }else
#endif
    pump = new MessagePumpDefault();
    pump_.reset(pump);
}
MessageLoop::~MessageLoop()
{
    assert(current());
    
    DeletePendingTasks();//TODO
    
    lazy_tls_pointer.Pointer()->Set(NULL);
}

void MessageLoop::Init()
{
    lazy_tls_pointer.Pointer()->Set(this);
    incoming_task_queue_.reset(new IncomingTaskQueue(this));
}

// Returns the MessageLoop object for the current thread, or null if none.
MessageLoop* MessageLoop::current()
{
    MessageLoop* message_loop = NULL;
    message_loop = lazy_tls_pointer.Pointer()->Get();
    assert(message_loop);
    return message_loop;
}

void MessageLoop::PostTask(const std::function<void()>& task)
{
    incoming_task_queue_->AddToIncomingQueue(task, TimeDelta(), true);
}

void MessageLoop::PostDelayedTask(const std::function<void()>& task, TimeDelta delay)
{
    incoming_task_queue_->AddToIncomingQueue(task, delay, true);
}
void MessageLoop::PostNonNestableTask(const std::function<void()>& task)
{
    incoming_task_queue_->AddToIncomingQueue(task, TimeDelta(), false);
}

void MessageLoop::PostNonNestableDelayedTask(const std::function<void()>& task, TimeDelta delay)
{
    incoming_task_queue_->AddToIncomingQueue(task, delay, false);
}
#ifdef USE_EVENT
MessagePumpLibevent* MessageLoop::PumpIO()
{
    MessagePumpLibevent* pump = dynamic_cast<MessagePumpLibevent*>(pump_.get());
    assert(pump);
    return pump;
}
#endif
    
//在thread的main函数里调用
void MessageLoop::Run()
{
    RunLoop run_loop;
    run_loop.Run();
}

void MessageLoop::RunUntilIdle()
{
    RunLoop run_loop;
    run_loop.Run();
}

void MessageLoop::Quit()
{
    assert(current());
    if (run_loop_) {
        pump_->Quit();
    }
}

void MessageLoop::QuitWhenIdle()
{
    assert(current());
    if (run_loop_) {
        run_loop_->quit_when_idle_received_ = true;
    }
}

void MessageLoop::RunHandler()
{
    pump_->Run(this);
}

bool MessageLoop::ProcessNextDelayedNonNestableTask() {
    if (run_loop_->run_depth_ != 1)
        return false;
    
    if (deferred_non_nestable_work_queue_.empty())
        return false;
    
    PendingTask pending_task = deferred_non_nestable_work_queue_.front();
    deferred_non_nestable_work_queue_.pop();
    
    RunTask(pending_task);
    return true;
}
void MessageLoop::RunTask(const PendingTask& pending_task)
{
    pending_task.task();
}

bool MessageLoop::DeferOrRunPendingTask(const PendingTask& pending_task)
{
    if (pending_task.nestable || run_loop_->run_depth_ == 1) {
        RunTask(pending_task);
        return true;
    }
    deferred_non_nestable_work_queue_.push(pending_task);
    return false;
}

void MessageLoop::AddToDelayedWorkQueue(const PendingTask& pending_task)
{
    delayed_work_queue_.push(pending_task);
}

bool MessageLoop::DeletePendingTasks()
{
    bool did_work = !work_queue_.empty();
    while (!work_queue_.empty()) {
        PendingTask pending_task = work_queue_.front();
        work_queue_.pop();
        if (!pending_task.delayed_run_time.is_null()) {
            AddToDelayedWorkQueue(pending_task);
        }
    }
    did_work |= !deferred_non_nestable_work_queue_.empty();
    while (!deferred_non_nestable_work_queue_.empty()) {
        deferred_non_nestable_work_queue_.pop();
    }
    did_work |= !delayed_work_queue_.empty();
    
    while (!delayed_work_queue_.empty()) {
        delayed_work_queue_.pop();
    }
    return did_work;
}

void MessageLoop::ReloadWorkQueue() {
    // We can improve performance of our loading tasks from the incoming queue to
    // |*work_queue| by waiting until the last minute (|*work_queue| is empty) to
    // load. That reduces the number of locks-per-task significantly when our
    // queues get large.
    if (work_queue_.empty()){
        incoming_task_queue_->ReloadWorkQueue(&work_queue_);
    }
}
//唤醒队列
void MessageLoop::ScheduleWork(bool was_empty)
{
    if (was_empty || AlwaysNotifyPump(type_)){
        pump_->ScheduleWork();
    }
}

bool MessageLoop::DoWork()
{
    for (;;) {
        ReloadWorkQueue();
        if (work_queue_.empty())
            break;
        // Execute oldest task.
        do {
            PendingTask pending_task = work_queue_.front();
            work_queue_.pop();
            if (!pending_task.delayed_run_time.is_null()) {
                AddToDelayedWorkQueue(pending_task);
                // If we changed the topmost task, then it is time to reschedule.
                if (delayed_work_queue_.top().sequence_num == pending_task.sequence_num)
                    pump_->ScheduleDelayedWork(pending_task.delayed_run_time);
            } else {
                if (DeferOrRunPendingTask(pending_task))
                    return true;
            }
        } while (!work_queue_.empty());
    }
    
    // Nothing happened.
    return false;
}
bool MessageLoop::DoDelayedWork(TimeTicks* next_delayed_work_time)
{
    if (delayed_work_queue_.empty()) {
        recent_time_ = *next_delayed_work_time = TimeTicks();
        return false;
    }
    TimeTicks next_run_time = delayed_work_queue_.top().delayed_run_time;
    if (next_run_time > recent_time_) {
        recent_time_ = TimeTicks::Now();  // Get a better view of Now();
        if (next_run_time > recent_time_) {
            *next_delayed_work_time = next_run_time;
            return false;
        }
    }
    
    PendingTask pending_task = delayed_work_queue_.top();
    delayed_work_queue_.pop();
    
    if (!delayed_work_queue_.empty())
        *next_delayed_work_time = delayed_work_queue_.top().delayed_run_time;
    
    return DeferOrRunPendingTask(pending_task);
}
bool MessageLoop::DoIdleWork()
{
    if (ProcessNextDelayedNonNestableTask()) {
        return true;
    }
    if (run_loop_->quit_when_idle_received_) {
        pump_->Quit();
    }
    return false;
}



#ifdef USE_NAMESPACE
};
#endif