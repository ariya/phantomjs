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
#include "NPObjectProxy.h"

#if ENABLE(PLUGIN_PROCESS)

#include "ArgumentCoders.h"
#include "Connection.h"
#include "NPIdentifierData.h"
#include "NPObjectMessageReceiverMessages.h"
#include "NPRemoteObjectMap.h"
#include "NPRuntimeUtilities.h"
#include "NPVariantData.h"
#include <WebCore/RunLoop.h>
#include <wtf/MainThread.h>

using namespace WebCore;

namespace WebKit {

NPObjectProxy* NPObjectProxy::create(NPRemoteObjectMap* npRemoteObjectMap, Plugin* plugin, uint64_t npObjectID)
{
    NPObjectProxy* npObjectProxy = toNPObjectProxy(createNPObject(0, npClass()));
    npObjectProxy->initialize(npRemoteObjectMap, plugin, npObjectID);

    return npObjectProxy;
}

NPObjectProxy::NPObjectProxy()
    : m_npRemoteObjectMap(0)
    , m_plugin(0)
    , m_npObjectID(0)
{
}

NPObjectProxy::~NPObjectProxy()
{
    ASSERT(isMainThread());

    if (!m_npRemoteObjectMap)
        return;

    m_npRemoteObjectMap->npObjectProxyDestroyed(this);
    m_npRemoteObjectMap->connection()->sendSync(Messages::NPObjectMessageReceiver::Deallocate(), Messages::NPObjectMessageReceiver::Deallocate::Reply(), m_npObjectID);
}

bool NPObjectProxy::isNPObjectProxy(NPObject* npObject)
{
    return npObject->_class == npClass();
}

void NPObjectProxy::invalidate()
{
    ASSERT(m_npRemoteObjectMap);
    ASSERT(m_plugin);

    m_npRemoteObjectMap = 0;
    m_plugin = 0;
}
    
void NPObjectProxy::initialize(NPRemoteObjectMap* npRemoteObjectMap, Plugin* plugin, uint64_t npObjectID)
{
    ASSERT(!m_npRemoteObjectMap);
    ASSERT(!m_plugin);
    ASSERT(!m_npObjectID);

    ASSERT(npRemoteObjectMap);
    ASSERT(plugin);
    ASSERT(npObjectID);

    m_npRemoteObjectMap = npRemoteObjectMap;
    m_plugin = plugin;
    m_npObjectID = npObjectID;
}

bool NPObjectProxy::hasMethod(NPIdentifier methodName)
{
    if (!m_npRemoteObjectMap)
        return false;
    
    NPIdentifierData methodNameData = NPIdentifierData::fromNPIdentifier(methodName);
    
    bool returnValue = false;
    
    if (!m_npRemoteObjectMap->connection()->sendSync(Messages::NPObjectMessageReceiver::HasMethod(methodNameData), Messages::NPObjectMessageReceiver::HasMethod::Reply(returnValue), m_npObjectID))
        return false;
    
    return returnValue;
}

bool NPObjectProxy::invoke(NPIdentifier methodName, const NPVariant* arguments, uint32_t argumentCount, NPVariant* result)
{
    if (!m_npRemoteObjectMap)
        return false;

    NPIdentifierData methodNameData = NPIdentifierData::fromNPIdentifier(methodName);
    Vector<NPVariantData> argumentsData;
    for (uint32_t i = 0; i < argumentCount; ++i)
        argumentsData.append(m_npRemoteObjectMap->npVariantToNPVariantData(arguments[i], m_plugin));

    bool returnValue = false;
    NPVariantData resultData;
    
    if (!m_npRemoteObjectMap->connection()->sendSync(Messages::NPObjectMessageReceiver::Invoke(methodNameData, argumentsData), Messages::NPObjectMessageReceiver::Invoke::Reply(returnValue, resultData), m_npObjectID))
        return false;
    
    if (!returnValue)
        return false;
    
    *result = m_npRemoteObjectMap->npVariantDataToNPVariant(resultData, m_plugin);
    return true;
}

bool NPObjectProxy::invokeDefault(const NPVariant* arguments, uint32_t argumentCount, NPVariant* result)
{
    if (!m_npRemoteObjectMap)
        return false;

    Vector<NPVariantData> argumentsData;
    for (uint32_t i = 0; i < argumentCount; ++i)
        argumentsData.append(m_npRemoteObjectMap->npVariantToNPVariantData(arguments[i], m_plugin));

    bool returnValue = false;
    NPVariantData resultData;
    
    if (!m_npRemoteObjectMap->connection()->sendSync(Messages::NPObjectMessageReceiver::InvokeDefault(argumentsData), Messages::NPObjectMessageReceiver::InvokeDefault::Reply(returnValue, resultData), m_npObjectID))
        return false;
    
    if (!returnValue)
        return false;
    
    *result = m_npRemoteObjectMap->npVariantDataToNPVariant(resultData, m_plugin);
    return true;
}

bool NPObjectProxy::hasProperty(NPIdentifier propertyName)
{
    if (!m_npRemoteObjectMap)
        return false;
    
    NPIdentifierData propertyNameData = NPIdentifierData::fromNPIdentifier(propertyName);

    bool returnValue = false;
    
    if (!m_npRemoteObjectMap->connection()->sendSync(Messages::NPObjectMessageReceiver::HasProperty(propertyNameData), Messages::NPObjectMessageReceiver::HasProperty::Reply(returnValue), m_npObjectID))
        return false;

    return returnValue;
}

bool NPObjectProxy::getProperty(NPIdentifier propertyName, NPVariant* result)
{
    if (!m_npRemoteObjectMap)
        return false;

    NPIdentifierData propertyNameData = NPIdentifierData::fromNPIdentifier(propertyName);

    bool returnValue = false;
    NPVariantData resultData;
    
    if (!m_npRemoteObjectMap->connection()->sendSync(Messages::NPObjectMessageReceiver::GetProperty(propertyNameData), Messages::NPObjectMessageReceiver::GetProperty::Reply(returnValue, resultData), m_npObjectID))
        return false;

    if (!returnValue)
        return false;

    *result = m_npRemoteObjectMap->npVariantDataToNPVariant(resultData, m_plugin);
    return true;
}

bool NPObjectProxy::setProperty(NPIdentifier propertyName, const NPVariant* value)
{
    if (!m_npRemoteObjectMap)
        return false;
    
    NPIdentifierData propertyNameData = NPIdentifierData::fromNPIdentifier(propertyName);
    NPVariantData propertyValueData = m_npRemoteObjectMap->npVariantToNPVariantData(*value, m_plugin);

    bool returnValue = false;

    if (!m_npRemoteObjectMap->connection()->sendSync(Messages::NPObjectMessageReceiver::SetProperty(propertyNameData, propertyValueData), Messages::NPObjectMessageReceiver::SetProperty::Reply(returnValue), m_npObjectID))
        return false;

    return returnValue;
}

bool NPObjectProxy::removeProperty(NPIdentifier propertyName)
{
    if (!m_npRemoteObjectMap)
        return false;
    
    NPIdentifierData propertyNameData = NPIdentifierData::fromNPIdentifier(propertyName);

    bool returnValue = false;
    
    if (!m_npRemoteObjectMap->connection()->sendSync(Messages::NPObjectMessageReceiver::RemoveProperty(propertyNameData), Messages::NPObjectMessageReceiver::RemoveProperty::Reply(returnValue), m_npObjectID))
        return false;

    return returnValue;
}

bool NPObjectProxy::enumerate(NPIdentifier** identifiers, uint32_t* identifierCount)
{
    if (!m_npRemoteObjectMap)
        return false;

    bool returnValue;
    Vector<NPIdentifierData> identifiersData;

    if (!m_npRemoteObjectMap->connection()->sendSync(Messages::NPObjectMessageReceiver::Enumerate(), Messages::NPObjectMessageReceiver::Enumerate::Reply(returnValue, identifiersData), m_npObjectID))
        return false;

    if (!returnValue)
        return false;

    NPIdentifier* nameIdentifiers = npnMemNewArray<NPIdentifier>(identifiersData.size());
    
    for (size_t i = 0; i < identifiersData.size(); ++i)
        nameIdentifiers[i] = identifiersData[i].createNPIdentifier();
    
    *identifiers = nameIdentifiers;
    *identifierCount = identifiersData.size();
    return true;
}

bool NPObjectProxy::construct(const NPVariant* arguments, uint32_t argumentCount, NPVariant* result)
{
    if (!m_npRemoteObjectMap)
        return false;

    Vector<NPVariantData> argumentsData;
    for (uint32_t i = 0; i < argumentCount; ++i)
        argumentsData.append(m_npRemoteObjectMap->npVariantToNPVariantData(arguments[i], m_plugin));

    bool returnValue = false;
    NPVariantData resultData;
    
    if (!m_npRemoteObjectMap->connection()->sendSync(Messages::NPObjectMessageReceiver::Construct(argumentsData), Messages::NPObjectMessageReceiver::Construct::Reply(returnValue, resultData), m_npObjectID))
        return false;
    
    if (!returnValue)
        return false;
    
    *result = m_npRemoteObjectMap->npVariantDataToNPVariant(resultData, m_plugin);
    return true;
}

NPClass* NPObjectProxy::npClass()
{
    static NPClass npClass = {
        NP_CLASS_STRUCT_VERSION,
        NP_Allocate,
        NP_Deallocate,
        0,
        NP_HasMethod,
        NP_Invoke,
        NP_InvokeDefault,
        NP_HasProperty,
        NP_GetProperty,
        NP_SetProperty,
        NP_RemoveProperty,
        NP_Enumerate,
        NP_Construct
    };

    return &npClass;
}

NPObject* NPObjectProxy::NP_Allocate(NPP npp, NPClass*)
{
    ASSERT_UNUSED(npp, !npp);

    return new NPObjectProxy;
}

void NPObjectProxy::NP_Deallocate(NPObject* npObject)
{
    // http://webkit.org/b/118535 - The Java Netscape Plug-in has a background thread do some of their NPP_Destroy work.
    // That background thread can call NP_Deallocate, and this leads to a WebProcess <-> PluginProcess deadlock.
    // Since NPAPI behavior on a background thread is undefined, it is okay to limit this workaround to the one API
    // that is known to be misused during plugin teardown, and to not be concerned about change in behavior if this
    // occured at any other time.
    if (!isMainThread()) {
        RunLoop::main()->dispatch(bind(&NPObjectProxy::NP_Deallocate, npObject));
        return;
    }
    
    NPObjectProxy* npObjectProxy = toNPObjectProxy(npObject);
    delete npObjectProxy;
}

bool NPObjectProxy::NP_HasMethod(NPObject* npObject, NPIdentifier methodName)
{
    return toNPObjectProxy(npObject)->hasMethod(methodName);
}

bool NPObjectProxy::NP_Invoke(NPObject* npObject, NPIdentifier methodName, const NPVariant* arguments, uint32_t argumentCount, NPVariant* result)
{
    return toNPObjectProxy(npObject)->invoke(methodName, arguments, argumentCount, result);
}

bool NPObjectProxy::NP_InvokeDefault(NPObject* npObject, const NPVariant* arguments, uint32_t argumentCount, NPVariant* result)
{
    return toNPObjectProxy(npObject)->invokeDefault(arguments, argumentCount, result);
}

bool NPObjectProxy::NP_HasProperty(NPObject* npObject, NPIdentifier propertyName)
{
    return toNPObjectProxy(npObject)->hasProperty(propertyName);
}

bool NPObjectProxy::NP_GetProperty(NPObject* npObject, NPIdentifier propertyName, NPVariant* result)
{
    return toNPObjectProxy(npObject)->getProperty(propertyName, result);
}

bool NPObjectProxy::NP_SetProperty(NPObject* npObject, NPIdentifier propertyName, const NPVariant* value)
{
    return toNPObjectProxy(npObject)->setProperty(propertyName, value);
}

bool NPObjectProxy::NP_RemoveProperty(NPObject* npObject, NPIdentifier propertyName)
{
    return toNPObjectProxy(npObject)->removeProperty(propertyName);
}

bool NPObjectProxy::NP_Enumerate(NPObject* npObject, NPIdentifier** identifiers, uint32_t* identifierCount)
{
    return toNPObjectProxy(npObject)->enumerate(identifiers, identifierCount);
}

bool NPObjectProxy::NP_Construct(NPObject* npObject, const NPVariant* arguments, uint32_t argumentCount, NPVariant* result)
{
    return toNPObjectProxy(npObject)->construct(arguments, argumentCount, result);
}

} // namespace WebKit

#endif // ENABLE(PLUGIN_PROCESS)
