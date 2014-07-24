# Copyright (c) 2013 Google Inc. All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are
# met:
#
#     * Redistributions of source code must retain the above copyright
# notice, this list of conditions and the following disclaimer.
#     * Redistributions in binary form must reproduce the above
# copyright notice, this list of conditions and the following disclaimer
# in the documentation and/or other materials provided with the
# distribution.
#     * Neither the name of Google Inc. nor the names of its
# contributors may be used to endorse or promote products derived from
# this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
# "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
# LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
# A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
# OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
# SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
# LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
# DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
# THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

# THis file contains string resources for CodeGeneratorInspector.
# Its syntax is a Python syntax subset, suitable for manual parsing.

frontend_domain_class = (
"""    class $domainClassName {
    public:
        $domainClassName(InspectorFrontendChannel* inspectorFrontendChannel) : m_inspectorFrontendChannel(inspectorFrontendChannel) { }
${frontendDomainMethodDeclarations}        void setInspectorFrontendChannel(InspectorFrontendChannel* inspectorFrontendChannel) { m_inspectorFrontendChannel = inspectorFrontendChannel; }
        InspectorFrontendChannel* getInspectorFrontendChannel() { return m_inspectorFrontendChannel; }
    private:
        InspectorFrontendChannel* m_inspectorFrontendChannel;
    };

    $domainClassName* $domainFieldName() { return &m_$domainFieldName; }

""")

backend_method = (
"""void InspectorBackendDispatcherImpl::${domainName}_$methodName(long callId, InspectorObject*$requestMessageObject)
{
    RefPtr<InspectorArray> protocolErrors = InspectorArray::create();

    if (!$agentField)
        protocolErrors->pushString("${domainName} handler is not available.");
$methodOutCode
$methodInCode
    RefPtr<InspectorObject> result = InspectorObject::create();
    ErrorString error;
    if (!protocolErrors->length()) {
        $agentField->$methodName(&error$agentCallParams);

${responseCook}
    }
    sendResponse(callId, result, commandNames[$commandNameIndex], protocolErrors, error);
}
""")

frontend_method = ("""void InspectorFrontend::$domainName::$eventName($parameters)
{
    RefPtr<InspectorObject> jsonMessage = InspectorObject::create();
    jsonMessage->setString("method", "$domainName.$eventName");
$code    if (m_inspectorFrontendChannel)
        m_inspectorFrontendChannel->sendMessageToFrontend(jsonMessage->toJSONString());
}
""")

callback_method = (
"""InspectorBackendDispatcher::$agentName::$callbackName::$callbackName(PassRefPtr<InspectorBackendDispatcherImpl> backendImpl, int id) : CallbackBase(backendImpl, id) {}

void InspectorBackendDispatcher::$agentName::$callbackName::sendSuccess($parameters)
{
    RefPtr<InspectorObject> jsonMessage = InspectorObject::create();
$code    sendIfActive(jsonMessage, ErrorString());
}
""")

frontend_h = (
"""#ifndef InspectorFrontend_h
#define InspectorFrontend_h

#include "InspectorTypeBuilder.h"
#include "InspectorValues.h"
#include <wtf/PassRefPtr.h>
#include <wtf/text/WTFString.h>

namespace WebCore {

class InspectorFrontendChannel;

// Both InspectorObject and InspectorArray may or may not be declared at this point as defined by ENABLED_INSPECTOR.
// Double-check we have them at least as forward declaration.
class InspectorArray;
class InspectorObject;

typedef String ErrorString;

#if ENABLE(INSPECTOR)

class InspectorFrontend {
public:
    InspectorFrontend(InspectorFrontendChannel*);


$domainClassList
private:
${fieldDeclarations}};

#endif // ENABLE(INSPECTOR)

} // namespace WebCore
#endif // !defined(InspectorFrontend_h)
""")

