/****************************************************************************
**
** Copyright (C) 2012 BogDan Vatra <bogdan@kde.org>
** Contact: http://www.qt-project.org/legal
**
** This file is part of the plugins of the Qt Toolkit.
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

#include "androidjniclipboard.h"
#include "androidjnimain.h"

using namespace QtAndroid;
namespace QtAndroidClipboard
{
    // Clipboard support
    static jmethodID m_registerClipboardManagerMethodID = 0;
    static jmethodID m_setClipboardTextMethodID = 0;
    static jmethodID m_hasClipboardTextMethodID = 0;
    static jmethodID m_getClipboardTextMethodID = 0;
    // Clipboard support

    void setClipboardListener(QAndroidPlatformClipboard *listener)
    {
        Q_UNUSED(listener);

        AttachedJNIEnv env;
        if (!env.jniEnv)
            return;

        env.jniEnv->CallStaticVoidMethod(applicationClass(), m_registerClipboardManagerMethodID);
    }

    void setClipboardText(const QString &text)
    {
        AttachedJNIEnv env;
        if (!env.jniEnv)
            return;

        jstring jtext = env.jniEnv->NewString(reinterpret_cast<const jchar *>(text.data()),
                                              text.length());
        env.jniEnv->CallStaticVoidMethod(applicationClass(), m_setClipboardTextMethodID, jtext);
        env.jniEnv->DeleteLocalRef(jtext);
    }

    bool hasClipboardText()
    {
        AttachedJNIEnv env;
        if (!env.jniEnv)
            return false;

        return env.jniEnv->CallStaticBooleanMethod(applicationClass(), m_hasClipboardTextMethodID);
    }

    QString clipboardText()
    {
        AttachedJNIEnv env;
        if (!env.jniEnv)
            return QString();

        jstring text = reinterpret_cast<jstring>(env.jniEnv->CallStaticObjectMethod(applicationClass(),
                                                                                    m_getClipboardTextMethodID));
        const jchar *jstr = env.jniEnv->GetStringChars(text, 0);
        QString str(reinterpret_cast<const QChar *>(jstr), env.jniEnv->GetStringLength(text));
        env.jniEnv->ReleaseStringChars(text, jstr);
        return str;
    }


#define GET_AND_CHECK_STATIC_METHOD(VAR, CLASS, METHOD_NAME, METHOD_SIGNATURE) \
    VAR = env->GetStaticMethodID(CLASS, METHOD_NAME, METHOD_SIGNATURE); \
    if (!VAR) { \
        __android_log_print(ANDROID_LOG_FATAL, qtTagText(), methodErrorMsgFmt(), METHOD_NAME, METHOD_SIGNATURE); \
        return false; \
    }

    bool registerNatives(JNIEnv *env)
    {
        jclass appClass = QtAndroid::applicationClass();

        GET_AND_CHECK_STATIC_METHOD(m_registerClipboardManagerMethodID, appClass, "registerClipboardManager", "()V");
        GET_AND_CHECK_STATIC_METHOD(m_setClipboardTextMethodID, appClass, "setClipboardText", "(Ljava/lang/String;)V");
        GET_AND_CHECK_STATIC_METHOD(m_hasClipboardTextMethodID, appClass, "hasClipboardText", "()Z");
        GET_AND_CHECK_STATIC_METHOD(m_getClipboardTextMethodID, appClass, "getClipboardText", "()Ljava/lang/String;");

        return true;
    }
}
