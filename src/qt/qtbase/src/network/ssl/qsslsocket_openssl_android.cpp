/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtNetwork module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

/****************************************************************************
**
** In addition, as a special exception, the copyright holders listed above give
** permission to link the code of its release of Qt with the OpenSSL project's
** "OpenSSL" library (or modified versions of the "OpenSSL" library that use the
** same license as the original version), and distribute the linked executables.
**
** You must comply with the GNU General Public License version 2 in all
** respects for all of the code used other than the "OpenSSL" code.  If you
** modify this file, you may extend this exception to your version of the file,
** but you are not obligated to do so.  If you do not wish to do so, delete
** this exception statement from your version of this file.
**
****************************************************************************/

#include "qsslsocket_openssl_p.h"



#include <jni.h>
#include <android/log.h>

static JavaVM *javaVM = 0;
static jclass appClass;

static jmethodID getSslCertificatesMethodID;

struct AttachedJNIEnv
{
    AttachedJNIEnv()
    {
        attached = false;
        if (javaVM->GetEnv((void**)&jniEnv, JNI_VERSION_1_6) < 0) {
            if (javaVM->AttachCurrentThread(&jniEnv, NULL) < 0) {
                __android_log_print(ANDROID_LOG_ERROR, "Qt", "AttachCurrentThread failed");
                jniEnv = 0;
                return;
            }
            attached = true;
        }
    }

    ~AttachedJNIEnv()
    {
        if (attached)
            javaVM->DetachCurrentThread();
    }
    bool attached;
    JNIEnv *jniEnv;
};

static const char logTag[] = "Qt";
static const char classErrorMsg[] = "Can't find class \"%s\"";
static const char methodErrorMsg[] = "Can't find method \"%s%s\"";


#define FIND_AND_CHECK_CLASS(CLASS_NAME) \
clazz = env->FindClass(CLASS_NAME); \
if (!clazz) { \
    __android_log_print(ANDROID_LOG_FATAL, logTag, classErrorMsg, CLASS_NAME); \
    return JNI_FALSE; \
}

#define GET_AND_CHECK_STATIC_METHOD(VAR, CLASS, METHOD_NAME, METHOD_SIGNATURE) \
VAR = env->GetStaticMethodID(CLASS, METHOD_NAME, METHOD_SIGNATURE); \
if (!VAR) { \
    __android_log_print(ANDROID_LOG_FATAL, logTag, methodErrorMsg, METHOD_NAME, METHOD_SIGNATURE); \
    return JNI_FALSE; \
}

static bool registerNatives(JNIEnv *env)
{
    jclass clazz;
    FIND_AND_CHECK_CLASS("org/qtproject/qt5/android/QtNative");
    appClass = static_cast<jclass>(env->NewGlobalRef(clazz));

#if 0 //we don't call C++ functions from Java at this time
    if (env->RegisterNatives(appClass, methods, sizeof(methods) / sizeof(methods[0])) < 0) {
        __android_log_print(ANDROID_LOG_FATAL, logTag, "RegisterNatives failed");
        return JNI_FALSE;
    }
#endif

    GET_AND_CHECK_STATIC_METHOD(getSslCertificatesMethodID, appClass, "getSSLCertificates", "()[[B");

    return true;
}

Q_DECL_EXPORT jint JNICALL JNI_OnLoad(JavaVM *vm, void * /*reserved*/)
{
    typedef union {
        JNIEnv *nativeEnvironment;
        void *venv;
    } UnionJNIEnvToVoid;

    __android_log_print(ANDROID_LOG_INFO, logTag, "Network start");
    UnionJNIEnvToVoid uenv;
    uenv.venv = NULL;
    javaVM = 0;

    if (vm->GetEnv(&uenv.venv, JNI_VERSION_1_4) != JNI_OK) {
        __android_log_print(ANDROID_LOG_FATAL, logTag, "GetEnv failed");
        return -1;
    }
    JNIEnv *env = uenv.nativeEnvironment;
    if (!registerNatives(env)) {
        __android_log_print(ANDROID_LOG_FATAL, logTag, "registerNatives failed");
        return -1;
    }

    javaVM = vm;
    return JNI_VERSION_1_4;
}

QList<QByteArray> QSslSocketPrivate::fetchSslCertificateData()
{
    QList<QByteArray> certificateData;
    AttachedJNIEnv env;

    if (env.jniEnv) {
        jobjectArray jcertificates =
            static_cast<jobjectArray>(env.jniEnv->CallStaticObjectMethod(appClass, getSslCertificatesMethodID));
        jint nCertificates = env.jniEnv->GetArrayLength(jcertificates);

        for (int i = 0; i < nCertificates; ++i) {
            jbyteArray jCert = static_cast<jbyteArray>(env.jniEnv->GetObjectArrayElement(jcertificates, i));

            const uint sz = env.jniEnv->GetArrayLength(jCert);
            jbyte *buffer = env.jniEnv->GetByteArrayElements(jCert, 0);
            certificateData.append(QByteArray(reinterpret_cast<char*>(buffer), sz));

            env.jniEnv->ReleaseByteArrayElements(jCert, buffer, JNI_ABORT); // don't copy back the elements
            env.jniEnv->DeleteLocalRef(jCert);
        }
    }

    return certificateData;
}
