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

#include "androidjnimenu.h"
#include "androidjnimain.h"
#include "qandroidplatformmenubar.h"
#include "qandroidplatformmenu.h"
#include "qandroidplatformmenuitem.h"

#include <QMutex>
#include <QSet>
#include <QQueue>
#include <QWindow>

using namespace QtAndroid;

namespace QtAndroidMenu
{
    static QQueue<QAndroidPlatformMenu *> pendingContextMenus;
    static QAndroidPlatformMenu *visibleMenu = 0;
    static QMutex visibleMenuMutex(QMutex::Recursive);

    static QSet<QAndroidPlatformMenuBar *> menuBars;
    static QAndroidPlatformMenuBar *visibleMenuBar = 0;
    static QWindow *activeTopLevelWindow = 0;
    static QMutex menuBarMutex(QMutex::Recursive);

    static jmethodID openContextMenuMethodID = 0;
    static jmethodID closeContextMenuMethodID = 0;
    static jmethodID resetOptionsMenuMethodID = 0;

    static jmethodID clearMenuMethodID = 0;
    static jmethodID addMenuItemMethodID = 0;
    static int menuNoneValue = 0;
    static jmethodID setHeaderTitleContextMenuMethodID = 0;

    static jmethodID setCheckableMenuItemMethodID = 0;
    static jmethodID setCheckedMenuItemMethodID = 0;
    static jmethodID setEnabledMenuItemMethodID = 0;
    static jmethodID setIconMenuItemMethodID = 0;
    static jmethodID setVisibleMenuItemMethodID = 0;

    void resetMenuBar()
    {
        AttachedJNIEnv env;
        if (env.jniEnv)
            env.jniEnv->CallStaticVoidMethod(applicationClass(), resetOptionsMenuMethodID);
    }

    void showContextMenu(QAndroidPlatformMenu *menu, JNIEnv *env)
    {
        QMutexLocker lock(&visibleMenuMutex);
        if (visibleMenu) {
            pendingContextMenus.enqueue(menu);
        } else {
            visibleMenu = menu;
            menu->aboutToShow();
            if (env) {
                env->CallStaticVoidMethod(applicationClass(), openContextMenuMethodID);
            } else {
                AttachedJNIEnv aenv;
                if (aenv.jniEnv)
                    aenv.jniEnv->CallStaticVoidMethod(applicationClass(), openContextMenuMethodID);
            }
        }
    }

    void hideContextMenu(QAndroidPlatformMenu *menu)
    {
        QMutexLocker lock(&visibleMenuMutex);
        if (visibleMenu == menu) {
            AttachedJNIEnv env;
            if (env.jniEnv)
                env.jniEnv->CallStaticVoidMethod(applicationClass(), openContextMenuMethodID);
        } else {
            pendingContextMenus.removeOne(menu);
        }
    }

    void syncMenu(QAndroidPlatformMenu */*menu*/)
    {
//        QMutexLocker lock(&visibleMenuMutex);
//        if (visibleMenu == menu)
//        {
//            hideContextMenu(menu);
//            showContextMenu(menu);
//        }
    }

    void androidPlatformMenuDestroyed(QAndroidPlatformMenu *menu)
    {
        QMutexLocker lock(&visibleMenuMutex);
        if (visibleMenu == menu)
            visibleMenu = 0;
    }

    void setMenuBar(QAndroidPlatformMenuBar *menuBar, QWindow *window)
    {
        if (activeTopLevelWindow == window && visibleMenuBar != menuBar) {
            visibleMenuBar = menuBar;
            resetMenuBar();
        }
    }

    void setActiveTopLevelWindow(QWindow *window)
    {
        Qt::WindowFlags flags = window ? window->flags() : Qt::WindowFlags();
        if (!window)
            return;

        bool isNonRegularWindow = flags & (Qt::Desktop | Qt::Popup | Qt::Dialog | Qt::Sheet) & ~Qt::Window;
        if (isNonRegularWindow)
            return;

        QMutexLocker lock(&menuBarMutex);
        if (activeTopLevelWindow == window)
            return;

        visibleMenuBar = 0;
        activeTopLevelWindow = window;
        foreach (QAndroidPlatformMenuBar *menuBar, menuBars) {
            if (menuBar->parentWindow() == window) {
                visibleMenuBar = menuBar;
                resetMenuBar();
                break;
            }
        }

    }

