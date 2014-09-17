/*
 * Copyright (C) 2009 Apple Inc. All Rights Reserved.
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

#ifndef JSByteArray_h
#define JSByteArray_h

#include "JSObject.h"

#include <wtf/ByteArray.h>

namespace JSC {

    class JSByteArray : public JSNonFinalObject {
        friend class JSGlobalData;
    public:
        typedef JSNonFinalObject Base;

        bool canAccessIndex(unsigned i) { return i < m_storage->length(); }
        JSValue getIndex(ExecState*, unsigned i)
        {
            ASSERT(canAccessIndex(i));
            return jsNumber(m_storage->data()[i]);
        }

        void setIndex(unsigned i, int value)
        {
            ASSERT(canAccessIndex(i));
            if (value & ~0xFF) {
                if (value < 0)
                    value = 0;
                else
                    value = 255;
            }
            m_storage->data()[i] = static_cast<unsigned char>(value);
        }
        
        void setIndex(unsigned i, double value)
        {
            ASSERT(canAccessIndex(i));
            if (!(value > 0)) // Clamp NaN to 0
                value = 0;
            else if (value > 255)
                value = 255;
            m_storage->data()[i] = static_cast<unsigned char>(value + 0.5);
        }
        
        void setIndex(ExecState* exec, unsigned i, JSValue value)
        {
            double byteValue = value.toNumber(exec);
            if (exec->hadException())
                return;
            if (canAccessIndex(i))
                setIndex(i, byteValue);
        }

        JSByteArray(ExecState*, Structure*, WTF::ByteArray* storage);
        static Structure* createStructure(JSGlobalData&, JSValue prototype, const JSC::ClassInfo* = &s_defaultInfo);

        virtual bool getOwnPropertySlot(JSC::ExecState*, const JSC::Identifier& propertyName, JSC::PropertySlot&);
        virtual bool getOwnPropertySlot(JSC::ExecState*, unsigned propertyName, JSC::PropertySlot&);
        virtual bool getOwnPropertyDescriptor(ExecState*, const Identifier&, PropertyDescriptor&);
        virtual void put(JSC::ExecState*, const JSC::Identifier& propertyName, JSC::JSValue, JSC::PutPropertySlot&);
        virtual void put(JSC::ExecState*, unsigned propertyName, JSC::JSValue);

        virtual void getOwnPropertyNames(JSC::ExecState*, JSC::PropertyNameArray&, EnumerationMode mode = ExcludeDontEnumProperties);

        static const ClassInfo s_defaultInfo;
        
        size_t length() const { return m_storage->length(); }

        WTF::ByteArray* storage() const { return m_storage.get(); }

#if !ASSERT_DISABLED
        virtual ~JSByteArray();
#endif

    protected:
        static const unsigned StructureFlags = OverridesGetOwnPropertySlot | OverridesGetPropertyNames | JSObject::StructureFlags;

    private:
        JSByteArray(VPtrStealingHackType)
            : JSNonFinalObject(VPtrStealingHack)
        {
        }

        RefPtr<WTF::ByteArray> m_storage;
    };
    
    JSByteArray* asByteArray(JSValue value);
    inline JSByteArray* asByteArray(JSValue value)
    {
        return static_cast<JSByteArray*>(value.asCell());
    }

    inline bool isJSByteArray(JSGlobalData* globalData, JSValue v) { return v.isCell() && v.asCell()->vptr() == globalData->jsByteArrayVPtr; }

} // namespace JSC

#endif // JSByteArray_h
