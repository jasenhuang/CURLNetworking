//
//  trunloop.cpp
//  lite
//
//  Created by jasenhuang on 15/5/21.
//  Copyright (c) 2015年 jasenhuang. All rights reserved.
//

#include "trun_loop.h"

#ifdef USE_NAMESPACE
namespace Lite {
#endif

RunLoop::RunLoop()
:loop_(MessageLoop::current()),running_(false),
quit_called_(false),run_called_(false),quit_when_idle_received_(false)
{
}
RunLoop::~RunLoop()
{
    
}
void RunLoop::Run()
{
    if (!BeforeRun()) return;
    loop_->RunHandler();
    AfterRun();
}
void RunLoop::RunUntilIdle()
{
    quit_when_idle_received_ = true;
    Run();
}
void RunLoop::Quit()
{
    quit_called_ = true;
    if (running_ && loop_->run_loop_ == this) {
        loop_->Quit();
    }
}

bool RunLoop::BeforeRun() {
    run_called_ = true;
    
    // Allow Quit to be called before Run.
    if (quit_called_)
        return false;
    
    // Push RunLoop stack:
    previous_run_loop_ = loop_->run_loop_;
    run_depth_ = previous_run_loop_? previous_run_loop_->run_depth_ + 1 : 1;
    loop_->run_loop_ = this;
    
    running_ = true;
    return true;
}

void RunLoop::AfterRun() {
    running_ = false;
    
    // Pop RunLoop stack:
    loop_->run_loop_ = previous_run_loop_;
    
    if (previous_run_loop_ && previous_run_loop_->quit_called_){
        loop_->Quit();
    }
}

#ifdef USE_NAMESPACE
};
#endif