/*
 * Copyright (C) 2008, 2012, 2013 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */

#ifndef JIT_h
#define JIT_h

#if ENABLE(JIT)

// Verbose logging of code generation
#define ENABLE_JIT_VERBOSE 0
// Verbose logging for OSR-related code.
#define ENABLE_JIT_VERBOSE_OSR 0

// We've run into some problems where changing the size of the class JIT leads to
// performance fluctuations.  Try forcing alignment in an attempt to stabalize this.
#if COMPILER(GCC)
#define JIT_CLASS_ALIGNMENT __attribute__ ((aligned (32)))
#else
#define JIT_CLASS_ALIGNMENT
#endif

#define ASSERT_JIT_OFFSET(actual, expected) ASSERT_WITH_MESSAGE(actual == expected, "JIT Offset \"%s\" should be %d, not %d.\n", #expected, static_cast<int>(expected), static_cast<int>(actual));

#include "CodeBlock.h"
#include "CompactJITCodeMap.h"
#include "Interpreter.h"
#include "JITDisassembler.h"
#include "JSInterfaceJIT.h"
#include "LegacyProfiler.h"
#include "Opcode.h"
#include "ResultType.h"
#include "UnusedPointer.h"
#include <bytecode/SamplingTool.h>

namespace JSC {

    class CodeBlock;
    class FunctionExecutable;
    class JIT;
    class JSPropertyNameIterator;
    class Interpreter;
    class JSScope;
    class JSStack;
    class MarkedAllocator;
    class Register;
    class StructureChain;

    struct CallLinkInfo;
    struct Instruction;
    struct OperandTypes;
    struct PolymorphicAccessStructureList;
    struct SimpleJumpTable;
    struct StringJumpTable;
    struct StructureStubInfo;

    struct CallRecord {
        MacroAssembler::Call from;
        unsigned bytecodeOffset;
        void* to;

        CallRecord()
        {
        }

        CallRecord(MacroAssembler::Call from, unsigned bytecodeOffset, void* to = 0)
            : from(from)
            , bytecodeOffset(bytecodeOffset)
            , to(to)
        {
        }
    };

    struct JumpTable {
        MacroAssembler::Jump from;
        unsigned toBytecodeOffset;

        JumpTable(MacroAssembler::Jump f, unsigned t)
            : from(f)
            , toBytecodeOffset(t)
        {
        }
    };

    struct SlowCaseEntry {
        MacroAssembler::Jump from;
        unsigned to;
        unsigned hint;
        
        SlowCaseEntry(MacroAssembler::Jump f, unsigned t, unsigned h = 0)
            : from(f)
            , to(t)
            , hint(h)
        {
        }
    };

    struct SwitchRecord {
        enum Type {
            Immediate,
            Character,
            String
        };

        Type type;

        union {
            SimpleJumpTable* simpleJumpTable;
            StringJumpTable* stringJumpTable;
        } jumpTable;

        unsigned bytecodeOffset;
        unsigned defaultOffset;

        SwitchRecord(SimpleJumpTable* jumpTable, unsigned bytecodeOffset, unsigned defaultOffset, Type type)
            : type(type)
            , bytecodeOffset(bytecodeOffset)
            , defaultOffset(defaultOffset)
        {
            this->jumpTable.simpleJumpTable = jumpTable;
        }

        SwitchRecord(StringJumpTable* jumpTable, unsigned bytecodeOffset, unsigned defaultOffset)
            : type(String)
            , bytecodeOffset(bytecodeOffset)
            , defaultOffset(defaultOffset)
        {
            this->jumpTable.stringJumpTable = jumpTable;
        }
    };

    enum PropertyStubGetById_T { PropertyStubGetById };
    enum PropertyStubPutById_T { PropertyStubPutById };

    struct PropertyStubCompilationInfo {
        enum Type { GetById, PutById } m_type;
    
        unsigned bytecodeIndex;
        MacroAssembler::Call callReturnLocation;
        MacroAssembler::Label hotPathBegin;
        MacroAssembler::DataLabelPtr getStructureToCompare;
        MacroAssembler::PatchableJump getStructureCheck;
        MacroAssembler::ConvertibleLoadLabel propertyStorageLoad;
#if USE(JSVALUE64)
        MacroAssembler::DataLabelCompact getDisplacementLabel;
#else
        MacroAssembler::DataLabelCompact getDisplacementLabel1;
        MacroAssembler::DataLabelCompact getDisplacementLabel2;
#endif
        MacroAssembler::Label getPutResult;
        MacroAssembler::Label getColdPathBegin;
        MacroAssembler::DataLabelPtr putStructureToCompare;
#if USE(JSVALUE64)
        MacroAssembler::DataLabel32 putDisplacementLabel;
#else
        MacroAssembler::DataLabel32 putDisplacementLabel1;
        MacroAssembler::DataLabel32 putDisplacementLabel2;
#endif

#if !ASSERT_DISABLED
        PropertyStubCompilationInfo()
            : bytecodeIndex(std::numeric_limits<unsigned>::max())
        {
        }
#endif


        PropertyStubCompilationInfo(
            PropertyStubGetById_T, unsigned bytecodeIndex, MacroAssembler::Label hotPathBegin,
            MacroAssembler::DataLabelPtr structureToCompare,
            MacroAssembler::PatchableJump structureCheck,
            MacroAssembler::ConvertibleLoadLabel propertyStorageLoad,
#if USE(JSVALUE64)
            MacroAssembler::DataLabelCompact displacementLabel,
#else
            MacroAssembler::DataLabelCompact displacementLabel1,
            MacroAssembler::DataLabelCompact displacementLabel2,
#endif
            MacroAssembler::Label putResult)
            : m_type(GetById)
            , bytecodeIndex(bytecodeIndex)
            , hotPathBegin(hotPathBegin)
            , getStructureToCompare(structureToCompare)
            , getStructureCheck(structureCheck)
            , propertyStorageLoad(propertyStorageLoad)
#if USE(JSVALUE64)
            , getDisplacementLabel(displacementLabel)
#else
            , getDisplacementLabel1(displacementLabel1)
            , getDisplacementLabel2(displacementLabel2)
#endif
            , getPutResult(putResult)
        {
        }