backend_h = (
"""#ifndef InspectorBackendDispatcher_h
#define InspectorBackendDispatcher_h

#include <wtf/PassRefPtr.h>
#include <wtf/RefCounted.h>
#include <wtf/text/WTFString.h>
#include "InspectorTypeBuilder.h"

namespace WebCore {

class InspectorAgent;
class InspectorObject;
class InspectorArray;
class InspectorFrontendChannel;

typedef String ErrorString;

class InspectorBackendDispatcherImpl;

class InspectorBackendDispatcher: public RefCounted<InspectorBackendDispatcher> {
public:
    static PassRefPtr<InspectorBackendDispatcher> create(InspectorFrontendChannel* inspectorFrontendChannel);
    virtual ~InspectorBackendDispatcher() { }

    class CallbackBase: public RefCounted<CallbackBase> {
    public:
        CallbackBase(PassRefPtr<InspectorBackendDispatcherImpl> backendImpl, int id);
        virtual ~CallbackBase();
        void sendFailure(const ErrorString&);
        bool isActive();

    protected:
        void sendIfActive(PassRefPtr<InspectorObject> partialMessage, const ErrorString& invocationError);

    private:
        void disable() { m_alreadySent = true; }

        RefPtr<InspectorBackendDispatcherImpl> m_backendImpl;
        int m_id;
        bool m_alreadySent;

        friend class InspectorBackendDispatcherImpl;
    };

$agentInterfaces
$virtualSetters

    virtual void clearFrontend() = 0;

    enum CommonErrorCode {
        ParseError = 0,
        InvalidRequest,
        MethodNotFound,
        InvalidParams,
        InternalError,
        ServerError,
        LastEntry,
    };

    void reportProtocolError(const long* const callId, CommonErrorCode, const String& errorMessage) const;
    virtual void reportProtocolError(const long* const callId, CommonErrorCode, const String& errorMessage, PassRefPtr<InspectorArray> data) const = 0;
    virtual void dispatch(const String& message) = 0;
    static bool getCommandName(const String& message, String* result);

    enum MethodNames {
$methodNamesEnumContent

        kMethodNamesEnumSize
    };

    static const char* commandNames[];
};

} // namespace WebCore
#endif // !defined(InspectorBackendDispatcher_h)


""")

