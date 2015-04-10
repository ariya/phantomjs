/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtCore module of the Qt Toolkit.
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

#include "qjni_p.h"
#include "qjnihelpers_p.h"
#include <QtCore/qthreadstorage.h>
#include <QtCore/qhash.h>
#include <QtCore/qstring.h>
#include <QtCore/QThread>

QT_BEGIN_NAMESPACE

static inline QString keyBase()
{
    return QStringLiteral("%1%2%3");
}

static inline QByteArray threadBaseName()
{
    return QByteArrayLiteral("QtThread-");
}

static QString qt_convertJString(jstring string)
{
    QJNIEnvironmentPrivate env;
    int strLength = env->GetStringLength(string);
    QString res(strLength, Qt::Uninitialized);
    env->GetStringRegion(string, 0, strLength, reinterpret_cast<jchar *>(res.data()));
    return res;
}

typedef QHash<QString, jclass> JClassHash;
Q_GLOBAL_STATIC(JClassHash, cachedClasses)

static jclass getCachedClass(JNIEnv *env, const char *className)
{
    jclass clazz = 0;
    QString key = QLatin1String(className);
    QHash<QString, jclass>::iterator it = cachedClasses->find(key);
    if (it == cachedClasses->end()) {
        QJNIObjectPrivate classLoader = QtAndroidPrivate::classLoader();
        if (!classLoader.isValid())
            return 0;

        QJNIObjectPrivate stringName = QJNIObjectPrivate::fromString(QString::fromLatin1(className).replace(QLatin1Char('/'),
                                                                                                            QLatin1Char('.')));
        QJNIObjectPrivate classObject = classLoader.callObjectMethod("loadClass",
                                                                     "(Ljava/lang/String;)Ljava/lang/Class;",
                                                                     stringName.object());
        if (env->ExceptionCheck()) {
#ifdef QT_DEBUG
            env->ExceptionDescribe();
#endif // QT_DEBUG
            env->ExceptionClear();
        }

        if (classObject.isValid())
            clazz = static_cast<jclass>(env->NewGlobalRef(classObject.object()));

        cachedClasses->insert(key, clazz);
    } else {
        clazz = it.value();
    }
    return clazz;
}

typedef QHash<QString, jmethodID> JMethodIDHash;
Q_GLOBAL_STATIC(JMethodIDHash, cachedMethodID)

static jmethodID getCachedMethodID(JNIEnv *env,
                                   jclass clazz,
                                   const char *name,
                                   const char *sig,
                                   bool isStatic = false)
{
    jmethodID id = 0;
    // TODO: We need to use something else then the ref. from clazz to avoid collisions.
    QString key = keyBase().arg(size_t(clazz)).arg(QLatin1String(name)).arg(QLatin1String(sig));
    QHash<QString, jmethodID>::iterator it = cachedMethodID->find(key);
    if (it == cachedMethodID->end()) {
        if (isStatic)
            id = env->GetStaticMethodID(clazz, name, sig);
        else
            id = env->GetMethodID(clazz, name, sig);

        if (env->ExceptionCheck()) {
            id = 0;
#ifdef QT_DEBUG
            env->ExceptionDescribe();
#endif // QT_DEBUG
            env->ExceptionClear();
        }

        cachedMethodID->insert(key, id);
    } else {
        id = it.value();
    }
    return id;
}

typedef QHash<QString, jfieldID> JFieldIDHash;
Q_GLOBAL_STATIC(JFieldIDHash, cachedFieldID)

static jfieldID getCachedFieldID(JNIEnv *env,
                                 jclass clazz,
                                 const char *name,
                                 const char *sig,
                                 bool isStatic = false)
{
    jfieldID id = 0;
    QString key = keyBase().arg(size_t(clazz)).arg(QLatin1String(name)).arg(QLatin1String(sig));
    QHash<QString, jfieldID>::iterator it = cachedFieldID->find(key);
    if (it == cachedFieldID->end()) {
        if (isStatic)
            id = env->GetStaticFieldID(clazz, name, sig);
        else
            id = env->GetFieldID(clazz, name, sig);

        if (env->ExceptionCheck()) {
            id = 0;
#ifdef QT_DEBUG
            env->ExceptionDescribe();
#endif // QT_DEBUG
            env->ExceptionClear();
        }

        cachedFieldID->insert(key, id);
    } else {
        id = it.value();
    }
    return id;
}

class QJNIEnvironmentPrivateTLS
{
public:
    inline ~QJNIEnvironmentPrivateTLS()
    {
        QtAndroidPrivate::javaVM()->DetachCurrentThread();
    }
};

Q_GLOBAL_STATIC(QThreadStorage<QJNIEnvironmentPrivateTLS *>, jniEnvTLS)

QJNIEnvironmentPrivate::QJNIEnvironmentPrivate()
    : jniEnv(0)
{
    JavaVM *vm = QtAndroidPrivate::javaVM();
    if (vm->GetEnv((void**)&jniEnv, JNI_VERSION_1_6) == JNI_EDETACHED) {
        const qulonglong id = reinterpret_cast<qulonglong>(QThread::currentThreadId());
        const QByteArray threadName = threadBaseName() + QByteArray::number(id);
        JavaVMAttachArgs args = { JNI_VERSION_1_6, threadName, Q_NULLPTR };
        if (vm->AttachCurrentThread(&jniEnv, &args) != JNI_OK)
            return;
    }

    if (!jniEnv)
        return;

    if (!jniEnvTLS->hasLocalData())
        jniEnvTLS->setLocalData(new QJNIEnvironmentPrivateTLS);
}

JNIEnv *QJNIEnvironmentPrivate::operator->()
{
    return jniEnv;
}

QJNIEnvironmentPrivate::operator JNIEnv* () const
{
    return jniEnv;
}

QJNIEnvironmentPrivate::~QJNIEnvironmentPrivate()
{
}

QJNIObjectData::QJNIObjectData()
    : m_jobject(0),
      m_jclass(0),
      m_own_jclass(true)
{

}

QJNIObjectData::~QJNIObjectData()
{
    QJNIEnvironmentPrivate env;
    if (m_jobject)
        env->DeleteGlobalRef(m_jobject);
    if (m_jclass && m_own_jclass)
        env->DeleteGlobalRef(m_jclass);
}

QJNIObjectPrivate::QJNIObjectPrivate()
    : d(new QJNIObjectData())
{

}

QJNIObjectPrivate::QJNIObjectPrivate(const char *className)
    : d(new QJNIObjectData())
{
    QJNIEnvironmentPrivate env;
    d->m_jclass = getCachedClass(env, className);
    d->m_own_jclass = false;
    if (d->m_jclass) {
        // get default constructor
        jmethodID constructorId = getCachedMethodID(env, d->m_jclass, "<init>", "()V");
        if (constructorId) {
            jobject obj = env->NewObject(d->m_jclass, constructorId);
            if (obj) {
                d->m_jobject = env->NewGlobalRef(obj);
                env->DeleteLocalRef(obj);
            }
        }
    }
}

QJNIObjectPrivate::QJNIObjectPrivate(const char *className, const char *sig, ...)
    : d(new QJNIObjectData())
{
    QJNIEnvironmentPrivate env;
    d->m_jclass = getCachedClass(env, className);
    d->m_own_jclass = false;
    if (d->m_jclass) {
        jmethodID constructorId = getCachedMethodID(env, d->m_jclass, "<init>", sig);
        if (constructorId) {
            va_list args;
            va_start(args, sig);
            jobject obj = env->NewObjectV(d->m_jclass, constructorId, args);
            va_end(args);
            if (obj) {
                d->m_jobject = env->NewGlobalRef(obj);
                env->DeleteLocalRef(obj);
            }
        }
    }
}

