/*
 * Copyright (C) 2011 Google Inc. All rights reserved.
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

#ifndef InspectorState_h
#define InspectorState_h

#if ENABLE(INSPECTOR)

#include "InspectorValues.h"
#include <wtf/HashMap.h>
#include <wtf/RefCounted.h>
#include <wtf/text/WTFString.h>

namespace WebCore {

class InspectorStateClient;

class InspectorStateUpdateListener {
public:
    virtual ~InspectorStateUpdateListener() { }
    virtual void inspectorStateUpdated() = 0;
};

class InspectorState {
    WTF_MAKE_FAST_ALLOCATED;
public:
    InspectorState(InspectorStateUpdateListener*, PassRefPtr<InspectorObject>);
    virtual ~InspectorState() {}

    void loadFromCookie(const String& inspectorStateCookie);

    void mute();
    void unmute();

    bool getBoolean(const String& propertyName);
    String getString(const String& propertyName);
    long getLong(const String& propertyName);
    double getDouble(const String& propertyName);
    PassRefPtr<InspectorObject> getObject(const String& propertyName);

    void setBoolean(const String& propertyName, bool value) { setValue(propertyName, InspectorBasicValue::create(value)); }
    void setString(const String& propertyName, const String& value) { setValue(propertyName, InspectorString::create(value)); }
    void setLong(const String& propertyName, long value) { setValue(propertyName, InspectorBasicValue::create((double)value)); }
    void setDouble(const String& propertyName, double value) { setValue(propertyName, InspectorBasicValue::create(value)); }
    void setObject(const String& propertyName, PassRefPtr<InspectorObject> value) { setValue(propertyName, value); }

    void remove(const String&);
private:
    void updateCookie();
    void setValue(const String& propertyName, PassRefPtr<InspectorValue>);

    // Gets called from InspectorCompositeState::loadFromCookie().
    void setFromCookie(PassRefPtr<InspectorObject>);

    friend class InspectorCompositeState;

    InspectorStateUpdateListener* m_listener;
    RefPtr<InspectorObject> m_properties;
};

class InspectorCompositeState : public InspectorStateUpdateListener {
public:
    InspectorCompositeState(InspectorStateClient* inspectorClient)
        : m_client(inspectorClient)
        , m_stateObject(InspectorObject::create())
        , m_isMuted(false)
    {
    }
    virtual ~InspectorCompositeState() { }

    void mute();
    void unmute();

    InspectorState* createAgentState(const String&);
    void loadFromCookie(const String&);

private:
    typedef HashMap<String, OwnPtr<InspectorState> > InspectorStateMap;

    // From InspectorStateUpdateListener.
    virtual void inspectorStateUpdated();

    InspectorStateClient* m_client;
    RefPtr<InspectorObject> m_stateObject;
    bool m_isMuted;
    InspectorStateMap m_inspectorStateMap;
};

} // namespace WebCore

#endif // ENABLE(INSPECTOR)
#endif // !defined(InspectorState_h)