backend_cpp = (
"""

#include "config.h"

#if ENABLE(INSPECTOR)

#include "InspectorBackendDispatcher.h"
#include <wtf/text/WTFString.h>
#include <wtf/text/CString.h>

#include "InspectorAgent.h"
#include "InspectorValues.h"
#include "InspectorFrontendChannel.h"
#include <wtf/text/WTFString.h>

namespace WebCore {

const char* InspectorBackendDispatcher::commandNames[] = {
$methodNameDeclarations
};


class InspectorBackendDispatcherImpl : public InspectorBackendDispatcher {
public:
    InspectorBackendDispatcherImpl(InspectorFrontendChannel* inspectorFrontendChannel)
        : m_inspectorFrontendChannel(inspectorFrontendChannel)
$constructorInit
    { }

    virtual void clearFrontend() { m_inspectorFrontendChannel = 0; }
    virtual void dispatch(const String& message);
    virtual void reportProtocolError(const long* const callId, CommonErrorCode, const String& errorMessage, PassRefPtr<InspectorArray> data) const;
    using InspectorBackendDispatcher::reportProtocolError;

    void sendResponse(long callId, PassRefPtr<InspectorObject> result, const ErrorString& invocationError);
    bool isActive() { return m_inspectorFrontendChannel; }

$setters
private:
$methodDeclarations

    InspectorFrontendChannel* m_inspectorFrontendChannel;
$fieldDeclarations

    template<typename R, typename V, typename V0>
    static R getPropertyValueImpl(InspectorObject* object, const String& name, bool* valueFound, InspectorArray* protocolErrors, V0 initial_value, bool (*as_method)(InspectorValue*, V*), const char* type_name);

    static int getInt(InspectorObject* object, const String& name, bool* valueFound, InspectorArray* protocolErrors);
    static double getDouble(InspectorObject* object, const String& name, bool* valueFound, InspectorArray* protocolErrors);
    static String getString(InspectorObject* object, const String& name, bool* valueFound, InspectorArray* protocolErrors);
    static bool getBoolean(InspectorObject* object, const String& name, bool* valueFound, InspectorArray* protocolErrors);
    static PassRefPtr<InspectorObject> getObject(InspectorObject* object, const String& name, bool* valueFound, InspectorArray* protocolErrors);
    static PassRefPtr<InspectorArray> getArray(InspectorObject* object, const String& name, bool* valueFound, InspectorArray* protocolErrors);

    void sendResponse(long callId, PassRefPtr<InspectorObject> result, const char* commandName, PassRefPtr<InspectorArray> protocolErrors, ErrorString invocationError);

};

$methods

PassRefPtr<InspectorBackendDispatcher> InspectorBackendDispatcher::create(InspectorFrontendChannel* inspectorFrontendChannel)
{
    return adoptRef(new InspectorBackendDispatcherImpl(inspectorFrontendChannel));
}


void InspectorBackendDispatcherImpl::dispatch(const String& message)
{
    RefPtr<InspectorBackendDispatcher> protect = this;
    typedef void (InspectorBackendDispatcherImpl::*CallHandler)(long callId, InspectorObject* messageObject);
    typedef HashMap<String, CallHandler> DispatchMap;
    DEFINE_STATIC_LOCAL(DispatchMap, dispatchMap, );
    long callId = 0;

    if (dispatchMap.isEmpty()) {
        static CallHandler handlers[] = {
$messageHandlers
        };
        size_t length = WTF_ARRAY_LENGTH(commandNames);
        for (size_t i = 0; i < length; ++i)
            dispatchMap.add(commandNames[i], handlers[i]);
    }

    RefPtr<InspectorValue> parsedMessage = InspectorValue::parseJSON(message);
    if (!parsedMessage) {
        reportProtocolError(0, ParseError, "Message must be in JSON format");
        return;
    }

    RefPtr<InspectorObject> messageObject = parsedMessage->asObject();
    if (!messageObject) {
        reportProtocolError(0, InvalidRequest, "Message must be a JSONified object");
        return;
    }

    RefPtr<InspectorValue> callIdValue = messageObject->get("id");
    if (!callIdValue) {
        reportProtocolError(0, InvalidRequest, "'id' property was not found");
        return;
    }

    if (!callIdValue->asNumber(&callId)) {
        reportProtocolError(0, InvalidRequest, "The type of 'id' property must be number");
        return;
    }

    RefPtr<InspectorValue> methodValue = messageObject->get("method");
    if (!methodValue) {
        reportProtocolError(&callId, InvalidRequest, "'method' property wasn't found");
        return;
    }

    String method;
    if (!methodValue->asString(&method)) {
        reportProtocolError(&callId, InvalidRequest, "The type of 'method' property must be string");
        return;
    }

    HashMap<String, CallHandler>::iterator it = dispatchMap.find(method);
    if (it == dispatchMap.end()) {
        reportProtocolError(&callId, MethodNotFound, "'" + method + "' wasn't found");
        return;
    }

    ((*this).*it->value)(callId, messageObject.get());
}

void InspectorBackendDispatcherImpl::sendResponse(long callId, PassRefPtr<InspectorObject> result, const char* commandName, PassRefPtr<InspectorArray> protocolErrors, ErrorString invocationError)
{
    if (protocolErrors->length()) {
        String errorMessage = String::format("Some arguments of method '%s' can't be processed", commandName);
        reportProtocolError(&callId, InvalidParams, errorMessage, protocolErrors);
        return;
    }
    sendResponse(callId, result, invocationError);
}

void InspectorBackendDispatcherImpl::sendResponse(long callId, PassRefPtr<InspectorObject> result, const ErrorString& invocationError)
{
    if (invocationError.length()) {
        reportProtocolError(&callId, ServerError, invocationError);
        return;
    }

    RefPtr<InspectorObject> responseMessage = InspectorObject::create();
    responseMessage->setObject("result", result);
    responseMessage->setNumber("id", callId);
    if (m_inspectorFrontendChannel)
        m_inspectorFrontendChannel->sendMessageToFrontend(responseMessage->toJSONString());
}

void InspectorBackendDispatcher::reportProtocolError(const long* const callId, CommonErrorCode code, const String& errorMessage) const
{
    reportProtocolError(callId, code, errorMessage, 0);
}

void InspectorBackendDispatcherImpl::reportProtocolError(const long* const callId, CommonErrorCode code, const String& errorMessage, PassRefPtr<InspectorArray> data) const
{
    DEFINE_STATIC_LOCAL(Vector<int>,s_commonErrors,);
    if (!s_commonErrors.size()) {
        s_commonErrors.insert(ParseError, -32700);
        s_commonErrors.insert(InvalidRequest, -32600);
        s_commonErrors.insert(MethodNotFound, -32601);
        s_commonErrors.insert(InvalidParams, -32602);
        s_commonErrors.insert(InternalError, -32603);
        s_commonErrors.insert(ServerError, -32000);
    }
    ASSERT(code >=0);
    ASSERT((unsigned)code < s_commonErrors.size());
    ASSERT(s_commonErrors[code]);
    RefPtr<InspectorObject> error = InspectorObject::create();
    error->setNumber("code", s_commonErrors[code]);
    error->setString("message", errorMessage);
    ASSERT(error);
    if (data)
        error->setArray("data", data);
    RefPtr<InspectorObject> message = InspectorObject::create();
    message->setObject("error", error);
    if (callId)
        message->setNumber("id", *callId);
    else
        message->setValue("id", InspectorValue::null());
    if (m_inspectorFrontendChannel)
        m_inspectorFrontendChannel->sendMessageToFrontend(message->toJSONString());
}

template<typename R, typename V, typename V0>
R InspectorBackendDispatcherImpl::getPropertyValueImpl(InspectorObject* object, const String& name, bool* valueFound, InspectorArray* protocolErrors, V0 initial_value, bool (*as_method)(InspectorValue*, V*), const char* type_name)
{
    ASSERT(protocolErrors);

    if (valueFound)
        *valueFound = false;

    V value = initial_value;

    if (!object) {
        if (!valueFound) {
            // Required parameter in missing params container.
            protocolErrors->pushString(String::format("'params' object must contain required parameter '%s' with type '%s'.", name.utf8().data(), type_name));
        }
        return value;
    }

    InspectorObject::const_iterator end = object->end();
    InspectorObject::const_iterator valueIterator = object->find(name);

    if (valueIterator == end) {
        if (!valueFound)
            protocolErrors->pushString(String::format("Parameter '%s' with type '%s' was not found.", name.utf8().data(), type_name));
        return value;
    }

    if (!as_method(valueIterator->value.get(), &value))
        protocolErrors->pushString(String::format("Parameter '%s' has wrong type. It must be '%s'.", name.utf8().data(), type_name));
    else
        if (valueFound)
            *valueFound = true;
    return value;
}

struct AsMethodBridges {
    static bool asInt(InspectorValue* value, int* output) { return value->asNumber(output); }
    static bool asDouble(InspectorValue* value, double* output) { return value->asNumber(output); }
    static bool asString(InspectorValue* value, String* output) { return value->asString(output); }
    static bool asBoolean(InspectorValue* value, bool* output) { return value->asBoolean(output); }
    static bool asObject(InspectorValue* value, RefPtr<InspectorObject>* output) { return value->asObject(output); }
    static bool asArray(InspectorValue* value, RefPtr<InspectorArray>* output) { return value->asArray(output); }
};

int InspectorBackendDispatcherImpl::getInt(InspectorObject* object, const String& name, bool* valueFound, InspectorArray* protocolErrors)
{
    return getPropertyValueImpl<int, int, int>(object, name, valueFound, protocolErrors, 0, AsMethodBridges::asInt, "Number");
}

double InspectorBackendDispatcherImpl::getDouble(InspectorObject* object, const String& name, bool* valueFound, InspectorArray* protocolErrors)
{
    return getPropertyValueImpl<double, double, double>(object, name, valueFound, protocolErrors, 0, AsMethodBridges::asDouble, "Number");
}

String InspectorBackendDispatcherImpl::getString(InspectorObject* object, const String& name, bool* valueFound, InspectorArray* protocolErrors)
{
    return getPropertyValueImpl<String, String, String>(object, name, valueFound, protocolErrors, "", AsMethodBridges::asString, "String");
}

bool InspectorBackendDispatcherImpl::getBoolean(InspectorObject* object, const String& name, bool* valueFound, InspectorArray* protocolErrors)
{
    return getPropertyValueImpl<bool, bool, bool>(object, name, valueFound, protocolErrors, false, AsMethodBridges::asBoolean, "Boolean");
}

PassRefPtr<InspectorObject> InspectorBackendDispatcherImpl::getObject(InspectorObject* object, const String& name, bool* valueFound, InspectorArray* protocolErrors)
{
    return getPropertyValueImpl<PassRefPtr<InspectorObject>, RefPtr<InspectorObject>, InspectorObject*>(object, name, valueFound, protocolErrors, 0, AsMethodBridges::asObject, "Object");
}

PassRefPtr<InspectorArray> InspectorBackendDispatcherImpl::getArray(InspectorObject* object, const String& name, bool* valueFound, InspectorArray* protocolErrors)
{
    return getPropertyValueImpl<PassRefPtr<InspectorArray>, RefPtr<InspectorArray>, InspectorArray*>(object, name, valueFound, protocolErrors, 0, AsMethodBridges::asArray, "Array");
}

bool InspectorBackendDispatcher::getCommandName(const String& message, String* result)
{
    RefPtr<InspectorValue> value = InspectorValue::parseJSON(message);
    if (!value)
        return false;

    RefPtr<InspectorObject> object = value->asObject();
    if (!object)
        return false;

    if (!object->getString("method", result))
        return false;

    return true;
}

InspectorBackendDispatcher::CallbackBase::CallbackBase(PassRefPtr<InspectorBackendDispatcherImpl> backendImpl, int id)
    : m_backendImpl(backendImpl), m_id(id), m_alreadySent(false) {}

InspectorBackendDispatcher::CallbackBase::~CallbackBase() {}

void InspectorBackendDispatcher::CallbackBase::sendFailure(const ErrorString& error)
{
    ASSERT(error.length());
    sendIfActive(0, error);
}

bool InspectorBackendDispatcher::CallbackBase::isActive()
{
    return !m_alreadySent && m_backendImpl->isActive();
}

void InspectorBackendDispatcher::CallbackBase::sendIfActive(PassRefPtr<InspectorObject> partialMessage, const ErrorString& invocationError)
{
    if (m_alreadySent)
        return;
    m_backendImpl->sendResponse(m_id, partialMessage, invocationError);
    m_alreadySent = true;
}

COMPILE_ASSERT(static_cast<int>(InspectorBackendDispatcher::kMethodNamesEnumSize) == WTF_ARRAY_LENGTH(InspectorBackendDispatcher::commandNames), command_name_array_problem);

} // namespace WebCore

#endif // ENABLE(INSPECTOR)
""")

