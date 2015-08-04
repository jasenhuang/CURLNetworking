//
//  tconnection_operation.h
//  mail
//
//  Created by jasenhuang on 15/7/24.
//  Copyright (c) 2015年 tencent. All rights reserved.
//

#ifndef __mail__tconnection_operation__
#define __mail__tconnection_operation__

#include <iostream>
#include "tconnection.h"
#include "trequest.h"
#include "tresponse.h"

#ifdef USE_NAMESPACE
namespace Network {
#endif

class ConnectionOperation;
class ConnectionOperationQueue;
class ConnectionOperationDelegate;

typedef std::function<void(std::shared_ptr<ConnectionOperation> oper, std::shared_ptr<Response> response)> SUCC_CALLBACK;
typedef std::function<void(std::shared_ptr<ConnectionOperation> oper, std::shared_ptr<Error> error)> FAIL_CALLBACK;
typedef std::function<void(unsigned int bytes, long long totalBytes, long long totalBytesExpected)> PROGRESS_CALLBACK;


class ConnectionOperation : public Connection::Delegate, public std::enable_shared_from_this<ConnectionOperation> {
    
public:
    explicit ConnectionOperation(std::shared_ptr<Request> request);
    virtual ~ConnectionOperation();

    void Start();
    void Cancel();
    
    void SetDelegate(ConnectionOperationDelegate* delegate){delegate_ = delegate;};
    
    /* completion callback */
    void SetCompletionCallback(const SUCC_CALLBACK& succ, const FAIL_CALLBACK& fail);
    /* depend on Contenct-Length http header */
    void SetDownloadProgressCallback(const PROGRESS_CALLBACK& callback);
    void SetSendProgressCallback(const PROGRESS_CALLBACK& callback);
    
    bool operator<(const ConnectionOperation& b) {
        return priority_ < b.priority_;
    }

protected:
    
    /* prepare curl easy handler */
    virtual void Prepare();
    
    static size_t ReadCallback(char *buffer,
                               size_t size,
                               size_t nitems,
                               void *instream);
    
    static size_t WriteCallback(char *buffer,
                                size_t size,
                                size_t nitems,
                                void *outstream);
    
    static size_t HeaderCallback(char *buffer,
                                size_t size,
                                size_t nitems,
                                void *outstream);
    
    static int ProgressCallback(void *clientp,
                                double dltotal,
                                double dlnow,
                                double ultotal,
                                double ulnow);
    /* connection delegate */
    
    virtual void OnSuccess();
    virtual void OnError(CURLcode code);
    
    
    SUCC_CALLBACK       success_callback_;
    FAIL_CALLBACK       fail_callback_;
    
    PROGRESS_CALLBACK   download_progress_callback_;
    PROGRESS_CALLBACK   send_progress_callback_;
    
private:
    friend class ConnectionOperationQueue;
    
    ConnectionOperationDelegate*  delegate_;
    
    std::shared_ptr<Connection>   connection_;
    
    std::shared_ptr<Request>        request_;
    std::shared_ptr<Response>       response_;
    curl_slist *                    request_headers_;
    
    int                             priority_;
    
    bool                            started_;
    bool                            cancel_;
};


#ifdef USE_NAMESPACE
};
#endif

#endif /* defined(__mail__tconnection_operation__) */
