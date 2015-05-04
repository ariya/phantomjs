/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the plugins of the Qt Toolkit.
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

#include "androidjniaccessibility.h"
#include "androidjnimain.h"
#include "qandroidplatformintegration.h"
#include "qpa/qplatformaccessibility.h"
#include <QtPlatformSupport/private/qaccessiblebridgeutils_p.h>
#include "qguiapplication.h"
#include "qwindow.h"
#include "qrect.h"
#include "QtGui/qaccessible.h"
#include <QtCore/qmath.h>
#include <QtCore/private/qjnihelpers_p.h>

#include "qdebug.h"

static const char m_qtTag[] = "Qt A11Y";
static const char m_classErrorMsg[] = "Can't find class \"%s\"";
static const char m_methodErrorMsg[] = "Can't find method \"%s%s\"";

QT_BEGIN_NAMESPACE

namespace QtAndroidAccessibility
{
    static jmethodID m_addActionMethodID = 0;
    static jmethodID m_setCheckableMethodID = 0;
    static jmethodID m_setCheckedMethodID = 0;
    static jmethodID m_setClickableMethodID = 0;
    static jmethodID m_setContentDescriptionMethodID = 0;
    static jmethodID m_setEnabledMethodID = 0;
    static jmethodID m_setFocusableMethodID = 0;
    static jmethodID m_setFocusedMethodID = 0;
    static jmethodID m_setScrollableMethodID = 0;
    static jmethodID m_setTextSelectionMethodID = 0;
    static jmethodID m_setVisibleToUserMethodID = 0;


    static void setActive(JNIEnv */*env*/, jobject /*thiz*/, jboolean active)
    {
        QAndroidPlatformIntegration *platformIntegration = QtAndroid::androidPlatformIntegration();
        if (platformIntegration)
            platformIntegration->accessibility()->setActive(active);
        else
            __android_log_print(ANDROID_LOG_WARN, m_qtTag, "Could not activate platform accessibility.");
    }

    QAccessibleInterface *interfaceFromId(jint objectId)
    {
        QAccessibleInterface *iface = 0;
        if (objectId == -1) {
            QWindow *win = qApp->focusWindow();
            if (win)
                iface = win->accessibleRoot();
        } else {
            iface = QAccessible::accessibleInterface(objectId);
        }
        return iface;
    }

    static jintArray childIdListForAccessibleObject(JNIEnv *env, jobject /*thiz*/, jint objectId)
    {
        QAccessibleInterface *iface = interfaceFromId(objectId);
        if (iface) {
            jintArray jArray = env->NewIntArray(jsize(iface->childCount()));
            for (int i = 0; i < iface->childCount(); ++i) {
                QAccessibleInterface *child = iface->child(i);
                if (child) {
                    QAccessible::Id ifaceId = QAccessible::uniqueId(child);
                    jint jid = ifaceId;
                    env->SetIntArrayRegion(jArray, i, 1, &jid);
                }
            }
            return jArray;
        }

        return env->NewIntArray(jsize(0));
    }

    static jint parentId(JNIEnv */*env*/, jobject /*thiz*/, jint objectId)
    {
        QAccessibleInterface *iface = interfaceFromId(objectId);
        if (iface) {
            QAccessibleInterface *parent = iface->parent();
            if (parent) {
                if (parent->role() == QAccessible::Application)
                    return -1;
                return QAccessible::uniqueId(parent);
            }
        }
        return -1;
    }

    static jobject screenRect(JNIEnv *env, jobject /*thiz*/, jint objectId)
    {
        QRect rect;
        QAccessibleInterface *iface = interfaceFromId(objectId);
        if (iface && iface->isValid()) {
            rect = iface->rect();
        }

        jclass rectClass = env->FindClass("android/graphics/Rect");
        jmethodID ctor = env->GetMethodID(rectClass, "<init>", "(IIII)V");
        jobject jrect = env->NewObject(rectClass, ctor, rect.left(), rect.top(), rect.right(), rect.bottom());
        return jrect;
    }

    static jint hitTest(JNIEnv */*env*/, jobject /*thiz*/, jfloat x, jfloat y)
    {
        QAccessibleInterface *root = interfaceFromId(-1);
        if (root) {
            QAccessibleInterface *child = root->childAt((int)x, (int)y);
            QAccessibleInterface *lastChild = 0;
            while (child && (child != lastChild)) {
                lastChild = child;
                child = child->childAt((int)x, (int)y);
            }
            if (lastChild)
                return QAccessible::uniqueId(lastChild);
        }
        return -1;
    }

