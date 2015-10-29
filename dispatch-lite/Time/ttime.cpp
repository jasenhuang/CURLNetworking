//
//  Time.cpp
//  lite
//
//  Created by jasenhuang on 15/5/20.
//  Copyright (c) 2015年 jasenhuang. All rights reserved.
//

#include "TTime.h"
#include "TLog.h"
#include <cmath>
#include <stdint.h>
#include <time.h>
#include <sys/time.h>
#include <sys/sysctl.h>
#include <sys/time.h>
#include <sys/types.h>
#include <assert.h>

#if defined(OS_ANDROID)
#include <time64.h>
#endif

#include <unistd.h>
#include <limits>
#include <ostream>

template <typename T, size_t N>
char (&ArraySizeHelper(T (&array)[N]))[N];
#define arraysize(array) (sizeof(ArraySizeHelper(array)))

#ifdef USE_NAMESPACE
namespace Lite {
#endif

// TimeDelta ------------------------------------------------------------------
// static
TimeDelta TimeDelta::Max() {
    return TimeDelta(std::numeric_limits<int64>::max());
}

int TimeDelta::InDays() const {
    if (is_max()) {
        // Preserve max to prevent overflow.
        return std::numeric_limits<int>::max();
    }
    return static_cast<int>(delta_ / Time::kMicrosecondsPerDay);
}

int TimeDelta::InHours() const {
    if (is_max()) {
        // Preserve max to prevent overflow.
        return std::numeric_limits<int>::max();
    }
    return static_cast<int>(delta_ / Time::kMicrosecondsPerHour);
}

int TimeDelta::InMinutes() const {
    if (is_max()) {
        // Preserve max to prevent overflow.
        return std::numeric_limits<int>::max();
    }
    return static_cast<int>(delta_ / Time::kMicrosecondsPerMinute);
}

double TimeDelta::InSecondsF() const {
    if (is_max()) {
        // Preserve max to prevent overflow.
        return std::numeric_limits<double>::infinity();
    }
    return static_cast<double>(delta_) / Time::kMicrosecondsPerSecond;
}

int64 TimeDelta::InSeconds() const {
    if (is_max()) {
        // Preserve max to prevent overflow.
        return std::numeric_limits<int64>::max();
    }
    return delta_ / Time::kMicrosecondsPerSecond;
}

double TimeDelta::InMillisecondsF() const {
    if (is_max()) {
        // Preserve max to prevent overflow.
        return std::numeric_limits<double>::infinity();
    }
    return static_cast<double>(delta_) / Time::kMicrosecondsPerMillisecond;
}

int64 TimeDelta::InMilliseconds() const {
    if (is_max()) {
        // Preserve max to prevent overflow.
        return std::numeric_limits<int64>::max();
    }
    return delta_ / Time::kMicrosecondsPerMillisecond;
}

int64 TimeDelta::InMillisecondsRoundedUp() const {
    if (is_max()) {
        // Preserve max to prevent overflow.
        return std::numeric_limits<int64>::max();
    }
    return (delta_ + Time::kMicrosecondsPerMillisecond - 1) /
    Time::kMicrosecondsPerMillisecond;
}

int64 TimeDelta::InMicroseconds() const {
    if (is_max()) {
        // Preserve max to prevent overflow.
        return std::numeric_limits<int64>::max();
    }
    return delta_;
}

TimeTicks TimeTicks::Now() {
    struct timeval time;
    gettimeofday(&time, NULL);
    return TimeTicks(time.tv_sec * Time::kMicrosecondsPerSecond + time.tv_usec);
}
// TimeTicks ........


#ifdef USE_NAMESPACE
};
#endif
