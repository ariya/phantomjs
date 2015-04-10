/*
 * Copyright (C) 2012 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

namespace aNamespace {

class MemoryInstrumentation {
public:
    enum ObjectType {
        DOM
    };
};

class MemoryObjectInfo {
};

template <typename T>
class MemoryClassInfo {
public:
    MemoryClassInfo(MemoryObjectInfo*, const T*, MemoryInstrumentation::ObjectType) { }
    template <typename M> void addMember(M&) { }
    template <typename M> void addInstrumentedMember(M&) { }
};

// ------ Mocked types
template <typename T>
class OwnPtr {
public:
    OwnPtr(T* ptr) : m_ptr(ptr) { }
private:
    T* m_ptr;
};

template <typename T>
class RefPtr {
public:
    RefPtr(T* ptr) : m_ptr(ptr) { }
private:
    T* m_ptr;
};

template <typename T>
class Vector {
private:
    T* m_data;
};

class String {
private:
    char* m_data;
};

// ------ User defined types
class NotInstrumentedClass {
public:
    int m_array[10];
};

class InstrumentedClass {
public:
    virtual void reportMemoryUsage(MemoryObjectInfo* memoryObjectInfo) const
    {
        MemoryClassInfo<InstrumentedClass> info(memoryObjectInfo, this, MemoryInstrumentation::DOM);
        info.addMember(m_notInstrumentedPtrReportedNoWarning);
        info.addInstrumentedMember(m_instrumentedPtrReportedNoWarning);
    }

private:

    // have to be skipped even if not reported
    int m_PODNotReportedNoWarning;
    int m_arrayNotReportedNoWarning[10];
    NotInstrumentedClass m_notInstrumentedNotReportedNoWarning;
    NotInstrumentedClass m_notInstrumentedArrayNotReportedNoWarning[10];
    enum { None } m_enumNotReportedNoWarning;
    bool m_boolNotReportedNoWarning;

    // reported
    NotInstrumentedClass* m_notInstrumentedPtrReportedNoWarning;
    InstrumentedClass* m_instrumentedPtrReportedNoWarning;

    // reported
    InstrumentedClass* m_instrumentedPtrNotReportedWarning;
    int* m_rawBufferNotReportedWarning;
    int* m_intPtrArrayNotReportedWarning[10];
    NotInstrumentedClass* m_objPtrArrayNotReportedWarning[10];
    OwnPtr<NotInstrumentedClass> m_ownPtrNotReportedWarning;
    RefPtr<NotInstrumentedClass> m_refPtrNotReportedWarning;
    String m_stringNotReportedWarning;
    Vector<NotInstrumentedClass> m_vectorNotReportedWarning;
};

}

using namespace aNamespace;

namespace {

class InstrumentedChildClass : public aNamespace::InstrumentedClass {
public:
    virtual void reportMemoryUsage(MemoryObjectInfo*) const;
};

void InstrumentedChildClass::reportMemoryUsage(MemoryObjectInfo* memoryObjectInfo) const
{
    MemoryClassInfo<InstrumentedChildClass> info(memoryObjectInfo, this, MemoryInstrumentation::DOM);
}

}

class InstrumentedChildChildClass : public InstrumentedChildClass {
public:
    virtual void reportMemoryUsage(MemoryObjectInfo*) const;
};

void InstrumentedChildChildClass::reportMemoryUsage(MemoryObjectInfo*) const
{
}

int aGlobalVariable = 0;

int main(int argc, char** argv)
{
    return 0;
}
