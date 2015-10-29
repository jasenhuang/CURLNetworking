//
//  tmessage_pump_default.cpp
//  lite
//
//  Created by jasenhuang on 15/7/15.
//  Copyright © 2015年 tencent. All rights reserved.
//

#include "tmessage_pump_default.h"
#include <assert.h>

#ifdef USE_NAMESPACE
namespace Lite {
#endif

MessagePumpDefault::MessagePumpDefault()
: keep_running_(true) {
}

MessagePumpDefault::~MessagePumpDefault() {
}

void MessagePumpDefault::Run(Delegate* delegate) {
    //DCHECK(keep_running_) << "Quit must have been called outside of Run!";
    assert(keep_running_);
    
    for (;;) {
        bool did_work = delegate->DoWork();
        if (!keep_running_)
            break;
        
        did_work |= delegate->DoDelayedWork(&delayed_work_time_);
        if (!keep_running_)
            break;
        
        if (did_work)
            continue;
        // idle now
        did_work = delegate->DoIdleWork();
        if (!keep_running_)
            break;
        
        if (did_work)
            continue;
        
        //ThreadRestrictions::ScopedAllowWait allow_wait;
        if (delayed_work_time_.is_null()) {
            std::unique_lock<std::mutex> lock(mutex_);
            event_.wait(lock);
        } else {
            TimeDelta delay = delayed_work_time_ - TimeTicks::Now();
            if (delay > TimeDelta()) {
                std::unique_lock<std::mutex> lock(mutex_);
                event_.wait_for(lock, std::chrono::milliseconds(delay.InMilliseconds()));
            } else {
                // It looks like delayed_work_time_ indicates a time in the past, so we
                // need to call DoDelayedWork now.
                delayed_work_time_ = TimeTicks();
            }
        }
        // Since event_ is auto-reset, we don't need to do anything special here
        // other than service each delegate method.
    }
    
    keep_running_ = true;
}

void MessagePumpDefault::Quit() {
    keep_running_ = false;
}

void MessagePumpDefault::ScheduleWork() {
    // Since this can be called on any thread, we need to ensure that our Run
    // loop wakes up.
    //event_.Signal();
    std::unique_lock<std::mutex> lock(mutex_);
    event_.notify_one();
}

void MessagePumpDefault::ScheduleDelayedWork(const TimeTicks& delayed_work_time) {
    // We know that we can't be blocked on Wait right now since this method can
    // only be called on the same thread as Run, so we only need to update our
    // record of how long to sleep when we do sleep.
    delayed_work_time_ = delayed_work_time;
}


#ifdef USE_NAMESPACE
};
#endif