QJNIObjectPrivate::QJNIObjectPrivate(const char *className, const char *sig, va_list args)
    : d(new QJNIObjectData())
{
    QJNIEnvironmentPrivate env;
    d->m_jclass = getCachedClass(env, className);
    d->m_own_jclass = false;
    if (d->m_jclass) {
        jmethodID constructorId = getCachedMethodID(env, d->m_jclass, "<init>", sig);
        if (constructorId) {
            jobject obj = env->NewObjectV(d->m_jclass, constructorId, args);
            if (obj) {
                d->m_jobject = env->NewGlobalRef(obj);
                env->DeleteLocalRef(obj);
            }
        }
    }
}

QJNIObjectPrivate::QJNIObjectPrivate(jclass clazz)
    : d(new QJNIObjectData())
{
    QJNIEnvironmentPrivate env;
    d->m_jclass = static_cast<jclass>(env->NewGlobalRef(clazz));
    if (d->m_jclass) {
        // get default constructor
        jmethodID constructorId = getCachedMethodID(env, d->m_jclass, "<init>", "()V");
        if (constructorId) {
            jobject obj = env->NewObject(d->m_jclass, constructorId);
            if (obj) {
                d->m_jobject = env->NewGlobalRef(obj);
                env->DeleteLocalRef(obj);
            }
        }
    }
}

QJNIObjectPrivate::QJNIObjectPrivate(jclass clazz, const char *sig, ...)
    : d(new QJNIObjectData())
{
    QJNIEnvironmentPrivate env;
    if (clazz) {
        d->m_jclass = static_cast<jclass>(env->NewGlobalRef(clazz));
        if (d->m_jclass) {
            jmethodID constructorId = getCachedMethodID(env, d->m_jclass, "<init>", sig);
            if (constructorId) {
                va_list args;
                va_start(args, sig);
                jobject obj = env->NewObjectV(d->m_jclass, constructorId, args);
                va_end(args);
                if (obj) {
                    d->m_jobject = env->NewGlobalRef(obj);
                    env->DeleteLocalRef(obj);
                }
            }
        }
    }
}

QJNIObjectPrivate::QJNIObjectPrivate(jclass clazz, const char *sig, va_list args)
    : d(new QJNIObjectData())
{
    QJNIEnvironmentPrivate env;
    if (clazz) {
        d->m_jclass = static_cast<jclass>(env->NewGlobalRef(clazz));
        if (d->m_jclass) {
            jmethodID constructorId = getCachedMethodID(env, d->m_jclass, "<init>", sig);
            if (constructorId) {
                jobject obj = env->NewObjectV(d->m_jclass, constructorId, args);
                if (obj) {
                    d->m_jobject = env->NewGlobalRef(obj);
                    env->DeleteLocalRef(obj);
                }
            }
        }
    }
}

QJNIObjectPrivate::QJNIObjectPrivate(jobject obj)
    : d(new QJNIObjectData())
{
    if (!obj)
        return;

    QJNIEnvironmentPrivate env;
    d->m_jobject = env->NewGlobalRef(obj);
    jclass objectClass = env->GetObjectClass(d->m_jobject);
    d->m_jclass = static_cast<jclass>(env->NewGlobalRef(objectClass));
    env->DeleteLocalRef(objectClass);
}

template <>
void QJNIObjectPrivate::callMethod<void>(const char *methodName, const char *sig, va_list args) const
{
    QJNIEnvironmentPrivate env;
    jmethodID id = getCachedMethodID(env, d->m_jclass, methodName, sig);
    if (id) {
        env->CallVoidMethodV(d->m_jobject, id, args);
    }
}

template <>
void QJNIObjectPrivate::callMethod<void>(const char *methodName, const char *sig, ...) const
{
    va_list args;
    va_start(args, sig);
    callMethod<void>(methodName, sig, args);
    va_end(args);
}

template <>
jboolean QJNIObjectPrivate::callMethod<jboolean>(const char *methodName, const char *sig, va_list args) const
{
    QJNIEnvironmentPrivate env;
    jboolean res = 0;
    jmethodID id = getCachedMethodID(env, d->m_jclass, methodName, sig);
    if (id) {
        res = env->CallBooleanMethodV(d->m_jobject, id, args);
    }
    return res;
}

template <>
jboolean QJNIObjectPrivate::callMethod<jboolean>(const char *methodName, const char *sig, ...) const
{
    va_list args;
    va_start(args, sig);
    jboolean res = callMethod<jboolean>(methodName, sig, args);
    va_end(args);
    return res;
}

template <>
jbyte QJNIObjectPrivate::callMethod<jbyte>(const char *methodName, const char *sig, va_list args) const
{
    QJNIEnvironmentPrivate env;
    jbyte res = 0;
    jmethodID id = getCachedMethodID(env, d->m_jclass, methodName, sig);
    if (id) {
        res = env->CallByteMethodV(d->m_jobject, id, args);
    }
    return res;
}

template <>
jbyte QJNIObjectPrivate::callMethod<jbyte>(const char *methodName, const char *sig, ...) const
{
    va_list args;
    va_start(args, sig);
    jbyte res = callMethod<jbyte>(methodName, sig, args);
    va_end(args);
    return res;
}

template <>
jchar QJNIObjectPrivate::callMethod<jchar>(const char *methodName, const char *sig, va_list args) const
{
    QJNIEnvironmentPrivate env;
    jchar res = 0;
    jmethodID id = getCachedMethodID(env, d->m_jclass, methodName, sig);
    if (id) {
        res = env->CallCharMethodV(d->m_jobject, id, args);
    }
    return res;
}

template <>
jchar QJNIObjectPrivate::callMethod<jchar>(const char *methodName, const char *sig, ...) const
{
    va_list args;
    va_start(args, sig);
    jchar res = callMethod<jchar>(methodName, sig, args);
    va_end(args);
    return res;
}

template <>
jshort QJNIObjectPrivate::callMethod<jshort>(const char *methodName, const char *sig, va_list args) const
{
    QJNIEnvironmentPrivate env;
    jshort res = 0;
    jmethodID id = getCachedMethodID(env, d->m_jclass, methodName, sig);
    if (id) {
        res = env->CallShortMethodV(d->m_jobject, id, args);
    }
    return res;
}

template <>
jshort QJNIObjectPrivate::callMethod<jshort>(const char *methodName, const char *sig, ...) const
{
    va_list args;
    va_start(args, sig);
    jshort res = callMethod<jshort>(methodName, sig, args);
    va_end(args);
    return res;
}

template <>
jint QJNIObjectPrivate::callMethod<jint>(const char *methodName, const char *sig, va_list args) const
{
    QJNIEnvironmentPrivate env;
    jint res = 0;
    jmethodID id = getCachedMethodID(env, d->m_jclass, methodName, sig);
    if (id) {
        res = env->CallIntMethodV(d->m_jobject, id, args);
    }
    return res;
}

template <>
jint QJNIObjectPrivate::callMethod<jint>(const char *methodName, const char *sig, ...) const
{
    va_list args;
    va_start(args, sig);
    jint res = callMethod<jint>(methodName, sig, args);
    va_end(args);
    return res;
}

template <>
jlong QJNIObjectPrivate::callMethod<jlong>(const char *methodName, const char *sig, va_list args) const
{
    QJNIEnvironmentPrivate env;
    jlong res = 0;
    jmethodID id = getCachedMethodID(env, d->m_jclass, methodName, sig);
    if (id) {
        res = env->CallLongMethodV(d->m_jobject, id, args);
    }
    return res;
}

template <>
jlong QJNIObjectPrivate::callMethod<jlong>(const char *methodName, const char *sig, ...) const
{
    va_list args;
    va_start(args, sig);
    jlong res = callMethod<jlong>(methodName, sig, args);
    va_end(args);
    return res;
}

template <>
jfloat QJNIObjectPrivate::callMethod<jfloat>(const char *methodName, const char *sig, va_list args) const
{
    QJNIEnvironmentPrivate env;
    jfloat res = 0.f;
    jmethodID id = getCachedMethodID(env, d->m_jclass, methodName, sig);
    if (id) {
        res = env->CallFloatMethodV(d->m_jobject, id, args);
    }
    return res;
}

template <>
jfloat QJNIObjectPrivate::callMethod<jfloat>(const char *methodName, const char *sig, ...) const
{
    va_list args;
    va_start(args, sig);
    jfloat res = callMethod<jfloat>(methodName, sig, args);
    va_end(args);
    return res;
}

