/*
 * Copyright (C) 2008 Apple Inc. All rights reserved.
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

// We've run into some problems where changing the size of the class JIT leads to
// performance fluctuations.  Try forcing alignment in an attempt to stabalize this.
#if COMPILER(GCC)
#define JIT_CLASS_ALIGNMENT __attribute__ ((aligned (32)))
#else
#define JIT_CLASS_ALIGNMENT
#endif

#define ASSERT_JIT_OFFSET_UNUSED(variable, actual, expected) ASSERT_WITH_MESSAGE_UNUSED(variable, actual == expected, "JIT Offset \"%s\" should be %d, not %d.\n", #expected, static_cast<int>(expected), static_cast<int>(actual));
#define ASSERT_JIT_OFFSET(actual, expected) ASSERT_WITH_MESSAGE(actual == expected, "JIT Offset \"%s\" should be %d, not %d.\n", #expected, static_cast<int>(expected), static_cast<int>(actual));

#include "CodeBlock.h"
#include "Interpreter.h"
#include "JSInterfaceJIT.h"
#include "Opcode.h"
#include "Profiler.h"
#include <bytecode/SamplingTool.h>

namespace JSC {

    class CodeBlock;
    class JIT;
    class JSPropertyNameIterator;
    class Interpreter;
    class Register;
    class RegisterFile;
    class ScopeChainNode;
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

    struct PropertyStubCompilationInfo {
        MacroAssembler::Call callReturnLocation;
        MacroAssembler::Label hotPathBegin;
    };

    struct StructureStubCompilationInfo {
        MacroAssembler::DataLabelPtr hotPathBegin;
        MacroAssembler::Call hotPathOther;
        MacroAssembler::Call callReturnLocation;
    };

    struct MethodCallCompilationInfo {
        MethodCallCompilationInfo(unsigned propertyAccessIndex)
            : propertyAccessIndex(propertyAccessIndex)
        {
        }

        MacroAssembler::DataLabelPtr structureToCompare;
        unsigned propertyAccessIndex;
    };

    // Near calls can only be patched to other JIT code, regular calls can be patched to JIT code or relinked to stub functions.
    void ctiPatchNearCallByReturnAddress(CodeBlock* codeblock, ReturnAddressPtr returnAddress, MacroAssemblerCodePtr newCalleeFunction);
    void ctiPatchCallByReturnAddress(CodeBlock* codeblock, ReturnAddressPtr returnAddress, MacroAssemblerCodePtr newCalleeFunction);
    void ctiPatchCallByReturnAddress(CodeBlock* codeblock, ReturnAddressPtr returnAddress, FunctionPtr newCalleeFunction);

    class JIT : private JSInterfaceJIT {
        friend class JITStubCall;

        using MacroAssembler::Jump;
        using MacroAssembler::JumpList;
        using MacroAssembler::Label;

        static const int patchGetByIdDefaultStructure = -1;
        // Magic number - initial offset cannot be representable as a signed 8bit value, or the X86Assembler
        // will compress the displacement, and we may not be able to fit a patched offset.
        static const int patchGetByIdDefaultOffset = 256;

    public:
        static JITCode compile(JSGlobalData* globalData, CodeBlock* codeBlock, CodePtr* functionEntryArityCheck = 0)
        {
            return JIT(globalData, codeBlock).privateCompile(functionEntryArityCheck);
        }

        static void compileGetByIdProto(JSGlobalData* globalData, CallFrame* callFrame, CodeBlock* codeBlock, StructureStubInfo* stubInfo, Structure* structure, Structure* prototypeStructure, const Identifier& ident, const PropertySlot& slot, size_t cachedOffset, ReturnAddressPtr returnAddress)
        {
            JIT jit(globalData, codeBlock);
            jit.privateCompileGetByIdProto(stubInfo, structure, prototypeStructure, ident, slot, cachedOffset, returnAddress, callFrame);
        }

        static void compileGetByIdSelfList(JSGlobalData* globalData, CodeBlock* codeBlock, StructureStubInfo* stubInfo, PolymorphicAccessStructureList* polymorphicStructures, int currentIndex, Structure* structure, const Identifier& ident, const PropertySlot& slot, size_t cachedOffset)
        {
            JIT jit(globalData, codeBlock);
            jit.privateCompileGetByIdSelfList(stubInfo, polymorphicStructures, currentIndex, structure, ident, slot, cachedOffset);
        }
        static void compileGetByIdProtoList(JSGlobalData* globalData, CallFrame* callFrame, CodeBlock* codeBlock, StructureStubInfo* stubInfo, PolymorphicAccessStructureList* prototypeStructureList, int currentIndex, Structure* structure, Structure* prototypeStructure, const Identifier& ident, const PropertySlot& slot, size_t cachedOffset)
        {
            JIT jit(globalData, codeBlock);
            jit.privateCompileGetByIdProtoList(stubInfo, prototypeStructureList, currentIndex, structure, prototypeStructure, ident, slot, cachedOffset, callFrame);
        }
        static void compileGetByIdChainList(JSGlobalData* globalData, CallFrame* callFrame, CodeBlock* codeBlock, StructureStubInfo* stubInfo, PolymorphicAccessStructureList* prototypeStructureList, int currentIndex, Structure* structure, StructureChain* chain, size_t count, const Identifier& ident, const PropertySlot& slot, size_t cachedOffset)
        {
            JIT jit(globalData, codeBlock);
            jit.privateCompileGetByIdChainList(stubInfo, prototypeStructureList, currentIndex, structure, chain, count, ident, slot, cachedOffset, callFrame);
        }

        static void compileGetByIdChain(JSGlobalData* globalData, CallFrame* callFrame, CodeBlock* codeBlock, StructureStubInfo* stubInfo, Structure* structure, StructureChain* chain, size_t count, const Identifier& ident, const PropertySlot& slot, size_t cachedOffset, ReturnAddressPtr returnAddress)
        {
            JIT jit(globalData, codeBlock);
            jit.privateCompileGetByIdChain(stubInfo, structure, chain, count, ident, slot, cachedOffset, returnAddress, callFrame);
        }
        
        static void compilePutByIdTransition(JSGlobalData* globalData, CodeBlock* codeBlock, StructureStubInfo* stubInfo, Structure* oldStructure, Structure* newStructure, size_t cachedOffset, StructureChain* chain, ReturnAddressPtr returnAddress, bool direct)
        {
            JIT jit(globalData, codeBlock);
            jit.privateCompilePutByIdTransition(stubInfo, oldStructure, newStructure, cachedOffset, chain, returnAddress, direct);
        }

        static void compileCTIMachineTrampolines(JSGlobalData* globalData, RefPtr<ExecutablePool>* executablePool, TrampolineStructure *trampolines)
        {
            if (!globalData->canUseJIT())
                return;
            JIT jit(globalData, 0);
            jit.privateCompileCTIMachineTrampolines(executablePool, globalData, trampolines);
        }

        static CodePtr compileCTINativeCall(JSGlobalData* globalData, PassRefPtr<ExecutablePool> executablePool, NativeFunction func)
        {
            if (!globalData->canUseJIT())
                return CodePtr();
            JIT jit(globalData, 0);
            return jit.privateCompileCTINativeCall(executablePool, globalData, func);
        }

        static void patchGetByIdSelf(CodeBlock* codeblock, StructureStubInfo*, Structure*, size_t cachedOffset, ReturnAddressPtr returnAddress);
        static void patchPutByIdReplace(CodeBlock* codeblock, StructureStubInfo*, Structure*, size_t cachedOffset, ReturnAddressPtr returnAddress, bool direct);
        static void patchMethodCallProto(JSGlobalData&, CodeBlock* codeblock, MethodCallLinkInfo&, JSFunction*, Structure*, JSObject*, ReturnAddressPtr);

        static void compilePatchGetArrayLength(JSGlobalData* globalData, CodeBlock* codeBlock, ReturnAddressPtr returnAddress)
        {
            JIT jit(globalData, codeBlock);
            return jit.privateCompilePatchGetArrayLength(returnAddress);
        }

        static void linkCall(JSFunction* callee, CodeBlock* callerCodeBlock, CodeBlock* calleeCodeBlock, CodePtr, CallLinkInfo*, int callerArgCount, JSGlobalData*);
        static void linkConstruct(JSFunction* callee, CodeBlock* callerCodeBlock, CodeBlock* calleeCodeBlock, CodePtr, CallLinkInfo*, int callerArgCount, JSGlobalData*);

    private:
        struct JSRInfo {
            DataLabelPtr storeLocation;
            Label target;

            JSRInfo(DataLabelPtr storeLocation, Label targetLocation)
                : storeLocation(storeLocation)
                , target(targetLocation)
            {
            }
        };

        JIT(JSGlobalData*, CodeBlock* = 0);

        void privateCompileMainPass();
        void privateCompileLinkPass();
        void privateCompileSlowCases();
        JITCode privateCompile(CodePtr* functionEntryArityCheck);
        void privateCompileGetByIdProto(StructureStubInfo*, Structure*, Structure* prototypeStructure, const Identifier&, const PropertySlot&, size_t cachedOffset, ReturnAddressPtr returnAddress, CallFrame* callFrame);
        void privateCompileGetByIdSelfList(StructureStubInfo*, PolymorphicAccessStructureList*, int, Structure*, const Identifier&, const PropertySlot&, size_t cachedOffset);
        void privateCompileGetByIdProtoList(StructureStubInfo*, PolymorphicAccessStructureList*, int, Structure*, Structure* prototypeStructure, const Identifier&, const PropertySlot&, size_t cachedOffset, CallFrame* callFrame);
        void privateCompileGetByIdChainList(StructureStubInfo*, PolymorphicAccessStructureList*, int, Structure*, StructureChain* chain, size_t count, const Identifier&, const PropertySlot&, size_t cachedOffset, CallFrame* callFrame);
        void privateCompileGetByIdChain(StructureStubInfo*, Structure*, StructureChain*, size_t count, const Identifier&, const PropertySlot&, size_t cachedOffset, ReturnAddressPtr returnAddress, CallFrame* callFrame);
        void privateCompilePutByIdTransition(StructureStubInfo*, Structure*, Structure*, size_t cachedOffset, StructureChain*, ReturnAddressPtr returnAddress, bool direct);

        void privateCompileCTIMachineTrampolines(RefPtr<ExecutablePool>* executablePool, JSGlobalData* data, TrampolineStructure *trampolines);
        Label privateCompileCTINativeCall(JSGlobalData*, bool isConstruct = false);
        CodePtr privateCompileCTINativeCall(PassRefPtr<ExecutablePool> executablePool, JSGlobalData* data, NativeFunction func);
        void privateCompilePatchGetArrayLength(ReturnAddressPtr returnAddress);

        void addSlowCase(Jump);
        void addSlowCase(JumpList);
        void addJump(Jump, int);
        void emitJumpSlowToHot(Jump, int);

        void compileOpCall(OpcodeID, Instruction* instruction, unsigned callLinkInfoIndex);
        void compileOpCallVarargs(Instruction* instruction);
        void compileOpCallInitializeCallFrame();
        void compileOpCallSlowCase(Instruction* instruction, Vector<SlowCaseEntry>::iterator& iter, unsigned callLinkInfoIndex, OpcodeID opcodeID);
        void compileOpCallVarargsSlowCase(Instruction* instruction, Vector<SlowCaseEntry>::iterator& iter);

        enum CompileOpStrictEqType { OpStrictEq, OpNStrictEq };
        void compileOpStrictEq(Instruction* instruction, CompileOpStrictEqType type);
        bool isOperandConstantImmediateDouble(unsigned src);
        
        void emitLoadDouble(unsigned index, FPRegisterID value);
        void emitLoadInt32ToDouble(unsigned index, FPRegisterID value);

        void testPrototype(JSValue, JumpList& failureCases);

#if USE(JSVALUE32_64)
        bool getOperandConstantImmediateInt(unsigned op1, unsigned op2, unsigned& op, int32_t& constant);

        void emitLoadTag(unsigned index, RegisterID tag);
        void emitLoadPayload(unsigned index, RegisterID payload);

        void emitLoad(const JSValue& v, RegisterID tag, RegisterID payload);
        void emitLoad(unsigned index, RegisterID tag, RegisterID payload, RegisterID base = callFrameRegister);
        void emitLoad2(unsigned index1, RegisterID tag1, RegisterID payload1, unsigned index2, RegisterID tag2, RegisterID payload2);

        void emitStore(unsigned index, RegisterID tag, RegisterID payload, RegisterID base = callFrameRegister);
        void emitStore(unsigned index, const JSValue constant, RegisterID base = callFrameRegister);
        void emitStoreInt32(unsigned index, RegisterID payload, bool indexIsInt32 = false);
        void emitStoreInt32(unsigned index, TrustedImm32 payload, bool indexIsInt32 = false);
        void emitStoreCell(unsigned index, RegisterID payload, bool indexIsCell = false);
        void emitStoreBool(unsigned index, RegisterID payload, bool indexIsBool = false);
        void emitStoreDouble(unsigned index, FPRegisterID value);

        bool isLabeled(unsigned bytecodeOffset);
        void map(unsigned bytecodeOffset, unsigned virtualRegisterIndex, RegisterID tag, RegisterID payload);
        void unmap(RegisterID);
        void unmap();
        bool isMapped(unsigned virtualRegisterIndex);
        bool getMappedPayload(unsigned virtualRegisterIndex, RegisterID& payload);
        bool getMappedTag(unsigned virtualRegisterIndex, RegisterID& tag);

        void emitJumpSlowCaseIfNotJSCell(unsigned virtualRegisterIndex);
        void emitJumpSlowCaseIfNotJSCell(unsigned virtualRegisterIndex, RegisterID tag);
        void linkSlowCaseIfNotJSCell(Vector<SlowCaseEntry>::iterator&, unsigned virtualRegisterIndex);

#if ENABLE(JIT_OPTIMIZE_PROPERTY_ACCESS)
        void compileGetByIdHotPath();
        void compileGetByIdSlowCase(int resultVReg, int baseVReg, Identifier* ident, Vector<SlowCaseEntry>::iterator& iter, bool isMethodCheck = false);
#endif
        void compileGetDirectOffset(RegisterID base, RegisterID resultTag, RegisterID resultPayload, Structure* structure, size_t cachedOffset);
        void compileGetDirectOffset(JSObject* base, RegisterID resultTag, RegisterID resultPayload, size_t cachedOffset);
        void compileGetDirectOffset(RegisterID base, RegisterID resultTag, RegisterID resultPayload, RegisterID offset);
        void compilePutDirectOffset(RegisterID base, RegisterID valueTag, RegisterID valuePayload, Structure* structure, size_t cachedOffset);

        // Arithmetic opcode helpers
        void emitAdd32Constant(unsigned dst, unsigned op, int32_t constant, ResultType opType);
        void emitSub32Constant(unsigned dst, unsigned op, int32_t constant, ResultType opType);
        void emitBinaryDoubleOp(OpcodeID, unsigned dst, unsigned op1, unsigned op2, OperandTypes, JumpList& notInt32Op1, JumpList& notInt32Op2, bool op1IsInRegisters = true, bool op2IsInRegisters = true);

#if CPU(X86)
        // These architecture specific value are used to enable patching - see comment on op_put_by_id.
        static const int patchOffsetPutByIdStructure = 7;
        static const int patchOffsetPutByIdPropertyMapOffset1 = 22;
        static const int patchOffsetPutByIdPropertyMapOffset2 = 28;
        // These architecture specific value are used to enable patching - see comment on op_get_by_id.
        static const int patchOffsetGetByIdStructure = 7;
        static const int patchOffsetGetByIdBranchToSlowCase = 13;
        static const int patchOffsetGetByIdPropertyMapOffset1 = 22;
        static const int patchOffsetGetByIdPropertyMapOffset2 = 28;
        static const int patchOffsetGetByIdPutResult = 28;
#if ENABLE(OPCODE_SAMPLING)
        static const int patchOffsetGetByIdSlowCaseCall = 37;
#else
        static const int patchOffsetGetByIdSlowCaseCall = 27;
#endif
        static const int patchOffsetOpCallCompareToJump = 6;

        static const int patchOffsetMethodCheckProtoObj = 11;
        static const int patchOffsetMethodCheckProtoStruct = 18;
        static const int patchOffsetMethodCheckPutFunction = 29;
#elif CPU(ARM_TRADITIONAL)
        // These architecture specific value are used to enable patching - see comment on op_put_by_id.
        static const int patchOffsetPutByIdStructure = 4;
        static const int patchOffsetPutByIdPropertyMapOffset1 = 20;
        static const int patchOffsetPutByIdPropertyMapOffset2 = 28;
        // These architecture specific value are used to enable patching - see comment on op_get_by_id.
        static const int patchOffsetGetByIdStructure = 4;
        static const int patchOffsetGetByIdBranchToSlowCase = 16;
        static const int patchOffsetGetByIdPropertyMapOffset1 = 20;
        static const int patchOffsetGetByIdPropertyMapOffset2 = 28;
        static const int patchOffsetGetByIdPutResult = 36;
#if ENABLE(OPCODE_SAMPLING)
        #error "OPCODE_SAMPLING is not yet supported"
#else
        static const int patchOffsetGetByIdSlowCaseCall = 32;
#endif
        static const int patchOffsetOpCallCompareToJump = 12;

        static const int patchOffsetMethodCheckProtoObj = 12;
        static const int patchOffsetMethodCheckProtoStruct = 20;
        static const int patchOffsetMethodCheckPutFunction = 32;

        // sequenceOpCall
        static const int sequenceOpCallInstructionSpace = 12;
        static const int sequenceOpCallConstantSpace = 2;
        // sequenceMethodCheck
        static const int sequenceMethodCheckInstructionSpace = 40;
        static const int sequenceMethodCheckConstantSpace = 6;
        // sequenceGetByIdHotPath
        static const int sequenceGetByIdHotPathInstructionSpace = 36;
        static const int sequenceGetByIdHotPathConstantSpace = 4;
        // sequenceGetByIdSlowCase
        static const int sequenceGetByIdSlowCaseInstructionSpace = 56;
        static const int sequenceGetByIdSlowCaseConstantSpace = 2;
        // sequencePutById
        static const int sequencePutByIdInstructionSpace = 36;
        static const int sequencePutByIdConstantSpace = 4;
#elif CPU(ARM_THUMB2)
        // These architecture specific value are used to enable patching - see comment on op_put_by_id.
        static const int patchOffsetPutByIdStructure = 10;
        static const int patchOffsetPutByIdPropertyMapOffset1 = 36;
        static const int patchOffsetPutByIdPropertyMapOffset2 = 48;
        // These architecture specific value are used to enable patching - see comment on op_get_by_id.
        static const int patchOffsetGetByIdStructure = 10;
        static const int patchOffsetGetByIdBranchToSlowCase = 26;
        static const int patchOffsetGetByIdPropertyMapOffset1 = 36;
        static const int patchOffsetGetByIdPropertyMapOffset2 = 48;
        static const int patchOffsetGetByIdPutResult = 52;
#if ENABLE(OPCODE_SAMPLING)
        #error "OPCODE_SAMPLING is not yet supported"
#else
        static const int patchOffsetGetByIdSlowCaseCall = 30;
#endif
        static const int patchOffsetOpCallCompareToJump = 16;

        static const int patchOffsetMethodCheckProtoObj = 24;
        static const int patchOffsetMethodCheckProtoStruct = 34;
        static const int patchOffsetMethodCheckPutFunction = 58;

        // sequenceOpCall
        static const int sequenceOpCallInstructionSpace = 12;
        static const int sequenceOpCallConstantSpace = 2;
        // sequenceMethodCheck
        static const int sequenceMethodCheckInstructionSpace = 40;
        static const int sequenceMethodCheckConstantSpace = 6;
        // sequenceGetByIdHotPath
        static const int sequenceGetByIdHotPathInstructionSpace = 36;
        static const int sequenceGetByIdHotPathConstantSpace = 4;
        // sequenceGetByIdSlowCase
        static const int sequenceGetByIdSlowCaseInstructionSpace = 40;
        static const int sequenceGetByIdSlowCaseConstantSpace = 2;
        // sequencePutById
        static const int sequencePutByIdInstructionSpace = 36;
        static const int sequencePutByIdConstantSpace = 4;
#elif CPU(MIPS)
#if WTF_MIPS_ISA(1)
        static const int patchOffsetPutByIdStructure = 16;
        static const int patchOffsetPutByIdPropertyMapOffset1 = 56;
        static const int patchOffsetPutByIdPropertyMapOffset2 = 72;
        static const int patchOffsetGetByIdStructure = 16;
        static const int patchOffsetGetByIdBranchToSlowCase = 48;
        static const int patchOffsetGetByIdPropertyMapOffset1 = 56;
        static const int patchOffsetGetByIdPropertyMapOffset2 = 76;
        static const int patchOffsetGetByIdPutResult = 96;
#if ENABLE(OPCODE_SAMPLING)
        #error "OPCODE_SAMPLING is not yet supported"
#else
        static const int patchOffsetGetByIdSlowCaseCall = 44;
#endif
        static const int patchOffsetOpCallCompareToJump = 32;
        static const int patchOffsetMethodCheckProtoObj = 32;
        static const int patchOffsetMethodCheckProtoStruct = 56;
        static const int patchOffsetMethodCheckPutFunction = 88;
#else // WTF_MIPS_ISA(1)
        static const int patchOffsetPutByIdStructure = 12;
        static const int patchOffsetPutByIdPropertyMapOffset1 = 48;
        static const int patchOffsetPutByIdPropertyMapOffset2 = 64;
        static const int patchOffsetGetByIdStructure = 12;
        static const int patchOffsetGetByIdBranchToSlowCase = 44;
        static const int patchOffsetGetByIdPropertyMapOffset1 = 48;
        static const int patchOffsetGetByIdPropertyMapOffset2 = 64;
        static const int patchOffsetGetByIdPutResult = 80;
#if ENABLE(OPCODE_SAMPLING)
        #error "OPCODE_SAMPLING is not yet supported"
#else
        static const int patchOffsetGetByIdSlowCaseCall = 44;
#endif
        static const int patchOffsetOpCallCompareToJump = 32;
        static const int patchOffsetMethodCheckProtoObj = 32;
        static const int patchOffsetMethodCheckProtoStruct = 52;
        static const int patchOffsetMethodCheckPutFunction = 84;
#endif
#elif CPU(SH4)
       // These architecture specific value are used to enable patching - see comment on op_put_by_id.
        static const int patchOffsetGetByIdStructure = 6;
        static const int patchOffsetPutByIdPropertyMapOffset = 24;
        static const int patchOffsetPutByIdStructure = 6;
        // These architecture specific value are used to enable patching - see comment on op_get_by_id.
        static const int patchOffsetGetByIdBranchToSlowCase = 10;
        static const int patchOffsetGetByIdPropertyMapOffset = 24;
        static const int patchOffsetGetByIdPutResult = 32;

        // sequenceOpCall
        static const int sequenceOpCallInstructionSpace = 12;
        static const int sequenceOpCallConstantSpace = 2;
        // sequenceMethodCheck
        static const int sequenceMethodCheckInstructionSpace = 40;
        static const int sequenceMethodCheckConstantSpace = 6;
        // sequenceGetByIdHotPath
        static const int sequenceGetByIdHotPathInstructionSpace = 36;
        static const int sequenceGetByIdHotPathConstantSpace = 5;
        // sequenceGetByIdSlowCase
        static const int sequenceGetByIdSlowCaseInstructionSpace = 26;
        static const int sequenceGetByIdSlowCaseConstantSpace = 2;
        // sequencePutById
        static const int sequencePutByIdInstructionSpace = 36;
        static const int sequencePutByIdConstantSpace = 5;

        static const int patchOffsetGetByIdPropertyMapOffset1 = 20;
        static const int patchOffsetGetByIdPropertyMapOffset2 = 26;

        static const int patchOffsetPutByIdPropertyMapOffset1 = 20;
        static const int patchOffsetPutByIdPropertyMapOffset2 = 26;

#if ENABLE(OPCODE_SAMPLING)
        static const int patchOffsetGetByIdSlowCaseCall = 0; // FIMXE
#else
        static const int patchOffsetGetByIdSlowCaseCall = 22;
#endif
        static const int patchOffsetOpCallCompareToJump = 4;

        static const int patchOffsetMethodCheckProtoObj = 12;
        static const int patchOffsetMethodCheckProtoStruct = 20;
        static const int patchOffsetMethodCheckPutFunction = 32;
#else
#error "JSVALUE32_64 not supported on this platform."
#endif

#else // USE(JSVALUE32_64)
        void emitGetVirtualRegister(int src, RegisterID dst);
        void emitGetVirtualRegisters(int src1, RegisterID dst1, int src2, RegisterID dst2);
        void emitPutVirtualRegister(unsigned dst, RegisterID from = regT0);

        int32_t getConstantOperandImmediateInt(unsigned src);

        void killLastResultRegister();

        Jump emitJumpIfJSCell(RegisterID);
        Jump emitJumpIfBothJSCells(RegisterID, RegisterID, RegisterID);
        void emitJumpSlowCaseIfJSCell(RegisterID);
        Jump emitJumpIfNotJSCell(RegisterID);
        void emitJumpSlowCaseIfNotJSCell(RegisterID);
        void emitJumpSlowCaseIfNotJSCell(RegisterID, int VReg);
#if USE(JSVALUE32_64)
        JIT::Jump emitJumpIfImmediateNumber(RegisterID reg)
        {
            return emitJumpIfImmediateInteger(reg);
        }
        
        JIT::Jump emitJumpIfNotImmediateNumber(RegisterID reg)
        {
            return emitJumpIfNotImmediateInteger(reg);
        }
#endif
        JIT::Jump emitJumpIfImmediateInteger(RegisterID);
        JIT::Jump emitJumpIfNotImmediateInteger(RegisterID);
        JIT::Jump emitJumpIfNotImmediateIntegers(RegisterID, RegisterID, RegisterID);
        void emitJumpSlowCaseIfNotImmediateInteger(RegisterID);
        void emitJumpSlowCaseIfNotImmediateNumber(RegisterID);
        void emitJumpSlowCaseIfNotImmediateIntegers(RegisterID, RegisterID, RegisterID);

#if USE(JSVALUE32_64)
        void emitFastArithDeTagImmediate(RegisterID);
        Jump emitFastArithDeTagImmediateJumpIfZero(RegisterID);
#endif
        void emitFastArithReTagImmediate(RegisterID src, RegisterID dest);
        void emitFastArithIntToImmNoCheck(RegisterID src, RegisterID dest);

        void emitTagAsBoolImmediate(RegisterID reg);
        void compileBinaryArithOp(OpcodeID, unsigned dst, unsigned src1, unsigned src2, OperandTypes opi);
#if USE(JSVALUE64)
        void compileBinaryArithOpSlowCase(OpcodeID, Vector<SlowCaseEntry>::iterator&, unsigned dst, unsigned src1, unsigned src2, OperandTypes, bool op1HasImmediateIntFastCase, bool op2HasImmediateIntFastCase);
#else
        void compileBinaryArithOpSlowCase(OpcodeID, Vector<SlowCaseEntry>::iterator&, unsigned dst, unsigned src1, unsigned src2, OperandTypes);
#endif

#if ENABLE(JIT_OPTIMIZE_PROPERTY_ACCESS)
        void compileGetByIdHotPath(int resultVReg, int baseVReg, Identifier* ident, unsigned propertyAccessInstructionIndex);
        void compileGetByIdSlowCase(int resultVReg, int baseVReg, Identifier* ident, Vector<SlowCaseEntry>::iterator& iter, bool isMethodCheck = false);
#endif
        void compileGetDirectOffset(RegisterID base, RegisterID result, Structure* structure, size_t cachedOffset);
        void compileGetDirectOffset(JSObject* base, RegisterID result, size_t cachedOffset);
        void compileGetDirectOffset(RegisterID base, RegisterID result, RegisterID offset, RegisterID scratch);
        void compilePutDirectOffset(RegisterID base, RegisterID value, Structure* structure, size_t cachedOffset);

#if CPU(X86_64)
        // These architecture specific value are used to enable patching - see comment on op_put_by_id.
        static const int patchOffsetPutByIdStructure = 10;
        static const int patchOffsetPutByIdPropertyMapOffset = 31;
        // These architecture specific value are used to enable patching - see comment on op_get_by_id.
        static const int patchOffsetGetByIdStructure = 10;
        static const int patchOffsetGetByIdBranchToSlowCase = 20;
        static const int patchOffsetGetByIdPropertyMapOffset = 31;
        static const int patchOffsetGetByIdPutResult = 31;
#if ENABLE(OPCODE_SAMPLING)
        static const int patchOffsetGetByIdSlowCaseCall = 64;
#else
        static const int patchOffsetGetByIdSlowCaseCall = 41;
#endif
        static const int patchOffsetOpCallCompareToJump = 9;

        static const int patchOffsetMethodCheckProtoObj = 20;
        static const int patchOffsetMethodCheckProtoStruct = 30;
        static const int patchOffsetMethodCheckPutFunction = 50;
#elif CPU(X86)
        // These architecture specific value are used to enable patching - see comment on op_put_by_id.
        static const int patchOffsetPutByIdStructure = 7;
        static const int patchOffsetPutByIdPropertyMapOffset = 22;
        // These architecture specific value are used to enable patching - see comment on op_get_by_id.
        static const int patchOffsetGetByIdStructure = 7;
        static const int patchOffsetGetByIdBranchToSlowCase = 13;
        static const int patchOffsetGetByIdPropertyMapOffset = 22;
        static const int patchOffsetGetByIdPutResult = 22;
#if ENABLE(OPCODE_SAMPLING)
        static const int patchOffsetGetByIdSlowCaseCall = 33;
#else
        static const int patchOffsetGetByIdSlowCaseCall = 23;
#endif
        static const int patchOffsetOpCallCompareToJump = 6;

        static const int patchOffsetMethodCheckProtoObj = 11;
        static const int patchOffsetMethodCheckProtoStruct = 18;
        static const int patchOffsetMethodCheckPutFunction = 29;
#elif CPU(ARM_THUMB2)
        // These architecture specific value are used to enable patching - see comment on op_put_by_id.
        static const int patchOffsetPutByIdStructure = 10;
        static const int patchOffsetPutByIdPropertyMapOffset = 46;
        // These architecture specific value are used to enable patching - see comment on op_get_by_id.
        static const int patchOffsetGetByIdStructure = 10;
        static const int patchOffsetGetByIdBranchToSlowCase = 26;
        static const int patchOffsetGetByIdPropertyMapOffset = 46;
        static const int patchOffsetGetByIdPutResult = 50;
#if ENABLE(OPCODE_SAMPLING)
        static const int patchOffsetGetByIdSlowCaseCall = 0; // FIMXE
#else
        static const int patchOffsetGetByIdSlowCaseCall = 28;
#endif
        static const int patchOffsetOpCallCompareToJump = 16;

        static const int patchOffsetMethodCheckProtoObj = 24;
        static const int patchOffsetMethodCheckProtoStruct = 34;
        static const int patchOffsetMethodCheckPutFunction = 58;
#elif CPU(ARM_TRADITIONAL)
        // These architecture specific value are used to enable patching - see comment on op_put_by_id.
        static const int patchOffsetPutByIdStructure = 4;
        static const int patchOffsetPutByIdPropertyMapOffset = 20;
        // These architecture specific value are used to enable patching - see comment on op_get_by_id.
        static const int patchOffsetGetByIdStructure = 4;
        static const int patchOffsetGetByIdBranchToSlowCase = 16;
        static const int patchOffsetGetByIdPropertyMapOffset = 20;
        static const int patchOffsetGetByIdPutResult = 28;
#if ENABLE(OPCODE_SAMPLING)
        #error "OPCODE_SAMPLING is not yet supported"
#else
        static const int patchOffsetGetByIdSlowCaseCall = 28;
#endif
        static const int patchOffsetOpCallCompareToJump = 12;

        static const int patchOffsetMethodCheckProtoObj = 12;
        static const int patchOffsetMethodCheckProtoStruct = 20;
        static const int patchOffsetMethodCheckPutFunction = 32;

        // sequenceOpCall
        static const int sequenceOpCallInstructionSpace = 12;
        static const int sequenceOpCallConstantSpace = 2;
        // sequenceMethodCheck
        static const int sequenceMethodCheckInstructionSpace = 40;
        static const int sequenceMethodCheckConstantSpace = 6;
        // sequenceGetByIdHotPath
        static const int sequenceGetByIdHotPathInstructionSpace = 28;
        static const int sequenceGetByIdHotPathConstantSpace = 3;
        // sequenceGetByIdSlowCase
        static const int sequenceGetByIdSlowCaseInstructionSpace = 32;
        static const int sequenceGetByIdSlowCaseConstantSpace = 2;
        // sequencePutById
        static const int sequencePutByIdInstructionSpace = 28;
        static const int sequencePutByIdConstantSpace = 3;
#elif CPU(MIPS)
#if WTF_MIPS_ISA(1)
        static const int patchOffsetPutByIdStructure = 16;
        static const int patchOffsetPutByIdPropertyMapOffset = 68;
        static const int patchOffsetGetByIdStructure = 16;
        static const int patchOffsetGetByIdBranchToSlowCase = 48;
        static const int patchOffsetGetByIdPropertyMapOffset = 68;
        static const int patchOffsetGetByIdPutResult = 88;
#if ENABLE(OPCODE_SAMPLING)
        #error "OPCODE_SAMPLING is not yet supported"
#else
        static const int patchOffsetGetByIdSlowCaseCall = 40;
#endif
        static const int patchOffsetOpCallCompareToJump = 32;
        static const int patchOffsetMethodCheckProtoObj = 32;
        static const int patchOffsetMethodCheckProtoStruct = 56;
        static const int patchOffsetMethodCheckPutFunction = 88;
#else // WTF_MIPS_ISA(1)
        static const int patchOffsetPutByIdStructure = 12;
        static const int patchOffsetPutByIdPropertyMapOffset = 60;
        static const int patchOffsetGetByIdStructure = 12;
        static const int patchOffsetGetByIdBranchToSlowCase = 44;
        static const int patchOffsetGetByIdPropertyMapOffset = 60;
        static const int patchOffsetGetByIdPutResult = 76;
#if ENABLE(OPCODE_SAMPLING)
        #error "OPCODE_SAMPLING is not yet supported"
#else
        static const int patchOffsetGetByIdSlowCaseCall = 40;
#endif
        static const int patchOffsetOpCallCompareToJump = 32;
        static const int patchOffsetMethodCheckProtoObj = 32;
        static const int patchOffsetMethodCheckProtoStruct = 52;
        static const int patchOffsetMethodCheckPutFunction = 84;
#endif
#endif
#endif // USE(JSVALUE32_64)

#if (defined(ASSEMBLER_HAS_CONSTANT_POOL) && ASSEMBLER_HAS_CONSTANT_POOL)
#define BEGIN_UNINTERRUPTED_SEQUENCE(name) do { beginUninterruptedSequence(name ## InstructionSpace, name ## ConstantSpace); } while (false)
#define END_UNINTERRUPTED_SEQUENCE_FOR_PUT(name, dst) do { endUninterruptedSequence(name ## InstructionSpace, name ## ConstantSpace, dst); } while (false)
#define END_UNINTERRUPTED_SEQUENCE(name) END_UNINTERRUPTED_SEQUENCE_FOR_PUT(name, 0)

        void beginUninterruptedSequence(int, int);
        void endUninterruptedSequence(int, int, int);

#else
#define BEGIN_UNINTERRUPTED_SEQUENCE(name)  do { beginUninterruptedSequence(); } while (false)
#define END_UNINTERRUPTED_SEQUENCE(name)  do { endUninterruptedSequence(); } while (false)
#define END_UNINTERRUPTED_SEQUENCE_FOR_PUT(name, dst) do { endUninterruptedSequence(); } while (false)
#endif

        void emit_op_add(Instruction*);
        void emit_op_bitand(Instruction*);
        void emit_op_bitnot(Instruction*);
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
        void emit_op_convert_this_strict(Instruction*);
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
        void emit_op_get_global_var(Instruction*);
        void emit_op_get_scoped_var(Instruction*);
        void emit_op_init_lazy_reg(Instruction*);
        void emit_op_check_has_instance(Instruction*);
        void emit_op_instanceof(Instruction*);
        void emit_op_jeq_null(Instruction*);
        void emit_op_jfalse(Instruction*);
        void emit_op_jmp(Instruction*);
        void emit_op_jmp_scopes(Instruction*);
        void emit_op_jneq_null(Instruction*);
        void emit_op_jneq_ptr(Instruction*);
        void emit_op_jnless(Instruction*);
        void emit_op_jless(Instruction*);
        void emit_op_jlesseq(Instruction*, bool invert = false);
        void emit_op_jnlesseq(Instruction*);
        void emit_op_jsr(Instruction*);
        void emit_op_jtrue(Instruction*);
        void emit_op_load_varargs(Instruction*);
        void emit_op_loop(Instruction*);
        void emit_op_loop_if_less(Instruction*);
        void emit_op_loop_if_lesseq(Instruction*);
        void emit_op_loop_if_true(Instruction*);
        void emit_op_loop_if_false(Instruction*);
        void emit_op_lshift(Instruction*);
        void emit_op_method_check(Instruction*);
        void emit_op_mod(Instruction*);
        void emit_op_mov(Instruction*);
        void emit_op_mul(Instruction*);
        void emit_op_negate(Instruction*);
        void emit_op_neq(Instruction*);
        void emit_op_neq_null(Instruction*);
        void emit_op_new_array(Instruction*);
        void emit_op_new_func(Instruction*);
        void emit_op_new_func_exp(Instruction*);
        void emit_op_new_object(Instruction*);
        void emit_op_new_regexp(Instruction*);
        void emit_op_get_pnames(Instruction*);
        void emit_op_next_pname(Instruction*);
        void emit_op_not(Instruction*);
        void emit_op_nstricteq(Instruction*);
        void emit_op_pop_scope(Instruction*);
        void emit_op_post_dec(Instruction*);
        void emit_op_post_inc(Instruction*);
        void emit_op_pre_dec(Instruction*);
        void emit_op_pre_inc(Instruction*);
        void emit_op_profile_did_call(Instruction*);
        void emit_op_profile_will_call(Instruction*);
        void emit_op_push_new_scope(Instruction*);
        void emit_op_push_scope(Instruction*);
        void emit_op_put_by_id(Instruction*);
        void emit_op_put_by_index(Instruction*);
        void emit_op_put_by_val(Instruction*);
        void emit_op_put_getter(Instruction*);
        void emit_op_put_global_var(Instruction*);
        void emit_op_put_scoped_var(Instruction*);
        void emit_op_put_setter(Instruction*);
        void emit_op_resolve(Instruction*);
        void emit_op_resolve_base(Instruction*);
        void emit_op_ensure_property_exists(Instruction*);
        void emit_op_resolve_global(Instruction*, bool dynamic = false);
        void emit_op_resolve_global_dynamic(Instruction*);
        void emit_op_resolve_skip(Instruction*);
        void emit_op_resolve_with_base(Instruction*);
        void emit_op_ret(Instruction*);
        void emit_op_ret_object_or_this(Instruction*);
        void emit_op_rshift(Instruction*);
        void emit_op_sret(Instruction*);
        void emit_op_strcat(Instruction*);
        void emit_op_stricteq(Instruction*);
        void emit_op_sub(Instruction*);
        void emit_op_switch_char(Instruction*);
        void emit_op_switch_imm(Instruction*);
        void emit_op_switch_string(Instruction*);
        void emit_op_tear_off_activation(Instruction*);
        void emit_op_tear_off_arguments(Instruction*);
        void emit_op_throw(Instruction*);
        void emit_op_throw_reference_error(Instruction*);
        void emit_op_to_jsnumber(Instruction*);
        void emit_op_to_primitive(Instruction*);
        void emit_op_unexpected_load(Instruction*);
        void emit_op_urshift(Instruction*);
#if ENABLE(JIT_USE_SOFT_MODULO)
        void softModulo();
#endif

        void emitSlow_op_add(Instruction*, Vector<SlowCaseEntry>::iterator&);
        void emitSlow_op_bitand(Instruction*, Vector<SlowCaseEntry>::iterator&);
        void emitSlow_op_bitnot(Instruction*, Vector<SlowCaseEntry>::iterator&);
        void emitSlow_op_bitor(Instruction*, Vector<SlowCaseEntry>::iterator&);
        void emitSlow_op_bitxor(Instruction*, Vector<SlowCaseEntry>::iterator&);
        void emitSlow_op_call(Instruction*, Vector<SlowCaseEntry>::iterator&);
        void emitSlow_op_call_eval(Instruction*, Vector<SlowCaseEntry>::iterator&);
        void emitSlow_op_call_varargs(Instruction*, Vector<SlowCaseEntry>::iterator&);
        void emitSlow_op_construct(Instruction*, Vector<SlowCaseEntry>::iterator&);
        void emitSlow_op_convert_this(Instruction*, Vector<SlowCaseEntry>::iterator&);
        void emitSlow_op_convert_this_strict(Instruction*, Vector<SlowCaseEntry>::iterator&);
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
        void emitSlow_op_jnless(Instruction*, Vector<SlowCaseEntry>::iterator&);
        void emitSlow_op_jless(Instruction*, Vector<SlowCaseEntry>::iterator&);
        void emitSlow_op_jlesseq(Instruction*, Vector<SlowCaseEntry>::iterator&, bool invert = false);
        void emitSlow_op_jnlesseq(Instruction*, Vector<SlowCaseEntry>::iterator&);
        void emitSlow_op_jtrue(Instruction*, Vector<SlowCaseEntry>::iterator&);
        void emitSlow_op_load_varargs(Instruction*, Vector<SlowCaseEntry>::iterator&);
        void emitSlow_op_loop_if_less(Instruction*, Vector<SlowCaseEntry>::iterator&);
        void emitSlow_op_loop_if_lesseq(Instruction*, Vector<SlowCaseEntry>::iterator&);
        void emitSlow_op_loop_if_true(Instruction*, Vector<SlowCaseEntry>::iterator&);
        void emitSlow_op_loop_if_false(Instruction*, Vector<SlowCaseEntry>::iterator&);
        void emitSlow_op_lshift(Instruction*, Vector<SlowCaseEntry>::iterator&);
        void emitSlow_op_method_check(Instruction*, Vector<SlowCaseEntry>::iterator&);
        void emitSlow_op_mod(Instruction*, Vector<SlowCaseEntry>::iterator&);
        void emitSlow_op_mul(Instruction*, Vector<SlowCaseEntry>::iterator&);
        void emitSlow_op_negate(Instruction*, Vector<SlowCaseEntry>::iterator&);
        void emitSlow_op_neq(Instruction*, Vector<SlowCaseEntry>::iterator&);
        void emitSlow_op_not(Instruction*, Vector<SlowCaseEntry>::iterator&);
        void emitSlow_op_nstricteq(Instruction*, Vector<SlowCaseEntry>::iterator&);
        void emitSlow_op_post_dec(Instruction*, Vector<SlowCaseEntry>::iterator&);
        void emitSlow_op_post_inc(Instruction*, Vector<SlowCaseEntry>::iterator&);
        void emitSlow_op_pre_dec(Instruction*, Vector<SlowCaseEntry>::iterator&);
        void emitSlow_op_pre_inc(Instruction*, Vector<SlowCaseEntry>::iterator&);
        void emitSlow_op_put_by_id(Instruction*, Vector<SlowCaseEntry>::iterator&);
        void emitSlow_op_put_by_val(Instruction*, Vector<SlowCaseEntry>::iterator&);
        void emitSlow_op_resolve_global(Instruction*, Vector<SlowCaseEntry>::iterator&);
        void emitSlow_op_resolve_global_dynamic(Instruction*, Vector<SlowCaseEntry>::iterator&);
        void emitSlow_op_rshift(Instruction*, Vector<SlowCaseEntry>::iterator&);
        void emitSlow_op_stricteq(Instruction*, Vector<SlowCaseEntry>::iterator&);
        void emitSlow_op_sub(Instruction*, Vector<SlowCaseEntry>::iterator&);
        void emitSlow_op_to_jsnumber(Instruction*, Vector<SlowCaseEntry>::iterator&);
        void emitSlow_op_to_primitive(Instruction*, Vector<SlowCaseEntry>::iterator&);
        void emitSlow_op_urshift(Instruction*, Vector<SlowCaseEntry>::iterator&);

        
        void emitRightShift(Instruction*, bool isUnsigned);
        void emitRightShiftSlowCase(Instruction*, Vector<SlowCaseEntry>::iterator&, bool isUnsigned);

        /* This function is deprecated. */
        void emitGetJITStubArg(unsigned argumentNumber, RegisterID dst);

        void emitInitRegister(unsigned dst);

        void emitPutToCallFrameHeader(RegisterID from, RegisterFile::CallFrameHeaderEntry entry);
        void emitPutCellToCallFrameHeader(RegisterID from, RegisterFile::CallFrameHeaderEntry);
        void emitPutIntToCallFrameHeader(RegisterID from, RegisterFile::CallFrameHeaderEntry);
        void emitPutImmediateToCallFrameHeader(void* value, RegisterFile::CallFrameHeaderEntry entry);
        void emitGetFromCallFrameHeaderPtr(RegisterFile::CallFrameHeaderEntry entry, RegisterID to, RegisterID from = callFrameRegister);
        void emitGetFromCallFrameHeader32(RegisterFile::CallFrameHeaderEntry entry, RegisterID to, RegisterID from = callFrameRegister);

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
        void linkSlowCaseIfNotJSCell(Vector<SlowCaseEntry>::iterator&, int vReg);

        Jump checkStructure(RegisterID reg, Structure* structure);

        void restoreArgumentReference();
        void restoreArgumentReferenceForTrampoline();

        Call emitNakedCall(CodePtr function = CodePtr());

        void preserveReturnAddressAfterCall(RegisterID);
        void restoreReturnAddressBeforeReturn(RegisterID);
        void restoreReturnAddressBeforeReturn(Address);

        // Loads the character value of a single character string into dst.
        void emitLoadCharacterString(RegisterID src, RegisterID dst, JumpList& failures);
        
        void emitTimeoutCheck();