    void addMenuBar(QAndroidPlatformMenuBar *menuBar)
    {
        QMutexLocker lock(&menuBarMutex);
        menuBars.insert(menuBar);
    }

    void removeMenuBar(QAndroidPlatformMenuBar *menuBar)
    {
        QMutexLocker lock(&menuBarMutex);
        menuBars.remove(menuBar);
        if (visibleMenuBar == menuBar) {
            visibleMenuBar = 0;
            resetMenuBar();
        }
    }

    static QString removeAmpersandEscapes(QString s)
    {
        int i = 0;
        while (i < s.size()) {
            ++i;
            if (s.at(i-1) != QLatin1Char('&'))
                continue;
            if (i < s.size() && s.at(i) == QLatin1Char('&'))
                ++i;
            s.remove(i-1,1);
        }
        return s.trimmed();
    }

    static void fillMenuItem(JNIEnv *env, jobject menuItem, bool checkable, bool checked, bool enabled, bool visible, const QIcon &icon=QIcon())
    {
        env->DeleteLocalRef(env->CallObjectMethod(menuItem, setCheckableMenuItemMethodID, checkable));
        env->DeleteLocalRef(env->CallObjectMethod(menuItem, setCheckedMenuItemMethodID, checked));
        env->DeleteLocalRef(env->CallObjectMethod(menuItem, setEnabledMenuItemMethodID, enabled));

        if (!icon.isNull()) { // isNull() only checks the d pointer, not the actual image data.
            int sz = qMax(36, qgetenv("QT_ANDROID_APP_ICON_SIZE").toInt());
            QImage img = icon.pixmap(QSize(sz,sz),
                                     enabled
                                        ? QIcon::Normal
                                        : QIcon::Disabled,
                                     QIcon::On).toImage();
            if (!img.isNull()) { // Make sure we have a valid image.
                env->DeleteLocalRef(env->CallObjectMethod(menuItem,
                                                          setIconMenuItemMethodID,
                                                          createBitmapDrawable(createBitmap(img, env), env)));
            }
        }

        env->DeleteLocalRef(env->CallObjectMethod(menuItem, setVisibleMenuItemMethodID, visible));
    }

    static int addAllMenuItemsToMenu(JNIEnv *env, jobject menu, QAndroidPlatformMenu *platformMenu) {
         int order = 0;
         QMutexLocker lock(platformMenu->menuItemsMutex());
         foreach (QAndroidPlatformMenuItem *item, platformMenu->menuItems()) {
             if (item->isSeparator())
                 continue;
             QString itemText = removeAmpersandEscapes(item->text());
             jstring jtext = env->NewString(reinterpret_cast<const jchar *>(itemText.data()),
                                           itemText.length());
             jobject menuItem = env->CallObjectMethod(menu,
                                                      addMenuItemMethodID,
                                                      menuNoneValue,
                                                      int(item->tag()),
                                                      order++,
                                                      jtext);
             env->DeleteLocalRef(jtext);
             fillMenuItem(env,
                          menuItem,
                          item->isCheckable(),
                          item->isChecked(),
                          item->isEnabled(),
                          item->isVisible(),
                          item->icon());
             env->DeleteLocalRef(menuItem);
         }

         return order;
    }

    static jboolean onPrepareOptionsMenu(JNIEnv *env, jobject /*thiz*/, jobject menu)
    {
        env->CallVoidMethod(menu, clearMenuMethodID);
        QMutexLocker lock(&menuBarMutex);
        if (!visibleMenuBar)
            return JNI_FALSE;

        const QAndroidPlatformMenuBar::PlatformMenusType &menus = visibleMenuBar->menus();
        int order = 0;
        QMutexLocker lockMenuBarMutex(visibleMenuBar->menusListMutex());
        if (menus.size() == 1) { // Expand the menu
            order = addAllMenuItemsToMenu(env, menu, static_cast<QAndroidPlatformMenu *>(menus.front()));
        } else {
            foreach (QAndroidPlatformMenu *item, menus) {
                QString itemText = removeAmpersandEscapes(item->text());
                jstring jtext = env->NewString(reinterpret_cast<const jchar *>(itemText.data()),
                                               itemText.length());
                jobject menuItem = env->CallObjectMethod(menu,
                                                         addMenuItemMethodID,
                                                         menuNoneValue,
                                                         int(item->tag()),
                                                         order++,
                                                         jtext);
                env->DeleteLocalRef(jtext);

                fillMenuItem(env,
                             menuItem,
                             false,
                             false,
                             item->isEnabled(),
                             item->isVisible(),
                             item->icon());
            }
        }
        return order ? JNI_TRUE : JNI_FALSE;
    }

