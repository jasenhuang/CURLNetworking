//
//  tconnection_operation.cpp
//  mail
//
//  Created by jasenhuang on 15/7/24.
//  Copyright (c) 2015年 tencent. All rights reserved.
//

#include "tconnection_operation.h"
#include "tconnection_operation_delegate.h"
#include "tdispatch.h"
#include "tlog.h"

#ifdef USE_NAMESPACE
namespace Network {
#endif

void ParseHttpReponseHeader(std::string& header, std::pair<std::string, std::string>& key_value)
{
    size_t iPos = std::string::npos;
    std::string key , value;
    if (( iPos = header.find(':', 0)) != std::string::npos){
        key = header.substr(0 , iPos);
        if (iPos + 1 < header.size()) value = header.substr(iPos + 1, header.size() - iPos);
        key_value = make_pair(key , value);
    }
}

ConnectionOperation::ConnectionOperation(std::shared_ptr<Request> request)
:priority_(0),started_(false),cancel_(false),delegate_(NULL),request_headers_(NULL),
request_(request),response_(new Response())
{
}

ConnectionOperation::~ConnectionOperation()
{
}

void ConnectionOperation::Start()
{
    if (started_) return;
    
    started_ = true;
    
    /* start with connection */
    if (connection_) {
        connection_->Initialize(this);  //initialize
        Prepare();/* prepare to start */
        //curl_easy_perform(connection_->handler_);
        connection_->Start();
    }
    
    /* delegate */
    if (delegate_) {
        delegate_->OnStart(this);
    }
}

void ConnectionOperation::Cancel()
{
    cancel_ = true;
}

void ConnectionOperation::Prepare()
{
    if (connection_) {
        
        /* custom request */
        if (!request_->custom_request_.empty()) {
            curl_easy_setopt(connection_->handler_, CURLOPT_CUSTOMREQUEST, request_->url_.c_str());
        }else{
            curl_easy_setopt(connection_->handler_, CURLOPT_POST, true) ;
        }
        
        /* request url */
        if (!request_->url_.empty()) {
            curl_easy_setopt(connection_->handler_, CURLOPT_URL, request_->url_.c_str());
        }
        
        /* header */
        if(request_headers_) {
            curl_slist_free_all(request_headers_);
            request_headers_ = NULL;
        }
        std::vector<std::pair<std::string , std::string> >::const_iterator iter = request_->headers_.begin() ;
        for (; iter != request_->headers_.end(); ++iter){
            curl_slist_append(request_headers_, (iter->first + ':' + iter->second).c_str()) ;
        }
        
        /* time out*/
        curl_easy_setopt(connection_->handler_, CURLOPT_TIMEOUT_MS, request_->read_timeout_ms_);
        curl_easy_setopt(connection_->handler_, CURLOPT_CONNECTTIMEOUT_MS , request_->connect_timeout_ms_);
        
        /* read callback */
        curl_easy_setopt(connection_->handler_, CURLOPT_READDATA, this) ;
        curl_easy_setopt(connection_->handler_, CURLOPT_READFUNCTION, &ConnectionOperation::ReadCallback);
        
        /* write callback */
        curl_easy_setopt(connection_->handler_, CURLOPT_WRITEDATA, this);
        curl_easy_setopt(connection_->handler_, CURLOPT_WRITEFUNCTION, &ConnectionOperation::WriteCallback);
        
        /* header callback */
        curl_easy_setopt(connection_->handler_, CURLOPT_HEADERDATA , this);
        curl_easy_setopt(connection_->handler_, CURLOPT_HEADERFUNCTION , &ConnectionOperation::HeaderCallback);
        
        /* progress callback */
        curl_easy_setopt(connection_->handler_, CURLOPT_NOPROGRESS , 0L);
        curl_easy_setopt(connection_->handler_, CURLOPT_PROGRESSDATA , this);
        curl_easy_setopt(connection_->handler_, CURLOPT_PROGRESSFUNCTION , &ConnectionOperation::ProgressCallback);
    }
}

/* connection callback */
void ConnectionOperation::OnSuccess()
{
    if (success_callback_) {
        //c++11 lamda would not capture member variable
        std::shared_ptr<Response> response(response_);
        std::shared_ptr<ConnectionOperation> operation(shared_from_this());
        dispatch_logic_async([=]{
            success_callback_(operation, response);
        });
    }
    if (delegate_) {
        delegate_->OnFinish(this);
        delegate_ = NULL;
    }
}
void ConnectionOperation::OnError(CURLcode code)
{
    if (fail_callback_) {
        std::shared_ptr<ConnectionOperation> operation(shared_from_this());
        dispatch_logic_async([=]{
            std::shared_ptr<Error> response_error(new Error());
            response_error->code = code;
            response_error->msg = curl_easy_strerror(code);
            fail_callback_(operation, response_error);
        });
    }
    if (delegate_) {
        delegate_->OnFinish(this);
        delegate_ = NULL;
    }
}

/* config callback */
void ConnectionOperation::SetCompletionCallback(const SUCC_CALLBACK& succ, const FAIL_CALLBACK& fail)
{
    success_callback_ = succ;
    fail_callback_ = fail;
}
void ConnectionOperation::SetDownloadProgressCallback(const PROGRESS_CALLBACK& callback)
{
    download_progress_callback_ = callback;
}
void ConnectionOperation::SetSendProgressCallback(const PROGRESS_CALLBACK& callback)
{
    send_progress_callback_ = callback;
}

/* curl callback */

size_t ConnectionOperation::ReadCallback(char *buffer,
                                size_t size,
                                size_t nitems,
                                void *instream){
    
    if(size * nitems < 1) return 0;
    
    ConnectionOperation* oper = static_cast<ConnectionOperation*>(instream);

    return oper->request_->Read(buffer, size * nitems);
}

size_t ConnectionOperation::WriteCallback(char *buffer,
                                 size_t size,
                                 size_t nitems,
                                 void *outstream){
    if(size * nitems < 1) return 0;
    
    ConnectionOperation* oper = static_cast<ConnectionOperation*>(outstream);
    
    size_t ret = 0;
    if (oper && buffer) {
        oper->response_->Write((char*)buffer, size * nitems);
        ret = size * nitems;
    }
    return ret;
}

size_t ConnectionOperation::HeaderCallback(char *buffer,
                                          size_t size,
                                          size_t nitems,
                                          void *outstream){
    if(size * nitems < 1) return 0;
    
    ConnectionOperation* oper = static_cast<ConnectionOperation*>(outstream);
    
    std::string header(buffer , size * nitems);
    std::pair<std::string, std::string > key_value;
    ParseHttpReponseHeader(header, key_value);
    if (key_value.first == "Content-Length"){
        oper->response_->total_ =  std::atoi(key_value.second.c_str());
    }
    oper->response_->headers_.push_back(key_value);
    return size * nitems;
}

int ConnectionOperation::ProgressCallback(void *clientp,
                                          double dltotal,
                                          double dlnow,
                                          double ultotal,
                                          double ulnow){
    ConnectionOperation* oper = static_cast<ConnectionOperation*>(clientp);
    if (oper->send_progress_callback_ && ulnow) {
        oper->send_progress_callback_(ulnow, ultotal, ultotal);
        //TLOGD("ulnow:%f, ultotal:%f", ulnow, ultotal);
    }
    if (oper->download_progress_callback_ && dlnow) {
        oper->download_progress_callback_(dlnow, dltotal, dltotal);
        //TLOGD("dlnow:%f, dltotal:%f", dlnow, dltotal);
    }
    return oper->cancel_;
}

#ifdef USE_NAMESPACE
};
#endif