#ifndef NDEBUG
        void printBytecodeOperandTypes(unsigned src1, unsigned src2);
#endif

#if ENABLE(SAMPLING_FLAGS)
        void setSamplingFlag(int32_t);
        void clearSamplingFlag(int32_t);
#endif

#if ENABLE(SAMPLING_COUNTERS)
        void emitCount(AbstractSamplingCounter&, uint32_t = 1);
#endif

#if ENABLE(OPCODE_SAMPLING)
        void sampleInstruction(Instruction*, bool = false);
#endif

#if ENABLE(CODEBLOCK_SAMPLING)
        void sampleCodeBlock(CodeBlock*);
#else
        void sampleCodeBlock(CodeBlock*) {}
#endif

        Interpreter* m_interpreter;
        JSGlobalData* m_globalData;
        CodeBlock* m_codeBlock;

        Vector<CallRecord> m_calls;
        Vector<Label> m_labels;
        Vector<PropertyStubCompilationInfo> m_propertyAccessCompilationInfo;
        Vector<StructureStubCompilationInfo> m_callStructureStubCompilationInfo;
        Vector<MethodCallCompilationInfo> m_methodCallCompilationInfo;
        Vector<JumpTable> m_jmpTable;

        unsigned m_bytecodeOffset;
        Vector<JSRInfo> m_jsrSites;
        Vector<SlowCaseEntry> m_slowCases;
        Vector<SwitchRecord> m_switches;

        unsigned m_propertyAccessInstructionIndex;
        unsigned m_globalResolveInfoIndex;
        unsigned m_callLinkInfoIndex;

