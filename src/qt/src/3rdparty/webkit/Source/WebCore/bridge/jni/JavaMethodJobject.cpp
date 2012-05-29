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
#include "JavaMethodJobject.h"

#if ENABLE(JAVA_BRIDGE)

#include "JavaString.h"

#if USE(JSC)
#include <runtime/JSObject.h>
#include <runtime/ScopeChain.h>
#endif
#include <wtf/text/StringBuilder.h>

using namespace JSC;
using namespace JSC::Bindings;

JavaMethodJobject::JavaMethodJobject(JNIEnv* env, jobject aMethod)
{
    // Get return type name
    jstring returnTypeName = 0;
    if (jobject returnType = callJNIMethod<jobject>(aMethod, "getReturnType", "()Ljava/lang/Class;")) {
            returnTypeName = static_cast<jstring>(callJNIMethod<jobject>(returnType, "getName", "()Ljava/lang/String;"));
        if (!returnTypeName)
            returnTypeName = env->NewStringUTF("<Unknown>");
        env->DeleteLocalRef(returnType);
    }
    m_returnTypeClassName = JavaString(env, returnTypeName);
    m_returnType = javaTypeFromClassName(m_returnTypeClassName.utf8());
    env->DeleteLocalRef(returnTypeName);

    // Get method name
    jstring methodName = static_cast<jstring>(callJNIMethod<jobject>(aMethod, "getName", "()Ljava/lang/String;"));
    if (!methodName)
        methodName = env->NewStringUTF("<Unknown>");
    m_name = JavaString(env, methodName);
    env->DeleteLocalRef(methodName);

    // Get parameters
    if (jarray jparameters = static_cast<jarray>(callJNIMethod<jobject>(aMethod, "getParameterTypes", "()[Ljava/lang/Class;"))) {
        unsigned int numParams = env->GetArrayLength(jparameters);

        for (unsigned int i = 0; i < numParams; i++) {
            jobject aParameter = env->GetObjectArrayElement(static_cast<jobjectArray>(jparameters), i);
            jstring parameterName = static_cast<jstring>(callJNIMethod<jobject>(aParameter, "getName", "()Ljava/lang/String;"));
            if (!parameterName)
                parameterName = env->NewStringUTF("<Unknown>");
            m_parameters.append(JavaString(env, parameterName).impl());
            env->DeleteLocalRef(aParameter);
            env->DeleteLocalRef(parameterName);
        }
        env->DeleteLocalRef(jparameters);
    }

    // Created lazily.
    m_signature = 0;

    jclass modifierClass = env->FindClass("java/lang/reflect/Modifier");
    int modifiers = callJNIMethod<jint>(aMethod, "getModifiers", "()I");
    m_isStatic = static_cast<bool>(callJNIStaticMethod<jboolean>(modifierClass, "isStatic", "(I)Z", modifiers));
    env->DeleteLocalRef(modifierClass);
}

JavaMethodJobject::~JavaMethodJobject()
{
    if (m_signature)
        fastFree(m_signature);
}

// JNI method signatures use '/' between components of a class name, but
// we get '.' between components from the reflection API.
static void appendClassName(StringBuilder& builder, const char* className)
{
#if USE(JSC)
    ASSERT(JSLock::lockCount() > 0);
#endif

    char* c = fastStrDup(className);

    char* result = c;
    while (*c) {
        if (*c == '.')
            *c = '/';
        c++;
    }

    builder.append(result);

    fastFree(result);
}

const char* JavaMethodJobject::signature() const
{
    if (!m_signature) {
#if USE(JSC)
        JSLock lock(SilenceAssertionsOnly);
#endif

        StringBuilder signatureBuilder;
        signatureBuilder.append('(');
        for (unsigned int i = 0; i < m_parameters.size(); i++) {
            CString javaClassName = parameterAt(i).utf8();
            JavaType type = javaTypeFromClassName(javaClassName.data());
            if (type == JavaTypeArray)
                appendClassName(signatureBuilder, javaClassName.data());
            else {
                signatureBuilder.append(signatureFromJavaType(type));
                if (type == JavaTypeObject) {
                    appendClassName(signatureBuilder, javaClassName.data());
                    signatureBuilder.append(';');
                }
            }
        }
        signatureBuilder.append(')');

        const char* returnType = m_returnTypeClassName.utf8();
        if (m_returnType == JavaTypeArray)
            appendClassName(signatureBuilder, returnType);
        else {
            signatureBuilder.append(signatureFromJavaType(m_returnType));
            if (m_returnType == JavaTypeObject) {
                appendClassName(signatureBuilder, returnType);
                signatureBuilder.append(';');
            }
        }

        String signatureString = signatureBuilder.toString();
        m_signature = fastStrDup(signatureString.utf8().data());
    }

    return m_signature;
}

#endif // ENABLE(JAVA_BRIDGE)