    static jboolean clickAction(JNIEnv */*env*/, jobject /*thiz*/, jint objectId)
    {
//        qDebug() << "A11Y: CLICK: " << objectId;
        QAccessibleInterface *iface = interfaceFromId(objectId);
        if (iface && iface->actionInterface()) {
            if (iface->actionInterface()->actionNames().contains(QAccessibleActionInterface::pressAction()))
                iface->actionInterface()->doAction(QAccessibleActionInterface::pressAction());
            else
                iface->actionInterface()->doAction(QAccessibleActionInterface::toggleAction());
        }
        return false;
    }

    static jboolean scrollForward(JNIEnv */*env*/, jobject /*thiz*/, jint objectId)
    {
        QAccessibleInterface *iface = interfaceFromId(objectId);
        return QAccessibleBridgeUtils::performEffectiveAction(iface, QAccessibleActionInterface::increaseAction());
    }

    static jboolean scrollBackward(JNIEnv */*env*/, jobject /*thiz*/, jint objectId)
    {
        QAccessibleInterface *iface = interfaceFromId(objectId);
        return QAccessibleBridgeUtils::performEffectiveAction(iface, QAccessibleActionInterface::decreaseAction());
    }


#define FIND_AND_CHECK_CLASS(CLASS_NAME) \
clazz = env->FindClass(CLASS_NAME); \
if (!clazz) { \
    __android_log_print(ANDROID_LOG_FATAL, m_qtTag, m_classErrorMsg, CLASS_NAME); \
    return JNI_FALSE; \
}

        //__android_log_print(ANDROID_LOG_FATAL, m_qtTag, m_methodErrorMsg, METHOD_NAME, METHOD_SIGNATURE);



    static jstring descriptionForAccessibleObject_helper(JNIEnv *env, QAccessibleInterface *iface)
    {
        QString desc;
        if (iface && iface->isValid()) {
            desc = iface->text(QAccessible::Name);
            if (desc.isEmpty())
                desc = iface->text(QAccessible::Description);
            if (desc.isEmpty()) {
                desc = iface->text(QAccessible::Value);
                if (desc.isEmpty()) {
                    if (QAccessibleValueInterface *valueIface = iface->valueInterface()) {
                        desc= valueIface->currentValue().toString();
                    }
                }
            }
        }
        return env->NewString((jchar*) desc.constData(), (jsize) desc.size());
    }

    static jstring descriptionForAccessibleObject(JNIEnv *env, jobject /*thiz*/, jint objectId)
    {
        QAccessibleInterface *iface = interfaceFromId(objectId);
        return descriptionForAccessibleObject_helper(env, iface);
    }

    static bool populateNode(JNIEnv *env, jobject /*thiz*/, jint objectId, jobject node)
    {
        QAccessibleInterface *iface = interfaceFromId(objectId);
        if (!iface || !iface->isValid()) {
            __android_log_print(ANDROID_LOG_WARN, m_qtTag, "Accessibility: populateNode for Invalid ID");
            return false;
        }
        QAccessible::State state = iface->state();
        const QStringList actions = QAccessibleBridgeUtils::effectiveActionNames(iface);
        const bool hasClickableAction = actions.contains(QAccessibleActionInterface::pressAction())
                                        || actions.contains(QAccessibleActionInterface::toggleAction());
        const bool hasIncreaseAction = actions.contains(QAccessibleActionInterface::increaseAction());
        const bool hasDecreaseAction = actions.contains(QAccessibleActionInterface::decreaseAction());

        // try to fill in the text property, this is what the screen reader reads
        jstring jdesc = descriptionForAccessibleObject_helper(env, iface);

        if (QAccessibleTextInterface *textIface = iface->textInterface()) {
            if (m_setTextSelectionMethodID && textIface->selectionCount() > 0) {
                int startSelection;
                int endSelection;
                textIface->selection(0, &startSelection, &endSelection);
                env->CallVoidMethod(node, m_setTextSelectionMethodID, startSelection, endSelection);
            }
        }

        env->CallVoidMethod(node, m_setEnabledMethodID, !state.disabled);
        env->CallVoidMethod(node, m_setCheckableMethodID, (bool)state.checkable);
        env->CallVoidMethod(node, m_setCheckedMethodID, (bool)state.checked);
        env->CallVoidMethod(node, m_setFocusableMethodID, (bool)state.focusable);
        env->CallVoidMethod(node, m_setFocusedMethodID, (bool)state.focused);
        env->CallVoidMethod(node, m_setVisibleToUserMethodID, !state.invisible);
        env->CallVoidMethod(node, m_setScrollableMethodID, hasIncreaseAction || hasDecreaseAction);
        env->CallVoidMethod(node, m_setClickableMethodID, hasClickableAction);

        // Add ACTION_CLICK
        if (hasClickableAction)
            env->CallVoidMethod(node, m_addActionMethodID, (int)16);    // ACTION_CLICK defined in AccessibilityNodeInfo

        // Add ACTION_SCROLL_FORWARD
        if (hasIncreaseAction)
            env->CallVoidMethod(node, m_addActionMethodID, (int)4096);    // ACTION_SCROLL_FORWARD defined in AccessibilityNodeInfo

        // Add ACTION_SCROLL_BACKWARD
        if (hasDecreaseAction)
            env->CallVoidMethod(node, m_addActionMethodID, (int)8192);    // ACTION_SCROLL_BACKWARD defined in AccessibilityNodeInfo


        //CALL_METHOD(node, "setText", "(Ljava/lang/CharSequence;)V", jdesc)
        env->CallVoidMethod(node, m_setContentDescriptionMethodID, jdesc);

        return true;
    }