        PropertyStubCompilationInfo(
            PropertyStubPutById_T, unsigned bytecodeIndex, MacroAssembler::Label hotPathBegin,
            MacroAssembler::DataLabelPtr structureToCompare,
            MacroAssembler::ConvertibleLoadLabel propertyStorageLoad,
#if USE(JSVALUE64)
            MacroAssembler::DataLabel32 displacementLabel
#else
            MacroAssembler::DataLabel32 displacementLabel1,
            MacroAssembler::DataLabel32 displacementLabel2
#endif
            )
            : m_type(PutById)
            , bytecodeIndex(bytecodeIndex)
            , hotPathBegin(hotPathBegin)
            , propertyStorageLoad(propertyStorageLoad)
            , putStructureToCompare(structureToCompare)
#if USE(JSVALUE64)
            , putDisplacementLabel(displacementLabel)
#else
            , putDisplacementLabel1(displacementLabel1)
            , putDisplacementLabel2(displacementLabel2)
#endif
        {
        }

        void slowCaseInfo(PropertyStubGetById_T, MacroAssembler::Label coldPathBegin, MacroAssembler::Call call)
        {
            ASSERT(m_type == GetById);
            callReturnLocation = call;
            getColdPathBegin = coldPathBegin;
        }

        void slowCaseInfo(PropertyStubPutById_T, MacroAssembler::Call call)
        {
            ASSERT(m_type == PutById);
            callReturnLocation = call;
        }

        void copyToStubInfo(StructureStubInfo& info, LinkBuffer &patchBuffer);
    };

    struct ByValCompilationInfo {
        ByValCompilationInfo() { }
        
        ByValCompilationInfo(unsigned bytecodeIndex, MacroAssembler::PatchableJump badTypeJump, JITArrayMode arrayMode, MacroAssembler::Label doneTarget)
            : bytecodeIndex(bytecodeIndex)
            , badTypeJump(badTypeJump)
            , arrayMode(arrayMode)
            , doneTarget(doneTarget)
        {
        }
        
        unsigned bytecodeIndex;
        MacroAssembler::PatchableJump badTypeJump;
        JITArrayMode arrayMode;
        MacroAssembler::Label doneTarget;
        MacroAssembler::Label slowPathTarget;
        MacroAssembler::Call returnAddress;
    };

    struct StructureStubCompilationInfo {
        MacroAssembler::DataLabelPtr hotPathBegin;
        MacroAssembler::Call hotPathOther;
        MacroAssembler::Call callReturnLocation;
        CallLinkInfo::CallType callType;
        unsigned bytecodeIndex;
    };

    // Near calls can only be patched to other JIT code, regular calls can be patched to JIT code or relinked to stub functions.
    void ctiPatchNearCallByReturnAddress(CodeBlock* codeblock, ReturnAddressPtr returnAddress, MacroAssemblerCodePtr newCalleeFunction);
    void ctiPatchCallByReturnAddress(CodeBlock* codeblock, ReturnAddressPtr returnAddress, MacroAssemblerCodePtr newCalleeFunction);
    void ctiPatchCallByReturnAddress(CodeBlock* codeblock, ReturnAddressPtr returnAddress, FunctionPtr newCalleeFunction);

    class JIT : private JSInterfaceJIT {
        friend class JITStubCall;
        friend struct PropertyStubCompilationInfo;

        using MacroAssembler::Jump;
        using MacroAssembler::JumpList;
        using MacroAssembler::Label;

        static const uintptr_t patchGetByIdDefaultStructure = unusedPointer;
        static const int patchGetByIdDefaultOffset = 0;
        // Magic number - initial offset cannot be representable as a signed 8bit value, or the X86Assembler
        // will compress the displacement, and we may not be able to fit a patched offset.
        static const int patchPutByIdDefaultOffset = 256;

    public:
        static JITCode compile(VM* vm, CodeBlock* codeBlock, JITCompilationEffort effort, CodePtr* functionEntryArityCheck = 0)
        {
            return JIT(vm, codeBlock).privateCompile(functionEntryArityCheck, effort);
        }
        
        static void compileClosureCall(VM* vm, CallLinkInfo* callLinkInfo, CodeBlock* callerCodeBlock, CodeBlock* calleeCodeBlock, Structure* expectedStructure, ExecutableBase* expectedExecutable, MacroAssemblerCodePtr codePtr)
        {
            JIT jit(vm, callerCodeBlock);
            jit.m_bytecodeOffset = callLinkInfo->codeOrigin.bytecodeIndex;
            jit.privateCompileClosureCall(callLinkInfo, calleeCodeBlock, expectedStructure, expectedExecutable, codePtr);
        }

        static void compileGetByIdProto(VM* vm, CallFrame* callFrame, CodeBlock* codeBlock, StructureStubInfo* stubInfo, Structure* structure, Structure* prototypeStructure, const Identifier& ident, const PropertySlot& slot, PropertyOffset cachedOffset, ReturnAddressPtr returnAddress)
        {
            JIT jit(vm, codeBlock);
            jit.m_bytecodeOffset = stubInfo->bytecodeIndex;
            jit.privateCompileGetByIdProto(stubInfo, structure, prototypeStructure, ident, slot, cachedOffset, returnAddress, callFrame);
        }

        static void compileGetByIdSelfList(VM* vm, CodeBlock* codeBlock, StructureStubInfo* stubInfo, PolymorphicAccessStructureList* polymorphicStructures, int currentIndex, Structure* structure, const Identifier& ident, const PropertySlot& slot, PropertyOffset cachedOffset)
        {
            JIT jit(vm, codeBlock);
            jit.m_bytecodeOffset = stubInfo->bytecodeIndex;
            jit.privateCompileGetByIdSelfList(stubInfo, polymorphicStructures, currentIndex, structure, ident, slot, cachedOffset);
        }
        static void compileGetByIdProtoList(VM* vm, CallFrame* callFrame, CodeBlock* codeBlock, StructureStubInfo* stubInfo, PolymorphicAccessStructureList* prototypeStructureList, int currentIndex, Structure* structure, Structure* prototypeStructure, const Identifier& ident, const PropertySlot& slot, PropertyOffset cachedOffset)
        {
            JIT jit(vm, codeBlock);
            jit.m_bytecodeOffset = stubInfo->bytecodeIndex;
            jit.privateCompileGetByIdProtoList(stubInfo, prototypeStructureList, currentIndex, structure, prototypeStructure, ident, slot, cachedOffset, callFrame);
        }
        static void compileGetByIdChainList(VM* vm, CallFrame* callFrame, CodeBlock* codeBlock, StructureStubInfo* stubInfo, PolymorphicAccessStructureList* prototypeStructureList, int currentIndex, Structure* structure, StructureChain* chain, size_t count, const Identifier& ident, const PropertySlot& slot, PropertyOffset cachedOffset)
        {
            JIT jit(vm, codeBlock);
            jit.m_bytecodeOffset = stubInfo->bytecodeIndex;
            jit.privateCompileGetByIdChainList(stubInfo, prototypeStructureList, currentIndex, structure, chain, count, ident, slot, cachedOffset, callFrame);
        }

