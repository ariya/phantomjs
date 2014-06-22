/*
 * Copyright (C) 2010 Google Inc. All rights reserved.
 * Copyright (C) 2012 Apple Inc. All rights reserved.
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

#ifndef Dictionary_h
#define Dictionary_h

#include "JSDictionary.h"
#include "JSEventListener.h"
#include "NotImplemented.h"
#include "ScriptValue.h"
#include <wtf/HashMap.h>
#include <wtf/text/CString.h>
#include <wtf/text/WTFString.h>

namespace JSC {
class JSValue;
}

namespace WebCore {
class EventListener;

class Dictionary {
public:
    Dictionary();
    Dictionary(JSC::ExecState*, JSC::JSValue);

    // Returns true if a value was found for the provided property.
    template <typename Result>
    bool get(const char* propertyName, Result&) const;
    template <typename Result>
    bool get(const String& propertyName, Result&) const;
    
    template <typename T>
    PassRefPtr<EventListener> getEventListener(const char* propertyName, T* target) const;
    template <typename T>
    PassRefPtr<EventListener> getEventListener(const String& propertyName, T* target) const;

    bool isObject() const { return m_dictionary.isValid(); }
    bool isUndefinedOrNull() const { return !m_dictionary.isValid(); }
    bool getOwnPropertiesAsStringHashMap(HashMap<String, String>&) const;
    bool getOwnPropertyNames(Vector<String>&) const;
    bool getWithUndefinedOrNullCheck(const String& propertyName, String& value) const;

private:
    template <typename T>
    JSC::JSObject* asJSObject(T*) const;
    
    JSDictionary m_dictionary;
};

template <typename Result>
bool Dictionary::get(const char* propertyName, Result& result) const
{
    if (!m_dictionary.isValid())
        return false;
    
    return m_dictionary.get(propertyName, result);
}

template <typename Result>
bool Dictionary::get(const String& propertyName, Result& result) const
{
    return get(propertyName.utf8().data(), result);
}

template <typename T>
PassRefPtr<EventListener> Dictionary::getEventListener(const char* propertyName, T* target) const
{
    if (!m_dictionary.isValid())
        return 0;

    ScriptValue eventListener;
    if (!m_dictionary.tryGetProperty(propertyName, eventListener))
        return 0;
    if (eventListener.hasNoValue())
        return 0;
    if (!eventListener.isObject())
        return 0;

    return JSEventListener::create(asObject(eventListener.jsValue()), asJSObject(target), true, currentWorld(m_dictionary.execState()));
}

template <typename T>
PassRefPtr<EventListener> Dictionary::getEventListener(const String& propertyName, T* target) const
{
    return getEventListener(propertyName.utf8().data(), target);
}

}

#endif // Dictionary_h