    static jboolean onOptionsItemSelected(JNIEnv *env, jobject /*thiz*/, jint menuId, jboolean checked)
    {
        QMutexLocker lock(&menuBarMutex);
        if (!visibleMenuBar)
            return JNI_FALSE;

        const QAndroidPlatformMenuBar::PlatformMenusType &menus = visibleMenuBar->menus();
        if (menus.size() == 1) { // Expanded menu
            QAndroidPlatformMenuItem *item = static_cast<QAndroidPlatformMenuItem *>(menus.front()->menuItemForTag(menuId));
            if (item) {
                if (item->menu()) {
                    showContextMenu(item->menu(), env);
                } else {
                    if (item->isCheckable())
                        item->setChecked(checked);
                    item->activated();
                }
            }
        } else {
            QAndroidPlatformMenu *menu = static_cast<QAndroidPlatformMenu *>(visibleMenuBar->menuForTag(menuId));
            if (menu)
                showContextMenu(menu, env);
        }

        return JNI_TRUE;
    }

    static void onOptionsMenuClosed(JNIEnv */*env*/, jobject /*thiz*/, jobject /*menu*/)
    {
    }

    static void onCreateContextMenu(JNIEnv *env, jobject /*thiz*/, jobject menu)
    {
        env->CallVoidMethod(menu, clearMenuMethodID);
        QMutexLocker lock(&visibleMenuMutex);
        if (!visibleMenu)
            return;

        QString menuText = removeAmpersandEscapes(visibleMenu->text());
        jstring jtext = env->NewString(reinterpret_cast<const jchar*>(menuText.data()),
                                       menuText.length());
        env->CallObjectMethod(menu, setHeaderTitleContextMenuMethodID, jtext);
        env->DeleteLocalRef(jtext);
        addAllMenuItemsToMenu(env, menu, visibleMenu);
    }

    static jboolean onContextItemSelected(JNIEnv *env, jobject /*thiz*/, jint menuId, jboolean checked)
    {
        QMutexLocker lock(&visibleMenuMutex);
        QAndroidPlatformMenuItem * item = static_cast<QAndroidPlatformMenuItem *>(visibleMenu->menuItemForTag(menuId));
        if (item) {
            if (item->menu()) {
                showContextMenu(item->menu(), env);
            } else {
                if (item->isCheckable())
                    item->setChecked(checked);
                item->activated();
            }
        }
        return JNI_TRUE;
    }

    static void onContextMenuClosed(JNIEnv *env, jobject /*thiz*/, jobject /*menu*/)
    {
        QMutexLocker lock(&visibleMenuMutex);
        if (!visibleMenu)
            return;
        visibleMenu->aboutToHide();
        visibleMenu = 0;
        if (!pendingContextMenus.empty())
            showContextMenu(pendingContextMenus.dequeue(), env);
    }