frontend_cpp = (
"""

#include "config.h"
#if ENABLE(INSPECTOR)

#include "InspectorFrontend.h"
#include <wtf/text/WTFString.h>
#include <wtf/text/CString.h>

#include "InspectorFrontendChannel.h"
#include "InspectorValues.h"
#include <wtf/text/WTFString.h>

namespace WebCore {

InspectorFrontend::InspectorFrontend(InspectorFrontendChannel* inspectorFrontendChannel)
    : $constructorInit{
}

$methods

} // namespace WebCore

#endif // ENABLE(INSPECTOR)
""")

typebuilder_h = (
"""
#ifndef InspectorTypeBuilder_h
#define InspectorTypeBuilder_h

#if ENABLE(INSPECTOR)

#include "InspectorValues.h"
#include <wtf/Assertions.h>
#include <wtf/PassRefPtr.h>

namespace WebCore {

namespace TypeBuilder {

template<typename T>
class OptOutput {
public:
    OptOutput() : m_assigned(false) { }

    void operator=(T value)
    {
        m_value = value;
        m_assigned = true;
    }

    bool isAssigned() { return m_assigned; }

    T getValue()
    {
        ASSERT(isAssigned());
        return m_value;
    }

private:
    T m_value;
    bool m_assigned;

    WTF_MAKE_NONCOPYABLE(OptOutput);
};


// A small transient wrapper around int type, that can be used as a funciton parameter type
// cleverly disallowing C++ implicit casts from float or double.
class ExactlyInt {
public:
    template<typename T>
    ExactlyInt(T t) : m_value(cast_to_int<T>(t)) {}

    ExactlyInt() {}

    operator int() { return m_value; }
private:
    int m_value;

    template<typename T>
    static int cast_to_int(T) { return T::default_case_cast_is_not_supported(); }
};

template<>
inline int ExactlyInt::cast_to_int<int>(int i) { return i; }

template<>
inline int ExactlyInt::cast_to_int<unsigned int>(unsigned int i) { return i; }

class RuntimeCastHelper {
public:
#if $validatorIfdefName
    template<InspectorValue::Type TYPE>
    static void assertType(InspectorValue* value)
    {
        ASSERT(value->type() == TYPE);
    }
    static void assertAny(InspectorValue*);
    static void assertInt(InspectorValue* value);
#endif
};


// This class provides "Traits" type for the input type T. It is programmed using C++ template specialization
// technique. By default it simply takes "ItemTraits" type from T, but it doesn't work with the base types.
template<typename T>
struct ArrayItemHelper {
    typedef typename T::ItemTraits Traits;
};

template<typename T>
class Array : public InspectorArrayBase {
private:
    Array() { }

    InspectorArray* openAccessors() {
        COMPILE_ASSERT(sizeof(InspectorArray) == sizeof(Array<T>), cannot_cast);
        return static_cast<InspectorArray*>(static_cast<InspectorArrayBase*>(this));
    }

public:
    void addItem(PassRefPtr<T> value)
    {
        ArrayItemHelper<T>::Traits::pushRefPtr(this->openAccessors(), value);
    }

    void addItem(T value)
    {
        ArrayItemHelper<T>::Traits::pushRaw(this->openAccessors(), value);
    }

    static PassRefPtr<Array<T> > create()
    {
        return adoptRef(new Array<T>());
    }

    static PassRefPtr<Array<T> > runtimeCast(PassRefPtr<InspectorValue> value)
    {
        RefPtr<InspectorArray> array;
        bool castRes = value->asArray(&array);
        ASSERT_UNUSED(castRes, castRes);
#if $validatorIfdefName
        assertCorrectValue(array.get());
#endif  // $validatorIfdefName
        COMPILE_ASSERT(sizeof(Array<T>) == sizeof(InspectorArray), type_cast_problem);
        return static_cast<Array<T>*>(static_cast<InspectorArrayBase*>(array.get()));
    }

#if $validatorIfdefName
    static void assertCorrectValue(InspectorValue* value)
    {
        RefPtr<InspectorArray> array;
        bool castRes = value->asArray(&array);
        ASSERT_UNUSED(castRes, castRes);
        for (unsigned i = 0; i < array->length(); i++)
            ArrayItemHelper<T>::Traits::template assertCorrectValue<T>(array->get(i).get());
    }

#endif // $validatorIfdefName
};

struct StructItemTraits {
    static void pushRefPtr(InspectorArray* array, PassRefPtr<InspectorValue> value)
    {
        array->pushValue(value);
    }

#if $validatorIfdefName
    template<typename T>
    static void assertCorrectValue(InspectorValue* value) {
        T::assertCorrectValue(value);
    }
#endif  // $validatorIfdefName
};

template<>
struct ArrayItemHelper<String> {
    struct Traits {
        static void pushRaw(InspectorArray* array, const String& value)
        {
            array->pushString(value);
        }

#if $validatorIfdefName
        template<typename T>
        static void assertCorrectValue(InspectorValue* value) {
            RuntimeCastHelper::assertType<InspectorValue::TypeString>(value);
        }
#endif  // $validatorIfdefName
    };
};

template<>
struct ArrayItemHelper<int> {
    struct Traits {
        static void pushRaw(InspectorArray* array, int value)
        {
            array->pushInt(value);
        }

#if $validatorIfdefName
        template<typename T>
        static void assertCorrectValue(InspectorValue* value) {
            RuntimeCastHelper::assertInt(value);
        }
#endif  // $validatorIfdefName
    };
};

template<>
struct ArrayItemHelper<double> {
    struct Traits {
        static void pushRaw(InspectorArray* array, double value)
        {
            array->pushNumber(value);
        }

#if $validatorIfdefName
        template<typename T>
        static void assertCorrectValue(InspectorValue* value) {
            RuntimeCastHelper::assertType<InspectorValue::TypeNumber>(value);
        }
#endif  // $validatorIfdefName
    };
};

template<>
struct ArrayItemHelper<bool> {
    struct Traits {
        static void pushRaw(InspectorArray* array, bool value)
        {
            array->pushBoolean(value);
        }

#if $validatorIfdefName
        template<typename T>
        static void assertCorrectValue(InspectorValue* value) {
            RuntimeCastHelper::assertType<InspectorValue::TypeBoolean>(value);
        }
#endif  // $validatorIfdefName
    };
};

template<>
struct ArrayItemHelper<InspectorValue> {
    struct Traits {
        static void pushRefPtr(InspectorArray* array, PassRefPtr<InspectorValue> value)
        {
            array->pushValue(value);
        }

#if $validatorIfdefName
        template<typename T>
        static void assertCorrectValue(InspectorValue* value) {
            RuntimeCastHelper::assertAny(value);
        }
#endif  // $validatorIfdefName
    };
};

template<>
struct ArrayItemHelper<InspectorObject> {
    struct Traits {
        static void pushRefPtr(InspectorArray* array, PassRefPtr<InspectorValue> value)
        {
            array->pushValue(value);
        }

#if $validatorIfdefName
        template<typename T>
        static void assertCorrectValue(InspectorValue* value) {
            RuntimeCastHelper::assertType<InspectorValue::TypeObject>(value);
        }
#endif  // $validatorIfdefName
    };
};

template<>
struct ArrayItemHelper<InspectorArray> {
    struct Traits {
        static void pushRefPtr(InspectorArray* array, PassRefPtr<InspectorArray> value)
        {
            array->pushArray(value);
        }

#if $validatorIfdefName
        template<typename T>
        static void assertCorrectValue(InspectorValue* value) {
            RuntimeCastHelper::assertType<InspectorValue::TypeArray>(value);
        }
#endif  // $validatorIfdefName
    };
};

template<typename T>
struct ArrayItemHelper<TypeBuilder::Array<T> > {
    struct Traits {
        static void pushRefPtr(InspectorArray* array, PassRefPtr<TypeBuilder::Array<T> > value)
        {
            array->pushValue(value);
        }

#if $validatorIfdefName
        template<typename S>
        static void assertCorrectValue(InspectorValue* value) {
            S::assertCorrectValue(value);
        }
#endif  // $validatorIfdefName
    };
};

${forwards}

String getEnumConstantValue(int code);

${typeBuilders}
} // namespace TypeBuilder


} // namespace WebCore

#endif // ENABLE(INSPECTOR)

#endif // !defined(InspectorTypeBuilder_h)

""")

