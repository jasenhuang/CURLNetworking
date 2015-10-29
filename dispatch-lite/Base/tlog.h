//
//  tlog.h
//  tbase
//
//  Created by jasenhuang-iMac on 4/29/14.
//  Copyright (c) 2014 tencent. All rights reserved.
//

#ifndef TBASE_TLOG_H
#define TBASE_TLOG_H

#include <stdio.h>

#if defined(__APPLE__)
#define TLOGD(format, ...) printf("[%s:%d] DEBUG:" format"\n", __FILE__, __LINE__, __VA_ARGS__) // 定义LOGD类型
#define TLOGI(format, ...) printf("[%s:%d] INFO:" format"\n", __FILE__, __LINE__, __VA_ARGS__) // 定义LOGI类型
#define TLOGW(format, ...) printf("[%s:%d] WARN:" format"\n", __FILE__, __LINE__, __VA_ARGS__) // 定义LOGW类型
#define TLOGE(format, ...) printf("[%s:%d] ERROR:" format"\n", __FILE__, __LINE__, __VA_ARGS__) // 定义LOGE类型
#define TLOGF(format, ...) printf("[%s:%d] FATAL:" format"\n", __FILE__, __LINE__, __VA_ARGS__) // 定义LOGF类型
#elif defined(ANDROID)
#include <android/log.h>
#define TAG "tbase"
#define TLOGD(format, ...) __android_log_print(ANDROID_LOG_DEBUG, TAG \
	,"[%s:%d] DEBUG:" format"", __FILE__, __LINE__, __VA_ARGS__) // 定义LOGD类型
#define TLOGI(format, ...) __android_log_print(ANDROID_LOG_INFO, TAG \
	,"[%s:%d] INFO:" format"", __FILE__, __LINE__, __VA_ARGS__) // 定义LOGI类型
#define TLOGW(format, ...) __android_log_print(ANDROID_LOG_WARN, TAG \
	,"[%s:%d] WARN:" format"", __FILE__, __LINE__, __VA_ARGS__) // 定义LOGW类型
#define TLOGE(format, ...) __android_log_print(ANDROID_LOG_ERROR, TAG \
	,"[%s:%d] ERROR:" format"", __FILE__, __LINE__, __VA_ARGS__) // 定义LOGE类型
#define TLOGF(format, ...) __android_log_print(ANDROID_LOG_FATAL, TAG \
	,"[%s:%d] FATAL:" format"", __FILE__, __LINE__, __VA_ARGS__) // 定义LOGF类型
#endif

#endif