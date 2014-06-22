/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
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

#include <QtCore/QtConfig>
#ifndef QT_NO_ACCESSIBILITY


#include <private/qsystemlibrary_p.h>

#include <QtCore/qlocale.h>
#include <QtCore/qmap.h>
#include <QtCore/qpair.h>
#include <QtCore/qpointer.h>
#include <QtCore/qsettings.h>
#include <QtGui/qaccessible.h>
#include <QtGui/private/qguiapplication_p.h>
#include <qpa/qplatformnativeinterface.h>
#include <qpa/qplatformintegration.h>
#include <QtGui/qwindow.h>
#include <QtGui/qguiapplication.h>

#include "qwindowsaccessibility.h"
#ifdef Q_CC_MINGW
# include "qwindowsmsaaaccessible.h"
#else
# include "iaccessible2.h"
#endif
#include "comutils.h"

#include <oleacc.h>

//#include <uiautomationcoreapi.h>
#ifndef UiaRootObjectId
#define UiaRootObjectId        -25
#endif

#include <winuser.h>
#if !defined(WINABLEAPI)
#  if defined(Q_OS_WINCE)
#    include <bldver.h>
#  endif
#  include <winable.h>
#endif

#include <servprov.h>
#if !defined(Q_CC_BOR) && !defined (Q_CC_GNU)
#include <comdef.h>
#endif

#include "../qtwindows_additional.h"

QT_BEGIN_NAMESPACE

/*!
    \!internal
    \class QWindowsAccessibility

    Implements QPlatformAccessibility

*/
QWindowsAccessibility::QWindowsAccessibility()
{
}

// Retrieve sound name by checking the icon property of a message box
static inline QString messageBoxAlertSound(const QObject *messageBox)
{
    enum MessageBoxIcon { // Keep in sync with QMessageBox::Icon
        Information = 1,
        Warning = 2,
        Critical = 3
    };
    switch (messageBox->property("icon").toInt()) {
    case Information:
        return QStringLiteral("SystemAsterisk");
    case Warning:
        return QStringLiteral("SystemExclamation");
    case Critical:
        return QStringLiteral("SystemHand");
    }
    return QString();
}

void QWindowsAccessibility::notifyAccessibilityUpdate(QAccessibleEvent *event)
{
    QString soundName;
    switch (event->type()) {
    case QAccessible::PopupMenuStart:
        soundName = QLatin1String("MenuPopup");
        break;

    case QAccessible::MenuCommand:
        soundName = QLatin1String("MenuCommand");
        break;

    case QAccessible::Alert:
        soundName = event->object()->inherits("QMessageBox") ?
            messageBoxAlertSound(event->object()) : QStringLiteral("SystemAsterisk");
        break;
    default:
        break;
    }

    if (!soundName.isEmpty()) {
#ifndef QT_NO_SETTINGS
        QSettings settings(QLatin1String("HKEY_CURRENT_USER\\AppEvents\\Schemes\\Apps\\.Default\\") + soundName,
                           QSettings::NativeFormat);
        QString file = settings.value(QLatin1String(".Current/.")).toString();
#else
        QString file;
#endif
        if (!file.isEmpty()) {
            PlaySound(reinterpret_cast<const wchar_t *>(soundName.utf16()), 0, SND_ALIAS | SND_ASYNC | SND_NODEFAULT | SND_NOWAIT);
        }
    }

#if defined(Q_OS_WINCE) // ### TODO: check for NotifyWinEvent in CE 6.0
    // There is no user32.lib nor NotifyWinEvent for CE
    return;
#else
    // An event has to be associated with a window,
    // so find the first parent that is a widget and that has a WId
    QAccessibleInterface *iface = event->accessibleInterface();
    if (!iface) // ### This should not happen, maybe make it an assert.
        return;
    QWindow *window = QWindowsAccessibility::windowHelper(iface);

    if (!window) {
        window = QGuiApplication::focusWindow();
        if (!window)
            return;
    }

    QPlatformNativeInterface *platform = QGuiApplication::platformNativeInterface();
    if (!window->handle()) // Called before show(), no native window yet.
        return;
    HWND hWnd = (HWND)platform->nativeResourceForWindow("handle", window);

    if (event->type() != QAccessible::MenuCommand && // MenuCommand is faked
        event->type() != QAccessible::ObjectDestroyed) {
        ::NotifyWinEvent(event->type(), hWnd, OBJID_CLIENT, QAccessible::uniqueId(iface));
    }
#endif // Q_OS_WINCE
}

QWindow *QWindowsAccessibility::windowHelper(const QAccessibleInterface *iface)
{
    QWindow *window = iface->window();
    if (!window) {
        QAccessibleInterface *acc = iface->parent();
        while (acc && acc->isValid() && !window) {
            window = acc->window();
            QAccessibleInterface *par = acc->parent();
            acc = par;
        }
    }
    return window;
}

/*!
  \internal
  helper to wrap a QAccessibleInterface inside a IAccessible*
*/
IAccessible *QWindowsAccessibility::wrap(QAccessibleInterface *acc)
{
    if (!acc)
        return 0;

    // ### FIXME: maybe we should accept double insertions into the cache
    if (!QAccessible::uniqueId(acc))
        QAccessible::registerAccessibleInterface(acc);

#ifdef Q_CC_MINGW
    QWindowsMsaaAccessible *wacc = new QWindowsMsaaAccessible(acc);
#else
    QWindowsIA2Accessible *wacc = new QWindowsIA2Accessible(acc);
#endif
    IAccessible *iacc = 0;
    wacc->QueryInterface(IID_IAccessible, (void**)&iacc);
    return iacc;
}

/*
void QWindowsAccessibility::setRootObject(QObject *o)
{

}

void QWindowsAccessibility::initialize()
{

}

void QWindowsAccessibility::cleanup()
{

}

*/

bool QWindowsAccessibility::handleAccessibleObjectFromWindowRequest(HWND hwnd, WPARAM wParam, LPARAM lParam, LRESULT *lResult)
{
    if (static_cast<long>(lParam) == static_cast<long>(UiaRootObjectId)) {
        /* For UI Automation */
    } else if ((DWORD)lParam == DWORD(OBJID_CLIENT)) {
        // Start handling accessibility internally
        QGuiApplicationPrivate::platformIntegration()->accessibility()->setActive(true);
#if 1
        // Ignoring all requests while starting up
        // ### Maybe QPA takes care of this???
        if (QCoreApplication::startingUp() || QCoreApplication::closingDown())
            return false;
#endif

        typedef LRESULT (WINAPI *PtrLresultFromObject)(REFIID, WPARAM, LPUNKNOWN);
        static PtrLresultFromObject ptrLresultFromObject = 0;
        static bool oleaccChecked = false;

        if (!oleaccChecked) {
            oleaccChecked = true;
#if !defined(Q_OS_WINCE)
            ptrLresultFromObject = (PtrLresultFromObject)QSystemLibrary::resolve(QLatin1String("oleacc"), "LresultFromObject");
#endif
        }

        if (ptrLresultFromObject) {
            QWindow *window = QWindowsContext::instance()->findWindow(hwnd);
            if (window) {
                QAccessibleInterface *acc = window->accessibleRoot();
                if (acc) {
                    if (IAccessible *iface = wrap(acc)) {
                        *lResult = ptrLresultFromObject(IID_IAccessible, wParam, iface);  // ref == 2
                        if (*lResult) {
                            iface->Release(); // the client will release the object again, and then it will destroy itself
                        }
                        return true;
                    }
                }
            }
        }
    }
    return false;
}

QT_END_NAMESPACE

#endif //QT_NO_ACCESSIBILITY
