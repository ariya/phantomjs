/*
 * Copyright (C) 2003, 2004, 2005, 2006, 2007, 2008, 2009, 2010 Apple Inc. All rights reserved.
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
#include "JavaClassJSC.h"

#if ENABLE(JAVA_BRIDGE)

#include "JSDOMWindow.h"
#include "JavaFieldJSC.h"
#include "JavaMethodJobject.h"
#include <runtime/Identifier.h>
#include <runtime/JSLock.h>

using namespace JSC::Bindings;

JavaClass::JavaClass(jobject anInstance)
{
    jobject aClass = callJNIMethod<jobject>(anInstance, "getClass", "()Ljava/lang/Class;");

    if (!aClass) {
        LOG_ERROR("Unable to call getClass on instance %p", anInstance);
        m_name = fastStrDup("<Unknown>");
        return;
    }

    if (jstring className = (jstring)callJNIMethod<jobject>(aClass, "getName", "()Ljava/lang/String;")) {
        const char* classNameC = getCharactersFromJString(className);
        m_name = fastStrDup(classNameC);
        releaseCharactersForJString(className, classNameC);
    } else
        m_name = fastStrDup("<Unknown>");

    int i;
    JNIEnv* env = getJNIEnv();

    // Get the fields
    if (jarray fields = (jarray)callJNIMethod<jobject>(aClass, "getFields", "()[Ljava/lang/reflect/Field;")) {
        int numFields = env->GetArrayLength(fields);
        for (i = 0; i < numFields; i++) {
            jobject aJField = env->GetObjectArrayElement((jobjectArray)fields, i);
            JavaField* aField = new JavaField(env, aJField); // deleted in the JavaClass destructor
            {
                JSLock lock(SilenceAssertionsOnly);
                m_fields.set(aField->name().impl(), aField);
            }
            env->DeleteLocalRef(aJField);
        }
        env->DeleteLocalRef(fields);
    }

    // Get the methods
    if (jarray methods = (jarray)callJNIMethod<jobject>(aClass, "getMethods", "()[Ljava/lang/reflect/Method;")) {
        int numMethods = env->GetArrayLength(methods);
        for (i = 0; i < numMethods; i++) {
            jobject aJMethod = env->GetObjectArrayElement((jobjectArray)methods, i);
            JavaMethod* aMethod = new JavaMethodJobject(env, aJMethod); // deleted in the JavaClass destructor
            MethodList* methodList;
            {
                JSLock lock(SilenceAssertionsOnly);

                methodList = m_methods.get(aMethod->name().impl());
                if (!methodList) {
                    methodList = new MethodList();
                    m_methods.set(aMethod->name().impl(), methodList);
                }
            }
            methodList->append(aMethod);
            env->DeleteLocalRef(aJMethod);
        }
        env->DeleteLocalRef(methods);
    }

    env->DeleteLocalRef(aClass);
}

JavaClass::~JavaClass()
{
    fastFree(const_cast<char*>(m_name));

    JSLock lock(SilenceAssertionsOnly);

    deleteAllValues(m_fields);
    m_fields.clear();

    MethodListMap::const_iterator end = m_methods.end();
    for (MethodListMap::const_iterator it = m_methods.begin(); it != end; ++it) {
        const MethodList* methodList = it->second;
        deleteAllValues(*methodList);
        delete methodList;
    }
    m_methods.clear();
}

MethodList JavaClass::methodsNamed(const Identifier& identifier, Instance*) const
{
    MethodList* methodList = m_methods.get(identifier.ustring().impl());

    if (methodList)
        return *methodList;
    return MethodList();
}

Field* JavaClass::fieldNamed(const Identifier& identifier, Instance*) const
{
    return m_fields.get(identifier.ustring().impl());
}

bool JavaClass::isNumberClass() const
{
    return (!strcmp(m_name, "java.lang.Byte")
        || !strcmp(m_name, "java.lang.Short")
        || !strcmp(m_name, "java.lang.Integer")
        || !strcmp(m_name, "java.lang.Long")
        || !strcmp(m_name, "java.lang.Float")
        || !strcmp(m_name, "java.lang.Double"));
}

bool JavaClass::isBooleanClass() const
{
    return !strcmp(m_name, "java.lang.Boolean");
}

bool JavaClass::isStringClass() const
{
    return !strcmp(m_name, "java.lang.String");
}

#endif // ENABLE(JAVA_BRIDGE)