        static void compileGetByIdChain(VM* vm, CallFrame* callFrame, CodeBlock* codeBlock, StructureStubInfo* stubInfo, Structure* structure, StructureChain* chain, size_t count, const Identifier& ident, const PropertySlot& slot, PropertyOffset cachedOffset, ReturnAddressPtr returnAddress)
        {
            JIT jit(vm, codeBlock);
            jit.m_bytecodeOffset = stubInfo->bytecodeIndex;
            jit.privateCompileGetByIdChain(stubInfo, structure, chain, count, ident, slot, cachedOffset, returnAddress, callFrame);
        }
        
        static void compilePutByIdTransition(VM* vm, CodeBlock* codeBlock, StructureStubInfo* stubInfo, Structure* oldStructure, Structure* newStructure, PropertyOffset cachedOffset, StructureChain* chain, ReturnAddressPtr returnAddress, bool direct)
        {
            JIT jit(vm, codeBlock);
            jit.m_bytecodeOffset = stubInfo->bytecodeIndex;
            jit.privateCompilePutByIdTransition(stubInfo, oldStructure, newStructure, cachedOffset, chain, returnAddress, direct);
        }
        
        static void compileGetByVal(VM* vm, CodeBlock* codeBlock, ByValInfo* byValInfo, ReturnAddressPtr returnAddress, JITArrayMode arrayMode)
        {
            JIT jit(vm, codeBlock);
            jit.m_bytecodeOffset = byValInfo->bytecodeIndex;
            jit.privateCompileGetByVal(byValInfo, returnAddress, arrayMode);
        }

        static void compilePutByVal(VM* vm, CodeBlock* codeBlock, ByValInfo* byValInfo, ReturnAddressPtr returnAddress, JITArrayMode arrayMode)
        {
            JIT jit(vm, codeBlock);
            jit.m_bytecodeOffset = byValInfo->bytecodeIndex;
            jit.privateCompilePutByVal(byValInfo, returnAddress, arrayMode);
        }

        static CodeRef compileCTINativeCall(VM* vm, NativeFunction func)
        {
            if (!vm->canUseJIT()) {
#if ENABLE(LLINT)
                return CodeRef::createLLIntCodeRef(llint_native_call_trampoline);
#else
                return CodeRef();
#endif
            }
            JIT jit(vm, 0);
            return jit.privateCompileCTINativeCall(vm, func);
        }

        static void resetPatchGetById(RepatchBuffer&, StructureStubInfo*);
        static void resetPatchPutById(RepatchBuffer&, StructureStubInfo*);
        static void patchGetByIdSelf(CodeBlock*, StructureStubInfo*, Structure*, PropertyOffset cachedOffset, ReturnAddressPtr);
        static void patchPutByIdReplace(CodeBlock*, StructureStubInfo*, Structure*, PropertyOffset cachedOffset, ReturnAddressPtr, bool direct);

        static void compilePatchGetArrayLength(VM* vm, CodeBlock* codeBlock, ReturnAddressPtr returnAddress)
        {
            JIT jit(vm, codeBlock);
#if ENABLE(DFG_JIT)
            // Force profiling to be enabled during stub generation.
            jit.m_canBeOptimized = true;
            jit.m_canBeOptimizedOrInlined = true;
            jit.m_shouldEmitProfiling = true;
#endif // ENABLE(DFG_JIT)
            return jit.privateCompilePatchGetArrayLength(returnAddress);
        }

        static void linkFor(JSFunction* callee, CodeBlock* callerCodeBlock, CodeBlock* calleeCodeBlock, CodePtr, CallLinkInfo*, VM*, CodeSpecializationKind);
        static void linkSlowCall(CodeBlock* callerCodeBlock, CallLinkInfo*);

    private:
        JIT(VM*, CodeBlock* = 0);

        void privateCompileMainPass();
        void privateCompileLinkPass();
        void privateCompileSlowCases();
        JITCode privateCompile(CodePtr* functionEntryArityCheck, JITCompilationEffort);
        
        void privateCompileClosureCall(CallLinkInfo*, CodeBlock* calleeCodeBlock, Structure*, ExecutableBase*, MacroAssemblerCodePtr);
        
        void privateCompileGetByIdProto(StructureStubInfo*, Structure*, Structure* prototypeStructure, const Identifier&, const PropertySlot&, PropertyOffset cachedOffset, ReturnAddressPtr, CallFrame*);
        void privateCompileGetByIdSelfList(StructureStubInfo*, PolymorphicAccessStructureList*, int, Structure*, const Identifier&, const PropertySlot&, PropertyOffset cachedOffset);
        void privateCompileGetByIdProtoList(StructureStubInfo*, PolymorphicAccessStructureList*, int, Structure*, Structure* prototypeStructure, const Identifier&, const PropertySlot&, PropertyOffset cachedOffset, CallFrame*);
        void privateCompileGetByIdChainList(StructureStubInfo*, PolymorphicAccessStructureList*, int, Structure*, StructureChain*, size_t count, const Identifier&, const PropertySlot&, PropertyOffset cachedOffset, CallFrame*);
        void privateCompileGetByIdChain(StructureStubInfo*, Structure*, StructureChain*, size_t count, const Identifier&, const PropertySlot&, PropertyOffset cachedOffset, ReturnAddressPtr, CallFrame*);
        void privateCompilePutByIdTransition(StructureStubInfo*, Structure*, Structure*, PropertyOffset cachedOffset, StructureChain*, ReturnAddressPtr, bool direct);
        
