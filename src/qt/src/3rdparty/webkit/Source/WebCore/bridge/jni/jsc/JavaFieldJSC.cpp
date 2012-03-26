/*
 * Copyright (C) 2003, 2004, 2005, 2007, 2009 Apple Inc. All rights reserved.
 * Copyright 2010, The Android Open Source Project
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
#include "JavaFieldJSC.h"

#if ENABLE(JAVA_BRIDGE)

#include "JNIUtilityPrivate.h"
#include "JavaArrayJSC.h"
#include "Logging.h"
#include "runtime_array.h"
#include "runtime_object.h"
#include <runtime/Error.h>

using namespace JSC;
using namespace JSC::Bindings;
using namespace WebCore;

JavaField::JavaField(JNIEnv* env, jobject aField)
{
    // Get field type name
    jstring fieldTypeName = 0;
    if (jobject fieldType = callJNIMethod<jobject>(aField, "getType", "()Ljava/lang/Class;"))
        fieldTypeName = static_cast<jstring>(callJNIMethod<jobject>(fieldType, "getName", "()Ljava/lang/String;"));
    if (!fieldTypeName)
        fieldTypeName = env->NewStringUTF("<Unknown>");
    m_typeClassName = JavaString(env, fieldTypeName);

    m_type = javaTypeFromClassName(m_typeClassName.utf8());

    // Get field name
    jstring fieldName = static_cast<jstring>(callJNIMethod<jobject>(aField, "getName", "()Ljava/lang/String;"));
    if (!fieldName)
        fieldName = env->NewStringUTF("<Unknown>");
    m_name = JavaString(env, fieldName);

    m_field = new JobjectWrapper(aField);
}

jvalue JavaField::dispatchValueFromInstance(ExecState* exec, const JavaInstance* instance, const char* name, const char* sig, JavaType returnType) const
{
    jobject jinstance = instance->javaInstance();
    jobject fieldJInstance = m_field->m_instance;
    JNIEnv* env = getJNIEnv();
    jvalue result;

    memset(&result, 0, sizeof(jvalue));
    jclass cls = env->GetObjectClass(fieldJInstance);
    if (cls) {
        jmethodID mid = env->GetMethodID(cls, name, sig);
        if (mid) {
            RootObject* rootObject = instance->rootObject();
            if (rootObject && rootObject->nativeHandle()) {
                JSValue exceptionDescription;
                jvalue args[1];

                args[0].l = jinstance;
                dispatchJNICall(exec, rootObject->nativeHandle(), fieldJInstance, false, returnType, mid, args, result, 0, exceptionDescription);
                if (exceptionDescription)
                    throwError(exec, createError(exec, exceptionDescription.toString(exec)));
            }
        }
    }
    return result;
}

JSValue JavaField::valueFromInstance(ExecState* exec, const Instance* i) const
{
    const JavaInstance* instance = static_cast<const JavaInstance*>(i);

    JSValue jsresult = jsUndefined();

    switch (m_type) {
    case JavaTypeArray:
    case JavaTypeObject:
        {
            jvalue result = dispatchValueFromInstance(exec, instance, "get", "(Ljava/lang/Object;)Ljava/lang/Object;", JavaTypeObject);
            jobject anObject = result.l;

            if (!anObject)
                return jsNull();

            const char* arrayType = typeClassName();
            if (arrayType[0] == '[')
                jsresult = JavaArray::convertJObjectToArray(exec, anObject, arrayType, instance->rootObject());
            else if (anObject)
                jsresult = JavaInstance::create(anObject, instance->rootObject())->createRuntimeObject(exec);
        }
        break;

    case JavaTypeBoolean:
        jsresult = jsBoolean(dispatchValueFromInstance(exec, instance, "getBoolean", "(Ljava/lang/Object;)Z", JavaTypeBoolean).z);
        break;

    case JavaTypeByte:
    case JavaTypeChar:
    case JavaTypeShort:

    case JavaTypeInt:
        {
            jint value;
            jvalue result = dispatchValueFromInstance(exec, instance, "getInt", "(Ljava/lang/Object;)I", JavaTypeInt);
            value = result.i;
            jsresult = jsNumber(static_cast<int>(value));
        }
        break;

    case JavaTypeLong:
    case JavaTypeFloat:
    case JavaTypeDouble:
        {
            jdouble value;
            jvalue result = dispatchValueFromInstance(exec, instance, "getDouble", "(Ljava/lang/Object;)D", JavaTypeDouble);
            value = result.i;
            jsresult = jsNumber(static_cast<double>(value));
        }
        break;
    default:
        break;
    }

    LOG(LiveConnect, "JavaField::valueFromInstance getting %s = %s", UString(name().impl()).utf8().data(), jsresult.toString(exec).ascii().data());

    return jsresult;
}

void JavaField::dispatchSetValueToInstance(ExecState* exec, const JavaInstance* instance, jvalue javaValue, const char* name, const char* sig) const
{
    jobject jinstance = instance->javaInstance();
    jobject fieldJInstance = m_field->m_instance;
    JNIEnv* env = getJNIEnv();

    jclass cls = env->GetObjectClass(fieldJInstance);
    if (cls) {
        jmethodID mid = env->GetMethodID(cls, name, sig);
        if (mid) {
            RootObject* rootObject = instance->rootObject();
            if (rootObject && rootObject->nativeHandle()) {
                JSValue exceptionDescription;
                jvalue args[2];
                jvalue result;

                args[0].l = jinstance;
                args[1] = javaValue;
                dispatchJNICall(exec, rootObject->nativeHandle(), fieldJInstance, false, JavaTypeVoid, mid, args, result, 0, exceptionDescription);
                if (exceptionDescription)
                    throwError(exec, createError(exec, exceptionDescription.toString(exec)));
            }
        }
    }
}

void JavaField::setValueToInstance(ExecState* exec, const Instance* i, JSValue aValue) const
{
    const JavaInstance* instance = static_cast<const JavaInstance*>(i);
    jvalue javaValue = convertValueToJValue(exec, i->rootObject(), aValue, m_type, typeClassName());

    LOG(LiveConnect, "JavaField::setValueToInstance setting value %s to %s", UString(name().impl()).utf8().data(), aValue.toString(exec).ascii().data());

    switch (m_type) {
    case JavaTypeArray:
    case JavaTypeObject:
        {
            dispatchSetValueToInstance(exec, instance, javaValue, "set", "(Ljava/lang/Object;Ljava/lang/Object;)V");
        }
        break;

    case JavaTypeBoolean:
        {
            dispatchSetValueToInstance(exec, instance, javaValue, "setBoolean", "(Ljava/lang/Object;Z)V");
        }
        break;

    case JavaTypeByte:
        {
            dispatchSetValueToInstance(exec, instance, javaValue, "setByte", "(Ljava/lang/Object;B)V");
        }
        break;

    case JavaTypeChar:
        {
            dispatchSetValueToInstance(exec, instance, javaValue, "setChar", "(Ljava/lang/Object;C)V");
        }
        break;

    case JavaTypeShort:
        {
            dispatchSetValueToInstance(exec, instance, javaValue, "setShort", "(Ljava/lang/Object;S)V");
        }
        break;

    case JavaTypeInt:
        {
            dispatchSetValueToInstance(exec, instance, javaValue, "setInt", "(Ljava/lang/Object;I)V");
        }
        break;

    case JavaTypeLong:
        {
            dispatchSetValueToInstance(exec, instance, javaValue, "setLong", "(Ljava/lang/Object;J)V");
        }
        break;

    case JavaTypeFloat:
        {
            dispatchSetValueToInstance(exec, instance, javaValue, "setFloat", "(Ljava/lang/Object;F)V");
        }
        break;

    case JavaTypeDouble:
        {
            dispatchSetValueToInstance(exec, instance, javaValue, "setDouble", "(Ljava/lang/Object;D)V");
        }
        break;
    default:
        break;
    }
}

#endif // ENABLE(JAVA_BRIDGE)
