//
//  tmessage_pump_default.hpp
//  lite
//
//  Created by jasenhuang on 15/7/15.
//  Copyright © 2015年 tencent. All rights reserved.
//

#ifndef tmessage_pump_default_cpp
#define tmessage_pump_default_cpp

#include "tmessage_pump.h"

#ifdef USE_NAMESPACE
namespace Lite {
#endif

class MessagePumpDefault : public MessagePump {
public:
    MessagePumpDefault();
    virtual ~MessagePumpDefault();
    
    // MessagePump methods:
    virtual void Run(Delegate* delegate);
    virtual void Quit();
    virtual void ScheduleWork();
    virtual void ScheduleDelayedWork(const TimeTicks& delayed_work_time);
    
private:
    // This flag is set to false when Run should return.
    bool keep_running_;
    
    // Used to sleep until there is more work to do.
    
    // The time at which we should call DoDelayedWork.
    TimeTicks delayed_work_time_;
    
    // Used to sleep until there is more work to do.
    std::mutex mutex_;
    std::condition_variable event_;
    
    DISALLOW_COPY_AND_ASSIGN(MessagePumpDefault);
};

#ifdef USE_NAMESPACE
};
#endif


#endif /* tmessage_pump_default_cpp */
