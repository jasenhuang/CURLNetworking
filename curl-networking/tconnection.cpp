//
//  tconnection.cpp
//  curl_networking
//
//  Created by jasenhuang on 15/7/24.
//  Copyright (c) 2015年 tencent. All rights reserved.
//

#include "tconnection.h"
#include "tlazy_instance.h"
#include "tdispatch.h"
#include "tlog.h"
#include <map>
#include <assert.h>
#ifdef USE_EVENT
#include "tmessage_loop_libevent.h"
#endif

#ifdef USE_NAMESPACE
namespace Network {
#endif

class ConnectionRunner;
Lite::LazyInstance<ConnectionRunner> lazy_runner_pointer = {0};

#ifdef USE_EVENT
class ConnectionWatcher : public Lite::MessagePumpLibevent::Watcher {
public:
    explicit ConnectionWatcher(Connection* conn):connection_(conn){}
    ~ ConnectionWatcher(){};
    
    virtual void OnFileCanReadWithoutBlocking(int fd) ;
    virtual void OnFileCanWriteWithoutBlocking(int fd) ;
    
    void SetMode(Lite::MessagePumpLibevent::Mode mode){mode_ = mode;}
    Lite::MessagePumpLibevent::Mode GetMode(){return mode_;}
    
    Lite::MessagePumpLibevent::FileDescriptorWatcher controller;
    
private:
    Connection* connection_;
    Lite::MessagePumpLibevent::Mode mode_;
};
#endif

class ConnectionRunner {
public:
    ConnectionRunner(){
        m_ = curl_multi_init();
#ifdef USE_EVENT
        curl_multi_setopt(m_, CURLMOPT_SOCKETFUNCTION, SocketCallback) ;
        curl_multi_setopt(m_, CURLMOPT_TIMERFUNCTION, TimerCallback) ;
        curl_multi_setopt(m_, CURLMOPT_SOCKETDATA, this) ;
        curl_multi_setopt(m_, CURLMOPT_TIMERDATA, this) ;
#endif
    }
    ~ ConnectionRunner(){
        curl_multi_cleanup(m_);m_ = NULL;
    }
    
    void Run(Connection* conn);
    
private:
    friend class ConnectionWatcher;
    
#ifdef USE_EVENT
    static int SocketCallback(CURL *easy,      /* easy handle */
                              curl_socket_t s, /* socket */
                              int what,        /* see above */
                              void *userp,     /* private callbackpointer */
                              void *socketp);
    
    static int TimerCallback(CURLM *multi,    /* multi handle */
                             long timeout_ms, /* see above */
                             void *userp);    /* private callback pointer */
    
    void StepSocket(curl_socket_t sock, int action, CURL* easy);
#else
    void Schedule();
#endif
    
    void ReadMessage();
    
    CURLM       * m_;
    