template <>
jdouble QJNIObjectPrivate::callMethod<jdouble>(const char *methodName, const char *sig, va_list args) const
{
    QJNIEnvironmentPrivate env;
    jdouble res = 0.;
    jmethodID id = getCachedMethodID(env, d->m_jclass, methodName, sig);
    if (id) {
        res = env->CallDoubleMethodV(d->m_jobject, id, args);
    }
    return res;
}

template <>
jdouble QJNIObjectPrivate::callMethod<jdouble>(const char *methodName, const char *sig, ...) const
{
    va_list args;
    va_start(args, sig);
    jdouble res = callMethod<jdouble>(methodName, sig, args);
    va_end(args);
    return res;
}

template <>
void QJNIObjectPrivate::callMethod<void>(const char *methodName) const
{
    callMethod<void>(methodName, "()V");
}

template <>
jboolean QJNIObjectPrivate::callMethod<jboolean>(const char *methodName) const
{
    return callMethod<jboolean>(methodName, "()Z");
}

template <>
jbyte QJNIObjectPrivate::callMethod<jbyte>(const char *methodName) const
{
    return callMethod<jbyte>(methodName, "()B");
}

template <>
jchar QJNIObjectPrivate::callMethod<jchar>(const char *methodName) const
{
    return callMethod<jchar>(methodName, "()C");
}

template <>
jshort QJNIObjectPrivate::callMethod<jshort>(const char *methodName) const
{
    return callMethod<jshort>(methodName, "()S");
}

template <>
jint QJNIObjectPrivate::callMethod<jint>(const char *methodName) const
{
    return callMethod<jint>(methodName, "()I");
}

template <>
jlong QJNIObjectPrivate::callMethod<jlong>(const char *methodName) const
{
    return callMethod<jlong>(methodName, "()J");
}

template <>
jfloat QJNIObjectPrivate::callMethod<jfloat>(const char *methodName) const
{
    return callMethod<jfloat>(methodName, "()F");
}

template <>
jdouble QJNIObjectPrivate::callMethod<jdouble>(const char *methodName) const
{
    return callMethod<jdouble>(methodName, "()D");
}

template <>
void QJNIObjectPrivate::callStaticMethod<void>(const char *className,
                                               const char *methodName,
                                               const char *sig,
                                               va_list args)
{
    QJNIEnvironmentPrivate env;
    jclass clazz = getCachedClass(env, className);
    if (clazz) {
        jmethodID id = getCachedMethodID(env, clazz, methodName, sig, true);
        if (id) {
            env->CallStaticVoidMethodV(clazz, id, args);
        }
    }
}

template <>
void QJNIObjectPrivate::callStaticMethod<void>(const char *className,
                                               const char *methodName,
                                               const char *sig,
                                               ...)
{
    va_list args;
    va_start(args, sig);
    callStaticMethod<void>(className, methodName, sig, args);
    va_end(args);
}

template <>
void QJNIObjectPrivate::callStaticMethod<void>(jclass clazz,
                                               const char *methodName,
                                               const char *sig,
                                               va_list args)
{
    QJNIEnvironmentPrivate env;
    jmethodID id = getCachedMethodID(env, clazz, methodName, sig, true);
    if (id) {
        env->CallStaticVoidMethodV(clazz, id, args);
    }
}

template <>
void QJNIObjectPrivate::callStaticMethod<void>(jclass clazz,
                                               const char *methodName,
                                               const char *sig,
                                               ...)
{
    va_list args;
    va_start(args, sig);
    callStaticMethod<void>(clazz, methodName, sig, args);
    va_end(args);
}

template <>
jboolean QJNIObjectPrivate::callStaticMethod<jboolean>(const char *className,
                                                       const char *methodName,
                                                       const char *sig,
                                                       va_list args)
{
    QJNIEnvironmentPrivate env;
    jboolean res = 0;
    jclass clazz = getCachedClass(env, className);
    if (clazz) {
        jmethodID id = getCachedMethodID(env, clazz, methodName, sig, true);
        if (id) {
            res = env->CallStaticBooleanMethodV(clazz, id, args);
        }
    }

    return res;
}

template <>
jboolean QJNIObjectPrivate::callStaticMethod<jboolean>(const char *className,
                                                       const char *methodName,
                                                       const char *sig,
                                                       ...)
{
    va_list args;
    va_start(args, sig);
    jboolean res = callStaticMethod<jboolean>(className, methodName, sig, args);
    va_end(args);
    return res;
}

template <>
jboolean QJNIObjectPrivate::callStaticMethod<jboolean>(jclass clazz,
                                                       const char *methodName,
                                                       const char *sig,
                                                       va_list args)
{
    QJNIEnvironmentPrivate env;
    jboolean res = 0;
    jmethodID id = getCachedMethodID(env, clazz, methodName, sig, true);
    if (id) {
        res = env->CallStaticBooleanMethodV(clazz, id, args);
    }

    return res;
}

template <>
jboolean QJNIObjectPrivate::callStaticMethod<jboolean>(jclass clazz,
                                                       const char *methodName,
                                                       const char *sig,
                                                       ...)
{
    va_list args;
    va_start(args, sig);
    jboolean res = callStaticMethod<jboolean>(clazz, methodName, sig);
    va_end(args);
    return res;
}

template <>
jbyte QJNIObjectPrivate::callStaticMethod<jbyte>(const char *className,
                                                 const char *methodName,
                                                 const char *sig,
                                                 va_list args)
{
    QJNIEnvironmentPrivate env;
    jbyte res = 0;
    jclass clazz = getCachedClass(env, className);
    if (clazz) {
        jmethodID id = getCachedMethodID(env, clazz, methodName, sig, true);
        if (id) {
            res = env->CallStaticByteMethodV(clazz, id, args);
        }
    }

    return res;
}

template <>
jbyte QJNIObjectPrivate::callStaticMethod<jbyte>(const char *className,
                                                 const char *methodName,
                                                 const char *sig,
                                                 ...)
{
    va_list args;
    va_start(args, sig);
    jbyte res = callStaticMethod<jbyte>(className, methodName, sig, args);
    va_end(args);
    return res;
}

template <>
jbyte QJNIObjectPrivate::callStaticMethod<jbyte>(jclass clazz,
                                                 const char *methodName,
                                                 const char *sig,
                                                 va_list args)
{
    QJNIEnvironmentPrivate env;
    jbyte res = 0;
    jmethodID id = getCachedMethodID(env, clazz, methodName, sig, true);
    if (id) {
        res = env->CallStaticByteMethodV(clazz, id, args);
    }

    return res;
}

template <>
jbyte QJNIObjectPrivate::callStaticMethod<jbyte>(jclass clazz,
                                                 const char *methodName,
                                                 const char *sig,
                                                 ...)
{
    va_list args;
    va_start(args, sig);
    jbyte res = callStaticMethod<jbyte>(clazz, methodName, sig, args);
    va_end(args);
    return res;
}

template <>
jchar QJNIObjectPrivate::callStaticMethod<jchar>(const char *className,
                                                 const char *methodName,
                                                 const char *sig,
                                                 va_list args)
{
    QJNIEnvironmentPrivate env;
    jchar res = 0;
    jclass clazz = getCachedClass(env, className);
    if (clazz) {
        jmethodID id = getCachedMethodID(env, clazz, methodName, sig, true);
        if (id) {
            res = env->CallStaticCharMethodV(clazz, id, args);
        }
    }

    return res;
}

template <>
jchar QJNIObjectPrivate::callStaticMethod<jchar>(const char *className,
                                                 const char *methodName,
                                                 const char *sig,
                                                 ...)
{
    va_list args;
    va_start(args, sig);
    jchar res = callStaticMethod<jchar>(className, methodName, sig, args);
    va_end(args);
    return res;
}

