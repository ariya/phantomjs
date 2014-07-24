/*
 * Copyright (C) 2011, 2012 Apple Inc. All rights reserved.
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

#ifndef ValueProfile_h
#define ValueProfile_h

#include <wtf/Platform.h>

#if ENABLE(VALUE_PROFILER)

#include "Heap.h"
#include "JSArray.h"
#include "SpeculatedType.h"
#include "Structure.h"
#include "WriteBarrier.h"
#include <wtf/PrintStream.h>
#include <wtf/StringPrintStream.h>

namespace JSC {

template<unsigned numberOfBucketsArgument>
struct ValueProfileBase {
    static const unsigned numberOfBuckets = numberOfBucketsArgument;
    static const unsigned numberOfSpecFailBuckets = 1;
    static const unsigned bucketIndexMask = numberOfBuckets - 1;
    static const unsigned totalNumberOfBuckets = numberOfBuckets + numberOfSpecFailBuckets;
    
    ValueProfileBase()
        : m_bytecodeOffset(-1)
        , m_prediction(SpecNone)
        , m_numberOfSamplesInPrediction(0)
        , m_singletonValueIsTop(false)
    {
        for (unsigned i = 0; i < totalNumberOfBuckets; ++i)
            m_buckets[i] = JSValue::encode(JSValue());
    }
    
    ValueProfileBase(int bytecodeOffset)
        : m_bytecodeOffset(bytecodeOffset)
        , m_prediction(SpecNone)
        , m_numberOfSamplesInPrediction(0)
        , m_singletonValueIsTop(false)
    {
        for (unsigned i = 0; i < totalNumberOfBuckets; ++i)
            m_buckets[i] = JSValue::encode(JSValue());
    }
    
    EncodedJSValue* specFailBucket(unsigned i)
    {
        ASSERT(numberOfBuckets + i < totalNumberOfBuckets);
        return m_buckets + numberOfBuckets + i;
    }
    
    const ClassInfo* classInfo(unsigned bucket) const
    {
        JSValue value = JSValue::decode(m_buckets[bucket]);
        if (!!value) {
            if (!value.isCell())
                return 0;
            return value.asCell()->structure()->classInfo();
        }
        return 0;
    }
    
    unsigned numberOfSamples() const
    {
        unsigned result = 0;
        for (unsigned i = 0; i < totalNumberOfBuckets; ++i) {
            if (!!JSValue::decode(m_buckets[i]))
                result++;
        }
        return result;
    }
    
    unsigned totalNumberOfSamples() const
    {
        return numberOfSamples() + m_numberOfSamplesInPrediction;
    }
    
    bool isLive() const
    {
        for (unsigned i = 0; i < totalNumberOfBuckets; ++i) {
            if (!!JSValue::decode(m_buckets[i]))
                return true;
        }
        return false;
    }
    
    CString briefDescription()
    {
        computeUpdatedPrediction();
        
        StringPrintStream out;
        
        if (m_singletonValueIsTop)
            out.print("predicting ", SpeculationDump(m_prediction));
        else if (m_singletonValue)
            out.print("predicting ", m_singletonValue);
        
        return out.toCString();
    }
    
    void dump(PrintStream& out)
    {
        out.print("samples = ", totalNumberOfSamples(), " prediction = ", SpeculationDump(m_prediction));
        out.printf(", value = ");
        if (m_singletonValueIsTop)
            out.printf("TOP");
        else
            out.print(m_singletonValue);
        bool first = true;
        for (unsigned i = 0; i < totalNumberOfBuckets; ++i) {
            JSValue value = JSValue::decode(m_buckets[i]);
            if (!!value) {
                if (first) {
                    out.printf(": ");
                    first = false;
                } else
                    out.printf(", ");
                out.print(value);
            }
        }
    }
    
    // Updates the prediction and returns the new one.
    SpeculatedType computeUpdatedPrediction(OperationInProgress operation = NoOperation)
    {
        for (unsigned i = 0; i < totalNumberOfBuckets; ++i) {
            JSValue value = JSValue::decode(m_buckets[i]);
            if (!value)
                continue;
            
            m_numberOfSamplesInPrediction++;
            mergeSpeculation(m_prediction, speculationFromValue(value));
            
            if (!m_singletonValueIsTop && !!value) {
                if (!m_singletonValue)
                    m_singletonValue = value;
                else if (m_singletonValue != value)
                    m_singletonValueIsTop = true;
            }
            
            m_buckets[i] = JSValue::encode(JSValue());
        }
        
        if (operation == Collection
            && !m_singletonValueIsTop
            && !!m_singletonValue
            && m_singletonValue.isCell()
            && !Heap::isMarked(m_singletonValue.asCell()))
            m_singletonValueIsTop = true;
            
        return m_prediction;
    }
    
    int m_bytecodeOffset; // -1 for prologue
    
    SpeculatedType m_prediction;
    unsigned m_numberOfSamplesInPrediction;
    
    bool m_singletonValueIsTop;
    JSValue m_singletonValue;

    EncodedJSValue m_buckets[totalNumberOfBuckets];
};

struct MinimalValueProfile : public ValueProfileBase<0> {
    MinimalValueProfile(): ValueProfileBase<0>() { }
    MinimalValueProfile(int bytecodeOffset): ValueProfileBase<0>(bytecodeOffset) { }
};

template<unsigned logNumberOfBucketsArgument>
struct ValueProfileWithLogNumberOfBuckets : public ValueProfileBase<1 << logNumberOfBucketsArgument> {
    static const unsigned logNumberOfBuckets = logNumberOfBucketsArgument;
    
    ValueProfileWithLogNumberOfBuckets()
        : ValueProfileBase<1 << logNumberOfBucketsArgument>()
    {
    }
    ValueProfileWithLogNumberOfBuckets(int bytecodeOffset)
        : ValueProfileBase<1 << logNumberOfBucketsArgument>(bytecodeOffset)
    {
    }
};

struct ValueProfile : public ValueProfileWithLogNumberOfBuckets<0> {
    ValueProfile(): ValueProfileWithLogNumberOfBuckets<0>() { }
    ValueProfile(int bytecodeOffset): ValueProfileWithLogNumberOfBuckets<0>(bytecodeOffset) { }
};

template<typename T>
inline int getValueProfileBytecodeOffset(T* valueProfile)
{
    return valueProfile->m_bytecodeOffset;
}

// This is a mini value profile to catch pathologies. It is a counter that gets
// incremented when we take the slow path on any instruction.
struct RareCaseProfile {
    RareCaseProfile(int bytecodeOffset)
        : m_bytecodeOffset(bytecodeOffset)
        , m_counter(0)
    {
    }
    
    int m_bytecodeOffset;
    uint32_t m_counter;
};

inline int getRareCaseProfileBytecodeOffset(RareCaseProfile* rareCaseProfile)
{
    return rareCaseProfile->m_bytecodeOffset;
}

} // namespace JSC

#endif // ENABLE(VALUE_PROFILER)

#endif // ValueProfile_h