    static JNINativeMethod methods[] = {
        {"setActive","(Z)V",(void*)setActive},
        {"childIdListForAccessibleObject", "(I)[I", (jintArray)childIdListForAccessibleObject},
        {"parentId", "(I)I", (void*)parentId},
        {"descriptionForAccessibleObject", "(I)Ljava/lang/String;", (jstring)descriptionForAccessibleObject},
        {"screenRect", "(I)Landroid/graphics/Rect;", (jobject)screenRect},
        {"hitTest", "(FF)I", (void*)hitTest},
        {"populateNode", "(ILandroid/view/accessibility/AccessibilityNodeInfo;)Z", (void*)populateNode},
        {"clickAction", "(I)Z", (void*)clickAction},
        {"scrollForward", "(I)Z", (void*)scrollForward},
        {"scrollBackward", "(I)Z", (void*)scrollBackward},
    };

#define GET_AND_CHECK_STATIC_METHOD(VAR, CLASS, METHOD_NAME, METHOD_SIGNATURE) \
    VAR = env->GetMethodID(CLASS, METHOD_NAME, METHOD_SIGNATURE); \
    if (!VAR) { \
        __android_log_print(ANDROID_LOG_FATAL, QtAndroid::qtTagText(), QtAndroid::methodErrorMsgFmt(), METHOD_NAME, METHOD_SIGNATURE); \
        return false; \
    }

    bool registerNatives(JNIEnv *env)
    {
        if (QtAndroidPrivate::androidSdkVersion() < 16)
            return true; // We need API level 16 or higher

        jclass clazz;
        FIND_AND_CHECK_CLASS("org/qtproject/qt5/android/accessibility/QtNativeAccessibility");
        jclass appClass = static_cast<jclass>(env->NewGlobalRef(clazz));

        if (env->RegisterNatives(appClass, methods, sizeof(methods) / sizeof(methods[0])) < 0) {
            __android_log_print(ANDROID_LOG_FATAL,"Qt A11y", "RegisterNatives failed");
            return false;
        }

        jclass nodeInfoClass = env->FindClass("android/view/accessibility/AccessibilityNodeInfo");
        GET_AND_CHECK_STATIC_METHOD(m_addActionMethodID, nodeInfoClass, "addAction", "(I)V");
        GET_AND_CHECK_STATIC_METHOD(m_setCheckableMethodID, nodeInfoClass, "setCheckable", "(Z)V");
        GET_AND_CHECK_STATIC_METHOD(m_setCheckedMethodID, nodeInfoClass, "setChecked", "(Z)V");
        GET_AND_CHECK_STATIC_METHOD(m_setClickableMethodID, nodeInfoClass, "setClickable", "(Z)V");
        GET_AND_CHECK_STATIC_METHOD(m_setContentDescriptionMethodID, nodeInfoClass, "setContentDescription", "(Ljava/lang/CharSequence;)V");
        GET_AND_CHECK_STATIC_METHOD(m_setEnabledMethodID, nodeInfoClass, "setEnabled", "(Z)V");
        GET_AND_CHECK_STATIC_METHOD(m_setFocusableMethodID, nodeInfoClass, "setFocusable", "(Z)V");
        GET_AND_CHECK_STATIC_METHOD(m_setFocusedMethodID, nodeInfoClass, "setFocused", "(Z)V");
        GET_AND_CHECK_STATIC_METHOD(m_setScrollableMethodID, nodeInfoClass, "setScrollable", "(Z)V");
        GET_AND_CHECK_STATIC_METHOD(m_setVisibleToUserMethodID, nodeInfoClass, "setVisibleToUser", "(Z)V");

        if (QtAndroidPrivate::androidSdkVersion() >= 18) {
            GET_AND_CHECK_STATIC_METHOD(m_setTextSelectionMethodID, nodeInfoClass, "setTextSelection", "(II)V");
        }

        return true;
    }
}

QT_END_NAMESPACE
