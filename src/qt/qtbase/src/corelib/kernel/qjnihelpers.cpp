/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtCore module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia. For licensing terms and
** conditions see http://qt.digia.com/licensing. For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights. These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qjnihelpers_p.h"
#include "qmutex.h"
#include "qlist.h"
#include <QtCore/qrunnable.h>

QT_BEGIN_NAMESPACE

static JavaVM *g_javaVM = Q_NULLPTR;
static jobject g_jActivity = Q_NULLPTR;
static jobject g_jClassLoader = Q_NULLPTR;
static jint g_androidSdkVersion = 0;
static jclass g_jNativeClass = Q_NULLPTR;
static jmethodID g_runQtOnUiThreadMethodID = Q_NULLPTR;

static void onAndroidUiThread(JNIEnv *, jclass, jlong thiz)
{
    QRunnable *runnable = reinterpret_cast<QRunnable *>(thiz);
    if (runnable == 0)
        return;

    runnable->run();
    if (runnable->autoDelete())
        delete runnable;
}

namespace {
    class ActivityResultListeners
    {
    public:
        QMutex mutex;
        QList<QtAndroidPrivate::ActivityResultListener *> listeners;
    };
}

Q_GLOBAL_STATIC(ActivityResultListeners, g_activityResultListeners)

void QtAndroidPrivate::registerActivityResultListener(ActivityResultListener *listener)
{
    QMutexLocker locker(&g_activityResultListeners()->mutex);
    g_activityResultListeners()->listeners.append(listener);
}

void QtAndroidPrivate::unregisterActivityResultListener(ActivityResultListener *listener)
{
    QMutexLocker locker(&g_activityResultListeners()->mutex);
    g_activityResultListeners()->listeners.removeAll(listener);
}

void QtAndroidPrivate::handleActivityResult(jint requestCode, jint resultCode, jobject data)
{
    QMutexLocker locker(&g_activityResultListeners()->mutex);
    const QList<QtAndroidPrivate::ActivityResultListener *> &listeners = g_activityResultListeners()->listeners;
    for (int i=0; i<listeners.size(); ++i) {
        if (listeners.at(i)->handleActivityResult(requestCode, resultCode, data))
            break;
    }
}

static inline bool exceptionCheck(JNIEnv *env)
{
    if (env->ExceptionCheck()) {
#ifdef QT_DEBUG
        env->ExceptionDescribe();
#endif // QT_DEBUG
        env->ExceptionClear();
        return true;
    }

    return false;
}

static void setAndroidSdkVersion(JNIEnv *env)
{
    jclass androidVersionClass = env->FindClass("android/os/Build$VERSION");
    if (exceptionCheck(env))
        return;

    jfieldID androidSDKFieldID = env->GetStaticFieldID(androidVersionClass, "SDK_INT", "I");
    if (exceptionCheck(env))
        return;

    g_androidSdkVersion = env->GetStaticIntField(androidVersionClass, androidSDKFieldID);
}

jint QtAndroidPrivate::initJNI(JavaVM *vm, JNIEnv *env)
{
    jclass jQtNative = env->FindClass("org/qtproject/qt5/android/QtNative");

    if (exceptionCheck(env))
        return JNI_ERR;

    jmethodID activityMethodID = env->GetStaticMethodID(jQtNative,
                                                        "activity",
                                                        "()Landroid/app/Activity;");

    if (exceptionCheck(env))
        return JNI_ERR;

    jobject activity = env->CallStaticObjectMethod(jQtNative, activityMethodID);
    if (exceptionCheck(env))
        return JNI_ERR;



    jmethodID classLoaderMethodID = env->GetStaticMethodID(jQtNative,
                                                           "classLoader",
                                                           "()Ljava/lang/ClassLoader;");

    if (exceptionCheck(env))
        return JNI_ERR;

    jobject classLoader = env->CallStaticObjectMethod(jQtNative, classLoaderMethodID);
    if (exceptionCheck(env))
        return JNI_ERR;

    setAndroidSdkVersion(env);

    g_jClassLoader = env->NewGlobalRef(classLoader);
    env->DeleteLocalRef(classLoader);
    g_jActivity = env->NewGlobalRef(activity);
    env->DeleteLocalRef(activity);
    g_javaVM = vm;

    static const JNINativeMethod methods[] = {
        {"onAndroidUiThread", "(J)V", reinterpret_cast<void *>(onAndroidUiThread)}
    };

    const bool regOk = (env->RegisterNatives(jQtNative, methods, sizeof(methods) / sizeof(methods[0])) == JNI_OK);

    if (!regOk && exceptionCheck(env))
        return JNI_ERR;

    g_runQtOnUiThreadMethodID = env->GetStaticMethodID(jQtNative,
                                                       "runQtOnUiThread",
                                                       "(J)V");

    g_jNativeClass = static_cast<jclass>(env->NewGlobalRef(jQtNative));
    env->DeleteLocalRef(jQtNative);

    return JNI_OK;
}


jobject QtAndroidPrivate::activity()
{
    return g_jActivity;
}

JavaVM *QtAndroidPrivate::javaVM()
{
    return g_javaVM;
}

jobject QtAndroidPrivate::classLoader()
{
    return g_jClassLoader;
}

jint QtAndroidPrivate::androidSdkVersion()
{
    return g_androidSdkVersion;
}

void QtAndroidPrivate::runOnUiThread(QRunnable *runnable, JNIEnv *env)
{
    Q_ASSERT(runnable != 0);
    env->CallStaticVoidMethod(g_jNativeClass, g_runQtOnUiThreadMethodID, reinterpret_cast<jlong>(runnable));
    if (exceptionCheck(env) && runnable != 0 && runnable->autoDelete())
        delete runnable;
}

QT_END_NAMESPACE
