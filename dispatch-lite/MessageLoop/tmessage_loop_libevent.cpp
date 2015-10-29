//
//  tmessage_loop_libevent.cpp
//  lite
//
//  Created by jasenhuang on 15/7/15.
//  Copyright © 2015年 tencent. All rights reserved.
//

#include "tmessage_loop_libevent.h"
#include <iostream>
#include "event.h"
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <assert.h>

#ifdef USE_NAMESPACE
namespace Lite {
#endif
    
#ifdef USE_EVENT

// Return 0 on success
// Too small a function to bother putting in a library?
static int SetNonBlocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1)
        flags = 0;
    return fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

MessagePumpLibevent::FileDescriptorWatcher::FileDescriptorWatcher()
: event_(NULL),
pump_(NULL),
watcher_(NULL) {
}

MessagePumpLibevent::FileDescriptorWatcher::~FileDescriptorWatcher() {
    if (event_) {
        StopWatchingFileDescriptor();
    }
}

bool MessagePumpLibevent::FileDescriptorWatcher::StopWatchingFileDescriptor() {
    event* e = ReleaseEvent();
    if (e == NULL)
        return true;
    
    // event_del() is a no-op if the event isn't active.
    int rv = event_del(e);
    delete e;
    pump_ = NULL;
    watcher_ = NULL;
    return (rv == 0);
}

void MessagePumpLibevent::FileDescriptorWatcher::Init(event *e) {
    event_ = e;
}

event *MessagePumpLibevent::FileDescriptorWatcher::ReleaseEvent() {
    struct event *e = event_;
    event_ = NULL;
    return e;
}

void MessagePumpLibevent::FileDescriptorWatcher::OnFileCanReadWithoutBlocking(int fd, MessagePumpLibevent* pump) {
    if (!watcher_) return;
    watcher_->OnFileCanReadWithoutBlocking(fd);
}

void MessagePumpLibevent::FileDescriptorWatcher::OnFileCanWriteWithoutBlocking(int fd, MessagePumpLibevent* pump) {
    watcher_->OnFileCanWriteWithoutBlocking(fd);
}

MessagePumpLibevent::MessagePumpLibevent()
: keep_running_(true),
processed_io_events_(false),
event_base_(event_base_new()),
wakeup_pipe_in_(-1),
wakeup_pipe_out_(-1) {
    Init();
}

MessagePumpLibevent::~MessagePumpLibevent() {
    assert(wakeup_event_);
    assert(event_base_);
    event_del(wakeup_event_);
    delete wakeup_event_;
    if (wakeup_pipe_in_ >= 0) {
        if (IGNORE_EINTR(close(wakeup_pipe_in_)) < 0)
            std::cerr << "close wakeup_pipe_in_";
    }
    if (wakeup_pipe_out_ >= 0) {
        if (IGNORE_EINTR(close(wakeup_pipe_out_)) < 0)
            std::cerr << "close wakeup_pipe_out_";
    }
    event_base_free(event_base_);
}

// WatchFileDescriptor should be called on the pump thread. It is not
// threadsafe, and your watcher may never be registered.

bool MessagePumpLibevent::WatchFileDescriptor(int fd,
                                              bool persistent,
                                              int mode,
                                              FileDescriptorWatcher *controller,
                                              Watcher *delegate) {
    assert(fd > 0);
    assert(controller);
    assert(delegate);
    assert(mode == WATCH_READ || mode == WATCH_WRITE || mode == WATCH_READ_WRITE);
    int event_mask = persistent ? EV_PERSIST : 0;
    if (mode & WATCH_READ) {
        event_mask |= EV_READ;
    }
    if (mode & WATCH_WRITE) {
        event_mask |= EV_WRITE;
    }
    
    std::unique_ptr<event> evt(controller->ReleaseEvent());
    if (evt.get() == NULL) {
        // Ownership is transferred to the controller.
        evt.reset(new event);
    } else {
        // Make sure we don't pick up any funky internal libevent masks.
        int old_interest_mask = evt.get()->ev_events &
        (EV_READ | EV_WRITE | EV_PERSIST);
        
        // Combine old/new event masks.
        event_mask |= old_interest_mask;
        
        // Must disarm the event before we can reuse it.
        event_del(evt.get());
        
        // It's illegal to use this function to listen on 2 separate fds with the
        // same |controller|.
        if (EVENT_FD(evt.get()) != fd) {
            std::cerr << "FDs don't match" << EVENT_FD(evt.get()) << "!=" << fd;
            return false;
        }
    }
    
    // Set current interest mask and message pump for this event.
    event_set(evt.get(), fd, event_mask, OnLibeventNotification, controller);
    
    // Tell libevent which message pump this socket will belong to when we add it.
    if (event_base_set(event_base_, evt.get())) {
        return false;
    }
    
    // Add this socket to the list of monitored sockets.
    if (event_add(evt.get(), NULL)) {
        return false;
    }
    
    // Transfer ownership of evt to controller.
    controller->Init(evt.release());
    
    controller->set_watcher(delegate);
    controller->set_pump(this);
    
    return true;
}
// Tell libevent to break out of inner loop.
static void timer_callback(int fd, short events, void *context)
{
    event_base_loopbreak((struct event_base *)context);
}
// static
void MessagePumpLibevent::OnLibeventNotification(int fd, short flags, void* context) {
    
    FileDescriptorWatcher* controller(static_cast<FileDescriptorWatcher*>(context));
    assert(controller);
    
    MessagePumpLibevent* pump = controller->pump();
    pump->processed_io_events_ = true;
    
    if (controller != NULL && flags & EV_WRITE) {
        controller->OnFileCanWriteWithoutBlocking(fd, pump);
    }
    if (controller != NULL && flags & EV_READ) {
        controller->OnFileCanReadWithoutBlocking(fd, pump);
    }
}
// static
void MessagePumpLibevent::OnWakeup(int socket, short flags, void* context) {
    MessagePumpLibevent* that = static_cast<MessagePumpLibevent*>(context);
    assert(that->wakeup_pipe_out_ == socket);
    // Remove and discard the wakeup byte.
    char buf;
    long nread = HANDLE_EINTR(read(socket, &buf, 1));
    assert(nread == 1);
    that->processed_io_events_ = true;
    // Tell libevent to break out of inner loop.
    event_base_loopbreak(that->event_base_);
}


