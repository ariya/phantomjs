/*
 * Copyright (C) 2003, 2004, 2005, 2007, 2009, 2010 Apple Inc. All rights reserved.
 * Copyright 2010, The Android Open Source Project
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
 * THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef JavaArrayJSC_h
#define JavaArrayJSC_h

#if ENABLE(JAVA_BRIDGE)

#include "BridgeJSC.h"
#include "JNIUtility.h"
#include "JobjectWrapper.h"

namespace JSC {

namespace Bindings {

class JavaArray : public Array {
public:
    JavaArray(jobject array, const char* type, PassRefPtr<RootObject>);
    virtual ~JavaArray();

    RootObject* rootObject() const;

    virtual void setValueAt(ExecState*, unsigned int index, JSValue) const;
    virtual JSValue valueAt(ExecState*, unsigned int index) const;
    virtual unsigned int getLength() const;

    jobject javaArray() const { return m_array->m_instance; }

    static JSValue convertJObjectToArray(ExecState*, jobject, const char* type, PassRefPtr<RootObject>);

private:
    RefPtr<JobjectWrapper> m_array;
    unsigned int m_length;
    const char* m_type;
};

} // namespace Bindings

} // namespace JSC

#endif // ENABLE(JAVA_BRIDGE)

#endif // JavaArrayJSC_h