    std::map<CURL*, Connection*> mapping_;
};


/* implement of ConnectionWatcher */
#ifdef USE_EVENT
void ConnectionWatcher::OnFileCanReadWithoutBlocking(int fd){
    lazy_runner_pointer.Get().StepSocket(fd, mode_, connection_->handler_);
}

void ConnectionWatcher::OnFileCanWriteWithoutBlocking(int fd){
    lazy_runner_pointer.Get().StepSocket(fd, mode_, connection_->handler_);
}
#endif

/* implement of ConnectionRunner */
void ConnectionRunner::Run(Connection* conn)
{
    assert(conn && conn->handler_);
#ifndef USE_EVENT
    bool start = !mapping_.size();
#endif
    
    if (mapping_.find(conn->handler_) != mapping_.end()) {
        if (mapping_[conn->handler_]) assert(0);
        else mapping_[conn->handler_] = conn;
    }else{
        mapping_.insert(std::make_pair(conn->handler_, conn));
    }
    
    curl_multi_add_handle(m_, conn->handler_);
#ifndef USE_EVENT
    if (start) Schedule();
#endif
}

void ConnectionRunner::ReadMessage()
{
    int msg_count = 0;
    CURLMsg * msg = NULL;
    while ((msg = curl_multi_info_read(m_, &msg_count))) {
        
        if (msg && msg->msg == CURLMSG_DONE) {
            
            CURL *handler = msg->easy_handle;
            
            curl_multi_remove_handle(m_, handler);
            
            CURLcode code = msg->data.result;
            
            Connection* conn = mapping_[handler];
            mapping_.erase(handler);
            
            assert(conn);
            if (code != CURLE_OK) {
                conn->OnError(code);
            }else{
                conn->OnSuccess();
            }
        }
    }
}

#ifdef USE_EVENT
void ConnectionRunner::StepSocket(curl_socket_t sock, int action, CURL* easy)
{
    if (!m_) return;
    
    int running_count = -1;
    CURLMcode action_code = curl_multi_socket_action(m_, sock, action, &running_count);
    
    Connection* conn = NULL;
    if (easy) {
        conn = mapping_[easy];
    }
    
    if (action_code > CURLM_OK){//error
        TLOGE("StepSocket Error:%d", action_code);
        curl_multi_remove_handle(m_, easy);
        if (conn) {
            conn->OnError(CURLE_FAILED_INIT);
        }
    } else if (action_code == CURLM_OK){
        /* read msg and callback */
        ReadMessage();
    }
}

// Socket Callback Called by curl_multi_socket_action
// you need to watch file descripter for readable or writable
// use select / poll / libevent
int ConnectionRunner::SocketCallback(CURL *easy,      /* easy handle */
                                     curl_socket_t s, /* socket */
                                     int what,        /* see above */
                                     void *userp,     /* private callbackpointer */
                                     void *socketp){
    
    ConnectionRunner* runner = reinterpret_cast<ConnectionRunner*>(userp);
    Connection* conn = runner->mapping_[easy];
    assert(conn);
    
    if (CURL_POLL_INOUT == what || CURL_POLL_IN == what || CURL_POLL_OUT == what) {
        Lite::MessagePumpLibevent::Mode mode ;
        if (CURL_POLL_INOUT == what)
            mode = Lite::MessagePumpLibevent::WATCH_READ_WRITE ;
        else if (CURL_POLL_IN == what)
            mode = Lite::MessagePumpLibevent::WATCH_READ ;
        else if (CURL_POLL_OUT)
            mode = Lite::MessagePumpLibevent::WATCH_WRITE ;
        
        conn->watcher_->controller.StopWatchingFileDescriptor();
        
        conn->watcher_->SetMode(mode);
        
        Lite::MessagePumpLibevent* pump = Lite::MessageLoop::current()->PumpIO();
        
        pump->WatchFileDescriptor(s, true, mode, &(conn->watcher_->controller), conn->watcher_);
    }
    else if (CURL_POLL_REMOVE){
        conn->watcher_->controller.StopWatchingFileDescriptor();
    }
    
    // According to source code, return value is useless
    return 0;
}

// invok curl_multi_socket_action
// http://curl.haxx.se/libcurl/c/libcurl-multi.html
// The CURLMOPT_TIMERFUNCTION callback is called to set a timeout.
// When that timeout expires, your application should call the curl_multi_socket_action function
// saying it was due to a timeout.
int ConnectionRunner::TimerCallback(CURLM *multi,    /* multi handle */
                                    long timeout_ms, /* see above */
                                    void *userp)    /* private callback pointer */{
    
    //TLOGD("TimerCallback:[%ld]", timeout_ms);
    ConnectionRunner* runner = reinterpret_cast<ConnectionRunner*>(userp);
    if (timeout_ms > 0){
        dispatch_current_after(timeout_ms, [=]{
            runner->StepSocket(CURL_SOCKET_TIMEOUT, CURL_POLL_NONE, NULL);
        });
    }else{
        runner->StepSocket(CURL_SOCKET_TIMEOUT, CURL_POLL_NONE, NULL);
    }
    // According to source code, return value is useless
    return 0;
}
#else

void ConnectionRunner::Schedule()
{
    assert(m_);
    int running_count = 0;
    long timeout_ms = 100L;
    int M = -1;
    fd_set R, W, E;
    struct timeval T;
    
    curl_multi_perform(m_, &running_count);
    
    if (running_count) {
        
        FD_ZERO(&R);
        FD_ZERO(&W);
        FD_ZERO(&E);
        
        if (curl_multi_fdset(m_, &R, &W, &E, &M)) {
            fprintf(stderr, "E: curl_multi_fdset\n");
            return ;
        }
        if (curl_multi_timeout(m_, &timeout_ms)) {
            fprintf(stderr, "E: curl_multi_timeout\n");
            return ;
        }
        if (timeout_ms == -1) timeout_ms = 100;
        if (M == -1) {
            dispatch_after(timeout_ms, std::bind(&ConnectionRunner::Schedule, this));
            return;
        }else{
            T.tv_sec = 0/1000;
            T.tv_usec = (0%1000)*1000;
            if (select(M+1, &R, &W, &E, &T) > 0){
                timeout_ms = 0L;
            }
        }
    }
    /* read msg and callback */
    ReadMessage();
    
    if (running_count) {
        timeout_ms = timeout_ms > 500L? 500L: timeout_ms;
        dispatch_after(timeout_ms, std::bind(&ConnectionRunner::Schedule, this));
    }
}
#endif

/* Connection */

Connection::Connection()
:fd_(-1),handler_(NULL),headers_(NULL),
delegate_()
{
#ifdef USE_EVENT
    watcher_ = new ConnectionWatcher(this);
#endif
}
Connection::~Connection()
{
    UnInitialize();
#ifdef USE_EVENT
    delete watcher_;watcher_ = NULL;
#endif
}

void Connection::Initialize(Delegate* delegate)
{
    UnInitialize();
    
    delegate_ = delegate;
    
    handler_ = curl_easy_init();
    
    curl_easy_setopt(handler_, CURLOPT_SSL_VERIFYPEER, 1L) ;
    curl_easy_setopt(handler_, CURLOPT_SSL_VERIFYHOST, 1L) ;
    curl_easy_setopt(handler_, CURLOPT_SSLVERSION, CURL_SSLVERSION_DEFAULT) ;
    curl_easy_setopt(handler_, CURLOPT_SSL_SESSIONID_CACHE, 1L) ;
    curl_easy_setopt(handler_, CURLOPT_NOSIGNAL, 1L) ;
    
    curl_easy_setopt(handler_, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(handler_, CURLOPT_MAXREDIRS, 10L);
    curl_easy_setopt(handler_, CURLOPT_AUTOREFERER, 1L);
    curl_easy_setopt(handler_, CURLOPT_VERBOSE, 1L);
    
}
void Connection::UnInitialize()
{
    delegate_ = NULL;
    fd_ = -1;
    if (handler_ != NULL) {
        curl_easy_cleanup(handler_);
        handler_ = NULL;
    }
    if (headers_ != NULL) {
        curl_slist_free_all(headers_);
        headers_ = NULL;
    }
}

void Connection::Start()
{
    assert(handler_);
    lazy_runner_pointer.Get().Run(this);
}

void Connection::OnSuccess()
{
    if (delegate_) {
        delegate_->OnSuccess();
        delegate_ = NULL;
    }
}

void Connection::OnError(CURLcode code)
{
    if (delegate_) {
        delegate_->OnError(code);
        delegate_ = NULL;
    }
}

#ifdef USE_NAMESPACE
};
#endif