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
#include "JavaArrayJSC.h"

#if ENABLE(JAVA_BRIDGE)

#include "JNIUtilityPrivate.h"
#include "JavaInstanceJSC.h"
#include "JobjectWrapper.h"
#include "Logging.h"
#include "runtime_array.h"
#include "runtime_object.h"
#include "runtime_root.h"
#include <runtime/Error.h>

using namespace JSC;
using namespace JSC::Bindings;
using namespace WebCore;

JSValue JavaArray::convertJObjectToArray(ExecState* exec, jobject anObject, const char* type, PassRefPtr<RootObject> rootObject)
{
    if (type[0] != '[')
        return jsUndefined();

    return new (exec) RuntimeArray(exec, new JavaArray(anObject, type, rootObject));
}

JavaArray::JavaArray(jobject array, const char* type, PassRefPtr<RootObject> rootObject)
    : Array(rootObject)
{
    m_array = new JobjectWrapper(array);
    // Java array are fixed length, so we can cache length.
    JNIEnv* env = getJNIEnv();
    m_length = env->GetArrayLength(static_cast<jarray>(m_array->m_instance));
    m_type = strdup(type);
}

JavaArray::~JavaArray()
{
    free(const_cast<char*>(m_type));
}

RootObject* JavaArray::rootObject() const
{
    return m_rootObject && m_rootObject->isValid() ? m_rootObject.get() : 0;
}

void JavaArray::setValueAt(ExecState* exec, unsigned index, JSValue aValue) const
{
    JNIEnv* env = getJNIEnv();
    char* javaClassName = 0;

    JavaType arrayType = javaTypeFromPrimitiveType(m_type[1]);
    if (m_type[1] == 'L') {
        // The type of the array will be something like:
        // "[Ljava.lang.string;". This is guaranteed, so no need
        // for extra sanity checks.
        javaClassName = strdup(&m_type[2]);
        javaClassName[strchr(javaClassName, ';')-javaClassName] = 0;
    }
    jvalue aJValue = convertValueToJValue(exec, m_rootObject.get(), aValue, arrayType, javaClassName);

    switch (arrayType) {
    case JavaTypeObject:
        {
            env->SetObjectArrayElement(static_cast<jobjectArray>(javaArray()), index, aJValue.l);
            break;
        }

    case JavaTypeBoolean:
        {
            env->SetBooleanArrayRegion(static_cast<jbooleanArray>(javaArray()), index, 1, &aJValue.z);
            break;
        }

    case JavaTypeByte:
        {
            env->SetByteArrayRegion(static_cast<jbyteArray>(javaArray()), index, 1, &aJValue.b);
            break;
        }

    case JavaTypeChar:
        {
            env->SetCharArrayRegion(static_cast<jcharArray>(javaArray()), index, 1, &aJValue.c);
            break;
        }

    case JavaTypeShort:
        {
            env->SetShortArrayRegion(static_cast<jshortArray>(javaArray()), index, 1, &aJValue.s);
            break;
        }

    case JavaTypeInt:
        {
            env->SetIntArrayRegion(static_cast<jintArray>(javaArray()), index, 1, &aJValue.i);
            break;
        }

    case JavaTypeLong:
        {
            env->SetLongArrayRegion(static_cast<jlongArray>(javaArray()), index, 1, &aJValue.j);
        }

    case JavaTypeFloat:
        {
            env->SetFloatArrayRegion(static_cast<jfloatArray>(javaArray()), index, 1, &aJValue.f);
            break;
        }

    case JavaTypeDouble:
        {
            env->SetDoubleArrayRegion(static_cast<jdoubleArray>(javaArray()), index, 1, &aJValue.d);
            break;
        }
    default:
        break;
    }

    if (javaClassName)
        free(const_cast<char*>(javaClassName));
}

JSValue JavaArray::valueAt(ExecState* exec, unsigned index) const
{
    JNIEnv* env = getJNIEnv();
    JavaType arrayType = javaTypeFromPrimitiveType(m_type[1]);
    switch (arrayType) {
    case JavaTypeObject:
        {
            jobjectArray objectArray = static_cast<jobjectArray>(javaArray());
            jobject anObject;
            anObject = env->GetObjectArrayElement(objectArray, index);

            // No object?
            if (!anObject)
                return jsNull();

            // Nested array?
            if (m_type[1] == '[')
                return JavaArray::convertJObjectToArray(exec, anObject, m_type + 1, rootObject());
            // or array of other object type?
            return JavaInstance::create(anObject, rootObject())->createRuntimeObject(exec);
        }

    case JavaTypeBoolean:
        {
            jbooleanArray booleanArray = static_cast<jbooleanArray>(javaArray());
            jboolean aBoolean;
            env->GetBooleanArrayRegion(booleanArray, index, 1, &aBoolean);
            return jsBoolean(aBoolean);
        }

    case JavaTypeByte:
        {
            jbyteArray byteArray = static_cast<jbyteArray>(javaArray());
            jbyte aByte;
            env->GetByteArrayRegion(byteArray, index, 1, &aByte);
            return jsNumber(aByte);
        }

    case JavaTypeChar:
        {
            jcharArray charArray = static_cast<jcharArray>(javaArray());
            jchar aChar;
            env->GetCharArrayRegion(charArray, index, 1, &aChar);
            return jsNumber(aChar);
            break;
        }

    case JavaTypeShort:
        {
            jshortArray shortArray = static_cast<jshortArray>(javaArray());
            jshort aShort;
            env->GetShortArrayRegion(shortArray, index, 1, &aShort);
            return jsNumber(aShort);
        }

    case JavaTypeInt:
        {
            jintArray intArray = static_cast<jintArray>(javaArray());
            jint anInt;
            env->GetIntArrayRegion(intArray, index, 1, &anInt);
            return jsNumber(anInt);
        }

    case JavaTypeLong:
        {
            jlongArray longArray = static_cast<jlongArray>(javaArray());
            jlong aLong;
            env->GetLongArrayRegion(longArray, index, 1, &aLong);
            return jsNumber(aLong);
        }

    case JavaTypeFloat:
        {
            jfloatArray floatArray = static_cast<jfloatArray>(javaArray());
            jfloat aFloat;
            env->GetFloatArrayRegion(floatArray, index, 1, &aFloat);
            return jsNumber(aFloat);
        }

    case JavaTypeDouble:
        {
            jdoubleArray doubleArray = static_cast<jdoubleArray>(javaArray());
            jdouble aDouble;
            env->GetDoubleArrayRegion(doubleArray, index, 1, &aDouble);
            return jsNumber(aDouble);
        }
    default:
        break;
    }
    return jsUndefined();
}

unsigned int JavaArray::getLength() const
{
    return m_length;
}

#endif // ENABLE(JAVA_BRIDGE)