template <>
jchar QJNIObjectPrivate::callStaticMethod<jchar>(jclass clazz,
                                                 const char *methodName,
                                                 const char *sig,
                                                 va_list args)
{
    QJNIEnvironmentPrivate env;
    jchar res = 0;
    jmethodID id = getCachedMethodID(env, clazz, methodName, sig, true);
    if (id) {
        res = env->CallStaticCharMethodV(clazz, id, args);
    }

    return res;
}

template <>
jchar QJNIObjectPrivate::callStaticMethod<jchar>(jclass clazz,
                                                 const char *methodName,
                                                 const char *sig,
                                                 ...)
{
    va_list args;
    va_start(args, sig);
    jchar res = callStaticMethod<jchar>(clazz, methodName, sig, args);
    va_end(args);
    return res;
}

template <>
jshort QJNIObjectPrivate::callStaticMethod<jshort>(const char *className,
                                                   const char *methodName,
                                                   const char *sig,
                                                   va_list args)
{
    QJNIEnvironmentPrivate env;
    jshort res = 0;
    jclass clazz = getCachedClass(env, className);
    if (clazz) {
        jmethodID id = getCachedMethodID(env, clazz, methodName, sig, true);
        if (id) {
            res = env->CallStaticShortMethodV(clazz, id, args);
        }
    }

    return res;
}

template <>
jshort QJNIObjectPrivate::callStaticMethod<jshort>(const char *className,
                                                   const char *methodName,
                                                   const char *sig,
                                                   ...)
{
    va_list args;
    va_start(args, sig);
    jshort res = callStaticMethod<jshort>(className, methodName, sig, args);
    va_end(args);
    return res;
}

template <>
jshort QJNIObjectPrivate::callStaticMethod<jshort>(jclass clazz,
                                                   const char *methodName,
                                                   const char *sig,
                                                   va_list args)
{
    QJNIEnvironmentPrivate env;
    jshort res = 0;
    jmethodID id = getCachedMethodID(env, clazz, methodName, sig, true);
    if (id) {
        res = env->CallStaticShortMethodV(clazz, id, args);
    }

    return res;
}

template <>
jshort QJNIObjectPrivate::callStaticMethod<jshort>(jclass clazz,
                                                   const char *methodName,
                                                   const char *sig,
                                                   ...)
{
    va_list args;
    va_start(args, sig);
    jshort res = callStaticMethod<jshort>(clazz, methodName, sig, args);
    va_end(args);
    return res;
}

template <>
jint QJNIObjectPrivate::callStaticMethod<jint>(const char *className,
                                               const char *methodName,
                                               const char *sig,
                                               va_list args)
{
    QJNIEnvironmentPrivate env;
    jint res = 0;
    jclass clazz = getCachedClass(env, className);
    if (clazz) {
        jmethodID id = getCachedMethodID(env, clazz, methodName, sig, true);
        if (id) {
            res = env->CallStaticIntMethodV(clazz, id, args);
        }
    }

    return res;
}

template <>
jint QJNIObjectPrivate::callStaticMethod<jint>(const char *className,
                                               const char *methodName,
                                               const char *sig,
                                               ...)
{
    va_list args;
    va_start(args, sig);
    jint res = callStaticMethod<jint>(className, methodName, sig, args);
    va_end(args);
    return res;
}

template <>
jint QJNIObjectPrivate::callStaticMethod<jint>(jclass clazz,
                                               const char *methodName,
                                               const char *sig,
                                               va_list args)
{
    QJNIEnvironmentPrivate env;
    jint res = 0;
    jmethodID id = getCachedMethodID(env, clazz, methodName, sig, true);
    if (id) {
        res = env->CallStaticIntMethodV(clazz, id, args);
    }

    return res;
}

template <>
jint QJNIObjectPrivate::callStaticMethod<jint>(jclass clazz,
                                               const char *methodName,
                                               const char *sig,
                                               ...)
{
    va_list args;
    va_start(args, sig);
    jint res = callStaticMethod<jint>(clazz, methodName, sig, args);
    va_end(args);
    return res;
}

template <>
jlong QJNIObjectPrivate::callStaticMethod<jlong>(const char *className,
                                                 const char *methodName,
                                                 const char *sig,
                                                 va_list args)
{
    QJNIEnvironmentPrivate env;
    jlong res = 0;
    jclass clazz = getCachedClass(env, className);
    if (clazz) {
        jmethodID id = getCachedMethodID(env, clazz, methodName, sig, true);
        if (id) {
            res = env->CallStaticLongMethodV(clazz, id, args);
        }
    }

    return res;
}

template <>
jlong QJNIObjectPrivate::callStaticMethod<jlong>(const char *className,
                                                 const char *methodName,
                                                 const char *sig,
                                                 ...)
{
    va_list args;
    va_start(args, sig);
    jlong res = callStaticMethod<jlong>(className, methodName, sig, args);
    va_end(args);
    return res;
}

template <>
jlong QJNIObjectPrivate::callStaticMethod<jlong>(jclass clazz,
                                                 const char *methodName,
                                                 const char *sig,
                                                 va_list args)
{
    QJNIEnvironmentPrivate env;
    jlong res = 0;
    jmethodID id = getCachedMethodID(env, clazz, methodName, sig, true);
    if (id) {
        res = env->CallStaticLongMethodV(clazz, id, args);
    }

    return res;
}

template <>
jlong QJNIObjectPrivate::callStaticMethod<jlong>(jclass clazz,
                                                 const char *methodName,
                                                 const char *sig,
                                                 ...)
{
    va_list args;
    va_start(args, sig);
    jlong res = callStaticMethod<jlong>(clazz, methodName, sig);
    va_end(args);
    return res;
}

template <>
jfloat QJNIObjectPrivate::callStaticMethod<jfloat>(const char *className,
                                                   const char *methodName,
                                                   const char *sig,
                                                   va_list args)
{
    QJNIEnvironmentPrivate env;
    jfloat res = 0.f;
    jclass clazz = getCachedClass(env, className);
    if (clazz) {
        jmethodID id = getCachedMethodID(env, clazz, methodName, sig, true);
        if (id) {
            res = env->CallStaticFloatMethodV(clazz, id, args);
        }
    }

    return res;
}

template <>
jfloat QJNIObjectPrivate::callStaticMethod<jfloat>(const char *className,
                                                   const char *methodName,
                                                   const char *sig,
                                                   ...)
{
    va_list args;
    va_start(args, sig);
    jfloat res = callStaticMethod<jfloat>(className, methodName, sig, args);
    va_end(args);
    return res;
}

template <>
jfloat QJNIObjectPrivate::callStaticMethod<jfloat>(jclass clazz,
                                                   const char *methodName,
                                                   const char *sig,
                                                   va_list args)
{
    QJNIEnvironmentPrivate env;
    jfloat res = 0.f;
    jmethodID id = getCachedMethodID(env, clazz, methodName, sig, true);
    if (id) {
        res = env->CallStaticFloatMethodV(clazz, id, args);
    }

    return res;
}

template <>
jfloat QJNIObjectPrivate::callStaticMethod<jfloat>(jclass clazz,
                                                   const char *methodName,
                                                   const char *sig,
                                                   ...)
{
    va_list args;
    va_start(args, sig);
    jfloat res = callStaticMethod<jfloat>(clazz, methodName, sig, args);
    va_end(args);
    return res;
}

template <>
jdouble QJNIObjectPrivate::callStaticMethod<jdouble>(const char *className,
                                                     const char *methodName,
                                                     const char *sig,
                                                     va_list args)
{
    QJNIEnvironmentPrivate env;
    jdouble res = 0.;
    jclass clazz = getCachedClass(env, className);
    if (clazz) {
        jmethodID id = getCachedMethodID(env, clazz, methodName, sig, true);
        if (id) {
            res = env->CallStaticDoubleMethodV(clazz, id, args);
        }
    }

    return res;
}

template <>
jdouble QJNIObjectPrivate::callStaticMethod<jdouble>(const char *className,
                                                     const char *methodName,
                                                     const char *sig,
                                                     ...)
{
    va_list args;
    va_start(args, sig);
    jdouble res = callStaticMethod<jdouble>(className, methodName, sig);
    va_end(args);
    return res;
}

