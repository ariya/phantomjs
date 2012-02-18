/*
 * Copyright (C) 2008 Apple Inc. All rights reserved.
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

#ifndef SamplingTool_h
#define SamplingTool_h

#include "Strong.h"
#include "Nodes.h"
#include "Opcode.h"
#include <wtf/Assertions.h>
#include <wtf/HashMap.h>
#include <wtf/Threading.h>

namespace JSC {

    class ScriptExecutable;

    class SamplingFlags {
    public:
        static void start();
        static void stop();

#if ENABLE(SAMPLING_FLAGS)
        static void setFlag(unsigned flag)
        {
            ASSERT(flag >= 1);
            ASSERT(flag <= 32);
            s_flags |= 1u << (flag - 1);
        }

        static void clearFlag(unsigned flag)
        {
            ASSERT(flag >= 1);
            ASSERT(flag <= 32);
            s_flags &= ~(1u << (flag - 1));
        }

        static void sample();

        class ScopedFlag {
        public:
            ScopedFlag(int flag)
                : m_flag(flag)
            {
                setFlag(flag);
            }

            ~ScopedFlag()
            {
                clearFlag(m_flag);
            }

        private:
            int m_flag;
        };
    
        static const void* addressOfFlags()
        {
            return &s_flags;
        }

#endif
    private:
        static uint32_t s_flags;
#if ENABLE(SAMPLING_FLAGS)
        static uint64_t s_flagCounts[33];
#endif
    };

    class CodeBlock;
    class ExecState;
    class Interpreter;
    class ScopeNode;
    struct Instruction;

    struct ScriptSampleRecord {
        ScriptSampleRecord(JSGlobalData& globalData, ScriptExecutable* executable)
            : m_executable(globalData, executable)
            , m_codeBlock(0)
            , m_sampleCount(0)
            , m_opcodeSampleCount(0)
            , m_samples(0)
            , m_size(0)
        {
        }
        
        ~ScriptSampleRecord()
        {
            if (m_samples)
                free(m_samples);
        }
        
        void sample(CodeBlock*, Instruction*);

        Strong<ScriptExecutable> m_executable;
        CodeBlock* m_codeBlock;
        int m_sampleCount;
        int m_opcodeSampleCount;
        int* m_samples;
        unsigned m_size;
    };

    typedef WTF::HashMap<ScriptExecutable*, ScriptSampleRecord*> ScriptSampleRecordMap;

    class SamplingThread {
    public:
        // Sampling thread state.
        static bool s_running;
        static unsigned s_hertz;
        static ThreadIdentifier s_samplingThread;

        static void start(unsigned hertz=10000);
        static void stop();

        static void* threadStartFunc(void*);
    };

    class SamplingTool {
    public:
        friend struct CallRecord;
        friend class HostCallRecord;
        
#if ENABLE(OPCODE_SAMPLING)
        class CallRecord {
            WTF_MAKE_NONCOPYABLE(CallRecord);
        public:
            CallRecord(SamplingTool* samplingTool)
                : m_samplingTool(samplingTool)
                , m_savedSample(samplingTool->m_sample)
                , m_savedCodeBlock(samplingTool->m_codeBlock)
            {
            }

            ~CallRecord()
            {
                m_samplingTool->m_sample = m_savedSample;
                m_samplingTool->m_codeBlock = m_savedCodeBlock;
            }

        private:
            SamplingTool* m_samplingTool;
            intptr_t m_savedSample;
            CodeBlock* m_savedCodeBlock;
        };
        
        class HostCallRecord : public CallRecord {
        public:
            HostCallRecord(SamplingTool* samplingTool)
                : CallRecord(samplingTool)
            {
                samplingTool->m_sample |= 0x1;
            }
        };
#else
        class CallRecord {
            WTF_MAKE_NONCOPYABLE(CallRecord);
        public:
            CallRecord(SamplingTool*)
            {
            }
        };

        class HostCallRecord : public CallRecord {
        public:
            HostCallRecord(SamplingTool* samplingTool)
                : CallRecord(samplingTool)
            {
            }
        };
#endif        

        SamplingTool(Interpreter* interpreter)
            : m_interpreter(interpreter)
            , m_codeBlock(0)
            , m_sample(0)
            , m_sampleCount(0)
            , m_opcodeSampleCount(0)
#if ENABLE(CODEBLOCK_SAMPLING)
            , m_scopeSampleMap(new ScriptSampleRecordMap())
#endif
        {
            memset(m_opcodeSamples, 0, sizeof(m_opcodeSamples));
            memset(m_opcodeSamplesInCTIFunctions, 0, sizeof(m_opcodeSamplesInCTIFunctions));
        }

        ~SamplingTool()
        {
#if ENABLE(CODEBLOCK_SAMPLING)
            deleteAllValues(*m_scopeSampleMap);
#endif
        }

        void setup();
        void dump(ExecState*);

        void notifyOfScope(ScriptExecutable* scope);

        void sample(CodeBlock* codeBlock, Instruction* vPC)
        {
            ASSERT(!(reinterpret_cast<intptr_t>(vPC) & 0x3));
            m_codeBlock = codeBlock;
            m_sample = reinterpret_cast<intptr_t>(vPC);
        }

        CodeBlock** codeBlockSlot() { return &m_codeBlock; }
        intptr_t* sampleSlot() { return &m_sample; }

        void* encodeSample(Instruction* vPC, bool inCTIFunction = false, bool inHostFunction = false)
        {
            ASSERT(!(reinterpret_cast<intptr_t>(vPC) & 0x3));
            return reinterpret_cast<void*>(reinterpret_cast<intptr_t>(vPC) | (static_cast<intptr_t>(inCTIFunction) << 1) | static_cast<intptr_t>(inHostFunction));
        }

        static void sample();

    private:
        class Sample {
        public:
            Sample(volatile intptr_t sample, CodeBlock* volatile codeBlock)
                : m_sample(sample)
                , m_codeBlock(codeBlock)
            {
            }
            
            bool isNull() { return !m_sample; }
            CodeBlock* codeBlock() { return m_codeBlock; }
            Instruction* vPC() { return reinterpret_cast<Instruction*>(m_sample & ~0x3); }
            bool inHostFunction() { return m_sample & 0x1; }
            bool inCTIFunction() { return m_sample & 0x2; }

        private:
            intptr_t m_sample;
            CodeBlock* m_codeBlock;
        };

        void doRun();
        static SamplingTool* s_samplingTool;
        
        Interpreter* m_interpreter;
        
        // State tracked by the main thread, used by the sampling thread.
        CodeBlock* m_codeBlock;
        intptr_t m_sample;

        // Gathered sample data.
        long long m_sampleCount;
        long long m_opcodeSampleCount;
        unsigned m_opcodeSamples[numOpcodeIDs];
        unsigned m_opcodeSamplesInCTIFunctions[numOpcodeIDs];
        
#if ENABLE(CODEBLOCK_SAMPLING)
        Mutex m_scriptSampleMapMutex;
        OwnPtr<ScriptSampleRecordMap> m_scopeSampleMap;
#endif
    };

    // AbstractSamplingCounter:
    //
    // Implements a named set of counters, printed on exit if ENABLE(SAMPLING_COUNTERS).
    // See subclasses below, SamplingCounter, GlobalSamplingCounter and DeletableSamplingCounter.
    class AbstractSamplingCounter {
        friend class DeletableSamplingCounter;
    public:
        void count(uint32_t count = 1)
        {
            m_counter += count;
        }

        static void dump();

        int64_t* addressOfCounter() { return &m_counter; }

    protected:
        // Effectively the contructor, however called lazily in the case of GlobalSamplingCounter.
        void init(const char* name)
        {
            m_counter = 0;
            m_name = name;

            // Set m_next to point to the head of the chain, and inform whatever is
            // currently at the head that this node will now hold the pointer to it.
            m_next = s_abstractSamplingCounterChain;
            s_abstractSamplingCounterChain->m_referer = &m_next;
            // Add this node to the head of the list.
            s_abstractSamplingCounterChain = this;
            m_referer = &s_abstractSamplingCounterChain;
        }

        int64_t m_counter;
        const char* m_name;
        AbstractSamplingCounter* m_next;
        // This is a pointer to the pointer to this node in the chain; used to
        // allow fast linked list deletion.
        AbstractSamplingCounter** m_referer;
        // Null object used to detect end of static chain.
        static AbstractSamplingCounter s_abstractSamplingCounterChainEnd;
        static AbstractSamplingCounter* s_abstractSamplingCounterChain;
        static bool s_completed;
    };

#if ENABLE(SAMPLING_COUNTERS)
    // SamplingCounter:
    //
    // This class is suitable and (hopefully!) convenient for cases where a counter is
    // required within the scope of a single function.  It can be instantiated as a
    // static variable since it contains a constructor but not a destructor (static
    // variables in WebKit cannot have destructors).
    //
    // For example:
    //
    // void someFunction()
    // {
    //     static SamplingCounter countMe("This is my counter.  There are many like it, but this one is mine.");
    //     countMe.count();
    //     // ...
    // }
    //
    class SamplingCounter : public AbstractSamplingCounter {
    public:
        SamplingCounter(const char* name) { init(name); }
    };

    // GlobalSamplingCounter:
    //
    // This class is suitable for use where a counter is to be declared globally,
    // since it contains neither a constructor nor destructor.  Instead, ensure
    // that 'name()' is called to provide the counter with a name (and also to
    // allow it to be printed out on exit).
    //
    // GlobalSamplingCounter globalCounter;
    //
    // void firstFunction()
    // {
    //     // Put this within a function that is definitely called!
    //     // (Or alternatively alongside all calls to 'count()').
    //     globalCounter.name("I Name You Destroyer.");
    //     globalCounter.count();
    //     // ...
    // }
    //
    // void secondFunction()
    // {
    //     globalCounter.count();
    //     // ...
    // }
    //
    class GlobalSamplingCounter : public AbstractSamplingCounter {
    public:
        void name(const char* name)
        {
            // Global objects should be mapped in zero filled memory, so this should
            // be a safe (albeit not necessarily threadsafe) check for 'first call'.
            if (!m_next)
                init(name);
        }
    };

    // DeletableSamplingCounter:
    //
    // The above classes (SamplingCounter, GlobalSamplingCounter), are intended for
    // use within a global or static scope, and as such cannot have a destructor.
    // This means there is no convenient way for them to remove themselves from the
    // static list of counters, and should an instance of either class be freed
    // before 'dump()' has walked over the list it will potentially walk over an
    // invalid pointer.
    //
    // This class is intended for use where the counter may possibly be deleted before
    // the program exits.  Should this occur, the counter will print it's value to
    // stderr, and remove itself from the static list.  Example:
    //
    // DeletableSamplingCounter* counter = new DeletableSamplingCounter("The Counter With No Name");
    // counter->count();
    // delete counter;
    //
    class DeletableSamplingCounter : public AbstractSamplingCounter {
    public:
        DeletableSamplingCounter(const char* name) { init(name); }

        ~DeletableSamplingCounter()
        {
            if (!s_completed)
                fprintf(stderr, "DeletableSamplingCounter \"%s\" deleted early (with count %lld)\n", m_name, m_counter);
            // Our m_referer pointer should know where the pointer to this node is,
            // and m_next should know that this node is the previous node in the list.
            ASSERT(*m_referer == this);
            ASSERT(m_next->m_referer == &m_next);
            // Remove this node from the list, and inform m_next that we have done so.
            m_next->m_referer = m_referer;
            *m_referer = m_next;
        }
    };
#endif

} // namespace JSC

#endif // SamplingTool_h