        void privateCompileGetByVal(ByValInfo*, ReturnAddressPtr, JITArrayMode);
        void privateCompilePutByVal(ByValInfo*, ReturnAddressPtr, JITArrayMode);

        Label privateCompileCTINativeCall(VM*, bool isConstruct = false);
        CodeRef privateCompileCTINativeCall(VM*, NativeFunction);
        void privateCompilePatchGetArrayLength(ReturnAddressPtr returnAddress);

        static bool isDirectPutById(StructureStubInfo*);

        void addSlowCase(Jump);
        void addSlowCase(JumpList);
        void addSlowCase();
        void addJump(Jump, int);
        void emitJumpSlowToHot(Jump, int);

        void compileOpCall(OpcodeID, Instruction*, unsigned callLinkInfoIndex);
        void compileOpCallSlowCase(OpcodeID, Instruction*, Vector<SlowCaseEntry>::iterator&, unsigned callLinkInfoIndex);
        void compileLoadVarargs(Instruction*);
        void compileCallEval();
        void compileCallEvalSlowCase(Vector<SlowCaseEntry>::iterator&);

        enum CompileOpStrictEqType { OpStrictEq, OpNStrictEq };
        void compileOpStrictEq(Instruction* instruction, CompileOpStrictEqType type);
        bool isOperandConstantImmediateDouble(unsigned src);
        
        void emitLoadDouble(int index, FPRegisterID value);
        void emitLoadInt32ToDouble(int index, FPRegisterID value);
        Jump emitJumpIfNotObject(RegisterID structureReg);

        Jump addStructureTransitionCheck(JSCell*, Structure*, StructureStubInfo*, RegisterID scratch);
        void addStructureTransitionCheck(JSCell*, Structure*, StructureStubInfo*, JumpList& failureCases, RegisterID scratch);
        void testPrototype(JSValue, JumpList& failureCases, StructureStubInfo*);

        enum WriteBarrierMode { UnconditionalWriteBarrier, ShouldFilterImmediates };
        // value register in write barrier is used before any scratch registers
        // so may safely be the same as either of the scratch registers.
        void emitWriteBarrier(RegisterID owner, RegisterID valueTag, RegisterID scratch, RegisterID scratch2, WriteBarrierMode, WriteBarrierUseKind);
        void emitWriteBarrier(JSCell* owner, RegisterID value, RegisterID scratch, WriteBarrierMode, WriteBarrierUseKind);

        template<typename StructureType> // StructureType can be RegisterID or ImmPtr.
        void emitAllocateJSObject(RegisterID allocator, StructureType, RegisterID result, RegisterID scratch);
        
#if ENABLE(VALUE_PROFILER)
        // This assumes that the value to profile is in regT0 and that regT3 is available for
        // scratch.
        void emitValueProfilingSite(ValueProfile*);
        void emitValueProfilingSite(unsigned bytecodeOffset);
        void emitValueProfilingSite();
#else
        void emitValueProfilingSite(unsigned) { }
        void emitValueProfilingSite() { }
#endif
        void emitArrayProfilingSite(RegisterID structureAndIndexingType, RegisterID scratch, ArrayProfile*);
        void emitArrayProfilingSiteForBytecodeIndex(RegisterID structureAndIndexingType, RegisterID scratch, unsigned bytecodeIndex);
        void emitArrayProfileStoreToHoleSpecialCase(ArrayProfile*);
        void emitArrayProfileOutOfBoundsSpecialCase(ArrayProfile*);
        
        JITArrayMode chooseArrayMode(ArrayProfile*);
        
        // Property is in regT1, base is in regT0. regT2 contains indexing type.
        // Property is int-checked and zero extended. Base is cell checked.
        // Structure is already profiled. Returns the slow cases. Fall-through
        // case contains result in regT0, and it is not yet profiled.
        JumpList emitInt32GetByVal(Instruction* instruction, PatchableJump& badType) { return emitContiguousGetByVal(instruction, badType, Int32Shape); }
        JumpList emitDoubleGetByVal(Instruction*, PatchableJump& badType);
        JumpList emitContiguousGetByVal(Instruction*, PatchableJump& badType, IndexingType expectedShape = ContiguousShape);
        JumpList emitArrayStorageGetByVal(Instruction*, PatchableJump& badType);
        JumpList emitIntTypedArrayGetByVal(Instruction*, PatchableJump& badType, const TypedArrayDescriptor&, size_t elementSize, TypedArraySignedness);
        JumpList emitFloatTypedArrayGetByVal(Instruction*, PatchableJump& badType, const TypedArrayDescriptor&, size_t elementSize);
        
        // Property is in regT0, base is in regT0. regT2 contains indecing type.
        // The value to store is not yet loaded. Property is int-checked and
        // zero-extended. Base is cell checked. Structure is already profiled.
        // returns the slow cases.
        JumpList emitInt32PutByVal(Instruction* currentInstruction, PatchableJump& badType)
        {
            return emitGenericContiguousPutByVal(currentInstruction, badType, Int32Shape);
        }
        JumpList emitDoublePutByVal(Instruction* currentInstruction, PatchableJump& badType)
        {
            return emitGenericContiguousPutByVal(currentInstruction, badType, DoubleShape);
        }
        JumpList emitContiguousPutByVal(Instruction* currentInstruction, PatchableJump& badType)
        {
            return emitGenericContiguousPutByVal(currentInstruction, badType);
        }
        JumpList emitGenericContiguousPutByVal(Instruction*, PatchableJump& badType, IndexingType indexingShape = ContiguousShape);
        JumpList emitArrayStoragePutByVal(Instruction*, PatchableJump& badType);
        JumpList emitIntTypedArrayPutByVal(Instruction*, PatchableJump& badType, const TypedArrayDescriptor&, size_t elementSize, TypedArraySignedness, TypedArrayRounding);
        JumpList emitFloatTypedArrayPutByVal(Instruction*, PatchableJump& badType, const TypedArrayDescriptor&, size_t elementSize);
        
        enum FinalObjectMode { MayBeFinal, KnownNotFinal };

#if USE(JSVALUE32_64)
        bool getOperandConstantImmediateInt(unsigned op1, unsigned op2, unsigned& op, int32_t& constant);