template <>
jdouble QJNIObjectPrivate::callStaticMethod<jdouble>(jclass clazz,
                                                     const char *methodName,
                                                     const char *sig,
                                                     va_list args)
{
    QJNIEnvironmentPrivate env;
    jdouble res = 0.;
    jmethodID id = getCachedMethodID(env, clazz, methodName, sig, true);
    if (id) {
        res = env->CallStaticDoubleMethodV(clazz, id, args);
    }

    return res;
}

template <>
jdouble QJNIObjectPrivate::callStaticMethod<jdouble>(jclass clazz,
                                                     const char *methodName,
                                                     const char *sig,
                                                     ...)
{
    va_list args;
    va_start(args, sig);
    jdouble res = callStaticMethod<jdouble>(clazz, methodName, sig, args);
    va_end(args);
    return res;
}

template <>
void QJNIObjectPrivate::callStaticMethod<void>(const char *className, const char *methodName)
{
    callStaticMethod<void>(className, methodName, "()V");
}

template <>
void QJNIObjectPrivate::callStaticMethod<void>(jclass clazz, const char *methodName)
{
    callStaticMethod<void>(clazz, methodName, "()V");
}

template <>
jboolean QJNIObjectPrivate::callStaticMethod<jboolean>(const char *className, const char *methodName)
{
    return callStaticMethod<jboolean>(className, methodName, "()Z");
}

template <>
jboolean QJNIObjectPrivate::callStaticMethod<jboolean>(jclass clazz, const char *methodName)
{
    return callStaticMethod<jboolean>(clazz, methodName, "()Z");
}

template <>
jbyte QJNIObjectPrivate::callStaticMethod<jbyte>(const char *className, const char *methodName)
{
    return callStaticMethod<jbyte>(className, methodName, "()B");
}

template <>
jbyte QJNIObjectPrivate::callStaticMethod<jbyte>(jclass clazz, const char *methodName)
{
    return callStaticMethod<jbyte>(clazz, methodName, "()B");
}

template <>
jchar QJNIObjectPrivate::callStaticMethod<jchar>(const char *className, const char *methodName)
{
    return callStaticMethod<jchar>(className, methodName, "()C");
}

template <>
jchar QJNIObjectPrivate::callStaticMethod<jchar>(jclass clazz, const char *methodName)
{
    return callStaticMethod<jchar>(clazz, methodName, "()C");
}

template <>
jshort QJNIObjectPrivate::callStaticMethod<jshort>(const char *className, const char *methodName)
{
    return callStaticMethod<jshort>(className, methodName, "()S");
}

template <>
jshort QJNIObjectPrivate::callStaticMethod<jshort>(jclass clazz, const char *methodName)
{
    return callStaticMethod<jshort>(clazz, methodName, "()S");
}

template <>
jint QJNIObjectPrivate::callStaticMethod<jint>(const char *className, const char *methodName)
{
    return callStaticMethod<jint>(className, methodName, "()I");
}

template <>
jint QJNIObjectPrivate::callStaticMethod<jint>(jclass clazz, const char *methodName)
{
    return callStaticMethod<jint>(clazz, methodName, "()I");
}

template <>
jlong QJNIObjectPrivate::callStaticMethod<jlong>(const char *className, const char *methodName)
{
    return callStaticMethod<jlong>(className, methodName, "()J");
}

template <>
jlong QJNIObjectPrivate::callStaticMethod<jlong>(jclass clazz, const char *methodName)
{
    return callStaticMethod<jlong>(clazz, methodName, "()J");
}

template <>
jfloat QJNIObjectPrivate::callStaticMethod<jfloat>(const char *className, const char *methodName)
{
    return callStaticMethod<jfloat>(className, methodName, "()F");
}

template <>
jfloat QJNIObjectPrivate::callStaticMethod<jfloat>(jclass clazz, const char *methodName)
{
    return callStaticMethod<jfloat>(clazz, methodName, "()F");
}

template <>
jdouble QJNIObjectPrivate::callStaticMethod<jdouble>(const char *className, const char *methodName)
{
    return callStaticMethod<jdouble>(className, methodName, "()D");
}

template <>
jdouble QJNIObjectPrivate::callStaticMethod<jdouble>(jclass clazz, const char *methodName)
{
    return callStaticMethod<jdouble>(clazz, methodName, "()D");
}

QJNIObjectPrivate QJNIObjectPrivate::callObjectMethod(const char *methodName,
                                                      const char *sig,
                                                      va_list args) const
{
    QJNIEnvironmentPrivate env;
    jobject res = 0;
    jmethodID id = getCachedMethodID(env, d->m_jclass, methodName, sig);
    if (id) {
        res = env->CallObjectMethodV(d->m_jobject, id, args);
        if (res && env->ExceptionCheck())
            res = 0;
    }

    QJNIObjectPrivate obj(res);
    env->DeleteLocalRef(res);
    return obj;
}

QJNIObjectPrivate QJNIObjectPrivate::callObjectMethod(const char *methodName,
                                                      const char *sig,
                                                      ...) const
{
    va_list args;
    va_start(args, sig);
    QJNIObjectPrivate res = callObjectMethod(methodName, sig, args);
    va_end(args);
    return res;
}

template <>
QJNIObjectPrivate QJNIObjectPrivate::callObjectMethod<jstring>(const char *methodName) const
{
    return callObjectMethod(methodName, "()Ljava/lang/String;");
}

template <>
QJNIObjectPrivate QJNIObjectPrivate::callObjectMethod<jbooleanArray>(const char *methodName) const
{
    return callObjectMethod(methodName, "()[Z");
}

template <>
QJNIObjectPrivate QJNIObjectPrivate::callObjectMethod<jbyteArray>(const char *methodName) const
{
    return callObjectMethod(methodName, "()[B");
}

template <>
QJNIObjectPrivate QJNIObjectPrivate::callObjectMethod<jshortArray>(const char *methodName) const
{
    return callObjectMethod(methodName, "()[S");
}

template <>
QJNIObjectPrivate QJNIObjectPrivate::callObjectMethod<jintArray>(const char *methodName) const
{
    return callObjectMethod(methodName, "()[I");
}

template <>
QJNIObjectPrivate QJNIObjectPrivate::callObjectMethod<jlongArray>(const char *methodName) const
{
    return callObjectMethod(methodName, "()[J");
}

template <>
QJNIObjectPrivate QJNIObjectPrivate::callObjectMethod<jfloatArray>(const char *methodName) const
{
    return callObjectMethod(methodName, "()[F");
}

template <>
QJNIObjectPrivate QJNIObjectPrivate::callObjectMethod<jdoubleArray>(const char *methodName) const
{
    return callObjectMethod(methodName, "()[D");
}

QJNIObjectPrivate QJNIObjectPrivate::callStaticObjectMethod(const char *className,
                                                            const char *methodName,
                                                            const char *sig,
                                                            va_list args)
{
    QJNIEnvironmentPrivate env;
    jobject res = 0;
    jclass clazz = getCachedClass(env, className);
    if (clazz) {
        jmethodID id = getCachedMethodID(env, clazz, methodName, sig, true);
        if (id) {
            res = env->CallStaticObjectMethodV(clazz, id, args);
            if (res && env->ExceptionCheck())
                res = 0;
        }
    }

    QJNIObjectPrivate obj(res);
    env->DeleteLocalRef(res);
    return obj;
}

QJNIObjectPrivate QJNIObjectPrivate::callStaticObjectMethod(const char *className,
                                                            const char *methodName,
                                                            const char *sig,
                                                            ...)
{
    va_list args;
    va_start(args, sig);
    QJNIObjectPrivate res = callStaticObjectMethod(className, methodName, sig, args);
    va_end(args);
    return res;
}

QJNIObjectPrivate QJNIObjectPrivate::callStaticObjectMethod(jclass clazz,
                                                            const char *methodName,
                                                            const char *sig,
                                                            va_list args)
{
    QJNIEnvironmentPrivate env;
    jobject res = 0;
    jmethodID id = getCachedMethodID(env, clazz, methodName, sig, true);
    if (id) {
        res = env->CallStaticObjectMethodV(clazz, id, args);
        if (res && env->ExceptionCheck())
            res = 0;
    }

    QJNIObjectPrivate obj(res);
    env->DeleteLocalRef(res);
    return obj;
}

