/*
 * Copyright (C) 2008, 2009 Apple Inc. All rights reserved.
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

#ifndef JSGlobalData_h
#define JSGlobalData_h

#include "CachedTranscendentalFunction.h"
#include "Heap.h"
#include "DateInstanceCache.h"
#include "ExecutableAllocator.h"
#include "Strong.h"
#include "JITStubs.h"
#include "JSValue.h"
#include "NumericStrings.h"
#include "SmallStrings.h"
#include "Terminator.h"
#include "TimeoutChecker.h"
#include "WeakRandom.h"
#include <wtf/BumpPointerAllocator.h>
#include <wtf/Forward.h>
#include <wtf/HashMap.h>
#include <wtf/RefCounted.h>
#include <wtf/ThreadSpecific.h>
#include <wtf/WTFThreadData.h>
#if ENABLE(REGEXP_TRACING)
#include <wtf/ListHashSet.h>
#endif

struct OpaqueJSClass;
struct OpaqueJSClassContextData;

namespace JSC {

    class CodeBlock;
    class CommonIdentifiers;
    class HandleStack;
    class IdentifierTable;
    class Interpreter;
    class JSGlobalObject;
    class JSObject;
    class Lexer;
    class NativeExecutable;
    class Parser;
    class RegExpCache;
    class Stringifier;
    class Structure;
    class UString;
#if ENABLE(REGEXP_TRACING)
    class RegExp;
#endif

    struct HashTable;
    struct Instruction;

    struct DSTOffsetCache {
        DSTOffsetCache()
        {
            reset();
        }
        
        void reset()
        {
            offset = 0.0;
            start = 0.0;
            end = -1.0;
            increment = 0.0;
        }

        double offset;
        double start;
        double end;
        double increment;
    };

    enum ThreadStackType {
        ThreadStackTypeLarge,
        ThreadStackTypeSmall
    };

    class JSGlobalData : public RefCounted<JSGlobalData> {
    public:
        // WebCore has a one-to-one mapping of threads to JSGlobalDatas;
        // either create() or createLeaked() should only be called once
        // on a thread, this is the 'default' JSGlobalData (it uses the
        // thread's default string uniquing table from wtfThreadData).
        // API contexts created using the new context group aware interface
        // create APIContextGroup objects which require less locking of JSC
        // than the old singleton APIShared JSGlobalData created for use by
        // the original API.
        enum GlobalDataType { Default, APIContextGroup, APIShared };

        struct ClientData {
            virtual ~ClientData() = 0;
        };

        bool isSharedInstance() { return globalDataType == APIShared; }
        bool usingAPI() { return globalDataType != Default; }
        static bool sharedInstanceExists();
        static JSGlobalData& sharedInstance();

        static PassRefPtr<JSGlobalData> create(ThreadStackType);
        static PassRefPtr<JSGlobalData> createLeaked(ThreadStackType);
        static PassRefPtr<JSGlobalData> createContextGroup(ThreadStackType);
        ~JSGlobalData();

#if ENABLE(JSC_MULTIPLE_THREADS)
        // Will start tracking threads that use the heap, which is resource-heavy.
        void makeUsableFromMultipleThreads() { heap.machineThreads().makeUsableFromMultipleThreads(); }
#endif

        GlobalDataType globalDataType;
        ClientData* clientData;

        const HashTable* arrayTable;
        const HashTable* dateTable;
        const HashTable* jsonTable;
        const HashTable* mathTable;
        const HashTable* numberTable;
        const HashTable* objectConstructorTable;
        const HashTable* regExpTable;
        const HashTable* regExpConstructorTable;
        const HashTable* stringTable;
        
        Strong<Structure> structureStructure;
        Strong<Structure> debuggerActivationStructure;
        Strong<Structure> activationStructure;
        Strong<Structure> interruptedExecutionErrorStructure;
        Strong<Structure> terminatedExecutionErrorStructure;
        Strong<Structure> staticScopeStructure;
        Strong<Structure> strictEvalActivationStructure;
        Strong<Structure> stringStructure;
        Strong<Structure> notAnObjectStructure;
        Strong<Structure> propertyNameIteratorStructure;
        Strong<Structure> getterSetterStructure;
        Strong<Structure> apiWrapperStructure;
        Strong<Structure> scopeChainNodeStructure;
        Strong<Structure> executableStructure;
        Strong<Structure> nativeExecutableStructure;
        Strong<Structure> evalExecutableStructure;
        Strong<Structure> programExecutableStructure;
        Strong<Structure> functionExecutableStructure;
        Strong<Structure> dummyMarkableCellStructure;
        Strong<Structure> structureChainStructure;

#if ENABLE(JSC_ZOMBIES)
        Strong<Structure> zombieStructure;
#endif

        static void storeVPtrs();
        static JS_EXPORTDATA void* jsArrayVPtr;
        static JS_EXPORTDATA void* jsByteArrayVPtr;
        static JS_EXPORTDATA void* jsStringVPtr;
        static JS_EXPORTDATA void* jsFunctionVPtr;

        IdentifierTable* identifierTable;
        CommonIdentifiers* propertyNames;
        const MarkedArgumentBuffer* emptyList; // Lists are supposed to be allocated on the stack to have their elements properly marked, which is not the case here - but this list has nothing to mark.
        SmallStrings smallStrings;
        NumericStrings numericStrings;
        DateInstanceCache dateInstanceCache;
        
#if ENABLE(ASSEMBLER)
        ExecutableAllocator executableAllocator;
        ExecutableAllocator regexAllocator;
#endif

#if !ENABLE(JIT)
        bool canUseJIT() { return false; } // interpreter only
#elif !ENABLE(INTERPRETER)
        bool canUseJIT() { return true; } // jit only
#else
        bool canUseJIT() { return m_canUseJIT; }
#endif

        const StackBounds& stack()
        {
            return (globalDataType == Default)
                ? m_stack
                : wtfThreadData().stack();
        }

        Lexer* lexer;
        Parser* parser;
        Interpreter* interpreter;
#if ENABLE(JIT)
        OwnPtr<JITThunks> jitStubs;
        MacroAssemblerCodePtr getCTIStub(ThunkGenerator generator)
        {
            return jitStubs->ctiStub(this, generator);
        }
        NativeExecutable* getHostFunction(NativeFunction, ThunkGenerator);
#endif
        NativeExecutable* getHostFunction(NativeFunction);

        TimeoutChecker timeoutChecker;
        Terminator terminator;
        Heap heap;

        JSValue exception;
#if ENABLE(JIT)
        ReturnAddressPtr exceptionLocation;
#endif

        HashMap<OpaqueJSClass*, OpaqueJSClassContextData*> opaqueJSClassData;

        unsigned globalObjectCount;
        JSGlobalObject* dynamicGlobalObject;

        HashSet<JSObject*> stringRecursionCheckVisitedObjects;

        double cachedUTCOffset;
        DSTOffsetCache dstOffsetCache;
        
        UString cachedDateString;
        double cachedDateStringValue;

        int maxReentryDepth;

        RegExpCache* m_regExpCache;
        BumpPointerAllocator m_regExpAllocator;

#if ENABLE(REGEXP_TRACING)
        typedef ListHashSet<RefPtr<RegExp> > RTTraceList;
        RTTraceList* m_rtTraceList;
#endif

#ifndef NDEBUG
        ThreadIdentifier exclusiveThread;
#endif

        CachedTranscendentalFunction<sin> cachedSin;

        void resetDateCache();

        void startSampling();
        void stopSampling();
        void dumpSampleData(ExecState* exec);
        void recompileAllJSFunctions();
        RegExpCache* regExpCache() { return m_regExpCache; }
#if ENABLE(REGEXP_TRACING)
        void addRegExpToTrace(PassRefPtr<RegExp> regExp);
#endif
        void dumpRegExpTrace();
        HandleSlot allocateGlobalHandle() { return heap.allocateGlobalHandle(); }
        HandleSlot allocateLocalHandle() { return heap.allocateLocalHandle(); }
        void clearBuiltinStructures();

    private:
        JSGlobalData(GlobalDataType, ThreadStackType);
        static JSGlobalData*& sharedInstanceInternal();
        void createNativeThunk();
#if ENABLE(JIT) && ENABLE(INTERPRETER)
        bool m_canUseJIT;
#endif
        StackBounds m_stack;
    };

    inline HandleSlot allocateGlobalHandle(JSGlobalData& globalData)
    {
        return globalData.allocateGlobalHandle();
    }

} // namespace JSC

#endif // JSGlobalData_h