        void emitLoadTag(int index, RegisterID tag);
        void emitLoadPayload(int index, RegisterID payload);

        void emitLoad(const JSValue& v, RegisterID tag, RegisterID payload);
        void emitLoad(int index, RegisterID tag, RegisterID payload, RegisterID base = callFrameRegister);
        void emitLoad2(int index1, RegisterID tag1, RegisterID payload1, int index2, RegisterID tag2, RegisterID payload2);

        void emitStore(int index, RegisterID tag, RegisterID payload, RegisterID base = callFrameRegister);
        void emitStore(int index, const JSValue constant, RegisterID base = callFrameRegister);
        void emitStoreInt32(int index, RegisterID payload, bool indexIsInt32 = false);
        void emitStoreInt32(int index, TrustedImm32 payload, bool indexIsInt32 = false);
        void emitStoreAndMapInt32(int index, RegisterID tag, RegisterID payload, bool indexIsInt32, size_t opcodeLength);
        void emitStoreCell(int index, RegisterID payload, bool indexIsCell = false);
        void emitStoreBool(int index, RegisterID payload, bool indexIsBool = false);
        void emitStoreDouble(int index, FPRegisterID value);

        bool isLabeled(unsigned bytecodeOffset);
        void map(unsigned bytecodeOffset, int virtualRegisterIndex, RegisterID tag, RegisterID payload);
        void unmap(RegisterID);
        void unmap();
        bool isMapped(int virtualRegisterIndex);
        bool getMappedPayload(int virtualRegisterIndex, RegisterID& payload);
        bool getMappedTag(int virtualRegisterIndex, RegisterID& tag);
        
        void emitJumpSlowCaseIfNotJSCell(int virtualRegisterIndex);
        void emitJumpSlowCaseIfNotJSCell(int virtualRegisterIndex, RegisterID tag);

        void compileGetByIdHotPath(Identifier*);
        void compileGetByIdSlowCase(int resultVReg, int baseVReg, Identifier*, Vector<SlowCaseEntry>::iterator&);
        void compileGetDirectOffset(RegisterID base, RegisterID resultTag, RegisterID resultPayload, PropertyOffset cachedOffset);
        void compileGetDirectOffset(JSObject* base, RegisterID resultTag, RegisterID resultPayload, PropertyOffset cachedOffset);
        void compileGetDirectOffset(RegisterID base, RegisterID resultTag, RegisterID resultPayload, RegisterID offset, FinalObjectMode = MayBeFinal);
        void compilePutDirectOffset(RegisterID base, RegisterID valueTag, RegisterID valuePayload, PropertyOffset cachedOffset);

        // Arithmetic opcode helpers
        void emitAdd32Constant(unsigned dst, unsigned op, int32_t constant, ResultType opType);
        void emitSub32Constant(unsigned dst, unsigned op, int32_t constant, ResultType opType);
        void emitBinaryDoubleOp(OpcodeID, unsigned dst, unsigned op1, unsigned op2, OperandTypes, JumpList& notInt32Op1, JumpList& notInt32Op2, bool op1IsInRegisters = true, bool op2IsInRegisters = true);

#if CPU(ARM_TRADITIONAL)
        // sequenceOpCall
        static const int sequenceOpCallInstructionSpace = 12;
        static const int sequenceOpCallConstantSpace = 2;
        // sequenceGetByIdHotPath
        static const int sequenceGetByIdHotPathInstructionSpace = 36;
        static const int sequenceGetByIdHotPathConstantSpace = 4;
        // sequenceGetByIdSlowCase
        static const int sequenceGetByIdSlowCaseInstructionSpace = 80;
        static const int sequenceGetByIdSlowCaseConstantSpace = 4;
        // sequencePutById
        static const int sequencePutByIdInstructionSpace = 36;
        static const int sequencePutByIdConstantSpace = 4;
#elif CPU(SH4)
        // sequenceOpCall
        static const int sequenceOpCallInstructionSpace = 12;
        static const int sequenceOpCallConstantSpace = 2;
        // sequenceGetByIdHotPath
        static const int sequenceGetByIdHotPathInstructionSpace = 36;
        static const int sequenceGetByIdHotPathConstantSpace = 5;
        // sequenceGetByIdSlowCase
        static const int sequenceGetByIdSlowCaseInstructionSpace = 38;
        static const int sequenceGetByIdSlowCaseConstantSpace = 4;
        // sequencePutById
        static const int sequencePutByIdInstructionSpace = 36;
        static const int sequencePutByIdConstantSpace = 5;
#endif

#else // USE(JSVALUE32_64)
        /* This function is deprecated. */
        void emitGetJITStubArg(unsigned argumentNumber, RegisterID dst);

        void emitGetVirtualRegister(int src, RegisterID dst);
        void emitGetVirtualRegisters(int src1, RegisterID dst1, int src2, RegisterID dst2);
        void emitPutVirtualRegister(unsigned dst, RegisterID from = regT0);
        void emitStoreCell(unsigned dst, RegisterID payload, bool /* only used in JSValue32_64 */ = false)
        {
            emitPutVirtualRegister(dst, payload);
        }

        int32_t getConstantOperandImmediateInt(unsigned src);

        void killLastResultRegister();

        Jump emitJumpIfJSCell(RegisterID);
        Jump emitJumpIfBothJSCells(RegisterID, RegisterID, RegisterID);
        void emitJumpSlowCaseIfJSCell(RegisterID);
        void emitJumpSlowCaseIfNotJSCell(RegisterID);
        void emitJumpSlowCaseIfNotJSCell(RegisterID, int VReg);
        Jump emitJumpIfImmediateInteger(RegisterID);
        Jump emitJumpIfNotImmediateInteger(RegisterID);
        Jump emitJumpIfNotImmediateIntegers(RegisterID, RegisterID, RegisterID);
        void emitJumpSlowCaseIfNotImmediateInteger(RegisterID);
        void emitJumpSlowCaseIfNotImmediateNumber(RegisterID);
        void emitJumpSlowCaseIfNotImmediateIntegers(RegisterID, RegisterID, RegisterID);

        void emitFastArithReTagImmediate(RegisterID src, RegisterID dest);