QJNIObjectPrivate QJNIObjectPrivate::callStaticObjectMethod(jclass clazz,
                                                            const char *methodName,
                                                            const char *sig,
                                                            ...)
{
    va_list args;
    va_start(args, sig);
    QJNIObjectPrivate res = callStaticObjectMethod(clazz, methodName, sig, args);
    va_end(args);
    return res;
}

template <>
jboolean QJNIObjectPrivate::getField<jboolean>(const char *fieldName) const
{
    QJNIEnvironmentPrivate env;
    jboolean res = 0;
    jfieldID id = getCachedFieldID(env, d->m_jclass, fieldName, "Z");
    if (id)
        res = env->GetBooleanField(d->m_jobject, id);

    return res;
}

template <>
jbyte QJNIObjectPrivate::getField<jbyte>(const char *fieldName) const
{
    QJNIEnvironmentPrivate env;
    jbyte res = 0;
    jfieldID id = getCachedFieldID(env, d->m_jclass, fieldName, "B");
    if (id)
        res = env->GetByteField(d->m_jobject, id);

    return res;
}

template <>
jchar QJNIObjectPrivate::getField<jchar>(const char *fieldName) const
{
    QJNIEnvironmentPrivate env;
    jchar res = 0;
    jfieldID id = getCachedFieldID(env, d->m_jclass, fieldName, "C");
    if (id)
        res = env->GetCharField(d->m_jobject, id);

    return res;
}

template <>
jshort QJNIObjectPrivate::getField<jshort>(const char *fieldName) const
{
    QJNIEnvironmentPrivate env;
    jshort res = 0;
    jfieldID id = getCachedFieldID(env, d->m_jclass, fieldName, "S");
    if (id)
        res = env->GetShortField(d->m_jobject, id);

    return res;
}

template <>
jint QJNIObjectPrivate::getField<jint>(const char *fieldName) const
{
    QJNIEnvironmentPrivate env;
    jint res = 0;
    jfieldID id = getCachedFieldID(env, d->m_jclass, fieldName, "I");
    if (id)
        res = env->GetIntField(d->m_jobject, id);

    return res;
}

template <>
jlong QJNIObjectPrivate::getField<jlong>(const char *fieldName) const
{
    QJNIEnvironmentPrivate env;
    jlong res = 0;
    jfieldID id = getCachedFieldID(env, d->m_jclass, fieldName, "J");
    if (id)
        res = env->GetLongField(d->m_jobject, id);

    return res;
}

template <>
jfloat QJNIObjectPrivate::getField<jfloat>(const char *fieldName) const
{
    QJNIEnvironmentPrivate env;
    jfloat res = 0.f;
    jfieldID id = getCachedFieldID(env, d->m_jclass, fieldName, "F");
    if (id)
        res = env->GetFloatField(d->m_jobject, id);

    return res;
}

template <>
jdouble QJNIObjectPrivate::getField<jdouble>(const char *fieldName) const
{
    QJNIEnvironmentPrivate env;
    jdouble res = 0.;
    jfieldID id = getCachedFieldID(env, d->m_jclass, fieldName, "D");
    if (id)
        res = env->GetDoubleField(d->m_jobject, id);

    return res;
}

template <>
jboolean QJNIObjectPrivate::getStaticField<jboolean>(jclass clazz, const char *fieldName)
{
    QJNIEnvironmentPrivate env;
    jboolean res = 0;
    jfieldID id = getCachedFieldID(env, clazz, fieldName, "Z", true);
    if (id)
        res = env->GetStaticBooleanField(clazz, id);

    return res;
}

template <>
jboolean QJNIObjectPrivate::getStaticField<jboolean>(const char *className, const char *fieldName)
{
    QJNIEnvironmentPrivate env;
    jboolean res = 0;
    jclass clazz = getCachedClass(env, className);
    if (clazz)
        res = getStaticField<jboolean>(clazz, fieldName);

    return res;
}

template <>
jbyte QJNIObjectPrivate::getStaticField<jbyte>(jclass clazz, const char *fieldName)
{
    QJNIEnvironmentPrivate env;
    jbyte res = 0;
    jfieldID id = getCachedFieldID(env, clazz, fieldName, "B", true);
    if (id)
        res = env->GetStaticByteField(clazz, id);

    return res;
}

template <>
jbyte QJNIObjectPrivate::getStaticField<jbyte>(const char *className, const char *fieldName)
{
    QJNIEnvironmentPrivate env;
    jbyte res = 0;
    jclass clazz = getCachedClass(env, className);
    if (clazz)
        res = getStaticField<jbyte>(clazz, fieldName);

    return res;
}

template <>
jchar QJNIObjectPrivate::getStaticField<jchar>(jclass clazz, const char *fieldName)
{
    QJNIEnvironmentPrivate env;
    jchar res = 0;
    jfieldID id = getCachedFieldID(env, clazz, fieldName, "C", true);
    if (id)
        res = env->GetStaticCharField(clazz, id);

    return res;
}

template <>
jchar QJNIObjectPrivate::getStaticField<jchar>(const char *className, const char *fieldName)
{
    QJNIEnvironmentPrivate env;
    jchar res = 0;
    jclass clazz = getCachedClass(env, className);
    if (clazz)
        res = getStaticField<jchar>(clazz, fieldName);

    return res;
}

template <>
jshort QJNIObjectPrivate::getStaticField<jshort>(jclass clazz, const char *fieldName)
{
    QJNIEnvironmentPrivate env;
    jshort res = 0;
    jfieldID id = getCachedFieldID(env, clazz, fieldName, "S", true);
    if (id)
        res = env->GetStaticShortField(clazz, id);

    return res;
}

template <>
jshort QJNIObjectPrivate::getStaticField<jshort>(const char *className, const char *fieldName)
{
    QJNIEnvironmentPrivate env;
    jshort res = 0;
    jclass clazz = getCachedClass(env, className);
    if (clazz)
        res = getStaticField<jshort>(clazz, fieldName);

    return res;
}

template <>
jint QJNIObjectPrivate::getStaticField<jint>(jclass clazz, const char *fieldName)
{
    QJNIEnvironmentPrivate env;
    jint res = 0;
    jfieldID id = getCachedFieldID(env, clazz, fieldName, "I", true);
    if (id)
        res = env->GetStaticIntField(clazz, id);

    return res;
}

template <>
jint QJNIObjectPrivate::getStaticField<jint>(const char *className, const char *fieldName)
{
    QJNIEnvironmentPrivate env;
    jint res = 0;
    jclass clazz = getCachedClass(env, className);
    if (clazz)
        res = getStaticField<jint>(clazz, fieldName);

    return res;
}

template <>
jlong QJNIObjectPrivate::getStaticField<jlong>(jclass clazz, const char *fieldName)
{
    QJNIEnvironmentPrivate env;
    jlong res = 0;
    jfieldID id = getCachedFieldID(env, clazz, fieldName, "J", true);
    if (id)
        res = env->GetStaticLongField(clazz, id);

    return res;
}

template <>
jlong QJNIObjectPrivate::getStaticField<jlong>(const char *className, const char *fieldName)
{
    QJNIEnvironmentPrivate env;
    jlong res = 0;
    jclass clazz = getCachedClass(env, className);
    if (clazz)
        res = getStaticField<jlong>(clazz, fieldName);

    return res;
}

template <>
jfloat QJNIObjectPrivate::getStaticField<jfloat>(jclass clazz, const char *fieldName)
{
    QJNIEnvironmentPrivate env;
    jfloat res = 0.f;
    jfieldID id = getCachedFieldID(env, clazz, fieldName, "F", true);
    if (id)
        res = env->GetStaticFloatField(clazz, id);

    return res;
}