typebuilder_cpp = (
"""

#include "config.h"
#if ENABLE(INSPECTOR)

#include "InspectorTypeBuilder.h"
#include <wtf/text/CString.h>

namespace WebCore {

namespace TypeBuilder {

const char* const enum_constant_values[] = {
$enumConstantValues};

String getEnumConstantValue(int code) {
    return enum_constant_values[code];
}

} // namespace TypeBuilder

$implCode

#if $validatorIfdefName

void TypeBuilder::RuntimeCastHelper::assertAny(InspectorValue*)
{
    // No-op.
}


void TypeBuilder::RuntimeCastHelper::assertInt(InspectorValue* value)
{
    double v;
    bool castRes = value->asNumber(&v);
    ASSERT_UNUSED(castRes, castRes);
    ASSERT(static_cast<double>(static_cast<int>(v)) == v);
}

$validatorCode

#endif // $validatorIfdefName

} // namespace WebCore

#endif // ENABLE(INSPECTOR)
""")

backend_js = (
"""

$domainInitializers
""")

param_container_access_code = """
    RefPtr<InspectorObject> paramsContainer = requestMessageObject->getObject("params");
    InspectorObject* paramsContainerPtr = paramsContainer.get();
    InspectorArray* protocolErrorsPtr = protocolErrors.get();
"""