#if USE(JSVALUE32_64)
        unsigned m_jumpTargetIndex;
        unsigned m_mappedBytecodeOffset;
        unsigned m_mappedVirtualRegisterIndex;
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
        static CodePtr stringGetByValStubGenerator(JSGlobalData* globalData, ExecutablePool* pool);
    } JIT_CLASS_ALIGNMENT;

    inline void JIT::emit_op_loop(Instruction* currentInstruction)
    {
        emitTimeoutCheck();
        emit_op_jmp(currentInstruction);
    }

    inline void JIT::emit_op_loop_if_true(Instruction* currentInstruction)
    {
        emitTimeoutCheck();
        emit_op_jtrue(currentInstruction);
    }

    inline void JIT::emitSlow_op_loop_if_true(Instruction* currentInstruction, Vector<SlowCaseEntry>::iterator& iter)
    {
        emitSlow_op_jtrue(currentInstruction, iter);
    }

    inline void JIT::emit_op_loop_if_false(Instruction* currentInstruction)
    {
        emitTimeoutCheck();
        emit_op_jfalse(currentInstruction);
    }

    inline void JIT::emitSlow_op_loop_if_false(Instruction* currentInstruction, Vector<SlowCaseEntry>::iterator& iter)
    {
        emitSlow_op_jfalse(currentInstruction, iter);
    }

    inline void JIT::emit_op_loop_if_less(Instruction* currentInstruction)
    {
        emitTimeoutCheck();
        emit_op_jless(currentInstruction);
    }

    inline void JIT::emitSlow_op_loop_if_less(Instruction* currentInstruction, Vector<SlowCaseEntry>::iterator& iter)
    {
        emitSlow_op_jless(currentInstruction, iter);
    }

} // namespace JSC

#endif // ENABLE(JIT)

#endif // JIT_h
