/*
 * Copyright (C) 2010 Apple Inc. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef NPVariantData_h
#define NPVariantData_h

#if ENABLE(PLUGIN_PROCESS)

#include <wtf/text/CString.h>

namespace CoreIPC {
    class ArgumentDecoder;
    class ArgumentEncoder;
}

namespace WebKit {

// The CoreIPC representation of an NPVariant.

class NPVariantData {
public:
    enum Type {
        Void,
        Null,
        Bool,
        Int32,
        Double,
        String,
        LocalNPObjectID,
        RemoteNPObjectID,
    };
    NPVariantData();

    static NPVariantData makeVoid();
    static NPVariantData makeNull();
    static NPVariantData makeBool(bool value);
    static NPVariantData makeInt32(int32_t value);
    static NPVariantData makeDouble(double value);
    static NPVariantData makeString(const char* string, unsigned length);
    static NPVariantData makeLocalNPObjectID(uint64_t value);
    static NPVariantData makeRemoteNPObjectID(uint64_t value);

    Type type() const { return static_cast<Type>(m_type); }

    bool boolValue() const
    {
        ASSERT(type() == NPVariantData::Bool);
        return m_boolValue;
    }

    int32_t int32Value() const
    {
        ASSERT(type() == NPVariantData::Int32);
        return m_int32Value;
    }

    double doubleValue() const
    {
        ASSERT(type() == NPVariantData::Double);
        return m_doubleValue;
    }

    const CString& stringValue() const
    {
        ASSERT(type() == NPVariantData::String);
        return m_stringValue;
    }

    uint64_t localNPObjectIDValue() const
    {
        ASSERT(type() == NPVariantData::LocalNPObjectID);
        return m_localNPObjectIDValue;
    }

    uint64_t remoteNPObjectIDValue() const
    {
        ASSERT(type() == NPVariantData::RemoteNPObjectID);
        return m_remoteNPObjectIDValue;
    }

    void encode(CoreIPC::ArgumentEncoder&) const;
    static bool decode(CoreIPC::ArgumentDecoder&, NPVariantData&);

private:
    uint32_t m_type;
    bool m_boolValue;
    int32_t m_int32Value;
    double m_doubleValue;
    CString m_stringValue;
    uint64_t m_localNPObjectIDValue;
    uint64_t m_remoteNPObjectIDValue;
};

} // namespace WebKit

#endif // ENABLE(PLUGIN_PROCESS)
    
#endif // NPVariantData_h
