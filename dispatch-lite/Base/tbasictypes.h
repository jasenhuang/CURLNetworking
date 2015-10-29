//
//  basictypes.h
//  lite
//
//  Created by jasenhuang on 15/5/20.
//  Copyright (c) 2015年 jasenhuang. All rights reserved.
//

#ifndef __lite__basictypes__
#define __lite__basictypes__

#if defined(__APPLE__)
#define OS_MACOSX 1
#elif defined(ANDROID)
#define OS_ANDROID 1
#elif defined(__native_client__)
#define OS_NACL 1
#elif defined(__linux__)
#define OS_LINUX 1
#elif defined(_WIN32)
#define OS_WIN 1
#define TOOLKIT_VIEWS 1
#elif defined(__FreeBSD__)
#define OS_FREEBSD 1
#define TOOLKIT_GTK
#elif defined(__OpenBSD__)
#define OS_OPENBSD 1
#define TOOLKIT_GTK
#elif defined(__sun)
#define OS_SOLARIS 1
#define TOOLKIT_GTK
#endif

#if defined(OS_MACOSX) || defined(OS_LINUX) || defined(OS_FREEBSD) ||     \
defined(OS_OPENBSD) || defined(OS_SOLARIS) || defined(OS_ANDROID) ||  \
defined(OS_NACL)
#define OS_POSIX 1
#endif

#ifndef USE_PTHREAD
#define USE_PTHREAD
#endif

#ifndef USE_NAMESPACE
#define USE_NAMESPACE
#endif

#ifndef USE_EVENT
#define USE_EVENT
#endif

#define DISALLOW_COPY_AND_ASSIGN(TypeName) \
TypeName(const TypeName&);               \
void operator=(const TypeName&)

typedef long long int64;
typedef unsigned long long uint64;


#define HANDLE_EINTR(x) ({ \
int eintr_wrapper_counter = 0; \
typeof(x) eintr_wrapper_result; \
do { \
eintr_wrapper_result = (x); \
} while (eintr_wrapper_result == -1 && errno == EINTR && \
eintr_wrapper_counter++ < 100); \
eintr_wrapper_result; \
})

#define IGNORE_EINTR(x) ({ \
typeof(x) eintr_wrapper_result; \
do { \
eintr_wrapper_result = (x); \
if (eintr_wrapper_result == -1 && errno == EINTR) { \
eintr_wrapper_result = 0; \
} \
} while (0); \
eintr_wrapper_result; \
})

#endif