template <>
jfloat QJNIObjectPrivate::getStaticField<jfloat>(const char *className, const char *fieldName)
{
    QJNIEnvironmentPrivate env;
    jfloat res = 0.f;
    jclass clazz = getCachedClass(env, className);
    if (clazz)
        res = getStaticField<jfloat>(clazz, fieldName);

    return res;
}

template <>
jdouble QJNIObjectPrivate::getStaticField<jdouble>(jclass clazz, const char *fieldName)
{
    QJNIEnvironmentPrivate env;
    jdouble res = 0.;
    jfieldID id = getCachedFieldID(env, clazz, fieldName, "D", true);
    if (id)
        res = env->GetStaticDoubleField(clazz, id);

    return res;
}

template <>
jdouble QJNIObjectPrivate::getStaticField<jdouble>(const char *className, const char *fieldName)
{
    QJNIEnvironmentPrivate env;
    jdouble res = 0.;
    jclass clazz = getCachedClass(env, className);
    if (clazz)
        res = getStaticField<jdouble>(clazz, fieldName);

    return res;
}

QJNIObjectPrivate QJNIObjectPrivate::getObjectField(const char *fieldName,
                                                    const char *sig) const
{
    QJNIEnvironmentPrivate env;
    jobject res = 0;
    jfieldID id = getCachedFieldID(env, d->m_jclass, fieldName, sig);
    if (id) {
        res = env->GetObjectField(d->m_jobject, id);
        if (res && env->ExceptionCheck())
            res = 0;
    }

    QJNIObjectPrivate obj(res);
    env->DeleteLocalRef(res);
    return obj;
}

QJNIObjectPrivate QJNIObjectPrivate::getStaticObjectField(const char *className,
                                                          const char *fieldName,
                                                          const char *sig)
{
    QJNIEnvironmentPrivate env;
    QJNIObjectPrivate res;
    jclass clazz = getCachedClass(env, className);
    if (clazz)
        res = getStaticObjectField(clazz, fieldName, sig);

    return res;
}

QJNIObjectPrivate QJNIObjectPrivate::getStaticObjectField(jclass clazz,
                                                          const char *fieldName,
                                                          const char *sig)
{
    QJNIEnvironmentPrivate env;
    jobject res = 0;
    jfieldID id = getCachedFieldID(env, clazz, fieldName, sig, true);
    if (id) {
        res = env->GetStaticObjectField(clazz, id);
        if (res && env->ExceptionCheck())
            res = 0;
    }

    QJNIObjectPrivate obj(res);
    env->DeleteLocalRef(res);
    return obj;
}

template <>
void QJNIObjectPrivate::setField<jboolean>(const char *fieldName, jboolean value)
{
    QJNIEnvironmentPrivate env;
    jfieldID id = getCachedFieldID(env, d->m_jclass, fieldName, "Z");
    if (id)
        env->SetBooleanField(d->m_jobject, id, value);

}

template <>
void QJNIObjectPrivate::setField<jbyte>(const char *fieldName, jbyte value)
{
    QJNIEnvironmentPrivate env;
    jfieldID id = getCachedFieldID(env, d->m_jclass, fieldName, "B");
    if (id)
        env->SetByteField(d->m_jobject, id, value);

}

template <>
void QJNIObjectPrivate::setField<jchar>(const char *fieldName, jchar value)
{
    QJNIEnvironmentPrivate env;
    jfieldID id = getCachedFieldID(env, d->m_jclass, fieldName, "C");
    if (id)
        env->SetCharField(d->m_jobject, id, value);

}

template <>
void QJNIObjectPrivate::setField<jshort>(const char *fieldName, jshort value)
{
    QJNIEnvironmentPrivate env;
    jfieldID id = getCachedFieldID(env, d->m_jclass, fieldName, "S");
    if (id)
        env->SetShortField(d->m_jobject, id, value);

}

template <>
void QJNIObjectPrivate::setField<jint>(const char *fieldName, jint value)
{
    QJNIEnvironmentPrivate env;
    jfieldID id = getCachedFieldID(env, d->m_jclass, fieldName, "I");
    if (id)
        env->SetIntField(d->m_jobject, id, value);

}

template <>
void QJNIObjectPrivate::setField<jlong>(const char *fieldName, jlong value)
{
    QJNIEnvironmentPrivate env;
    jfieldID id = getCachedFieldID(env, d->m_jclass, fieldName, "J");
    if (id)
        env->SetLongField(d->m_jobject, id, value);

}

template <>
void QJNIObjectPrivate::setField<jfloat>(const char *fieldName, jfloat value)
{
    QJNIEnvironmentPrivate env;
    jfieldID id = getCachedFieldID(env, d->m_jclass, fieldName, "F");
    if (id)
        env->SetFloatField(d->m_jobject, id, value);

}

template <>
void QJNIObjectPrivate::setField<jdouble>(const char *fieldName, jdouble value)
{
    QJNIEnvironmentPrivate env;
    jfieldID id = getCachedFieldID(env, d->m_jclass, fieldName, "D");
    if (id)
        env->SetDoubleField(d->m_jobject, id, value);

}

template <>
void QJNIObjectPrivate::setField<jbooleanArray>(const char *fieldName, jbooleanArray value)
{
    QJNIEnvironmentPrivate env;
    jfieldID id = getCachedFieldID(env, d->m_jclass, fieldName, "[Z");
    if (id)
        env->SetObjectField(d->m_jobject, id, value);

}

template <>
void QJNIObjectPrivate::setField<jbyteArray>(const char *fieldName, jbyteArray value)
{
    QJNIEnvironmentPrivate env;
    jfieldID id = getCachedFieldID(env, d->m_jclass, fieldName, "[B");
    if (id)
        env->SetObjectField(d->m_jobject, id, value);

}

template <>
void QJNIObjectPrivate::setField<jcharArray>(const char *fieldName, jcharArray value)
{
    QJNIEnvironmentPrivate env;
    jfieldID id = getCachedFieldID(env, d->m_jclass, fieldName, "[C");
    if (id)
        env->SetObjectField(d->m_jobject, id, value);

}

template <>
void QJNIObjectPrivate::setField<jshortArray>(const char *fieldName, jshortArray value)
{
    QJNIEnvironmentPrivate env;
    jfieldID id = getCachedFieldID(env, d->m_jclass, fieldName, "[S");
    if (id)
        env->SetObjectField(d->m_jobject, id, value);

}

template <>
void QJNIObjectPrivate::setField<jintArray>(const char *fieldName, jintArray value)
{
    QJNIEnvironmentPrivate env;
    jfieldID id = getCachedFieldID(env, d->m_jclass, fieldName, "[I");
    if (id)
        env->SetObjectField(d->m_jobject, id, value);

}

template <>
void QJNIObjectPrivate::setField<jlongArray>(const char *fieldName, jlongArray value)
{
    QJNIEnvironmentPrivate env;
    jfieldID id = getCachedFieldID(env, d->m_jclass, fieldName, "[J");
    if (id)
        env->SetObjectField(d->m_jobject, id, value);

}

template <>
void QJNIObjectPrivate::setField<jfloatArray>(const char *fieldName, jfloatArray value)
{
    QJNIEnvironmentPrivate env;
    jfieldID id = getCachedFieldID(env, d->m_jclass, fieldName, "[F");
    if (id)
        env->SetObjectField(d->m_jobject, id, value);

}

template <>
void QJNIObjectPrivate::setField<jdoubleArray>(const char *fieldName, jdoubleArray value)
{
    QJNIEnvironmentPrivate env;
    jfieldID id = getCachedFieldID(env, d->m_jclass, fieldName, "[D");
    if (id)
        env->SetObjectField(d->m_jobject, id, value);

}

template <>
void QJNIObjectPrivate::setField<jstring>(const char *fieldName, jstring value)
{
    QJNIEnvironmentPrivate env;
    jfieldID id = getCachedFieldID(env, d->m_jclass, fieldName, "Ljava/lang/String;");
    if (id)
        env->SetObjectField(d->m_jobject, id, value);

}

