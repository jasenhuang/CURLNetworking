//
//  Time.h
//  lite
//
//  Created by jasenhuang on 15/5/20.
//  Copyright (c) 2015年 jasenhuang. All rights reserved.
//

#ifndef __lite__ttime__
#define __lite__ttime__

#include <limits>
#include <time.h>
#include <unistd.h>
#include "tbasictypes.h"

#ifdef USE_NAMESPACE
namespace Lite {
#endif

class TimeTicks;
class TimeDelta{
public:
    TimeDelta() : delta_(0) {
    }
    
    static TimeDelta FromDays(int days);
    static TimeDelta FromHours(int hours);
    static TimeDelta FromMinutes(int minutes);
    static TimeDelta FromSeconds(int64 secs);
    static TimeDelta FromMilliseconds(int64 ms);
    static TimeDelta FromMicroseconds(int64 us);
    
    static TimeDelta FromInternalValue(int64 delta) {
        return TimeDelta(delta);
    }
    
    // Returns the maximum time delta, which should be greater than any reasonable
    // time delta we might compare it to. Adding or subtracting the maximum time
    // delta to a time or another time delta has an undefined result.
    static TimeDelta Max();
    
    // Returns the internal numeric value of the TimeDelta object. Please don't
    // use this and do arithmetic on it, as it is more error prone than using the
    // provided operators.
    // For serializing, use FromInternalValue to reconstitute.
    int64 ToInternalValue() const {
        return delta_;
    }
    
    // Returns true if the time delta is the maximum time delta.
    bool is_max() const {
        return delta_ == std::numeric_limits<int64>::max();
    }

    struct timespec ToTimeSpec() const;
    
    // Returns the time delta in some unit. The F versions return a floating
    // point value, the "regular" versions return a rounded-down value.
    //
    // InMillisecondsRoundedUp() instead returns an integer that is rounded up
    // to the next full millisecond.
    int InDays() const;
    int InHours() const;
    int InMinutes() const;
    double InSecondsF() const;
    int64 InSeconds() const;
    double InMillisecondsF() const;
    int64 InMilliseconds() const;
    int64 InMillisecondsRoundedUp() const;
    int64 InMicroseconds() const;
    
    TimeDelta& operator=(TimeDelta other) {
        delta_ = other.delta_;
        return *this;
    }
    
    // Computations with other deltas.
    TimeDelta operator+(TimeDelta other) const {
        return TimeDelta(delta_ + other.delta_);
    }
    TimeDelta operator-(TimeDelta other) const {
        return TimeDelta(delta_ - other.delta_);
    }
    
    TimeDelta& operator+=(TimeDelta other) {
        delta_ += other.delta_;
        return *this;
    }
    TimeDelta& operator-=(TimeDelta other) {
        delta_ -= other.delta_;
        return *this;
    }
    TimeDelta operator-() const {
        return TimeDelta(-delta_);
    }
    
    // Computations with ints, note that we only allow multiplicative operations
    // with ints, and additive operations with other deltas.
    TimeDelta operator*(int64 a) const {
        return TimeDelta(delta_ * a);
    }
    TimeDelta operator/(int64 a) const {
        return TimeDelta(delta_ / a);
    }
    TimeDelta& operator*=(int64 a) {
        delta_ *= a;
        return *this;
    }
    TimeDelta& operator/=(int64 a) {
        delta_ /= a;
        return *this;
    }
    int64 operator/(TimeDelta a) const {
        return delta_ / a.delta_;
    }
    
    TimeTicks operator+(TimeTicks t) const;
    
    // Comparison operators.
    bool operator==(TimeDelta other) const {
        return delta_ == other.delta_;
    }
    bool operator!=(TimeDelta other) const {
        return delta_ != other.delta_;
    }
    bool operator<(TimeDelta other) const {
        return delta_ < other.delta_;
    }
    bool operator<=(TimeDelta other) const {
        return delta_ <= other.delta_;
    }
    bool operator>(TimeDelta other) const {
        return delta_ > other.delta_;
    }
    bool operator>=(TimeDelta other) const {
        return delta_ >= other.delta_;
    }
    
private:
    friend class TimeTicks;
    friend TimeDelta operator*(int64 a, TimeDelta td);
    
    // Constructs a delta given the duration in microseconds. This is private
    // to avoid confusion by callers with an integer constructor. Use
    // FromSeconds, FromMilliseconds, etc. instead.
    explicit TimeDelta(int64 delta_us) : delta_(delta_us) {
    }
    
