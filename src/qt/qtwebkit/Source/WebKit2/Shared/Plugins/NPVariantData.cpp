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

#include "config.h"
#include "NPVariantData.h"

#if ENABLE(PLUGIN_PROCESS)

#include "ArgumentDecoder.h"
#include "ArgumentEncoder.h"
#include "WebCoreArgumentCoders.h"

namespace WebKit {

NPVariantData::NPVariantData()
    : m_type(NPVariantData::Void)
    , m_boolValue(false)
    , m_int32Value(0)
    , m_doubleValue(0)
    , m_localNPObjectIDValue(0)
    , m_remoteNPObjectIDValue(0)
{
}

NPVariantData NPVariantData::makeVoid()
{
    return NPVariantData();
}

NPVariantData NPVariantData::makeNull()
{
    NPVariantData npVariantData;
    
    npVariantData.m_type = NPVariantData::Null;
    
    return npVariantData;
}
    
NPVariantData NPVariantData::makeBool(bool value)
{
    NPVariantData npVariantData;

    npVariantData.m_type = NPVariantData::Bool;
    npVariantData.m_boolValue = value;

    return npVariantData;
}

NPVariantData NPVariantData::makeInt32(int32_t value)
{
    NPVariantData npVariantData;

    npVariantData.m_type = NPVariantData::Int32;
    npVariantData.m_int32Value = value;

    return npVariantData;
}    

NPVariantData NPVariantData::makeDouble(double value)
{
    NPVariantData npVariantData;

    npVariantData.m_type = NPVariantData::Double;
    npVariantData.m_doubleValue = value;

    return npVariantData;
}

NPVariantData NPVariantData::makeString(const char* string, unsigned length)
{
    NPVariantData npVariantData;
    
    npVariantData.m_type = NPVariantData::String;
    npVariantData.m_stringValue = CString(string, length);
    
    return npVariantData;
}

NPVariantData NPVariantData::makeLocalNPObjectID(uint64_t value)
{
    NPVariantData npVariantData;

    npVariantData.m_type = NPVariantData::LocalNPObjectID;
    npVariantData.m_localNPObjectIDValue = value;

    return npVariantData;
}

NPVariantData NPVariantData::makeRemoteNPObjectID(uint64_t value)
{
    NPVariantData npVariantData;

    npVariantData.m_type = NPVariantData::RemoteNPObjectID;
    npVariantData.m_remoteNPObjectIDValue = value;

    return npVariantData;
}

void NPVariantData::encode(CoreIPC::ArgumentEncoder& encoder) const
{
    encoder << m_type;

    switch (type()) {
    case NPVariantData::Void:
    case NPVariantData::Null:
        break;
    case NPVariantData::Bool:
        encoder << boolValue();
        break;
    case NPVariantData::Int32:
        encoder << int32Value();
        break;
    case NPVariantData::Double:
        encoder << doubleValue();
        break;
    case NPVariantData::String:
        encoder << stringValue();
        break;
    case NPVariantData::LocalNPObjectID:
        encoder << localNPObjectIDValue();
        break;
    case NPVariantData::RemoteNPObjectID:
        encoder << remoteNPObjectIDValue();
        break;
    }
}

bool NPVariantData::decode(CoreIPC::ArgumentDecoder& decoder, NPVariantData& result)
{
    uint32_t type;
    if (!decoder.decode(type))
        return false;

    // We special-case LocalNPObjectID and RemoteNPObjectID here so a LocalNPObjectID is
    // decoded as a RemoteNPObjectID and vice versa.
    // This is done because the type is from the perspective of the other connection, and
    // thus we have to adjust it to match our own perspective.
    if (type == NPVariantData::LocalNPObjectID)
        type = NPVariantData::RemoteNPObjectID;
    else if (type == NPVariantData::RemoteNPObjectID)
        type = NPVariantData::LocalNPObjectID;

    result.m_type = type;

    switch (result.m_type) {
    case NPVariantData::Void:
    case NPVariantData::Null:
        return true;
    case NPVariantData::Bool:
        return decoder.decode(result.m_boolValue);
    case NPVariantData::Int32:
        return decoder.decode(result.m_int32Value);
    case NPVariantData::Double:
        return decoder.decode(result.m_doubleValue);
    case NPVariantData::String:
        return decoder.decode(result.m_stringValue);
    case NPVariantData::LocalNPObjectID:
        return decoder.decode(result.m_localNPObjectIDValue);
    case NPVariantData::RemoteNPObjectID:
        return decoder.decode(result.m_remoteNPObjectIDValue);
    }

    return false;
}

} // namespace WebKit

#endif // ENABLE(PLUGIN_PROCESS)
