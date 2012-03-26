/*
 * Copyright (C) 2009 Apple Inc. All rights reserved.
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

#ifndef CodeLocation_h
#define CodeLocation_h

#include "MacroAssemblerCodeRef.h"

#if ENABLE(ASSEMBLER)

namespace JSC {

class CodeLocationInstruction;
class CodeLocationLabel;
class CodeLocationJump;
class CodeLocationCall;
class CodeLocationNearCall;
class CodeLocationDataLabel32;
class CodeLocationDataLabelPtr;

// The CodeLocation* types are all pretty much do-nothing wrappers around
// CodePtr (or MacroAssemblerCodePtr, to give it its full name).  These
// classes only exist to provide type-safety when linking and patching code.
//
// The one new piece of functionallity introduced by these classes is the
// ability to create (or put another way, to re-discover) another CodeLocation
// at an offset from one you already know.  When patching code to optimize it
// we often want to patch a number of instructions that are short, fixed
// offsets apart.  To reduce memory overhead we will only retain a pointer to
// one of the instructions, and we will use the *AtOffset methods provided by
// CodeLocationCommon to find the other points in the code to modify.
class CodeLocationCommon : public MacroAssemblerCodePtr {
public:
    CodeLocationInstruction instructionAtOffset(int offset);
    CodeLocationLabel labelAtOffset(int offset);
    CodeLocationJump jumpAtOffset(int offset);
    CodeLocationCall callAtOffset(int offset);
    CodeLocationNearCall nearCallAtOffset(int offset);
    CodeLocationDataLabelPtr dataLabelPtrAtOffset(int offset);
    CodeLocationDataLabel32 dataLabel32AtOffset(int offset);

protected:
    CodeLocationCommon()
    {
    }

    CodeLocationCommon(MacroAssemblerCodePtr location)
        : MacroAssemblerCodePtr(location)
    {
    }
};

class CodeLocationInstruction : public CodeLocationCommon {
public:
    CodeLocationInstruction() {}
    explicit CodeLocationInstruction(MacroAssemblerCodePtr location)
        : CodeLocationCommon(location) {}
    explicit CodeLocationInstruction(void* location)
        : CodeLocationCommon(MacroAssemblerCodePtr(location)) {}
};

class CodeLocationLabel : public CodeLocationCommon {
public:
    CodeLocationLabel() {}
    explicit CodeLocationLabel(MacroAssemblerCodePtr location)
        : CodeLocationCommon(location) {}
    explicit CodeLocationLabel(void* location)
        : CodeLocationCommon(MacroAssemblerCodePtr(location)) {}
};

class CodeLocationJump : public CodeLocationCommon {
public:
    CodeLocationJump() {}
    explicit CodeLocationJump(MacroAssemblerCodePtr location)
        : CodeLocationCommon(location) {}
    explicit CodeLocationJump(void* location)
        : CodeLocationCommon(MacroAssemblerCodePtr(location)) {}
};

class CodeLocationCall : public CodeLocationCommon {
public:
    CodeLocationCall() {}
    explicit CodeLocationCall(MacroAssemblerCodePtr location)
        : CodeLocationCommon(location) {}
    explicit CodeLocationCall(void* location)
        : CodeLocationCommon(MacroAssemblerCodePtr(location)) {}
};

class CodeLocationNearCall : public CodeLocationCommon {
public:
    CodeLocationNearCall() {}
    explicit CodeLocationNearCall(MacroAssemblerCodePtr location)
        : CodeLocationCommon(location) {}
    explicit CodeLocationNearCall(void* location)
        : CodeLocationCommon(MacroAssemblerCodePtr(location)) {}
};

class CodeLocationDataLabel32 : public CodeLocationCommon {
public:
    CodeLocationDataLabel32() {}
    explicit CodeLocationDataLabel32(MacroAssemblerCodePtr location)
        : CodeLocationCommon(location) {}
    explicit CodeLocationDataLabel32(void* location)
        : CodeLocationCommon(MacroAssemblerCodePtr(location)) {}
};

class CodeLocationDataLabelPtr : public CodeLocationCommon {
public:
    CodeLocationDataLabelPtr() {}
    explicit CodeLocationDataLabelPtr(MacroAssemblerCodePtr location)
        : CodeLocationCommon(location) {}
    explicit CodeLocationDataLabelPtr(void* location)
        : CodeLocationCommon(MacroAssemblerCodePtr(location)) {}
};

inline CodeLocationInstruction CodeLocationCommon::instructionAtOffset(int offset)
{
    ASSERT_VALID_CODE_OFFSET(offset);
    return CodeLocationInstruction(reinterpret_cast<char*>(dataLocation()) + offset);
}

inline CodeLocationLabel CodeLocationCommon::labelAtOffset(int offset)
{
    ASSERT_VALID_CODE_OFFSET(offset);
    return CodeLocationLabel(reinterpret_cast<char*>(dataLocation()) + offset);
}

inline CodeLocationJump CodeLocationCommon::jumpAtOffset(int offset)
{
    ASSERT_VALID_CODE_OFFSET(offset);
    return CodeLocationJump(reinterpret_cast<char*>(dataLocation()) + offset);
}

inline CodeLocationCall CodeLocationCommon::callAtOffset(int offset)
{
    ASSERT_VALID_CODE_OFFSET(offset);
    return CodeLocationCall(reinterpret_cast<char*>(dataLocation()) + offset);
}

inline CodeLocationNearCall CodeLocationCommon::nearCallAtOffset(int offset)
{
    ASSERT_VALID_CODE_OFFSET(offset);
    return CodeLocationNearCall(reinterpret_cast<char*>(dataLocation()) + offset);
}

inline CodeLocationDataLabelPtr CodeLocationCommon::dataLabelPtrAtOffset(int offset)
{
    ASSERT_VALID_CODE_OFFSET(offset);
    return CodeLocationDataLabelPtr(reinterpret_cast<char*>(dataLocation()) + offset);
}

inline CodeLocationDataLabel32 CodeLocationCommon::dataLabel32AtOffset(int offset)
{
    ASSERT_VALID_CODE_OFFSET(offset);
    return CodeLocationDataLabel32(reinterpret_cast<char*>(dataLocation()) + offset);
}

} // namespace JSC

#endif // ENABLE(ASSEMBLER)

#endif // CodeLocation_h
