//
//  trunloop.h
//  lite
//
//  Created by jasenhuang on 15/5/21.
//  Copyright (c) 2015年 jasenhuang. All rights reserved.
//

#ifndef __lite__trunloop__
#define __lite__trunloop__

#include "tmessage_loop.h"

#ifdef USE_NAMESPACE
namespace Lite {
#endif

/* 通过RunLoop去控制MessageLoop */
class RunLoop {
public:
    
    RunLoop();
    ~RunLoop();
    
    void Run();
    void RunUntilIdle();
    
    void Quit();
    
    bool running() const { return running_; }
    
private:
    friend class MessageLoop;
    
    bool BeforeRun();
    
    void AfterRun();
    
    bool running_;
    
    bool run_called_;
    
    bool quit_called_;
    
    bool quit_when_idle_received_;
    
    MessageLoop* loop_;
    
    RunLoop* previous_run_loop_;
    
    int run_depth_;
    
    DISALLOW_COPY_AND_ASSIGN(RunLoop);
};

#ifdef USE_NAMESPACE
};
#endif

#endif /* defined(dispatchlitetrunloop__) */