class_binding_builder_part_1 = (
"""        AllFieldsSet = %s
    };

    template<int STATE>
    class Builder {
    private:
        RefPtr<InspectorObject> m_result;

        template<int STEP> Builder<STATE | STEP>& castState()
        {
            return *reinterpret_cast<Builder<STATE | STEP>*>(this);
        }

        Builder(PassRefPtr</*%s*/InspectorObject> ptr)
        {
            COMPILE_ASSERT(STATE == NoFieldsSet, builder_created_in_non_init_state);
            m_result = ptr;
        }
        friend class %s;
    public:
""")

class_binding_builder_part_2 = ("""
        Builder<STATE | %s>& set%s(%s value)
        {
            COMPILE_ASSERT(!(STATE & %s), property_%s_already_set);
            m_result->set%s("%s", %s);
            return castState<%s>();
        }
""")

class_binding_builder_part_3 = ("""
        operator RefPtr<%s>& ()
        {
            COMPILE_ASSERT(STATE == AllFieldsSet, result_is_not_ready);
            COMPILE_ASSERT(sizeof(%s) == sizeof(InspectorObject), cannot_cast);
            return *reinterpret_cast<RefPtr<%s>*>(&m_result);
        }

        PassRefPtr<%s> release()
        {
            return RefPtr<%s>(*this).release();
        }
    };

""")

class_binding_builder_part_4 = (
"""    static Builder<NoFieldsSet> create()
    {
        return Builder<NoFieldsSet>(InspectorObject::create());
    }
""")
