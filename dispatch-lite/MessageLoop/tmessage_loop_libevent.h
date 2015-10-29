//
//  tmessage_loop_libevent.hpp
//  lite
//
//  Created by jasenhuang on 15/7/15.
//  Copyright © 2015年 tencent. All rights reserved.
//

#ifndef tmessage_loop_libevent_cpp
#define tmessage_loop_libevent_cpp

#include "tmessage_pump.h"

struct event_base;
struct event;

#ifdef USE_NAMESPACE
namespace Lite {
#endif
    
#ifdef USE_EVENT

class MessagePumpLibevent : public MessagePump{
    
public:
    MessagePumpLibevent();
    virtual ~MessagePumpLibevent();
    
    // MessagePump methods:
    virtual void Run(Delegate* delegate);
    virtual void Quit();
    virtual void ScheduleWork();
    virtual void ScheduleDelayedWork(const TimeTicks& delayed_work_time);
    
    class Watcher {
    public:
        virtual void OnFileCanReadWithoutBlocking(int fd) = 0;
        virtual void OnFileCanWriteWithoutBlocking(int fd) = 0;
        
    protected:
        virtual ~Watcher() {}
    };
    
    class FileDescriptorWatcher {
    public:
        FileDescriptorWatcher();
        ~FileDescriptorWatcher();  // Implicitly calls StopWatchingFileDescriptor.
        
        bool StopWatchingFileDescriptor();
        
    private:
        friend class MessagePumpLibevent;
        
        // Called by MessagePumpLibevent, ownership of |e| is transferred to this
        // object.
        void Init(event* e);
        
        // Used by MessagePumpLibevent to take ownership of event_.
        event* ReleaseEvent();
        
        void set_pump(MessagePumpLibevent* pump) { pump_ = pump; }
        MessagePumpLibevent* pump() const { return pump_; }
        
        void set_watcher(Watcher* watcher) { watcher_ = watcher; }
        
        void OnFileCanReadWithoutBlocking(int fd, MessagePumpLibevent* pump);
        void OnFileCanWriteWithoutBlocking(int fd, MessagePumpLibevent* pump);
        
        event* event_;
        MessagePumpLibevent* pump_;
        Watcher* watcher_;
        
        DISALLOW_COPY_AND_ASSIGN(FileDescriptorWatcher);
    };
    enum Mode {
        WATCH_READ = 1 << 0,
        WATCH_WRITE = 1 << 1,
        WATCH_READ_WRITE = WATCH_READ | WATCH_WRITE
    };

    bool WatchFileDescriptor(int fd,
                             bool persistent,
                             int mode,
                             FileDescriptorWatcher *controller,
                             Watcher *delegate);
    
private:
    
    bool Init();
    
    static void OnLibeventNotification(int fd, short flags,void* context);
    static void OnWakeup(int socket, short flags, void* context);
    
    // This flag is set to false when Run should return.
    bool keep_running_;
    
    bool processed_io_events_;
    
    // The time at which we should call DoDelayedWork.
    TimeTicks delayed_work_time_;
    
    event_base* event_base_;
    
    // ... write end; ScheduleWork() writes a single byte to it
    int wakeup_pipe_in_;
    // ... read end; OnWakeup reads it and then breaks Run() out of its sleep
    int wakeup_pipe_out_;
    
    event* wakeup_event_;
    
    // Used to sleep until there is more work to do.
    std::mutex mutex_;
    std::condition_variable event_;
    
};
#endif

#ifdef USE_NAMESPACE
};
#endif

#endif /* tmessage_loop_libevent_cpp */
