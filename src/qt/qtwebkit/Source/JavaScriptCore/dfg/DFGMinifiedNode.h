/*
 * Copyright (C) 2012 Apple Inc. All rights reserved.
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

#ifndef DFGMinifiedNode_h
#define DFGMinifiedNode_h

#include <wtf/Platform.h>

#if ENABLE(DFG_JIT)

#include "DFGCommon.h"
#include "DFGMinifiedID.h"
#include "DFGNodeType.h"

namespace JSC { namespace DFG {

struct Node;

inline bool belongsInMinifiedGraph(NodeType type)
{
    switch (type) {
    case JSConstant:
    case WeakJSConstant:
    case ValueToInt32:
    case Int32ToDouble:
    case ForwardInt32ToDouble:
    case UInt32ToNumber:
    case DoubleAsInt32:
    case PhantomArguments:
        return true;
    default:
        return false;
    }
}

class MinifiedNode {
public:
    MinifiedNode() { }
    
    static MinifiedNode fromNode(Node*);
    
    MinifiedID id() const { return m_id; }
    NodeType op() const { return m_op; }
    
    bool hasChild1() const { return hasChild(m_op); }
    
    MinifiedID child1() const
    {
        ASSERT(hasChild(m_op));
        return MinifiedID::fromBits(m_childOrInfo);
    }
    
    bool hasConstant() const { return hasConstantNumber() || hasWeakConstant(); }
    
    bool hasConstantNumber() const { return hasConstantNumber(m_op); }
    
    unsigned constantNumber() const
    {
        ASSERT(hasConstantNumber(m_op));
        return m_childOrInfo;
    }
    
    bool hasWeakConstant() const { return hasWeakConstant(m_op); }
    
    JSCell* weakConstant() const
    {
        ASSERT(hasWeakConstant(m_op));
        return bitwise_cast<JSCell*>(m_childOrInfo);
    }
    
    static MinifiedID getID(MinifiedNode* node) { return node->id(); }
    static bool compareByNodeIndex(const MinifiedNode& a, const MinifiedNode& b)
    {
        return a.m_id < b.m_id;
    }
    
private:
    static bool hasChild(NodeType type)
    {
        switch (type) {
        case ValueToInt32:
        case Int32ToDouble:
        case ForwardInt32ToDouble:
        case UInt32ToNumber:
        case DoubleAsInt32:
            return true;
        default:
            return false;
        }
    }
    static bool hasConstantNumber(NodeType type)
    {
        return type == JSConstant;
    }
    static bool hasWeakConstant(NodeType type)
    {
        return type == WeakJSConstant;
    }
    
    MinifiedID m_id;
    uintptr_t m_childOrInfo; // Nodes in the minified graph have only one child each.
    NodeType m_op;
};

} } // namespace JSC::DFG

#endif // ENABLE(DFG_JIT)

#endif // DFGMinifiedNode_h