    static JNINativeMethod methods[] = {
        {"onPrepareOptionsMenu", "(Landroid/view/Menu;)Z", (void *)onPrepareOptionsMenu},
        {"onOptionsItemSelected", "(IZ)Z", (void *)onOptionsItemSelected},
        {"onOptionsMenuClosed", "(Landroid/view/Menu;)V", (void*)onOptionsMenuClosed},
        {"onCreateContextMenu", "(Landroid/view/ContextMenu;)V", (void *)onCreateContextMenu},
        {"onContextItemSelected", "(IZ)Z", (void *)onContextItemSelected},
        {"onContextMenuClosed", "(Landroid/view/Menu;)V", (void*)onContextMenuClosed},
    };

#define FIND_AND_CHECK_CLASS(CLASS_NAME) \
    clazz = env->FindClass(CLASS_NAME); \
    if (!clazz) { \
        __android_log_print(ANDROID_LOG_FATAL, qtTagText(), classErrorMsgFmt(), CLASS_NAME); \
        return false; \
    }

#define GET_AND_CHECK_METHOD(VAR, CLASS, METHOD_NAME, METHOD_SIGNATURE) \
    VAR = env->GetMethodID(CLASS, METHOD_NAME, METHOD_SIGNATURE); \
    if (!VAR) { \
        __android_log_print(ANDROID_LOG_FATAL, qtTagText(), methodErrorMsgFmt(), METHOD_NAME, METHOD_SIGNATURE); \
        return false; \
    }

#define GET_AND_CHECK_STATIC_METHOD(VAR, CLASS, METHOD_NAME, METHOD_SIGNATURE) \
    VAR = env->GetStaticMethodID(CLASS, METHOD_NAME, METHOD_SIGNATURE); \
    if (!VAR) { \
        __android_log_print(ANDROID_LOG_FATAL, qtTagText(), methodErrorMsgFmt(), METHOD_NAME, METHOD_SIGNATURE); \
        return false; \
    }

#define GET_AND_CHECK_STATIC_FIELD(VAR, CLASS, FIELD_NAME, FIELD_SIGNATURE) \
    VAR = env->GetStaticFieldID(CLASS, FIELD_NAME, FIELD_SIGNATURE); \
    if (!VAR) { \
        __android_log_print(ANDROID_LOG_FATAL, qtTagText(), methodErrorMsgFmt(), FIELD_NAME, FIELD_SIGNATURE); \
        return false; \
    }

    bool registerNatives(JNIEnv *env)
    {
        jclass appClass = applicationClass();

        if (env->RegisterNatives(appClass, methods,  sizeof(methods) / sizeof(methods[0])) < 0) {
            __android_log_print(ANDROID_LOG_FATAL,"Qt", "RegisterNatives failed");
            return false;
        }

        GET_AND_CHECK_STATIC_METHOD(openContextMenuMethodID, appClass, "openContextMenu", "()V");
        GET_AND_CHECK_STATIC_METHOD(closeContextMenuMethodID, appClass, "closeContextMenu", "()V");
        GET_AND_CHECK_STATIC_METHOD(resetOptionsMenuMethodID, appClass, "resetOptionsMenu", "()V");

        jclass clazz;
        FIND_AND_CHECK_CLASS("android/view/Menu");
        GET_AND_CHECK_METHOD(clearMenuMethodID, clazz, "clear", "()V");
        GET_AND_CHECK_METHOD(addMenuItemMethodID, clazz, "add", "(IIILjava/lang/CharSequence;)Landroid/view/MenuItem;");
        jfieldID menuNoneFiledId;
        GET_AND_CHECK_STATIC_FIELD(menuNoneFiledId, clazz, "NONE", "I");
        menuNoneValue = env->GetStaticIntField(clazz, menuNoneFiledId);

        FIND_AND_CHECK_CLASS("android/view/ContextMenu");
        GET_AND_CHECK_METHOD(setHeaderTitleContextMenuMethodID, clazz, "setHeaderTitle","(Ljava/lang/CharSequence;)Landroid/view/ContextMenu;");

        FIND_AND_CHECK_CLASS("android/view/MenuItem");
        GET_AND_CHECK_METHOD(setCheckableMenuItemMethodID, clazz, "setCheckable", "(Z)Landroid/view/MenuItem;");
        GET_AND_CHECK_METHOD(setCheckedMenuItemMethodID, clazz, "setChecked", "(Z)Landroid/view/MenuItem;");
        GET_AND_CHECK_METHOD(setEnabledMenuItemMethodID, clazz, "setEnabled", "(Z)Landroid/view/MenuItem;");
        GET_AND_CHECK_METHOD(setIconMenuItemMethodID, clazz, "setIcon", "(Landroid/graphics/drawable/Drawable;)Landroid/view/MenuItem;");
        GET_AND_CHECK_METHOD(setVisibleMenuItemMethodID, clazz, "setVisible", "(Z)Landroid/view/MenuItem;");
        return true;
    }
}