    // Delta in microseconds.
    int64 delta_;
};

inline TimeDelta operator*(int64 a, TimeDelta td) {
    return TimeDelta(a * td.delta_);
    
};

class Time {
public:
    static const int64 kMillisecondsPerSecond = 1000;
    static const int64 kMicrosecondsPerMillisecond = 1000;
    static const int64 kMicrosecondsPerSecond = kMicrosecondsPerMillisecond *
    kMillisecondsPerSecond;
    static const int64 kMicrosecondsPerMinute = kMicrosecondsPerSecond * 60;
    static const int64 kMicrosecondsPerHour = kMicrosecondsPerMinute * 60;
    static const int64 kMicrosecondsPerDay = kMicrosecondsPerHour * 24;
    static const int64 kMicrosecondsPerWeek = kMicrosecondsPerDay * 7;
    static const int64 kNanosecondsPerMicrosecond = 1000;
    static const int64 kNanosecondsPerSecond = kNanosecondsPerMicrosecond *
    kMicrosecondsPerSecond;
};

// Inline the TimeDelta factory methods, for fast TimeDelta construction.

// static
inline TimeDelta TimeDelta::FromDays(int days) {
    // Preserve max to prevent overflow.
    if (days == std::numeric_limits<int>::max())
        return Max();
    return TimeDelta(days * Time::kMicrosecondsPerDay);
}

// static
inline TimeDelta TimeDelta::FromHours(int hours) {
    // Preserve max to prevent overflow.
    if (hours == std::numeric_limits<int>::max())
        return Max();
    return TimeDelta(hours * Time::kMicrosecondsPerHour);
}

// static
inline TimeDelta TimeDelta::FromMinutes(int minutes) {
    // Preserve max to prevent overflow.
    if (minutes == std::numeric_limits<int>::max())
        return Max();
    return TimeDelta(minutes * Time::kMicrosecondsPerMinute);
}

// static
inline TimeDelta TimeDelta::FromSeconds(int64 secs) {
    // Preserve max to prevent overflow.
    if (secs == std::numeric_limits<int64>::max())
        return Max();
    return TimeDelta(secs * Time::kMicrosecondsPerSecond);
}

// static
inline TimeDelta TimeDelta::FromMilliseconds(int64 ms) {
    // Preserve max to prevent overflow.
    if (ms == std::numeric_limits<int64>::max())
        return Max();
    return TimeDelta(ms * Time::kMicrosecondsPerMillisecond);
}

// static
inline TimeDelta TimeDelta::FromMicroseconds(int64 us) {
    // Preserve max to prevent overflow.
    if (us == std::numeric_limits<int64>::max())
        return Max();
    return TimeDelta(us);
}

// TimeTicks ------------------------------------------------------------------

class TimeTicks {
public:
    TimeTicks() : ticks_(0) {
    }
    
    static TimeTicks Now();
    
    // Returns the internal numeric value of the TimeTicks object.
    // For serializing, use FromInternalValue to reconstitute.
    int64 ToInternalValue() const {
        return ticks_;
    }
    
    TimeTicks& operator=(TimeTicks other) {
        ticks_ = other.ticks_;
        return *this;
    }
    
    // Compute the difference between two times.
    TimeDelta operator-(TimeTicks other) const {
        return TimeDelta(ticks_ - other.ticks_);
    }
    
    // Modify by some time delta.
    TimeTicks& operator+=(TimeDelta delta) {
        ticks_ += delta.delta_;
        return *this;
    }
    TimeTicks& operator-=(TimeDelta delta) {
        ticks_ -= delta.delta_;
        return *this;
    }
    bool is_null() const {
        return ticks_ == 0;
    }
    // Return a new TimeTicks modified by some delta.
    TimeTicks operator+(TimeDelta delta) const {
        return TimeTicks(ticks_ + delta.delta_);
    }
    TimeTicks operator-(TimeDelta delta) const {
        return TimeTicks(ticks_ - delta.delta_);
    }
    // Comparison operators
    bool operator==(TimeTicks other) const {
        return ticks_ == other.ticks_;
    }
    bool operator!=(TimeTicks other) const {
        return ticks_ != other.ticks_;
    }
    bool operator<(TimeTicks other) const {
        return ticks_ < other.ticks_;
    }
    bool operator<=(TimeTicks other) const {
        return ticks_ <= other.ticks_;
    }
    bool operator>(TimeTicks other) const {
        return ticks_ > other.ticks_;
    }
    bool operator>=(TimeTicks other) const {
        return ticks_ >= other.ticks_;
    }
    
    friend class TimeDelta;
    
protected:
    explicit TimeTicks(int64 ticks) : ticks_(ticks) {
    }
private:
    int64 ticks_;
};
inline TimeTicks TimeDelta::operator+(TimeTicks t) const {
    return TimeTicks(t.ticks_ + delta_);
}

#ifdef USE_NAMESPACE
};
#endif

#endif /* defined(dispatchliteTime__) */