/* virtual function implement*/

bool MessagePumpLibevent::Init() {
    int fds[2];
    if (pipe(fds)) {
        std::cerr << "pipe() failed, errno: " << errno;
        return false;
    }
    if (SetNonBlocking(fds[0])) {
        std::cerr << "SetNonBlocking for pipe fd[0] failed, errno: " << errno;
        return false;
    }
    if (SetNonBlocking(fds[1])) {
        std::cerr << "SetNonBlocking for pipe fd[1] failed, errno: " << errno;
        return false;
    }
    wakeup_pipe_out_ = fds[0];
    wakeup_pipe_in_ = fds[1];
    
    wakeup_event_ = new event;
    event_set(wakeup_event_, wakeup_pipe_out_, EV_READ | EV_PERSIST,
              OnWakeup, this);
    event_base_set(event_base_, wakeup_event_);
    
    if (event_add(wakeup_event_, 0))
        return false;
    return true;
}

void MessagePumpLibevent::Run(Delegate* delegate) {
    //DCHECK(keep_running_) << "Quit must have been called outside of Run!";
    assert(keep_running_);
    
    // event_base_loopexit() + EVLOOP_ONCE is leaky, see http://crbug.com/25641.
    // Instead, make our own timer and reuse it on each call to event_base_loop().
    std::unique_ptr<event> timer_event(new event);
    
    for (;;) {
        
        bool did_work = delegate->DoWork();
        if (!keep_running_)
            break;
        
        event_base_loop(event_base_, EVLOOP_NONBLOCK);
        did_work |= processed_io_events_;
        processed_io_events_ = false;
        if (!keep_running_)
            break;
        
        did_work |= delegate->DoDelayedWork(&delayed_work_time_);
        if (!keep_running_)
            break;
        
        if (did_work)
            continue;
        
        did_work = delegate->DoIdleWork();
        if (!keep_running_)
            break;
        
        if (did_work)
            continue;
        
        // EVLOOP_ONCE tells libevent to only block once,
        // but to service all pending events when it wakes up.
        if (delayed_work_time_.is_null()) {
            event_base_loop(event_base_, EVLOOP_ONCE);
        } else {
            TimeDelta delay = delayed_work_time_ - TimeTicks::Now();
            if (delay > TimeDelta()) {
                struct timeval poll_tv;
                poll_tv.tv_sec = (long)delay.InSeconds();
                poll_tv.tv_usec = delay.InMicroseconds() % Time::kMicrosecondsPerSecond;
                event_set(timer_event.get(), -1, 0, timer_callback, event_base_);
                event_base_set(event_base_, timer_event.get());
                event_add(timer_event.get(), &poll_tv);
                event_base_loop(event_base_, EVLOOP_ONCE);
                event_del(timer_event.get());
            } else {
                delayed_work_time_ = TimeTicks();
            }
        }
    }
    
    keep_running_ = true;
}

void MessagePumpLibevent::Quit() {
    // Tell both libevent and Run that they should break out of their loops.
    keep_running_ = false;
    ScheduleWork();
}

void MessagePumpLibevent::ScheduleWork() {
    // Tell libevent (in a threadsafe way) that it should break out of its loop.
    char buf = 0;
    long nwrite = HANDLE_EINTR(write(wakeup_pipe_in_, &buf, 1));
    assert(nwrite == 1 || errno == EAGAIN);
}

void MessagePumpLibevent::ScheduleDelayedWork(const TimeTicks& delayed_work_time) {
    delayed_work_time_ = delayed_work_time;
}

#endif
    
#ifdef USE_NAMESPACE
};
#endif