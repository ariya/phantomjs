/*
 * Copyright (C) 2008, 2009, 2013 Apple Inc. All rights reserved.
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

#ifndef VM_h
#define VM_h

#include "CachedTranscendentalFunction.h"
#include "DateInstanceCache.h"
#include "ExecutableAllocator.h"
#include "Heap.h"
#include "Intrinsic.h"
#include "JITThunks.h"
#include "JITThunks.h"
#include "JSCJSValue.h"
#include "JSLock.h"
#include "LLIntData.h"
#include "MacroAssemblerCodeRef.h"
#include "NumericStrings.h"
#include "ProfilerDatabase.h"
#include "PrivateName.h"
#include "PrototypeMap.h"
#include "SmallStrings.h"
#include "Strong.h"
#include "ThunkGenerators.h"
#include "TypedArrayDescriptor.h"
#include "Watchdog.h"
#include "WeakRandom.h"
#include <wtf/BumpPointerAllocator.h>
#include <wtf/DateMath.h>
#include <wtf/Forward.h>
#include <wtf/HashMap.h>
#include <wtf/RefCountedArray.h>
#include <wtf/SimpleStats.h>
#include <wtf/ThreadSafeRefCounted.h>
#include <wtf/ThreadSpecific.h>
#include <wtf/WTFThreadData.h>
#if ENABLE(REGEXP_TRACING)
#include <wtf/ListHashSet.h>
#endif

namespace JSC {

    class CodeBlock;
    class CodeCache;
    class CommonIdentifiers;
    class ExecState;
    class HandleStack;
    class IdentifierTable;
    class Interpreter;
    class JSGlobalObject;
    class JSObject;
    class Keywords;
    class LLIntOffsetsExtractor;
    class LegacyProfiler;
    class NativeExecutable;
    class ParserArena;
    class RegExpCache;
    class SourceProvider;
    class SourceProviderCache;
    struct StackFrame;
    class Stringifier;
    class Structure;
#if ENABLE(REGEXP_TRACING)
    class RegExp;
#endif
    class UnlinkedCodeBlock;
    class UnlinkedEvalCodeBlock;
    class UnlinkedFunctionExecutable;
    class UnlinkedProgramCodeBlock;

#if ENABLE(DFG_JIT)
    namespace DFG {
    class LongLivedState;
    }
#endif // ENABLE(DFG_JIT)

    struct HashTable;
    struct Instruction;

    struct LocalTimeOffsetCache {
        LocalTimeOffsetCache()
            : start(0.0)
            , end(-1.0)
            , increment(0.0)
        {
        }
        
        void reset()
        {
            offset = LocalTimeOffset();
            start = 0.0;
            end = -1.0;
            increment = 0.0;
        }

        LocalTimeOffset offset;
        double start;
        double end;
        double increment;
    };

#if ENABLE(DFG_JIT)
    class ConservativeRoots;

#if COMPILER(MSVC)
#pragma warning(push)
#pragma warning(disable: 4200) // Disable "zero-sized array in struct/union" warning
#endif
    struct ScratchBuffer {
        ScratchBuffer()
        {
            u.m_activeLength = 0;
        }

        static ScratchBuffer* create(size_t size)
        {
            ScratchBuffer* result = new (fastMalloc(ScratchBuffer::allocationSize(size))) ScratchBuffer;

            return result;
        }

        static size_t allocationSize(size_t bufferSize) { return sizeof(ScratchBuffer) + bufferSize; }
        void setActiveLength(size_t activeLength) { u.m_activeLength = activeLength; }
        size_t activeLength() const { return u.m_activeLength; };
        size_t* activeLengthPtr() { return &u.m_activeLength; };
        void* dataBuffer() { return m_buffer; }

        union {
            size_t m_activeLength;
            double pad; // Make sure m_buffer is double aligned.
        } u;
#if CPU(MIPS) && (defined WTF_MIPS_ARCH_REV && WTF_MIPS_ARCH_REV == 2)
        void* m_buffer[0] __attribute__((aligned(8)));
#else
        void* m_buffer[0];
#endif
    };
#if COMPILER(MSVC)
#pragma warning(pop)
#endif
#endif

    class VM : public ThreadSafeRefCounted<VM> {
    public:
        // WebCore has a one-to-one mapping of threads to VMs;
        // either create() or createLeaked() should only be called once
        // on a thread, this is the 'default' VM (it uses the
        // thread's default string uniquing table from wtfThreadData).
        // API contexts created using the new context group aware interface
        // create APIContextGroup objects which require less locking of JSC
        // than the old singleton APIShared VM created for use by
        // the original API.
        enum VMType { Default, APIContextGroup, APIShared };
        
        struct ClientData {
            JS_EXPORT_PRIVATE virtual ~ClientData() = 0;
        };

        bool isSharedInstance() { return vmType == APIShared; }
        bool usingAPI() { return vmType != Default; }
        static bool sharedInstanceExists();
        JS_EXPORT_PRIVATE static VM& sharedInstance();

        JS_EXPORT_PRIVATE static PassRefPtr<VM> create(HeapType = SmallHeap);
        JS_EXPORT_PRIVATE static PassRefPtr<VM> createLeaked(HeapType = SmallHeap);
        static PassRefPtr<VM> createContextGroup(HeapType = SmallHeap);
        JS_EXPORT_PRIVATE ~VM();

        void makeUsableFromMultipleThreads() { heap.machineThreads().makeUsableFromMultipleThreads(); }

    private:
        RefPtr<JSLock> m_apiLock;

    public:
#if ENABLE(ASSEMBLER)
        // executableAllocator should be destructed after the heap, as the heap can call executableAllocator
        // in its destructor.
        ExecutableAllocator executableAllocator;
#endif

        // The heap should be just after executableAllocator and before other members to ensure that it's
        // destructed after all the objects that reference it.
        Heap heap;
        
#if ENABLE(DFG_JIT)
        OwnPtr<DFG::LongLivedState> m_dfgState;
#endif // ENABLE(DFG_JIT)

        VMType vmType;
        ClientData* clientData;
        ExecState* topCallFrame;
        Watchdog watchdog;

        const HashTable* arrayConstructorTable;
        const HashTable* arrayPrototypeTable;
        const HashTable* booleanPrototypeTable;
        const HashTable* dateTable;
        const HashTable* dateConstructorTable;
        const HashTable* errorPrototypeTable;
        const HashTable* globalObjectTable;
        const HashTable* jsonTable;
        const HashTable* mathTable;
        const HashTable* numberConstructorTable;
        const HashTable* numberPrototypeTable;
        const HashTable* objectConstructorTable;
        const HashTable* privateNamePrototypeTable;
        const HashTable* regExpTable;
        const HashTable* regExpConstructorTable;
        const HashTable* regExpPrototypeTable;
        const HashTable* stringConstructorTable;
        
        Strong<Structure> structureStructure;
        Strong<Structure> structureRareDataStructure;
        Strong<Structure> debuggerActivationStructure;
        Strong<Structure> terminatedExecutionErrorStructure;
        Strong<Structure> stringStructure;
        Strong<Structure> notAnObjectStructure;
        Strong<Structure> propertyNameIteratorStructure;
        Strong<Structure> getterSetterStructure;
        Strong<Structure> apiWrapperStructure;
        Strong<Structure> JSScopeStructure;
        Strong<Structure> executableStructure;
        Strong<Structure> nativeExecutableStructure;
        Strong<Structure> evalExecutableStructure;
        Strong<Structure> programExecutableStructure;
        Strong<Structure> functionExecutableStructure;
        Strong<Structure> regExpStructure;
        Strong<Structure> sharedSymbolTableStructure;
        Strong<Structure> structureChainStructure;
        Strong<Structure> sparseArrayValueMapStructure;
        Strong<Structure> withScopeStructure;
        Strong<Structure> unlinkedFunctionExecutableStructure;
        Strong<Structure> unlinkedProgramCodeBlockStructure;
        Strong<Structure> unlinkedEvalCodeBlockStructure;
        Strong<Structure> unlinkedFunctionCodeBlockStructure;
        Strong<Structure> propertyTableStructure;

        IdentifierTable* identifierTable;
        CommonIdentifiers* propertyNames;
        const MarkedArgumentBuffer* emptyList; // Lists are supposed to be allocated on the stack to have their elements properly marked, which is not the case here - but this list has nothing to mark.
        SmallStrings smallStrings;
        NumericStrings numericStrings;
        DateInstanceCache dateInstanceCache;
        WTF::SimpleStats machineCodeBytesPerBytecodeWordForBaselineJIT;
        Vector<CodeBlock*> codeBlocksBeingCompiled;
        void startedCompiling(CodeBlock* codeBlock)
        {
            codeBlocksBeingCompiled.append(codeBlock);
        }

        void finishedCompiling(CodeBlock* codeBlock)
        {
            ASSERT_UNUSED(codeBlock, codeBlock == codeBlocksBeingCompiled.last());
            codeBlocksBeingCompiled.removeLast();
        }

        void setInDefineOwnProperty(bool inDefineOwnProperty)
        {
            m_inDefineOwnProperty = inDefineOwnProperty;
        }

        bool isInDefineOwnProperty()
        {
            return m_inDefineOwnProperty;
        }

        LegacyProfiler* enabledProfiler()
        {
            return m_enabledProfiler;
        }

#if ENABLE(JIT) && ENABLE(LLINT)
        bool canUseJIT() { return m_canUseJIT; }
#elif ENABLE(JIT)
        bool canUseJIT() { return true; } // jit only
#else
        bool canUseJIT() { return false; } // interpreter only
#endif

#if ENABLE(YARR_JIT)
        bool canUseRegExpJIT() { return m_canUseRegExpJIT; }
#else
        bool canUseRegExpJIT() { return false; } // interpreter only
#endif

        SourceProviderCache* addSourceProviderCache(SourceProvider*);
        void clearSourceProviderCaches();

        PrototypeMap prototypeMap;

        OwnPtr<ParserArena> parserArena;
        typedef HashMap<RefPtr<SourceProvider>, RefPtr<SourceProviderCache> > SourceProviderCacheMap;
        SourceProviderCacheMap sourceProviderCacheMap;
        OwnPtr<Keywords> keywords;
        Interpreter* interpreter;
#if ENABLE(JIT)
        OwnPtr<JITThunks> jitStubs;
        MacroAssemblerCodeRef getCTIStub(ThunkGenerator generator)
        {
            return jitStubs->ctiStub(this, generator);
        }
        NativeExecutable* getHostFunction(NativeFunction, Intrinsic);
#endif
        NativeExecutable* getHostFunction(NativeFunction, NativeFunction constructor);

        JSValue exception;
        JS_EXPORT_PRIVATE void clearExceptionStack();
        RefCountedArray<StackFrame>& exceptionStack() { return m_exceptionStack; }

        const ClassInfo* const jsArrayClassInfo;
        const ClassInfo* const jsFinalObjectClassInfo;

        ReturnAddressPtr exceptionLocation;
        JSValue hostCallReturnValue;
        ExecState* callFrameForThrow;
        void* targetMachinePCForThrow;
        Instruction* targetInterpreterPCForThrow;
#if ENABLE(DFG_JIT)
        uint32_t osrExitIndex;
        void* osrExitJumpDestination;
        Vector<ScratchBuffer*> scratchBuffers;
        size_t sizeOfLastScratchBuffer;
        
        ScratchBuffer* scratchBufferForSize(size_t size)
        {
            if (!size)
                return 0;
            
            if (size > sizeOfLastScratchBuffer) {
                // Protect against a N^2 memory usage pathology by ensuring
                // that at worst, we get a geometric series, meaning that the
                // total memory usage is somewhere around
                // max(scratch buffer size) * 4.
                sizeOfLastScratchBuffer = size * 2;

                scratchBuffers.append(ScratchBuffer::create(sizeOfLastScratchBuffer));
            }

            ScratchBuffer* result = scratchBuffers.last();
            result->setActiveLength(0);
            return result;
        }

        void gatherConservativeRoots(ConservativeRoots&);
#endif

        JSGlobalObject* dynamicGlobalObject;

        HashSet<JSObject*> stringRecursionCheckVisitedObjects;

        LocalTimeOffsetCache localTimeOffsetCache;
        
        String cachedDateString;
        double cachedDateStringValue;

        LegacyProfiler* m_enabledProfiler;
        OwnPtr<Profiler::Database> m_perBytecodeProfiler;
        RegExpCache* m_regExpCache;
        BumpPointerAllocator m_regExpAllocator;

#if ENABLE(REGEXP_TRACING)
        typedef ListHashSet<RefPtr<RegExp> > RTTraceList;
        RTTraceList* m_rtTraceList;
#endif

#ifndef NDEBUG
        ThreadIdentifier exclusiveThread;
#endif

        CachedTranscendentalFunction<std::sin> cachedSin;

        JS_EXPORT_PRIVATE void resetDateCache();

        JS_EXPORT_PRIVATE void startSampling();
        JS_EXPORT_PRIVATE void stopSampling();
        JS_EXPORT_PRIVATE void dumpSampleData(ExecState* exec);
        RegExpCache* regExpCache() { return m_regExpCache; }
#if ENABLE(REGEXP_TRACING)
        void addRegExpToTrace(PassRefPtr<RegExp> regExp);
#endif
        JS_EXPORT_PRIVATE void dumpRegExpTrace();

        bool isCollectorBusy() { return heap.isBusy(); }
        JS_EXPORT_PRIVATE void releaseExecutableMemory();

#if ENABLE(GC_VALIDATION)
        bool isInitializingObject() const; 
        void setInitializingObjectClass(const ClassInfo*);
#endif

        unsigned m_newStringsSinceLastHashCons;

        static const unsigned s_minNumberOfNewStringsToHashCons = 100;

        bool haveEnoughNewStringsToHashCons() { return m_newStringsSinceLastHashCons > s_minNumberOfNewStringsToHashCons; }
        void resetNewStringsSinceLastHashCons() { m_newStringsSinceLastHashCons = 0; }

#define registerTypedArrayFunction(type, capitalizedType) \
        void registerTypedArrayDescriptor(const capitalizedType##Array*, const TypedArrayDescriptor& descriptor) \
        { \
            ASSERT(!m_##type##ArrayDescriptor.m_classInfo || m_##type##ArrayDescriptor.m_classInfo == descriptor.m_classInfo); \
            m_##type##ArrayDescriptor = descriptor; \
            ASSERT(m_##type##ArrayDescriptor.m_classInfo); \
        } \
        const TypedArrayDescriptor& type##ArrayDescriptor() const { ASSERT(m_##type##ArrayDescriptor.m_classInfo); return m_##type##ArrayDescriptor; }

        registerTypedArrayFunction(int8, Int8);
        registerTypedArrayFunction(int16, Int16);
        registerTypedArrayFunction(int32, Int32);
        registerTypedArrayFunction(uint8, Uint8);
        registerTypedArrayFunction(uint8Clamped, Uint8Clamped);
        registerTypedArrayFunction(uint16, Uint16);
        registerTypedArrayFunction(uint32, Uint32);
        registerTypedArrayFunction(float32, Float32);
        registerTypedArrayFunction(float64, Float64);
#undef registerTypedArrayFunction
        
        const TypedArrayDescriptor* typedArrayDescriptor(TypedArrayType type) const
        {
            switch (type) {
            case TypedArrayNone:
                return 0;
            case TypedArrayInt8:
                return &int8ArrayDescriptor();
            case TypedArrayInt16:
                return &int16ArrayDescriptor();
            case TypedArrayInt32:
                return &int32ArrayDescriptor();
            case TypedArrayUint8:
                return &uint8ArrayDescriptor();
            case TypedArrayUint8Clamped:
                return &uint8ClampedArrayDescriptor();
            case TypedArrayUint16:
                return &uint16ArrayDescriptor();
            case TypedArrayUint32:
                return &uint32ArrayDescriptor();
            case TypedArrayFloat32:
                return &float32ArrayDescriptor();
            case TypedArrayFloat64:
                return &float64ArrayDescriptor();
            default:
                CRASH();
                return 0;
            }
        }

        JSLock& apiLock() { return *m_apiLock; }
        CodeCache* codeCache() { return m_codeCache.get(); }

        JS_EXPORT_PRIVATE void discardAllCode();

    private:
        friend class LLIntOffsetsExtractor;
        
        VM(VMType, HeapType);
        static VM*& sharedInstanceInternal();
        void createNativeThunk();
#if ENABLE(ASSEMBLER)
        bool m_canUseAssembler;
#endif
#if ENABLE(JIT)
        bool m_canUseJIT;
#endif
#if ENABLE(YARR_JIT)
        bool m_canUseRegExpJIT;
#endif
#if ENABLE(GC_VALIDATION)
        const ClassInfo* m_initializingObjectClass;
#endif
        bool m_inDefineOwnProperty;
        RefPtr<CodeCache> m_codeCache;
        RefCountedArray<StackFrame> m_exceptionStack;

        TypedArrayDescriptor m_int8ArrayDescriptor;
        TypedArrayDescriptor m_int16ArrayDescriptor;
        TypedArrayDescriptor m_int32ArrayDescriptor;
        TypedArrayDescriptor m_uint8ArrayDescriptor;
        TypedArrayDescriptor m_uint8ClampedArrayDescriptor;
        TypedArrayDescriptor m_uint16ArrayDescriptor;
        TypedArrayDescriptor m_uint32ArrayDescriptor;
        TypedArrayDescriptor m_float32ArrayDescriptor;
        TypedArrayDescriptor m_float64ArrayDescriptor;
    };

#if ENABLE(GC_VALIDATION)
    inline bool VM::isInitializingObject() const
    {
        return !!m_initializingObjectClass;
    }

    inline void VM::setInitializingObjectClass(const ClassInfo* initializingObjectClass)
    {
        m_initializingObjectClass = initializingObjectClass;
    }
#endif

    inline Heap* WeakSet::heap() const
    {
        return &m_vm->heap;
    }

} // namespace JSC

#endif // VM_h
