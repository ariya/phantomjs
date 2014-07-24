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
#ifndef QWINDOWSMSAAACCESSIBLE_H
#define QWINDOWSMSAAACCESSIBLE_H

#include <QtCore/QtConfig>
#ifndef QT_NO_ACCESSIBILITY
#include <QtCore/qglobal.h>

#include "../qtwindows_additional.h"
#include <QtCore/qsharedpointer.h>
#include <QtGui/qaccessible.h>
#ifndef Q_CC_MINGW
# include <oleacc.h>
# include "Accessible2.h"   // IAccessible2 inherits from IAccessible
#else
    // MinGW
# include <basetyps.h>
# include <oleacc.h>
#endif

QT_BEGIN_NAMESPACE

#ifndef QT_NO_DEBUG_OUTPUT
#define DEBUG_SHOW_ATCLIENT_COMMANDS
#endif
#if defined(DEBUG_SHOW_ATCLIENT_COMMANDS)
void accessibleDebugClientCalls_helper(const char* funcName, const QAccessibleInterface *iface);
# define accessibleDebugClientCalls(iface) accessibleDebugClientCalls_helper(Q_FUNC_INFO, iface)
#else
# define accessibleDebugClientCalls(iface)
#endif

QWindow *window_helper(const QAccessibleInterface *iface);

/**************************************************************\
 *                     QWindowsAccessible                     *
 **************************************************************/
class QWindowsMsaaAccessible : public
#ifdef Q_CC_MINGW
        IAccessible
#else
        IAccessible2
#endif
        , public IOleWindow
{
public:
    QWindowsMsaaAccessible(QAccessibleInterface *a)
        : ref(0)
    {
        id = QAccessible::uniqueId(a);
    }

    virtual ~QWindowsMsaaAccessible()
    {
    }

    /* IUnknown */
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID, LPVOID *);
    ULONG STDMETHODCALLTYPE AddRef();
    ULONG STDMETHODCALLTYPE Release();

    /* IDispatch */
    HRESULT STDMETHODCALLTYPE GetTypeInfoCount(unsigned int *);
    HRESULT STDMETHODCALLTYPE GetTypeInfo(unsigned int, unsigned long, ITypeInfo **);
    HRESULT STDMETHODCALLTYPE GetIDsOfNames(const _GUID &, wchar_t **, unsigned int, unsigned long, long *);
    HRESULT STDMETHODCALLTYPE Invoke(long, const _GUID &, unsigned long, unsigned short, tagDISPPARAMS *, tagVARIANT *, tagEXCEPINFO *, unsigned int *);

    /* IAccessible */
    HRESULT STDMETHODCALLTYPE accHitTest(long xLeft, long yTop, VARIANT *pvarID);
    HRESULT STDMETHODCALLTYPE accLocation(long *pxLeft, long *pyTop, long *pcxWidth, long *pcyHeight, VARIANT varID);
    HRESULT STDMETHODCALLTYPE accNavigate(long navDir, VARIANT varStart, VARIANT *pvarEnd);
    HRESULT STDMETHODCALLTYPE get_accChild(VARIANT varChildID, IDispatch** ppdispChild);
    HRESULT STDMETHODCALLTYPE get_accChildCount(long* pcountChildren);
    HRESULT STDMETHODCALLTYPE get_accParent(IDispatch** ppdispParent);

    HRESULT STDMETHODCALLTYPE accDoDefaultAction(VARIANT varID);
    HRESULT STDMETHODCALLTYPE get_accDefaultAction(VARIANT varID, BSTR* pszDefaultAction);
    HRESULT STDMETHODCALLTYPE get_accDescription(VARIANT varID, BSTR* pszDescription);
    HRESULT STDMETHODCALLTYPE get_accHelp(VARIANT varID, BSTR *pszHelp);
    HRESULT STDMETHODCALLTYPE get_accHelpTopic(BSTR *pszHelpFile, VARIANT varChild, long *pidTopic);
    HRESULT STDMETHODCALLTYPE get_accKeyboardShortcut(VARIANT varID, BSTR *pszKeyboardShortcut);
    HRESULT STDMETHODCALLTYPE get_accName(VARIANT varID, BSTR* pszName);
    HRESULT STDMETHODCALLTYPE put_accName(VARIANT varChild, BSTR szName);
    HRESULT STDMETHODCALLTYPE get_accRole(VARIANT varID, VARIANT *pvarRole);
    HRESULT STDMETHODCALLTYPE get_accState(VARIANT varID, VARIANT *pvarState);
    HRESULT STDMETHODCALLTYPE get_accValue(VARIANT varID, BSTR* pszValue);
    HRESULT STDMETHODCALLTYPE put_accValue(VARIANT varChild, BSTR szValue);

    HRESULT STDMETHODCALLTYPE accSelect(long flagsSelect, VARIANT varID);
    HRESULT STDMETHODCALLTYPE get_accFocus(VARIANT *pvarID);
    HRESULT STDMETHODCALLTYPE get_accSelection(VARIANT *pvarChildren);

    /* IOleWindow */
    HRESULT STDMETHODCALLTYPE GetWindow(HWND *phwnd);
    HRESULT STDMETHODCALLTYPE ContextSensitiveHelp(BOOL fEnterMode);

protected:
    virtual QByteArray IIDToString(REFIID id);

    QAccessible::Id id;

    QAccessibleInterface *accessibleInterface() const
    {
         QAccessibleInterface *iface = QAccessible::accessibleInterface(id);
         if (iface && iface->isValid())
             return iface;
         return 0;
    }

    static QAccessibleInterface *childPointer(QAccessibleInterface *parent, VARIANT varID)
    {
        // -1 since windows API always uses 1 for the first child
        Q_ASSERT(parent);

        QAccessibleInterface *acc = 0;
        int childIndex = varID.lVal;
        if (childIndex == 0) {
            // Yes, some AT clients (Active Accessibility Object Inspector)
            // actually ask for the same object. As a consequence, we need to clone ourselves:
            acc = parent;
        } else if (childIndex < 0) {
            acc = QAccessible::accessibleInterface((QAccessible::Id)childIndex);
        } else {
            acc = parent->child(childIndex - 1);
        }
        return acc;
    }

private:
    ULONG ref;

};

QT_END_NAMESPACE

#endif //QT_NO_ACCESSIBILITY

#endif // QWINDOWSMSAAACCESSIBLE_H
