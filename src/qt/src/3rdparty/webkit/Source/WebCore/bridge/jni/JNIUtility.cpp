/*
 * Copyright (C) 2003, 2004, 2005, 2007, 2008, 2009, 2010 Apple Inc. All rights reserved.
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
#include "JNIUtility.h"

#if ENABLE(JAVA_BRIDGE)

#include <dlfcn.h>

namespace JSC {

namespace Bindings {

static jint KJSGetCreatedJavaVMs(JavaVM** vmBuf, jsize bufLen, jsize* nVMs)
{
    static void* javaVMFramework = 0;
    if (!javaVMFramework)
        javaVMFramework = dlopen("/System/Library/Frameworks/JavaVM.framework/JavaVM", RTLD_LAZY);
    if (!javaVMFramework)
        return JNI_ERR;

    typedef jint(*FunctionPointerType)(JavaVM**, jsize, jsize*);
    static FunctionPointerType functionPointer = 0;
    if (!functionPointer)
        functionPointer = reinterpret_cast<FunctionPointerType>(dlsym(javaVMFramework, "JNI_GetCreatedJavaVMs"));
    if (!functionPointer)
        return JNI_ERR;
    return functionPointer(vmBuf, bufLen, nVMs);
}

static JavaVM* jvm = 0;

// Provide the ability for an outside component to specify the JavaVM to use
// If the jvm value is set, the getJavaVM function below will just return.
// In getJNIEnv(), if AttachCurrentThread is called to a VM that is already
// attached, the result is a no-op.
void setJavaVM(JavaVM* javaVM)
{
    jvm = javaVM;
}

JavaVM* getJavaVM()
{
    if (jvm)
        return jvm;

    JavaVM* jvmArray[1];
    jsize bufLen = 1;
    jsize nJVMs = 0;
    jint jniError = 0;

    // Assumes JVM is already running ..., one per process
    jniError = KJSGetCreatedJavaVMs(jvmArray, bufLen, &nJVMs);
    if (jniError == JNI_OK && nJVMs > 0)
        jvm = jvmArray[0];
    else
        LOG_ERROR("JNI_GetCreatedJavaVMs failed, returned %ld", static_cast<long>(jniError));

    return jvm;
}

JNIEnv* getJNIEnv()
{
    union {
        JNIEnv* env;
        void* dummy;
    } u;
    jint jniError = 0;

#if OS(ANDROID)
    jniError = getJavaVM()->AttachCurrentThread(&u.env, 0);
#else
    jniError = getJavaVM()->AttachCurrentThread(&u.dummy, 0);
#endif
    if (jniError == JNI_OK)
        return u.env;
    LOG_ERROR("AttachCurrentThread failed, returned %ld", static_cast<long>(jniError));
    return 0;
}

jmethodID getMethodID(jobject obj, const char* name, const char* sig)
{
    JNIEnv* env = getJNIEnv();
    jmethodID mid = 0;

    if (env) {
        jclass cls = env->GetObjectClass(obj);
        if (cls) {
            mid = env->GetMethodID(cls, name, sig);
            if (!mid) {
                env->ExceptionClear();
                mid = env->GetStaticMethodID(cls, name, sig);
                if (!mid)
                    env->ExceptionClear();
            }
        }
        env->DeleteLocalRef(cls);
    }
    return mid;
}

const char* getCharactersFromJString(jstring aJString)
{
    return getCharactersFromJStringInEnv(getJNIEnv(), aJString);
}

void releaseCharactersForJString(jstring aJString, const char* s)
{
    releaseCharactersForJStringInEnv(getJNIEnv(), aJString, s);
}

const char* getCharactersFromJStringInEnv(JNIEnv* env, jstring aJString)
{
    jboolean isCopy;
    const char* s = env->GetStringUTFChars(aJString, &isCopy);
    if (!s) {
        env->ExceptionDescribe();
        env->ExceptionClear();
        fprintf(stderr, "\n");
    }
    return s;
}

void releaseCharactersForJStringInEnv(JNIEnv* env, jstring aJString, const char* s)
{
    env->ReleaseStringUTFChars(aJString, s);
}

const jchar* getUCharactersFromJStringInEnv(JNIEnv* env, jstring aJString)
{
    jboolean isCopy;
    const jchar* s = env->GetStringChars(aJString, &isCopy);
    if (!s) {
        env->ExceptionDescribe();
        env->ExceptionClear();
        fprintf(stderr, "\n");
    }
    return s;
}

void releaseUCharactersForJStringInEnv(JNIEnv* env, jstring aJString, const jchar* s)
{
    env->ReleaseStringChars(aJString, s);
}

JavaType javaTypeFromClassName(const char* name)
{
    JavaType type;

    if (!strcmp("byte", name))
        type = JavaTypeByte;
    else if (!strcmp("short", name))
        type = JavaTypeShort;
    else if (!strcmp("int", name))
        type = JavaTypeInt;
    else if (!strcmp("long", name))
        type = JavaTypeLong;
    else if (!strcmp("float", name))
        type = JavaTypeFloat;
    else if (!strcmp("double", name))
        type = JavaTypeDouble;
    else if (!strcmp("char", name))
        type = JavaTypeChar;
    else if (!strcmp("boolean", name))
        type = JavaTypeBoolean;
    else if (!strcmp("void", name))
        type = JavaTypeVoid;
    else if ('[' == name[0])
        type = JavaTypeArray;
#if USE(V8)
    else if (!strcmp("java.lang.String", name))
        type = JavaTypeString;
#endif
    else
        type = JavaTypeObject;

    return type;
}

const char* signatureFromJavaType(JavaType type)
{
    switch (type) {
    case JavaTypeVoid:
        return "V";

    case JavaTypeArray:
        return "[";

    case JavaTypeObject:
#if USE(V8)
    case JavaTypeString:
#endif
        return "L";

    case JavaTypeBoolean:
        return "Z";

    case JavaTypeByte:
        return "B";

    case JavaTypeChar:
        return "C";

    case JavaTypeShort:
        return "S";

    case JavaTypeInt:
        return "I";

    case JavaTypeLong:
        return "J";

    case JavaTypeFloat:
        return "F";

    case JavaTypeDouble:
        return "D";

    case JavaTypeInvalid:
    default:
        break;
    }
    return "";
}

JavaType javaTypeFromPrimitiveType(char type)
{
    switch (type) {
    case 'V':
        return JavaTypeVoid;

    case 'L':
        return JavaTypeObject;

    case '[':
        return JavaTypeArray;

    case 'Z':
        return JavaTypeBoolean;

    case 'B':
        return JavaTypeByte;

    case 'C':
        return JavaTypeChar;

    case 'S':
        return JavaTypeShort;

    case 'I':
        return JavaTypeInt;

    case 'J':
        return JavaTypeLong;

    case 'F':
        return JavaTypeFloat;

    case 'D':
        return JavaTypeDouble;

    default:
        break;
    }
    return JavaTypeInvalid;
}

jvalue getJNIField(jobject obj, JavaType type, const char* name, const char* signature)
{
    JavaVM* jvm = getJavaVM();
    JNIEnv* env = getJNIEnv();
    jvalue result;

    memset(&result, 0, sizeof(jvalue));
    if (obj && jvm && env) {
        jclass cls = env->GetObjectClass(obj);
        if (cls) {
            jfieldID field = env->GetFieldID(cls, name, signature);
            if (field) {
                switch (type) {
                case JavaTypeArray:
                case JavaTypeObject:
#if USE(V8)
                case JavaTypeString:
#endif
                    result.l = env->functions->GetObjectField(env, obj, field);
                    break;
                case JavaTypeBoolean:
                    result.z = env->functions->GetBooleanField(env, obj, field);
                    break;
                case JavaTypeByte:
                    result.b = env->functions->GetByteField(env, obj, field);
                    break;
                case JavaTypeChar:
                    result.c = env->functions->GetCharField(env, obj, field);
                    break;
                case JavaTypeShort:
                    result.s = env->functions->GetShortField(env, obj, field);
                    break;
                case JavaTypeInt:
                    result.i = env->functions->GetIntField(env, obj, field);
                    break;
                case JavaTypeLong:
                    result.j = env->functions->GetLongField(env, obj, field);
                    break;
                case JavaTypeFloat:
                    result.f = env->functions->GetFloatField(env, obj, field);
                    break;
                case JavaTypeDouble:
                    result.d = env->functions->GetDoubleField(env, obj, field);
                    break;
                default:
                    LOG_ERROR("Invalid field type (%d)", static_cast<int>(type));
                }
            } else {
                LOG_ERROR("Could not find field: %s", name);
                env->ExceptionDescribe();
                env->ExceptionClear();
                fprintf(stderr, "\n");
            }

            env->DeleteLocalRef(cls);
        } else
            LOG_ERROR("Could not find class for object");
    }

    return result;
}

jvalue callJNIMethod(jobject object, JavaType returnType, const char* name, const char* signature, jvalue* args)
{
    jmethodID methodId = getMethodID(object, name, signature);
    jvalue result;
    switch (returnType) {
    case JavaTypeVoid:
        callJNIMethodIDA<void>(object, methodId, args);
        break;
    case JavaTypeObject:
#if USE(V8)
    case JavaTypeString:
#endif
        result.l = callJNIMethodIDA<jobject>(object, methodId, args);
        break;
    case JavaTypeBoolean:
        result.z = callJNIMethodIDA<jboolean>(object, methodId, args);
        break;
    case JavaTypeByte:
        result.b = callJNIMethodIDA<jbyte>(object, methodId, args);
        break;
    case JavaTypeChar:
        result.c = callJNIMethodIDA<jchar>(object, methodId, args);
        break;
    case JavaTypeShort:
        result.s = callJNIMethodIDA<jshort>(object, methodId, args);
        break;
    case JavaTypeInt:
        result.i = callJNIMethodIDA<jint>(object, methodId, args);
        break;
    case JavaTypeLong:
        result.j = callJNIMethodIDA<jlong>(object, methodId, args);
        break;
    case JavaTypeFloat:
        result.f = callJNIMethodIDA<jfloat>(object, methodId, args);
        break;
    case JavaTypeDouble:
        result.d = callJNIMethodIDA<jdouble>(object, methodId, args);
        break;
    default:
        break;
    }
    return result;
}

} // namespace Bindings

} // namespace JSC

#endif // ENABLE(JAVA_BRIDGE)
