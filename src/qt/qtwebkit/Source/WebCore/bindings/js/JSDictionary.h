/*
 * Copyright (C) 2011 Apple Inc. All rights reserved.
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

#ifndef JSDictionary_h
#define JSDictionary_h

#include "MessagePort.h"
#include <heap/Strong.h>
#include <heap/StrongInlines.h>
#include <interpreter/CallFrame.h>
#include <runtime/Operations.h>
#include <wtf/Forward.h>

namespace WebCore {

class ArrayValue;
class CSSFontFaceRule;
class Dictionary;
class DOMError;
class DOMWindow;
class EventTarget;
class MediaKeyError;
class MediaStream;
class Node;
class ScriptValue;
class SerializedScriptValue;
class Storage;
class TrackBase;
class VoidCallback;

#if ENABLE(SCRIPTED_SPEECH)
class SpeechRecognitionResultList;
#endif

class JSDictionary {
public:
    JSDictionary(JSC::ExecState* exec, JSC::JSObject* initializerObject)
        : m_exec(exec)
    {
        if (exec && initializerObject)
            m_initializerObject = JSC::Strong<JSC::JSObject>(exec->vm(), initializerObject);
    }

    // Returns false if any exceptions were thrown, regardless of whether the property was found.
    template <typename Result>
    bool tryGetProperty(const char* propertyName, Result&) const;
    template <typename T, typename Result>
    bool tryGetProperty(const char* propertyName, T* context, void (*setter)(T* context, const Result&)) const;

    // Returns true if the property was found in the dictionary, and the value could be converted to the desired type.
    template <typename Result>
    bool get(const char* propertyName, Result&) const;
    bool getWithUndefinedOrNullCheck(const String& propertyName, String& value) const;

    JSC::ExecState* execState() const { return m_exec; }
    JSC::JSObject* initializerObject() const { return m_initializerObject.get(); }
    bool isValid() const { return m_exec && m_initializerObject; }

private:
    template <typename Result>
    struct IdentitySetter {
        static void identitySetter(Result* context, const Result& result)
        {
            *context = result;
        }
    };

    enum GetPropertyResult {
        ExceptionThrown,
        NoPropertyFound,
        PropertyFound
    };
    
    template <typename T, typename Result>
    GetPropertyResult tryGetPropertyAndResult(const char* propertyName, T* context, void (*setter)(T* context, const Result&)) const;
    GetPropertyResult tryGetProperty(const char* propertyName, JSC::JSValue&) const;

    static void convertValue(JSC::ExecState*, JSC::JSValue, bool& result);
    static void convertValue(JSC::ExecState*, JSC::JSValue, int& result);
    static void convertValue(JSC::ExecState*, JSC::JSValue, unsigned& result);
    static void convertValue(JSC::ExecState*, JSC::JSValue, unsigned short& result);
    static void convertValue(JSC::ExecState*, JSC::JSValue, unsigned long& result);
    static void convertValue(JSC::ExecState*, JSC::JSValue, unsigned long long& result);
    static void convertValue(JSC::ExecState*, JSC::JSValue, double& result);
    static void convertValue(JSC::ExecState*, JSC::JSValue, Dictionary& result);
    static void convertValue(JSC::ExecState*, JSC::JSValue, String& result);
    static void convertValue(JSC::ExecState*, JSC::JSValue, ScriptValue& result);
    static void convertValue(JSC::ExecState*, JSC::JSValue, Vector<String>& result);
    static void convertValue(JSC::ExecState*, JSC::JSValue, RefPtr<SerializedScriptValue>& result);
    static void convertValue(JSC::ExecState*, JSC::JSValue, RefPtr<DOMWindow>& result);
    static void convertValue(JSC::ExecState*, JSC::JSValue, RefPtr<EventTarget>& result);
    static void convertValue(JSC::ExecState*, JSC::JSValue, RefPtr<Node>& result);
    static void convertValue(JSC::ExecState*, JSC::JSValue, RefPtr<Storage>& result);
    static void convertValue(JSC::ExecState*, JSC::JSValue, MessagePortArray& result);
#if ENABLE(VIDEO_TRACK)
    static void convertValue(JSC::ExecState*, JSC::JSValue, RefPtr<TrackBase>& result);
#endif
    static void convertValue(JSC::ExecState*, JSC::JSValue, HashSet<AtomicString>& result);
    static void convertValue(JSC::ExecState*, JSC::JSValue, ArrayValue& result);
    static void convertValue(JSC::ExecState*, JSC::JSValue, RefPtr<Uint8Array>& result);
#if ENABLE(ENCRYPTED_MEDIA)
    static void convertValue(JSC::ExecState*, JSC::JSValue, RefPtr<MediaKeyError>& result);
#endif
#if ENABLE(MEDIA_STREAM)
    static void convertValue(JSC::ExecState*, JSC::JSValue, RefPtr<MediaStream>& result);
#endif
#if ENABLE(FONT_LOAD_EVENTS)
    static void convertValue(JSC::ExecState*, JSC::JSValue, RefPtr<CSSFontFaceRule>& result);
    static void convertValue(JSC::ExecState*, JSC::JSValue, RefPtr<DOMError>& result);
    static void convertValue(JSC::ExecState*, JSC::JSValue, RefPtr<VoidCallback>& result);
#endif
#if ENABLE(SCRIPTED_SPEECH)
    static void convertValue(JSC::ExecState*, JSC::JSValue, RefPtr<SpeechRecognitionResultList>&);
#endif

    JSC::ExecState* m_exec;
    JSC::Strong<JSC::JSObject> m_initializerObject;
};

template <typename T, typename Result>
bool JSDictionary::tryGetProperty(const char* propertyName, T* context, void (*setter)(T* context, const Result&)) const
{
    return tryGetPropertyAndResult(propertyName, context, setter) != ExceptionThrown;
}

template <typename Result>
bool JSDictionary::tryGetProperty(const char* propertyName, Result& finalResult) const
{
    return tryGetPropertyAndResult(propertyName, &finalResult, IdentitySetter<Result>::identitySetter) != ExceptionThrown;
}

template <typename Result>
bool JSDictionary::get(const char* propertyName, Result& finalResult) const
{
    return tryGetPropertyAndResult(propertyName, &finalResult, IdentitySetter<Result>::identitySetter) == PropertyFound;
}

template <typename T, typename Result>
JSDictionary::GetPropertyResult JSDictionary::tryGetPropertyAndResult(const char* propertyName, T* context, void (*setter)(T* context, const Result&)) const
{
    JSC::JSValue value;
    GetPropertyResult getPropertyResult = tryGetProperty(propertyName, value);
    switch (getPropertyResult) {
    case ExceptionThrown:
        return getPropertyResult;
    case PropertyFound: {
        Result result;
        convertValue(m_exec, value, result);

        if (m_exec->hadException())
            return ExceptionThrown;

        setter(context, result);
        break;
    }
    case NoPropertyFound:
        break;
    }

    return getPropertyResult;
}

} // namespace WebCore

#endif // JSDictionary_h