template <>
void QJNIObjectPrivate::setField<jobject>(const char *fieldName,
                                          const char *sig,
                                          jobject value)
{
    QJNIEnvironmentPrivate env;
    jfieldID id = getCachedFieldID(env, d->m_jclass, fieldName, sig);
    if (id)
        env->SetObjectField(d->m_jobject, id, value);

}

template <>
void QJNIObjectPrivate::setField<jobjectArray>(const char *fieldName,
                                               const char *sig,
                                               jobjectArray value)
{
    QJNIEnvironmentPrivate env;
    jfieldID id = getCachedFieldID(env, d->m_jclass, fieldName, sig);
    if (id)
        env->SetObjectField(d->m_jobject, id, value);

}

template <>
void QJNIObjectPrivate::setStaticField<jboolean>(jclass clazz,
                                                 const char *fieldName,
                                                 jboolean value)
{
    QJNIEnvironmentPrivate env;
    jfieldID id = getCachedFieldID(env, clazz, fieldName, "Z", true);
    if (id)
        env->SetStaticBooleanField(clazz, id, value);
}

template <>
void QJNIObjectPrivate::setStaticField<jboolean>(const char *className,
                                                 const char *fieldName,
                                                 jboolean value)
{
    QJNIEnvironmentPrivate env;
    jclass clazz = getCachedClass(env, className);
    if (clazz)
        setStaticField<jboolean>(clazz, fieldName, value);
}

template <>
void QJNIObjectPrivate::setStaticField<jbyte>(jclass clazz,
                                              const char *fieldName,
                                              jbyte value)
{
    QJNIEnvironmentPrivate env;
    jfieldID id = getCachedFieldID(env, clazz, fieldName, "B", true);
    if (id)
        env->SetStaticByteField(clazz, id, value);
}

template <>
void QJNIObjectPrivate::setStaticField<jbyte>(const char *className,
                                              const char *fieldName,
                                              jbyte value)
{
    QJNIEnvironmentPrivate env;
    jclass clazz = getCachedClass(env, className);
    if (clazz)
        setStaticField<jbyte>(clazz, fieldName, value);
}

template <>
void QJNIObjectPrivate::setStaticField<jchar>(jclass clazz,
                                              const char *fieldName,
                                              jchar value)
{
    QJNIEnvironmentPrivate env;
    jfieldID id = getCachedFieldID(env, clazz, fieldName, "C", true);
    if (id)
        env->SetStaticCharField(clazz, id, value);
}

template <>
void QJNIObjectPrivate::setStaticField<jchar>(const char *className,
                                              const char *fieldName,
                                              jchar value)
{
    QJNIEnvironmentPrivate env;
    jclass clazz = getCachedClass(env, className);
    if (clazz)
        setStaticField<jchar>(clazz, fieldName, value);
}

template <>
void QJNIObjectPrivate::setStaticField<jshort>(jclass clazz,
                                               const char *fieldName,
                                               jshort value)
{
    QJNIEnvironmentPrivate env;
    jfieldID id = getCachedFieldID(env, clazz, fieldName, "S", true);
    if (id)
        env->SetStaticShortField(clazz, id, value);
}

template <>
void QJNIObjectPrivate::setStaticField<jshort>(const char *className,
                                               const char *fieldName,
                                               jshort value)
{
    QJNIEnvironmentPrivate env;
    jclass clazz = getCachedClass(env, className);
    if (clazz)
        setStaticField<jshort>(clazz, fieldName, value);
}

template <>
void QJNIObjectPrivate::setStaticField<jint>(jclass clazz,
                                             const char *fieldName,
                                             jint value)
{
    QJNIEnvironmentPrivate env;
    jfieldID id = getCachedFieldID(env, clazz, fieldName, "I", true);
    if (id)
        env->SetStaticIntField(clazz, id, value);
}

template <>
void QJNIObjectPrivate::setStaticField<jint>(const char *className,
                                             const char *fieldName,
                                             jint value)
{
    QJNIEnvironmentPrivate env;
    jclass clazz = getCachedClass(env, className);
    if (clazz)
        setStaticField<jint>(clazz, fieldName, value);
}

template <>
void QJNIObjectPrivate::setStaticField<jlong>(jclass clazz,
                                              const char *fieldName,
                                              jlong value)
{
    QJNIEnvironmentPrivate env;
    jfieldID id = getCachedFieldID(env, clazz, fieldName, "J", true);
    if (id)
        env->SetStaticLongField(clazz, id, value);
}

template <>
void QJNIObjectPrivate::setStaticField<jlong>(const char *className,
                                              const char *fieldName,
                                              jlong value)
{
    QJNIEnvironmentPrivate env;
    jclass clazz = getCachedClass(env, className);
    if (clazz)
        setStaticField<jlong>(clazz, fieldName, value);
}

template <>
void QJNIObjectPrivate::setStaticField<jfloat>(jclass clazz,
                                               const char *fieldName,
                                               jfloat value)
{
    QJNIEnvironmentPrivate env;
    jfieldID id = getCachedFieldID(env, clazz, fieldName, "F", true);
    if (id)
        env->SetStaticFloatField(clazz, id, value);
}

template <>
void QJNIObjectPrivate::setStaticField<jfloat>(const char *className,
                                               const char *fieldName,
                                               jfloat value)
{
    QJNIEnvironmentPrivate env;
    jclass clazz = getCachedClass(env, className);
    if (clazz)
        setStaticField<jfloat>(clazz, fieldName, value);
}

template <>
void QJNIObjectPrivate::setStaticField<jdouble>(jclass clazz,
                                                const char *fieldName,
                                                jdouble value)
{
    QJNIEnvironmentPrivate env;
    jfieldID id = getCachedFieldID(env, clazz, fieldName, "D", true);
    if (id)
        env->SetStaticDoubleField(clazz, id, value);
}

template <>
void QJNIObjectPrivate::setStaticField<jdouble>(const char *className,
                                                const char *fieldName,
                                                jdouble value)
{
    QJNIEnvironmentPrivate env;
    jclass clazz = getCachedClass(env, className);
    if (clazz)
        setStaticField<jdouble>(clazz, fieldName, value);
}

template <>
void QJNIObjectPrivate::setStaticField<jobject>(jclass clazz,
                                                const char *fieldName,
                                                const char *sig,
                                                jobject value)
{
    QJNIEnvironmentPrivate env;
    jfieldID id = getCachedFieldID(env, clazz, fieldName, sig, true);
    if (id)
        env->SetStaticObjectField(clazz, id, value);
}

template <>
void QJNIObjectPrivate::setStaticField<jobject>(const char *className,
                                                const char *fieldName,
                                                const char *sig,
                                                jobject value)
{
    QJNIEnvironmentPrivate env;
    jclass clazz = getCachedClass(env, className);
    if (clazz)
        setStaticField<jobject>(clazz, fieldName, sig, value);
}

QJNIObjectPrivate QJNIObjectPrivate::fromString(const QString &string)
{
    QJNIEnvironmentPrivate env;
    jstring res = env->NewString(reinterpret_cast<const jchar*>(string.constData()),
                                        string.length());
    QJNIObjectPrivate obj(res);
    env->DeleteLocalRef(res);
    return obj;
}

QString QJNIObjectPrivate::toString() const
{
    if (!isValid())
        return QString();

    QJNIObjectPrivate string = callObjectMethod<jstring>("toString");
    return qt_convertJString(static_cast<jstring>(string.object()));
}

bool QJNIObjectPrivate::isClassAvailable(const char *className)
{
    QJNIEnvironmentPrivate env;

    if (!env)
        return false;

    jclass clazz = getCachedClass(env, className);
    return (clazz != 0);
}

bool QJNIObjectPrivate::isValid() const
{
    return d->m_jobject;
}

bool QJNIObjectPrivate::isSameObject(jobject obj) const
{
    QJNIEnvironmentPrivate env;
    return env->IsSameObject(d->m_jobject, obj);
}

bool QJNIObjectPrivate::isSameObject(const QJNIObjectPrivate &other) const
{
    return isSameObject(other.d->m_jobject);
}

QT_END_NAMESPACE

