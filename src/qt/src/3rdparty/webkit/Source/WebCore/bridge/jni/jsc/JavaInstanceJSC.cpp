/*
 * Copyright (C) 2003, 2008, 2010 Apple Inc. All rights reserved.
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

#include "config.h"
#include "JavaInstanceJSC.h"

#if ENABLE(JAVA_BRIDGE)

#include "JavaRuntimeObject.h"
#include "JNIUtilityPrivate.h"
#include "JSDOMBinding.h"
#include "JavaArrayJSC.h"
#include "JavaClassJSC.h"
#include "JavaMethod.h"
#include "JavaString.h"
#include "Logging.h"
#include "jni_jsobject.h"
#include "runtime_method.h"
#include "runtime_object.h"
#include "runtime_root.h"
#include <runtime/ArgList.h>
#include <runtime/Error.h>
#include <runtime/FunctionPrototype.h>
#include <runtime/JSLock.h>

using namespace JSC::Bindings;
using namespace JSC;
using namespace WebCore;

JavaInstance::JavaInstance(jobject instance, PassRefPtr<RootObject> rootObject)
    : Instance(rootObject)
{
    m_instance = new JobjectWrapper(instance);
    m_class = 0;
}

JavaInstance::~JavaInstance()
{
    delete m_class;
}

RuntimeObject* JavaInstance::newRuntimeObject(ExecState* exec)
{
    return new (exec) JavaRuntimeObject(exec, exec->lexicalGlobalObject(), this);
}

#define NUM_LOCAL_REFS 64

void JavaInstance::virtualBegin()
{
    getJNIEnv()->PushLocalFrame(NUM_LOCAL_REFS);
}

void JavaInstance::virtualEnd()
{
    getJNIEnv()->PopLocalFrame(0);
}

Class* JavaInstance::getClass() const
{
    if (!m_class)
        m_class = new JavaClass (m_instance->m_instance);
    return m_class;
}

JSValue JavaInstance::stringValue(ExecState* exec) const
{
    JSLock lock(SilenceAssertionsOnly);

    jstring stringValue = (jstring)callJNIMethod<jobject>(m_instance->m_instance, "toString", "()Ljava/lang/String;");

    // Should throw a JS exception, rather than returning ""? - but better than a null dereference.
    if (!stringValue)
        return jsString(exec, UString());

    JNIEnv* env = getJNIEnv();
    const jchar* c = getUCharactersFromJStringInEnv(env, stringValue);
    UString u((const UChar*)c, (int)env->GetStringLength(stringValue));
    releaseUCharactersForJStringInEnv(env, stringValue, c);
    return jsString(exec, u);
}

JSValue JavaInstance::numberValue(ExecState*) const
{
    jdouble doubleValue = callJNIMethod<jdouble>(m_instance->m_instance, "doubleValue", "()D");
    return jsNumber(doubleValue);
}

JSValue JavaInstance::booleanValue() const
{
    jboolean booleanValue = callJNIMethod<jboolean>(m_instance->m_instance, "booleanValue", "()Z");
    return jsBoolean(booleanValue);
}

class JavaRuntimeMethod : public RuntimeMethod {
public:
    JavaRuntimeMethod(ExecState* exec, JSGlobalObject* globalObject, const Identifier& name, Bindings::MethodList& list)
        // FIXME: deprecatedGetDOMStructure uses the prototype off of the wrong global object
        // We need to pass in the right global object for "i".
        : RuntimeMethod(exec, globalObject, WebCore::deprecatedGetDOMStructure<JavaRuntimeMethod>(exec), name, list)
    {
        ASSERT(inherits(&s_info));
    }

    static Structure* createStructure(JSGlobalData& globalData, JSValue prototype)
    {
        return Structure::create(globalData, prototype, TypeInfo(ObjectType, StructureFlags), AnonymousSlotCount, &s_info);
    }

    static const ClassInfo s_info;
};

const ClassInfo JavaRuntimeMethod::s_info = { "JavaRuntimeMethod", &RuntimeMethod::s_info, 0, 0 };

JSValue JavaInstance::getMethod(ExecState* exec, const Identifier& propertyName)
{
    MethodList methodList = getClass()->methodsNamed(propertyName, this);
    return new (exec) JavaRuntimeMethod(exec, exec->lexicalGlobalObject(), propertyName, methodList);
}

JSValue JavaInstance::invokeMethod(ExecState* exec, RuntimeMethod* runtimeMethod)
{
    if (!asObject(runtimeMethod)->inherits(&JavaRuntimeMethod::s_info))
        return throwError(exec, createTypeError(exec, "Attempt to invoke non-Java method on Java object."));

    const MethodList& methodList = *runtimeMethod->methods();

    int i;
    int count = exec->argumentCount();
    JSValue resultValue;
    Method* method = 0;
    size_t numMethods = methodList.size();

    // Try to find a good match for the overloaded method.  The
    // fundamental problem is that JavaScript doesn't have the
    // notion of method overloading and Java does.  We could
    // get a bit more sophisticated and attempt to does some
    // type checking as we as checking the number of parameters.
    for (size_t methodIndex = 0; methodIndex < numMethods; methodIndex++) {
        Method* aMethod = methodList[methodIndex];
        if (aMethod->numParameters() == count) {
            method = aMethod;
            break;
        }
    }
    if (!method) {
        LOG(LiveConnect, "JavaInstance::invokeMethod unable to find an appropiate method");
        return jsUndefined();
    }

    const JavaMethod* jMethod = static_cast<const JavaMethod*>(method);
    LOG(LiveConnect, "JavaInstance::invokeMethod call %s %s on %p", UString(jMethod->name().impl()).utf8().data(), jMethod->signature(), m_instance->m_instance);

    Vector<jvalue> jArgs(count);

    for (i = 0; i < count; i++) {
        CString javaClassName = jMethod->parameterAt(i).utf8();
        jArgs[i] = convertValueToJValue(exec, m_rootObject.get(), exec->argument(i), javaTypeFromClassName(javaClassName.data()), javaClassName.data());
        LOG(LiveConnect, "JavaInstance::invokeMethod arg[%d] = %s", i, exec->argument(i).toString(exec).ascii().data());
    }

    jvalue result;

    // Try to use the JNI abstraction first, otherwise fall back to
    // normal JNI.  The JNI dispatch abstraction allows the Java plugin
    // to dispatch the call on the appropriate internal VM thread.
    RootObject* rootObject = this->rootObject();
    if (!rootObject)
        return jsUndefined();

    bool handled = false;
    if (rootObject->nativeHandle()) {
        jobject obj = m_instance->m_instance;
        JSValue exceptionDescription;
        const char *callingURL = 0; // FIXME, need to propagate calling URL to Java
        jmethodID methodId = getMethodID(obj, jMethod->name().utf8().data(), jMethod->signature());
        handled = dispatchJNICall(exec, rootObject->nativeHandle(), obj, jMethod->isStatic(), jMethod->returnType(), methodId, jArgs.data(), result, callingURL, exceptionDescription);
        if (exceptionDescription) {
            throwError(exec, createError(exec, exceptionDescription.toString(exec)));
            return jsUndefined();
        }
    }

// This is a deprecated code path which should not be required on Android.
// Remove this guard once Bug 39476 is fixed.
#if PLATFORM(ANDROID)
    if (!handled)
        result = callJNIMethod(m_instance->m_instance, jMethod->returnType(), jMethod->name().utf8().data(), jMethod->signature(), jArgs.data());
#endif

    switch (jMethod->returnType()) {
    case JavaTypeVoid:
        {
            resultValue = jsUndefined();
        }
        break;

    case JavaTypeObject:
        {
            if (result.l) {
                // FIXME: JavaTypeArray return type is handled below, can we actually get an array here?
                const char* arrayType = jMethod->returnTypeClassName();
                if (arrayType[0] == '[')
                    resultValue = JavaArray::convertJObjectToArray(exec, result.l, arrayType, rootObject);
                else {
                    jobject classOfInstance = callJNIMethod<jobject>(result.l, "getClass", "()Ljava/lang/Class;");
                    jstring className = static_cast<jstring>(callJNIMethod<jobject>(classOfInstance, "getName", "()Ljava/lang/String;"));
                    if (!strcmp(JavaString(className).utf8(), "sun.plugin.javascript.webkit.JSObject")) {
                        // Pull the nativeJSObject value from the Java instance.  This is a pointer to the JSObject.
                        JNIEnv* env = getJNIEnv();
                        jfieldID fieldID = env->GetFieldID(static_cast<jclass>(classOfInstance), "nativeJSObject", "J");
                        jlong nativeHandle = env->GetLongField(result.l, fieldID);
                        // FIXME: Handling of undefined values differs between functions in JNIUtilityPrivate.cpp and those in those in jni_jsobject.mm,
                        // and so it does between different versions of LiveConnect spec. There should not be multiple code paths to do the same work.
                        if (nativeHandle == 1 /* UndefinedHandle */)
                            return jsUndefined();
                        return static_cast<JSObject*>(jlong_to_ptr(nativeHandle));
                    } else
                        return JavaInstance::create(result.l, rootObject)->createRuntimeObject(exec);
                }
            } else
                return jsUndefined();
        }
        break;

    case JavaTypeBoolean:
        {
            resultValue = jsBoolean(result.z);
        }
        break;

    case JavaTypeByte:
        {
            resultValue = jsNumber(result.b);
        }
        break;

    case JavaTypeChar:
        {
            resultValue = jsNumber(result.c);
        }
        break;

    case JavaTypeShort:
        {
            resultValue = jsNumber(result.s);
        }
        break;

    case JavaTypeInt:
        {
            resultValue = jsNumber(result.i);
        }
        break;

    case JavaTypeLong:
        {
            resultValue = jsNumber(result.j);
        }
        break;

    case JavaTypeFloat:
        {
            resultValue = jsNumber(result.f);
        }
        break;

    case JavaTypeDouble:
        {
            resultValue = jsNumber(result.d);
        }
        break;

    case JavaTypeArray:
        {
            const char* arrayType = jMethod->returnTypeClassName();
            ASSERT(arrayType[0] == '[');
            resultValue = JavaArray::convertJObjectToArray(exec, result.l, arrayType, rootObject);
        }
        break;

    case JavaTypeInvalid:
        {
            resultValue = jsUndefined();
        }
        break;
    }

    return resultValue;
}

JSValue JavaInstance::defaultValue(ExecState* exec, PreferredPrimitiveType hint) const
{
    if (hint == PreferString)
        return stringValue(exec);
    if (hint == PreferNumber)
        return numberValue(exec);
    JavaClass* aClass = static_cast<JavaClass*>(getClass());
    if (aClass->isStringClass())
        return stringValue(exec);
    if (aClass->isNumberClass())
        return numberValue(exec);
    if (aClass->isBooleanClass())
        return booleanValue();
    return valueOf(exec);
}

JSValue JavaInstance::valueOf(ExecState* exec) const
{
    return stringValue(exec);
}

#endif // ENABLE(JAVA_BRIDGE)
