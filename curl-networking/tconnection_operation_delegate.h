//
//  tconnection_operation_delegate.h
//  mail
//
//  Created by jasenhuang on 15/7/30.
//  Copyright (c) 2015年 tencent. All rights reserved.
//

#ifndef __mail__tconnection_operation_delegate__
#define __mail__tconnection_operation_delegate__
#include "tbasictypes.h"

#ifdef USE_NAMESPACE
namespace Network {
#endif

class ConnectionOperation;
class ConnectionOperationDelegate{
public:
    ConnectionOperationDelegate() {}
    virtual ~ConnectionOperationDelegate() {}

    virtual void OnStart(ConnectionOperation* oper) = 0;
    virtual void OnFinish(ConnectionOperation* oper) = 0;
};

#ifdef USE_NAMESPACE
};
#endif

#endif /* defined(__mail__tconnection_operation_delegate__) */