        void emitTagAsBoolImmediate(RegisterID reg);
        void compileBinaryArithOp(OpcodeID, unsigned dst, unsigned src1, unsigned src2, OperandTypes opi);
        void compileBinaryArithOpSlowCase(OpcodeID, Vector<SlowCaseEntry>::iterator&, unsigned dst, unsigned src1, unsigned src2, OperandTypes, bool op1HasImmediateIntFastCase, bool op2HasImmediateIntFastCase);

        void compileGetByIdHotPath(int baseVReg, Identifier*);
        void compileGetByIdSlowCase(int resultVReg, int baseVReg, Identifier*, Vector<SlowCaseEntry>::iterator&);
        void compileGetDirectOffset(RegisterID base, RegisterID result, PropertyOffset cachedOffset);
        void compileGetDirectOffset(JSObject* base, RegisterID result, PropertyOffset cachedOffset);
        void compileGetDirectOffset(RegisterID base, RegisterID result, RegisterID offset, RegisterID scratch, FinalObjectMode = MayBeFinal);
        void compilePutDirectOffset(RegisterID base, RegisterID value, PropertyOffset cachedOffset);

#endif // USE(JSVALUE32_64)

#if (defined(ASSEMBLER_HAS_CONSTANT_POOL) && ASSEMBLER_HAS_CONSTANT_POOL)
#define BEGIN_UNINTERRUPTED_SEQUENCE(name) do { beginUninterruptedSequence(name ## InstructionSpace, name ## ConstantSpace); } while (false)
#define END_UNINTERRUPTED_SEQUENCE_FOR_PUT(name, dst) do { endUninterruptedSequence(name ## InstructionSpace, name ## ConstantSpace, dst); } while (false)
#define END_UNINTERRUPTED_SEQUENCE(name) END_UNINTERRUPTED_SEQUENCE_FOR_PUT(name, 0)

        void beginUninterruptedSequence(int, int);
        void endUninterruptedSequence(int, int, int);

#else
#define BEGIN_UNINTERRUPTED_SEQUENCE(name)
#define END_UNINTERRUPTED_SEQUENCE(name)
#define END_UNINTERRUPTED_SEQUENCE_FOR_PUT(name, dst)
#endif

        void emit_compareAndJump(OpcodeID, unsigned op1, unsigned op2, unsigned target, RelationalCondition);
        void emit_compareAndJumpSlow(unsigned op1, unsigned op2, unsigned target, DoubleCondition, int (JIT_STUB *stub)(STUB_ARGS_DECLARATION), bool invert, Vector<SlowCaseEntry>::iterator&);

        void emit_op_add(Instruction*);
        void emit_op_bitand(Instruction*);
        void emit_op_bitor(Instruction*);
        void emit_op_bitxor(Instruction*);
        void emit_op_call(Instruction*);
        void emit_op_call_eval(Instruction*);
        void emit_op_call_varargs(Instruction*);
        void emit_op_call_put_result(Instruction*);
        void emit_op_catch(Instruction*);
        void emit_op_construct(Instruction*);
        void emit_op_get_callee(Instruction*);
        void emit_op_create_this(Instruction*);
        void emit_op_convert_this(Instruction*);
        void emit_op_create_arguments(Instruction*);
        void emit_op_debug(Instruction*);
        void emit_op_del_by_id(Instruction*);
        void emit_op_div(Instruction*);
        void emit_op_end(Instruction*);
        void emit_op_enter(Instruction*);
        void emit_op_create_activation(Instruction*);
        void emit_op_eq(Instruction*);
        void emit_op_eq_null(Instruction*);
        void emit_op_get_by_id(Instruction*);
        void emit_op_get_arguments_length(Instruction*);
        void emit_op_get_by_val(Instruction*);
        void emit_op_get_argument_by_val(Instruction*);
        void emit_op_get_by_pname(Instruction*);
        void emit_op_init_lazy_reg(Instruction*);
        void emit_op_check_has_instance(Instruction*);
        void emit_op_instanceof(Instruction*);
        void emit_op_is_undefined(Instruction*);
        void emit_op_is_boolean(Instruction*);
        void emit_op_is_number(Instruction*);
        void emit_op_is_string(Instruction*);
        void emit_op_jeq_null(Instruction*);
        void emit_op_jfalse(Instruction*);
        void emit_op_jmp(Instruction*);
        void emit_op_jneq_null(Instruction*);
        void emit_op_jneq_ptr(Instruction*);
        void emit_op_jless(Instruction*);
        void emit_op_jlesseq(Instruction*);
        void emit_op_jgreater(Instruction*);
        void emit_op_jgreatereq(Instruction*);
        void emit_op_jnless(Instruction*);
        void emit_op_jnlesseq(Instruction*);
        void emit_op_jngreater(Instruction*);
        void emit_op_jngreatereq(Instruction*);
        void emit_op_jtrue(Instruction*);
        void emit_op_loop_hint(Instruction*);
        void emit_op_lshift(Instruction*);
        void emit_op_mod(Instruction*);
        void emit_op_mov(Instruction*);
        void emit_op_mul(Instruction*);
        void emit_op_negate(Instruction*);
        void emit_op_neq(Instruction*);
        void emit_op_neq_null(Instruction*);
        void emit_op_new_array(Instruction*);
        void emit_op_new_array_with_size(Instruction*);
        void emit_op_new_array_buffer(Instruction*);
        void emit_op_new_func(Instruction*);
        void emit_op_new_func_exp(Instruction*);
        void emit_op_new_object(Instruction*);
        void emit_op_new_regexp(Instruction*);
        void emit_op_get_pnames(Instruction*);
        void emit_op_next_pname(Instruction*);
        void emit_op_not(Instruction*);
        void emit_op_nstricteq(Instruction*);
        void emit_op_pop_scope(Instruction*);
        void emit_op_dec(Instruction*);
        void emit_op_inc(Instruction*);
        void emit_op_profile_did_call(Instruction*);
        void emit_op_profile_will_call(Instruction*);
        void emit_op_push_name_scope(Instruction*);
        void emit_op_push_with_scope(Instruction*);
        void emit_op_put_by_id(Instruction*);
        void emit_op_put_by_index(Instruction*);
        void emit_op_put_by_val(Instruction*);
        void emit_op_put_getter_setter(Instruction*);
        void emit_op_init_global_const(Instruction*);
        void emit_op_init_global_const_check(Instruction*);
        void emit_resolve_operations(ResolveOperations*, const int* base, const int* value);
        void emitSlow_link_resolve_operations(ResolveOperations*, Vector<SlowCaseEntry>::iterator&);
        void emit_op_resolve(Instruction*);
        void emit_op_resolve_base(Instruction*);
        void emit_op_resolve_with_base(Instruction*);
        void emit_op_resolve_with_this(Instruction*);
        void emit_op_put_to_base(Instruction*);
        void emit_op_ret(Instruction*);
        void emit_op_ret_object_or_this(Instruction*);
        void emit_op_rshift(Instruction*);
        void emit_op_strcat(Instruction*);
        void emit_op_stricteq(Instruction*);
        void emit_op_sub(Instruction*);
        void emit_op_switch_char(Instruction*);
        void emit_op_switch_imm(Instruction*);
        void emit_op_switch_string(Instruction*);
        void emit_op_tear_off_activation(Instruction*);
        void emit_op_tear_off_arguments(Instruction*);
        void emit_op_throw(Instruction*);
        void emit_op_throw_static_error(Instruction*);
        void emit_op_to_number(Instruction*);
        void emit_op_to_primitive(Instruction*);
        void emit_op_unexpected_load(Instruction*);
        void emit_op_urshift(Instruction*);
        void emit_op_get_scoped_var(Instruction*);
        void emit_op_put_scoped_var(Instruction*);

