//
//  networktest.m
//  curl_networking
//
//  Created by jasenhuang on 15/7/24.
//  Copyright (c) 2015年 tencent. All rights reserved.
//

#import <UIKit/UIKit.h>
#import <XCTest/XCTest.h>
#include "tconnection_operation_queue.h"
#include "tconnection_operation.h"
#include "tdispatch.h"

using namespace Network;

@interface networktest : XCTestCase
{
    ConnectionOperationQueue* queue_;
}
@end

@implementation networktest

- (void)setUp {
    [super setUp];
    // Put setup code here. This method is called before the invocation of each test method in the class.
    queue_ = new ConnectionOperationQueue();
}

- (void)tearDown {
    // Put teardown code here. This method is called after the invocation of each test method in the class.
    [super tearDown];
    delete queue_;
}

- (void)testExample {
    // This is an example of a functional test case.
    
    XCTAssert(YES, @"Pass");
    
    dispatch_init();
    
    for (int i = 0 ; i < 1; ++i) {
        
        std::shared_ptr<Request> request = std::make_shared<Request>();
        request->SetURL("http://www.qq.com");
        std::shared_ptr<ConnectionOperation> oper = std::make_shared<ConnectionOperation>(request);
        
        oper->SetCompletionCallback([=](std::shared_ptr<ConnectionOperation> operation, std::shared_ptr<Response> response){
            
            //std::cout<< "response [" << i << "] text = " << response->ResponseText() << std::endl;
            std::cout<< "response [" << i << "]"<<std::endl;
            
        }, [=](std::shared_ptr<ConnectionOperation> operation, std::shared_ptr<Error> error){
            
            std::cout<< "response [" << i << "] error = " << error->msg << std::endl;
            //std::cout<< "response [" << i << "] error" << std::endl;
        });
        oper->SetDownloadProgressCallback([](unsigned int bytes, long long totalBytes, long long totalBytesExpected){
            
        });
        oper->SetSendProgressCallback([](unsigned int bytes, long long totalBytes, long long totalBytesExpected){
            
        });
        queue_->AddOperation(oper);
        
    }
    
    sleep(60000); //60 second
}

- (void)testPerformanceExample {
    // This is an example of a performance test case.
    [self measureBlock:^{
        // Put the code you want to measure the time of here.
    }];
}

@end
