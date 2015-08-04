# network
c++ http network library base on curl

1：totally asynchronous

2：easy api like AFNetworking(ObjC networking framework)

3：callback by c++11 lamda function

4: wrapper of curl

Usage:

		std::shared_ptr<Request> request = std::make_shared<Request>();
        request->SetURL("http://www.qq.com");
        std::shared_ptr<ConnectionOperation> oper = std::make_shared<ConnectionOperation>(request);
        
        oper->SetCompletionCallback([=](std::shared_ptr<ConnectionOperation> operation, std::shared_ptr<Response> response){
            
            std::cout<< "response [" << i << "] text = " << response->ResponseText() << std::endl;
            
        }, [=](std::shared_ptr<ConnectionOperation> operation, std::shared_ptr<Error> error){
            
            std::cout<< "response [" << i << "] error = " << error->msg << std::endl;
        });
        oper->SetDownloadProgressCallback([](unsigned int bytes, long long totalBytes, long long totalBytesExpected){
            
        });
        oper->SetSendProgressCallback([](unsigned int bytes, long long totalBytes, long long totalBytesExpected){
            
        });
        queue_->AddOperation(oper);
        
        
Denpendies:

1：[curl](https://github.com/jasenhuang/curl-ios)  (required)

2：[dispatch-lite](https://github.com/jasenhuang/dispatch-lite)   (required) 

3：libevent(optional by macro USE_EVENT)

4：c++11 and above