        void emitSlow_op_add(Instruction*, Vector<SlowCaseEntry>::iterator&);
        void emitSlow_op_bitand(Instruction*, Vector<SlowCaseEntry>::iterator&);
        void emitSlow_op_bitor(Instruction*, Vector<SlowCaseEntry>::iterator&);
        void emitSlow_op_bitxor(Instruction*, Vector<SlowCaseEntry>::iterator&);
        void emitSlow_op_call(Instruction*, Vector<SlowCaseEntry>::iterator&);
        void emitSlow_op_call_eval(Instruction*, Vector<SlowCaseEntry>::iterator&);
        void emitSlow_op_call_varargs(Instruction*, Vector<SlowCaseEntry>::iterator&);
        void emitSlow_op_construct(Instruction*, Vector<SlowCaseEntry>::iterator&);
        void emitSlow_op_convert_this(Instruction*, Vector<SlowCaseEntry>::iterator&);
        void emitSlow_op_create_this(Instruction*, Vector<SlowCaseEntry>::iterator&);
        void emitSlow_op_div(Instruction*, Vector<SlowCaseEntry>::iterator&);
        void emitSlow_op_eq(Instruction*, Vector<SlowCaseEntry>::iterator&);
        void emitSlow_op_get_by_id(Instruction*, Vector<SlowCaseEntry>::iterator&);
        void emitSlow_op_get_arguments_length(Instruction*, Vector<SlowCaseEntry>::iterator&);
        void emitSlow_op_get_by_val(Instruction*, Vector<SlowCaseEntry>::iterator&);
        void emitSlow_op_get_argument_by_val(Instruction*, Vector<SlowCaseEntry>::iterator&);
        void emitSlow_op_get_by_pname(Instruction*, Vector<SlowCaseEntry>::iterator&);
        void emitSlow_op_check_has_instance(Instruction*, Vector<SlowCaseEntry>::iterator&);
        void emitSlow_op_instanceof(Instruction*, Vector<SlowCaseEntry>::iterator&);
        void emitSlow_op_jfalse(Instruction*, Vector<SlowCaseEntry>::iterator&);
        void emitSlow_op_jless(Instruction*, Vector<SlowCaseEntry>::iterator&);
        void emitSlow_op_jlesseq(Instruction*, Vector<SlowCaseEntry>::iterator&);
        void emitSlow_op_jgreater(Instruction*, Vector<SlowCaseEntry>::iterator&);
        void emitSlow_op_jgreatereq(Instruction*, Vector<SlowCaseEntry>::iterator&);
        void emitSlow_op_jnless(Instruction*, Vector<SlowCaseEntry>::iterator&);
        void emitSlow_op_jnlesseq(Instruction*, Vector<SlowCaseEntry>::iterator&);
        void emitSlow_op_jngreater(Instruction*, Vector<SlowCaseEntry>::iterator&);
        void emitSlow_op_jngreatereq(Instruction*, Vector<SlowCaseEntry>::iterator&);
        void emitSlow_op_jtrue(Instruction*, Vector<SlowCaseEntry>::iterator&);
        void emitSlow_op_loop_hint(Instruction*, Vector<SlowCaseEntry>::iterator&);
        void emitSlow_op_lshift(Instruction*, Vector<SlowCaseEntry>::iterator&);
        void emitSlow_op_mod(Instruction*, Vector<SlowCaseEntry>::iterator&);
        void emitSlow_op_mul(Instruction*, Vector<SlowCaseEntry>::iterator&);
        void emitSlow_op_negate(Instruction*, Vector<SlowCaseEntry>::iterator&);
        void emitSlow_op_neq(Instruction*, Vector<SlowCaseEntry>::iterator&);
        void emitSlow_op_new_object(Instruction*, Vector<SlowCaseEntry>::iterator&);
        void emitSlow_op_not(Instruction*, Vector<SlowCaseEntry>::iterator&);
        void emitSlow_op_nstricteq(Instruction*, Vector<SlowCaseEntry>::iterator&);
        void emitSlow_op_dec(Instruction*, Vector<SlowCaseEntry>::iterator&);
        void emitSlow_op_inc(Instruction*, Vector<SlowCaseEntry>::iterator&);
        void emitSlow_op_put_by_id(Instruction*, Vector<SlowCaseEntry>::iterator&);
        void emitSlow_op_put_by_val(Instruction*, Vector<SlowCaseEntry>::iterator&);
        void emitSlow_op_init_global_const_check(Instruction*, Vector<SlowCaseEntry>::iterator&);
        void emitSlow_op_rshift(Instruction*, Vector<SlowCaseEntry>::iterator&);
        void emitSlow_op_stricteq(Instruction*, Vector<SlowCaseEntry>::iterator&);
        void emitSlow_op_sub(Instruction*, Vector<SlowCaseEntry>::iterator&);
        void emitSlow_op_to_number(Instruction*, Vector<SlowCaseEntry>::iterator&);
        void emitSlow_op_to_primitive(Instruction*, Vector<SlowCaseEntry>::iterator&);
        void emitSlow_op_urshift(Instruction*, Vector<SlowCaseEntry>::iterator&);

