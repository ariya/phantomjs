/*
 * Copyright (C) 2012, 2013 Apple Inc. All rights reserved.
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

#ifndef DFGSilentRegisterSavePlan_h
#define DFGSilentRegisterSavePlan_h

#include <wtf/Platform.h>

#if ENABLE(DFG_JIT)

#include "DFGCommon.h"
#include "DFGFPRInfo.h"
#include "DFGGPRInfo.h"

namespace JSC { namespace DFG {

enum SilentSpillAction {
    DoNothingForSpill,
    Store32Tag,
    Store32Payload,
    StorePtr,
    Store64,
    StoreDouble
};

enum SilentFillAction {
    DoNothingForFill,
    SetInt32Constant,
    SetBooleanConstant,
    SetCellConstant,
    SetTrustedJSConstant,
    SetJSConstant,
    SetJSConstantTag,
    SetJSConstantPayload,
    SetInt32Tag,
    SetCellTag,
    SetBooleanTag,
    SetDoubleConstant,
    Load32Tag,
    Load32Payload,
    Load32PayloadBoxInt,
    LoadPtr,
    Load64,
    LoadDouble,
    LoadDoubleBoxDouble,
    LoadJSUnboxDouble
};

class SilentRegisterSavePlan {
public:
    SilentRegisterSavePlan()
        : m_spillAction(DoNothingForSpill)
        , m_fillAction(DoNothingForFill)
        , m_register(-1)
        , m_node(0)
    {
    }
    
    SilentRegisterSavePlan(
        SilentSpillAction spillAction,
        SilentFillAction fillAction,
        Node* node,
        GPRReg gpr)
        : m_spillAction(spillAction)
        , m_fillAction(fillAction)
        , m_register(gpr)
        , m_node(node)
    {
    }
    
    SilentRegisterSavePlan(
        SilentSpillAction spillAction,
        SilentFillAction fillAction,
        Node* node,
        FPRReg fpr)
        : m_spillAction(spillAction)
        , m_fillAction(fillAction)
        , m_register(fpr)
        , m_node(node)
    {
    }
    
    SilentSpillAction spillAction() const { return static_cast<SilentSpillAction>(m_spillAction); }
    SilentFillAction fillAction() const { return static_cast<SilentFillAction>(m_fillAction); }
    
    Node* node() const { return m_node; }
    
    GPRReg gpr() const { return static_cast<GPRReg>(m_register); }
    FPRReg fpr() const { return static_cast<FPRReg>(m_register); }
    
private:
    int8_t m_spillAction;
    int8_t m_fillAction;
    int8_t m_register;
    Node* m_node;
};

} } // namespace JSC::DFG

#endif // ENABLE(DFG_JIT)

#endif // DFGSilentRegisterSavePlan_h

