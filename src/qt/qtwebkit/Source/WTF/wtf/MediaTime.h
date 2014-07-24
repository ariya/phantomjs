/*
 * Copyright (C) 2012 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer. 
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution. 
 * 3.  Neither the name of Apple Computer, Inc. ("Apple") nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission. 
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "FastAllocBase.h"

#include <cmath>
#include <limits>
#include <math.h>
#include <stdint.h>

namespace WTF {

class WTF_EXPORT_PRIVATE MediaTime {
    WTF_MAKE_FAST_ALLOCATED;
public:
    enum {
        Valid = 1 << 0,
        HasBeenRounded = 1 << 1,
        PositiveInfinite = 1 << 2,
        NegativeInfinite = 1 << 3,
        Indefinite = 1 << 4,
    };

    MediaTime();
    MediaTime(int64_t value, int32_t scale = DefaultTimeScale, uint32_t flags = Valid);
    MediaTime(const MediaTime& rhs);
    ~MediaTime();

    static MediaTime createWithFloat(float floatTime, int32_t timeScale = DefaultTimeScale);
    static MediaTime createWithDouble(double doubleTime, int32_t timeScale = DefaultTimeScale);

    float toFloat() const;
    double toDouble() const;

    MediaTime& operator=(const MediaTime& rhs);
    MediaTime operator+(const MediaTime& rhs) const;
    MediaTime operator-(const MediaTime& rhs) const;
    bool operator<(const MediaTime& rhs) const;
    bool operator>(const MediaTime& rhs) const;
    bool operator==(const MediaTime& rhs) const;
    bool operator>=(const MediaTime& rhs) const;
    bool operator<=(const MediaTime& rhs) const;

    typedef enum {
        LessThan = -1,
        EqualTo = 0,
        GreaterThan = 1,
    } ComparisonFlags;

    ComparisonFlags compare(const MediaTime& rhs) const;

    bool isValid() const { return m_timeFlags & Valid; }
    bool isInvalid() const { return !isValid(); }
    bool hasBeenRounded() const { return m_timeFlags & HasBeenRounded; }
    bool isPositiveInfinite() const { return m_timeFlags & PositiveInfinite; }
    bool isNegativeInfinite() const { return m_timeFlags & NegativeInfinite; }
    bool isIndefinite() const { return m_timeFlags & Indefinite; }

    static const MediaTime& zeroTime();
    static const MediaTime& invalidTime();
    static const MediaTime& positiveInfiniteTime();
    static const MediaTime& negativeInfiniteTime();
    static const MediaTime& indefiniteTime();

    const int64_t& timeValue() const { return m_timeValue; }
    const int32_t& timeScale() const { return m_timeScale; }

    friend WTF_EXPORT_PRIVATE MediaTime abs(const MediaTime& rhs);
private:
    static const int32_t DefaultTimeScale = 6000;
    static const int32_t MaximumTimeScale;

    void setTimeScale(int32_t);

    int64_t m_timeValue;
    int32_t m_timeScale;
    uint32_t m_timeFlags;
};

WTF_EXPORT_PRIVATE extern MediaTime abs(const MediaTime& rhs);
}

using WTF::MediaTime;
using WTF::abs;
