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

#ifndef LabelScope_h
#define LabelScope_h

#include <wtf/PassRefPtr.h>
#include "Label.h"

namespace JSC {

    class Identifier;

    class LabelScope {
    public:
        enum Type { Loop, Switch, NamedLabel };

        LabelScope(Type type, const Identifier* name, int scopeDepth, PassRefPtr<Label> breakTarget, PassRefPtr<Label> continueTarget)
            : m_refCount(0)
            , m_type(type)
            , m_name(name)
            , m_scopeDepth(scopeDepth)
            , m_breakTarget(breakTarget)
            , m_continueTarget(continueTarget)
        {
        }
        int refCount() const { return m_refCount; }

        Label* breakTarget() const { return m_breakTarget.get(); }
        Label* continueTarget() const { return m_continueTarget.get(); }

        Type type() const { return m_type; }
        const Identifier* name() const { return m_name; }
        int scopeDepth() const { return m_scopeDepth; }

    private:
        friend class LabelScopePtr;

        void ref() { ++m_refCount; }
        void deref()
        {
            --m_refCount;
            ASSERT(m_refCount >= 0);
        }

        int m_refCount;
        Type m_type;
        const Identifier* m_name;
        int m_scopeDepth;
        RefPtr<Label> m_breakTarget;
        RefPtr<Label> m_continueTarget;
    };

    typedef Vector<LabelScope, 8> LabelScopeStore;

    class LabelScopePtr {
    public:
        LabelScopePtr()
            : m_owner(0)
            , m_index(0)
        {
        }
        LabelScopePtr(LabelScopeStore* owner, size_t index)
            : m_owner(owner)
            , m_index(index)
        {
            m_owner->at(index).ref();
        }

        LabelScopePtr(const LabelScopePtr& other)
            : m_owner(other.m_owner)
            , m_index(other.m_index)
        {
            if (m_owner)
                m_owner->at(m_index).ref();
        }

        const LabelScopePtr& operator=(const LabelScopePtr& other)
        {
            if (other.m_owner)
                other.m_owner->at(other.m_index).ref();
            if (m_owner)
                m_owner->at(m_index).deref();
            m_owner = other.m_owner;
            m_index = other.m_index;
            return *this;
        }

        ~LabelScopePtr()
        {
            if (m_owner)
                m_owner->at(m_index).deref();
        }

        LabelScope& operator*() { ASSERT(m_owner); return m_owner->at(m_index); }
        LabelScope* operator->() { ASSERT(m_owner); return &m_owner->at(m_index); }
        const LabelScope& operator*() const { ASSERT(m_owner); return m_owner->at(m_index); }
        const LabelScope* operator->() const { ASSERT(m_owner); return &m_owner->at(m_index); }

    private:
        LabelScopeStore* m_owner;
        size_t m_index;
    };

} // namespace JSC

#endif // LabelScope_h
