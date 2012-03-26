/*
 * Copyright (C) 2011 Google Inc. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY GOOGLE AND ITS CONTRIBUTORS "AS IS" AND ANY
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

#include "config.h"
#include "InspectorState.h"

#if ENABLE(INSPECTOR)

#include "InspectorClient.h"

namespace WebCore {

InspectorState::InspectorState(InspectorClient* client)
    : m_client(client)
    , m_properties(InspectorObject::create())
    , m_isOnMute(false)
{
}

void InspectorState::loadFromCookie(const String& inspectorStateCookie)
{
    m_properties.clear();
    RefPtr<InspectorValue> cookie = InspectorValue::parseJSON(inspectorStateCookie);
    if (cookie)
        m_properties = cookie->asObject();
    if (!m_properties)
        m_properties = InspectorObject::create();
}

void InspectorState::mute()
{
    m_isOnMute = true;
}

void InspectorState::unmute()
{
    m_isOnMute = false;
}

void InspectorState::updateCookie()
{
    if (m_client && !m_isOnMute)
        m_client->updateInspectorStateCookie(m_properties->toJSONString());
}

void InspectorState::setValue(const String& propertyName, PassRefPtr<InspectorValue> value)
{
    m_properties->setValue(propertyName, value);
    updateCookie();
}

bool InspectorState::getBoolean(const String& propertyName)
{
    InspectorObject::iterator it = m_properties->find(propertyName);
    bool value = false;
    if (it != m_properties->end())
        it->second->asBoolean(&value);
    return value;
}

String InspectorState::getString(const String& propertyName)
{
    InspectorObject::iterator it = m_properties->find(propertyName);
    String value;
    if (it != m_properties->end())
        it->second->asString(&value);
    return value;
}

long InspectorState::getLong(const String& propertyName)
{
    InspectorObject::iterator it = m_properties->find(propertyName);
    long value = 0;
    if (it != m_properties->end())
        it->second->asNumber(&value);
    return value;
}

PassRefPtr<InspectorObject> InspectorState::getObject(const String& propertyName)
{
    InspectorObject::iterator it = m_properties->find(propertyName);
    if (it == m_properties->end()) {
        m_properties->setObject(propertyName, InspectorObject::create());
        it = m_properties->find(propertyName);
    }
    return it->second->asObject();
}

} // namespace WebCore

#endif // ENABLE(INSPECTOR)