        void emitSlow_op_resolve(Instruction*, Vector<SlowCaseEntry>::iterator&);
        void emitSlow_op_resolve_base(Instruction*, Vector<SlowCaseEntry>::iterator&);
        void emitSlow_op_resolve_with_base(Instruction*, Vector<SlowCaseEntry>::iterator&);
        void emitSlow_op_resolve_with_this(Instruction*, Vector<SlowCaseEntry>::iterator&);
        void emitSlow_op_put_to_base(Instruction*, Vector<SlowCaseEntry>::iterator&);

        void emitRightShift(Instruction*, bool isUnsigned);
        void emitRightShiftSlowCase(Instruction*, Vector<SlowCaseEntry>::iterator&, bool isUnsigned);

        void emitInitRegister(unsigned dst);

        void emitPutIntToCallFrameHeader(RegisterID from, JSStack::CallFrameHeaderEntry);
        void emitGetFromCallFrameHeaderPtr(JSStack::CallFrameHeaderEntry, RegisterID to, RegisterID from = callFrameRegister);
        void emitGetFromCallFrameHeader32(JSStack::CallFrameHeaderEntry, RegisterID to, RegisterID from = callFrameRegister);
#if USE(JSVALUE64)
        void emitGetFromCallFrameHeader64(JSStack::CallFrameHeaderEntry, RegisterID to, RegisterID from = callFrameRegister);
#endif

        JSValue getConstantOperand(unsigned src);
        bool isOperandConstantImmediateInt(unsigned src);
        bool isOperandConstantImmediateChar(unsigned src);

        bool atJumpTarget();

        Jump getSlowCase(Vector<SlowCaseEntry>::iterator& iter)
        {
            return iter++->from;
        }
        void linkSlowCase(Vector<SlowCaseEntry>::iterator& iter)
        {
            iter->from.link(this);
            ++iter;
        }
        void linkDummySlowCase(Vector<SlowCaseEntry>::iterator& iter)
        {
            ASSERT(!iter->from.isSet());
            ++iter;
        }
        void linkSlowCaseIfNotJSCell(Vector<SlowCaseEntry>::iterator&, int virtualRegisterIndex);

        Jump checkStructure(RegisterID reg, Structure* structure);

        void restoreArgumentReferenceForTrampoline();
        void updateTopCallFrame();

        Call emitNakedCall(CodePtr function = CodePtr());

        // Loads the character value of a single character string into dst.
        void emitLoadCharacterString(RegisterID src, RegisterID dst, JumpList& failures);
        
#if ENABLE(DFG_JIT)
        void emitEnterOptimizationCheck();
#else
        void emitEnterOptimizationCheck() { }
#endif

#ifndef NDEBUG
        void printBytecodeOperandTypes(unsigned src1, unsigned src2);
#endif

#if ENABLE(SAMPLING_FLAGS)
        void setSamplingFlag(int32_t);
        void clearSamplingFlag(int32_t);
#endif

#if ENABLE(SAMPLING_COUNTERS)
        void emitCount(AbstractSamplingCounter&, int32_t = 1);
#endif

#if ENABLE(OPCODE_SAMPLING)
        void sampleInstruction(Instruction*, bool = false);
#endif

#if ENABLE(CODEBLOCK_SAMPLING)
        void sampleCodeBlock(CodeBlock*);
#else
        void sampleCodeBlock(CodeBlock*) {}
#endif

#if ENABLE(DFG_JIT)
        bool canBeOptimized() { return m_canBeOptimized; }
        bool canBeOptimizedOrInlined() { return m_canBeOptimizedOrInlined; }
        bool shouldEmitProfiling() { return m_shouldEmitProfiling; }
#else
        bool canBeOptimized() { return false; }
        bool canBeOptimizedOrInlined() { return false; }
        // Enables use of value profiler with tiered compilation turned off,
        // in which case all code gets profiled.
        bool shouldEmitProfiling() { return false; }
#endif

        Interpreter* m_interpreter;
        VM* m_vm;
        CodeBlock* m_codeBlock;

        Vector<CallRecord> m_calls;
        Vector<Label> m_labels;
        Vector<PropertyStubCompilationInfo> m_propertyAccessCompilationInfo;
        Vector<ByValCompilationInfo> m_byValCompilationInfo;
        Vector<StructureStubCompilationInfo> m_callStructureStubCompilationInfo;
        Vector<JumpTable> m_jmpTable;

        unsigned m_bytecodeOffset;
        Vector<SlowCaseEntry> m_slowCases;
        Vector<SwitchRecord> m_switches;

        unsigned m_propertyAccessInstructionIndex;
        unsigned m_byValInstructionIndex;
        unsigned m_globalResolveInfoIndex;
        unsigned m_callLinkInfoIndex;

#if USE(JSVALUE32_64)
        unsigned m_jumpTargetIndex;
        unsigned m_mappedBytecodeOffset;
        int m_mappedVirtualRegisterIndex;
        RegisterID m_mappedTag;
        RegisterID m_mappedPayload;
#else
        int m_lastResultBytecodeRegister;
#endif
        unsigned m_jumpTargetsPosition;

#ifndef NDEBUG
#if defined(ASSEMBLER_HAS_CONSTANT_POOL) && ASSEMBLER_HAS_CONSTANT_POOL
        Label m_uninterruptedInstructionSequenceBegin;
        int m_uninterruptedConstantSequenceBegin;
#endif
#endif
        OwnPtr<JITDisassembler> m_disassembler;
        RefPtr<Profiler::Compilation> m_compilation;
        WeakRandom m_randomGenerator;
        static CodeRef stringGetByValStubGenerator(VM*);

#if ENABLE(VALUE_PROFILER)
        bool m_canBeOptimized;
        bool m_canBeOptimizedOrInlined;
        bool m_shouldEmitProfiling;
#endif
    } JIT_CLASS_ALIGNMENT;

} // namespace JSC

#endif // ENABLE(JIT)

#endif // JIT_h
