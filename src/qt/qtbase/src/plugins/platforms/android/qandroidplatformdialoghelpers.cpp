/****************************************************************************
**
** Copyright (C) 2013 BogDan Vatra <bogdan@kde.org>
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

#include "qandroidplatformdialoghelpers.h"
#include "androidjnimain.h"

#include <QTextDocument>

#include <private/qguiapplication_p.h>
#include <qpa/qplatformtheme.h>

namespace QtAndroidDialogHelpers {
static jclass g_messageDialogHelperClass = 0;

QAndroidPlatformMessageDialogHelper::QAndroidPlatformMessageDialogHelper()
    :m_buttonId(-1)
    ,m_javaMessageDialog(g_messageDialogHelperClass, "(Landroid/app/Activity;)V", QtAndroid::activity())
    ,m_shown(false)
{
}

void QAndroidPlatformMessageDialogHelper::exec()
{
    if (!m_shown)
        show(Qt::Dialog, Qt::ApplicationModal, 0);
    m_loop.exec();
}

static QString htmlText(QString text)
{
    if (Qt::mightBeRichText(text))
        return text;
    text.remove(QLatin1Char('\r'));
    return text.toHtmlEscaped().replace(QLatin1Char('\n'), QLatin1String("<br />"));
}

bool QAndroidPlatformMessageDialogHelper::show(Qt::WindowFlags windowFlags
                                         , Qt::WindowModality windowModality
                                         , QWindow *parent)
{
    Q_UNUSED(windowFlags)
    Q_UNUSED(windowModality)
    Q_UNUSED(parent)
    QSharedPointer<QMessageDialogOptions> opt = options();
    if (!opt.data())
        return false;

    m_javaMessageDialog.callMethod<void>("setIcon", "(I)V", opt->icon());

    QString str = htmlText(opt->windowTitle());
    if (!str.isEmpty())
        m_javaMessageDialog.callMethod<void>("setTile", "(Ljava/lang/String;)V", QJNIObjectPrivate::fromString(str).object());

    str = htmlText(opt->text());
    if (!str.isEmpty())
        m_javaMessageDialog.callMethod<void>("setText", "(Ljava/lang/String;)V", QJNIObjectPrivate::fromString(str).object());

    str = htmlText(opt->informativeText());
    if (!str.isEmpty())
        m_javaMessageDialog.callMethod<void>("setInformativeText", "(Ljava/lang/String;)V", QJNIObjectPrivate::fromString(str).object());

    str = htmlText(opt->detailedText());
    if (!str.isEmpty())
        m_javaMessageDialog.callMethod<void>("setDetailedText", "(Ljava/lang/String;)V", QJNIObjectPrivate::fromString(str).object());

    for (int i = QPlatformDialogHelper::FirstButton; i < QPlatformDialogHelper::LastButton; i<<=1) {
        if ( opt->standardButtons() & i ) {
            const QString text = QGuiApplicationPrivate::platformTheme()->standardButtonText(i);
            m_javaMessageDialog.callMethod<void>("addButton", "(ILjava/lang/String;)V", i, QJNIObjectPrivate::fromString(text).object());
        }
    }

    m_javaMessageDialog.callMethod<void>("show", "(J)V", jlong(static_cast<QObject*>(this)));
    m_shown = true;
    return true;
}

void QAndroidPlatformMessageDialogHelper::hide()
{
    m_javaMessageDialog.callMethod<void>("hide", "()V");
    m_shown = false;
}

void QAndroidPlatformMessageDialogHelper::dialogResult(int buttonID)
{
    m_buttonId = buttonID;
    if (m_loop.isRunning())
        m_loop.exit();
    if (m_buttonId < 0) {
        emit reject();
        return;
    }

    QPlatformDialogHelper::StandardButton standardButton = static_cast<QPlatformDialogHelper::StandardButton>(buttonID);
    QPlatformDialogHelper::ButtonRole role = QPlatformDialogHelper::buttonRole(standardButton);
    emit clicked(standardButton, role);
}

static void dialogResult(JNIEnv * /*env*/, jobject /*thiz*/, jlong handler, int buttonID)
{
    QObject *object = reinterpret_cast<QObject *>(handler);
    QMetaObject::invokeMethod(object, "dialogResult", Qt::QueuedConnection, Q_ARG(int, buttonID));
}

static JNINativeMethod methods[] = {
    {"dialogResult", "(JI)V", (void *)dialogResult}
};


#define FIND_AND_CHECK_CLASS(CLASS_NAME) \
    clazz = env->FindClass(CLASS_NAME); \
    if (!clazz) { \
        __android_log_print(ANDROID_LOG_FATAL, QtAndroid::qtTagText(), QtAndroid::classErrorMsgFmt(), CLASS_NAME); \
        return false; \
    }

bool registerNatives(JNIEnv *env)
{
    jclass clazz = QtAndroid::findClass("org.qtproject.qt5.android.QtMessageDialogHelper", env);
    if (!clazz) {
        __android_log_print(ANDROID_LOG_FATAL, QtAndroid::qtTagText(), QtAndroid::classErrorMsgFmt()
                            , "org/qtproject/qt5/android/QtMessageDialogHelper");
        return false;
    }
    g_messageDialogHelperClass = static_cast<jclass>(env->NewGlobalRef(clazz));
    FIND_AND_CHECK_CLASS("org/qtproject/qt5/android/QtNativeDialogHelper");
    jclass appClass = static_cast<jclass>(env->NewGlobalRef(clazz));

    if (env->RegisterNatives(appClass, methods, sizeof(methods) / sizeof(methods[0])) < 0) {
        __android_log_print(ANDROID_LOG_FATAL, "Qt", "RegisterNatives failed");
        return false;
    }

    return true;
}
}
