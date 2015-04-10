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

#include "iaccessible2.h"
#include "qwindowsaccessibility.h"

#include <QtGui/qaccessible.h>
#include <QtGui/qclipboard.h>
#include <QtWidgets/qapplication.h>
#include <QtCore/qdebug.h>

QT_BEGIN_NAMESPACE

/**************************************************************\
 *                     AccessibleApplication                  *
 **************************************************************/
//  IUnknown
HRESULT STDMETHODCALLTYPE AccessibleApplication::QueryInterface(REFIID id, LPVOID *iface)
{
    *iface = 0;
    if (id == IID_IUnknown) {
        qCDebug(lcQpaAccessibility) << "AccessibleApplication::QI(): IID_IUnknown";
        *iface = (IUnknown*)this;
    } else if (id == IID_IAccessibleApplication) {
        qCDebug(lcQpaAccessibility) << "AccessibleApplication::QI(): IID_IAccessibleApplication";
        *iface = static_cast<IAccessibleApplication*>(this);
    }

    if (*iface) {
        AddRef();
        return S_OK;
    }
    return E_NOINTERFACE;
}

ULONG STDMETHODCALLTYPE AccessibleApplication::AddRef()
{
    return ++m_ref;
}

ULONG STDMETHODCALLTYPE AccessibleApplication::Release()
{
    if (!--m_ref) {
        delete this;
        return 0;
    }
    return m_ref;
}

/* IAccessibleApplication */
HRESULT STDMETHODCALLTYPE AccessibleApplication::get_appName(/* [retval][out] */ BSTR *name)
{
    const QString appName = QGuiApplication::applicationName();
    *name = QStringToBSTR(appName);
    return S_OK;
}

HRESULT STDMETHODCALLTYPE AccessibleApplication::get_appVersion(/* [retval][out] */ BSTR *version)
{
    const QString appName = QGuiApplication::applicationVersion();
    *version = QStringToBSTR(appName);
    return S_OK;
}

HRESULT STDMETHODCALLTYPE AccessibleApplication::get_toolkitName(/* [retval][out] */ BSTR *name)
{
    *name = ::SysAllocString(L"Qt");
    return S_OK;
}

HRESULT STDMETHODCALLTYPE AccessibleApplication::get_toolkitVersion(/* [retval][out] */ BSTR *version)
{
    *version = ::SysAllocString(QT_UNICODE_LITERAL(QT_VERSION_STR));
    return S_OK;
}


/**************************************************************\
 *                     AccessibleRelation                     *
 **************************************************************/
AccessibleRelation::AccessibleRelation(const QList<QAccessibleInterface *> &targets,
                    QAccessible::Relation relation)
    : m_targets(targets), m_relation(relation), m_ref(1)
{
    Q_ASSERT(m_targets.count());
}

/* IUnknown */
HRESULT STDMETHODCALLTYPE AccessibleRelation::QueryInterface(REFIID id, LPVOID *iface)
{
    *iface = 0;
    if (id == IID_IUnknown)
        *iface = (IUnknown*)this;

    if (*iface) {
        AddRef();
        return S_OK;
    }

    return E_NOINTERFACE;
}

ULONG STDMETHODCALLTYPE AccessibleRelation::AddRef()
{
    return ++m_ref;
}

ULONG STDMETHODCALLTYPE AccessibleRelation::Release()
{
    if (!--m_ref) {
        delete this;
        return 0;
    }
    return m_ref;
}

/* IAccessibleRelation */
HRESULT STDMETHODCALLTYPE AccessibleRelation::get_relationType(
    /* [retval][out] */ BSTR *relationType)
{
    *relationType = relationToBSTR(m_relation);
    return S_OK;
}

HRESULT STDMETHODCALLTYPE AccessibleRelation::get_localizedRelationType(
    /* [retval][out] */ BSTR *localizedRelationType)
{
    // Who ever needs this???
    *localizedRelationType = relationToBSTR(m_relation);
    return S_OK;
}

HRESULT STDMETHODCALLTYPE AccessibleRelation::get_nTargets(
    /* [retval][out] */ long *nTargets)
{
    // ### always one target
    *nTargets = m_targets.count();
    return S_OK;
}

/*!
  \internal
  Client allocates and deallocates array
  (see "Special Consideration when using Arrays", in Accessible2.idl)
  */
HRESULT STDMETHODCALLTYPE AccessibleRelation::get_target(
    /* [in] */ long targetIndex,
    /* [retval][out] */ IUnknown **target)
{
    if (targetIndex >= 0 && targetIndex < m_targets.count()) {
        QAccessibleInterface *iface = m_targets.at(targetIndex);
        *target = QWindowsAccessibility::wrap(iface);
        if (*target)
            return S_OK;
        return E_FAIL;
    }
    return E_INVALIDARG;
}

/*!
  \internal
  Client allocates and deallocates \a targets array
  (see "Special Consideration when using Arrays", in Accessible2.idl)
  */
HRESULT STDMETHODCALLTYPE AccessibleRelation::get_targets(
    /* [in] */ long maxTargets,
    /* [length_is][size_is][out] */ IUnknown **targets,
    /* [retval][out] */ long *nTargets)
{

    const int numTargets = qMin((int)maxTargets, m_targets.count());
    for (int i = 0; i < numTargets; ++i) {
        QAccessibleInterface *iface = m_targets.at(i);
        IAccessible *iacc = QWindowsAccessibility::wrap(iface);
        if (!iacc)
            return E_FAIL;
        *targets = iacc;
        ++targets;
    }
    *nTargets = numTargets;
    // \a targets array is allocated by client.
    return numTargets > 0 ? S_OK : S_FALSE;
}


/**************************************************************\
 *                                                             *
 *                        IUnknown                             *
 *                                                             *
 **************************************************************/
HRESULT STDMETHODCALLTYPE QWindowsIA2Accessible::QueryInterface(REFIID id, LPVOID *iface)
{
    QAccessibleInterface *accessible = accessibleInterface();
    if (!accessible)
        return E_NOINTERFACE;

    HRESULT hr = QWindowsMsaaAccessible::QueryInterface(id, iface);
    if (!SUCCEEDED(hr)) {
        if (id == IID_IServiceProvider) {
            *iface = (IServiceProvider*)this;
        } else if (id == IID_IAccessible2) {
            *iface = (IAccessible2*)this;
        } else if (id == IID_IAccessibleAction) {
            if (accessible->actionInterface())
                *iface = (IAccessibleAction*)this;
        } else if (id == IID_IAccessibleComponent) {
            *iface = (IAccessibleComponent*)this;
        } else if (id == IID_IAccessibleEditableText) {
            if (accessible->editableTextInterface() ||
                accessible->role() == QAccessible::EditableText)
            {
                *iface = (IAccessibleEditableText*)this;
            }
        } else if (id == IID_IAccessibleHyperlink) {
            //*iface = (IAccessibleHyperlink*)this;
        } else if (id == IID_IAccessibleHypertext) {
            //*iface = (IAccessibleHypertext*)this;
        } else if (id == IID_IAccessibleImage) {
            //*iface = (IAccessibleImage*)this;
        } else if (id == IID_IAccessibleRelation) {
            *iface = (IAccessibleRelation*)this;
        } else if (id == IID_IAccessibleTable) {
            //*iface = (IAccessibleTable*)this; // not supported
        } else if (id == IID_IAccessibleTable2) {
            if (accessible->tableInterface())
                *iface = (IAccessibleTable2*)this;
        } else if (id == IID_IAccessibleTableCell) {
            if (accessible->tableCellInterface())
                *iface = (IAccessibleTableCell*)this;
        } else if (id == IID_IAccessibleText) {
            if (accessible->textInterface())
                *iface = (IAccessibleText*)this;
        } else if (id == IID_IAccessibleValue) {
            if (accessible->valueInterface())
                *iface = (IAccessibleValue*)this;
        }
        if (*iface) {
            AddRef();
            hr = S_OK;
        } else {
            hr = E_NOINTERFACE;
        }
    }
    return hr;
}


/* Note that IUnknown is inherited from several interfaces. Therefore we must reimplement all its
   functions in the concrete class to avoid ambiguity.
*/
ULONG STDMETHODCALLTYPE QWindowsIA2Accessible::AddRef()
{
    return QWindowsMsaaAccessible::AddRef();
}

ULONG STDMETHODCALLTYPE QWindowsIA2Accessible::Release()
{
    return QWindowsMsaaAccessible::Release();
}

/**************************************************************\
 *                                                             *
 *                        IAccessible2                         *
 *                                                             *
 **************************************************************/
HRESULT STDMETHODCALLTYPE QWindowsIA2Accessible::get_nRelations(long *nRelations)
{
    QAccessibleInterface *accessible = accessibleInterface();
    accessibleDebugClientCalls(accessible);
    if (!nRelations)
      return E_INVALIDARG;
    if (!accessible)
        return E_FAIL;

    return getRelationsHelper(0, 0, 0, nRelations);
}

HRESULT STDMETHODCALLTYPE QWindowsIA2Accessible::get_relation(long relationIndex, IAccessibleRelation **relation)
{
    QAccessibleInterface *accessible = accessibleInterface();
    accessibleDebugClientCalls(accessible);
    if (!relation)
      return E_INVALIDARG;
    if (!accessible)
        return E_FAIL;

    return getRelationsHelper(relation, relationIndex,  1);
}

/*!
  \internal
  Client allocates and deallocates array
  (see "Special Consideration when using Arrays", in Accessible2.idl)
  */
HRESULT STDMETHODCALLTYPE QWindowsIA2Accessible::get_relations(long maxRelations,
                                        IAccessibleRelation **relations,
                                        long *nRelations)
{
    QAccessibleInterface *accessible = accessibleInterface();
    accessibleDebugClientCalls(accessible);
    if (!accessible)
        return E_FAIL;

    return getRelationsHelper(relations, 0, maxRelations, nRelations);
}


HRESULT STDMETHODCALLTYPE QWindowsIA2Accessible::role(long *ia2role)
{
    QAccessibleInterface *accessible = accessibleInterface();
    accessibleDebugClientCalls(accessible);
    if (!accessible)
        return E_FAIL;

    long r = accessible->role();

    switch (r) {
    case QAccessible::LayeredPane: r = IA2_ROLE_LAYERED_PANE; break;
    case QAccessible::Terminal: r = IA2_ROLE_TERMINAL; break;
    case QAccessible::Desktop: r = IA2_ROLE_DESKTOP_PANE; break;
    default: break;
    }

    *ia2role = r;
    return S_OK;
}


HRESULT STDMETHODCALLTYPE QWindowsIA2Accessible::scrollTo(enum IA2ScrollType /*scrollType*/)
{
    //### Ignore for now
    return E_NOTIMPL;
}


HRESULT STDMETHODCALLTYPE QWindowsIA2Accessible::scrollToPoint(enum IA2CoordinateType /*coordinateType*/, long /*x*/, long /*y*/)
{
    //### Ignore for now
    return E_NOTIMPL;
}


HRESULT STDMETHODCALLTYPE QWindowsIA2Accessible::get_groupPosition(long *groupLevel,
                                            long *similarItemsInGroup,
                                            long *positionInGroup)
{
    // ### Ignore for now. Not sure what this is used for.....
    *groupLevel = 0;            // Not applicable
    *similarItemsInGroup = 0;   // Not applicable
    *positionInGroup = 0;       // Not applicable
    return S_FALSE;
}


HRESULT STDMETHODCALLTYPE QWindowsIA2Accessible::get_states(AccessibleStates *states)
{
    QAccessibleInterface *accessible = accessibleInterface();
    accessibleDebugClientCalls(accessible);
    if (!accessible)
        return E_FAIL;
    if (!states)
        return E_POINTER;
    QAccessible::State st = accessible->state();
    AccessibleStates ia2states = 0;
    if (st.active)
        ia2states |= IA2_STATE_ACTIVE;
    if (st.invalid)
        ia2states |= IA2_STATE_DEFUNCT;
    if (st.editable)
        ia2states |= IA2_STATE_EDITABLE;
    if (st.multiLine)
        ia2states |= IA2_STATE_MULTI_LINE;
    if (st.selectableText)
        ia2states |= IA2_STATE_SELECTABLE_TEXT;
    if (st.supportsAutoCompletion)
        ia2states |= IA2_STATE_SUPPORTS_AUTOCOMPLETION;

    *states = ia2states;
    return S_OK;
}


HRESULT STDMETHODCALLTYPE QWindowsIA2Accessible::get_extendedRole(BSTR *extendedRole)
{
    //###
    *extendedRole = 0;
    return E_NOTIMPL;   // mozilla does this
    //return S_FALSE;
}


HRESULT STDMETHODCALLTYPE QWindowsIA2Accessible::get_localizedExtendedRole(BSTR *localizedExtendedRole)
{
    //###
    *localizedExtendedRole = 0;
    return E_NOTIMPL;   // mozilla does this
    //return S_FALSE;
}


HRESULT STDMETHODCALLTYPE QWindowsIA2Accessible::get_nExtendedStates(long *nExtendedStates)
{
    // Who will ever intepret these values into something meaningful??
    *nExtendedStates = 0;
    return E_NOTIMPL;   // mozilla does this
    //return S_FALSE;
}


HRESULT STDMETHODCALLTYPE QWindowsIA2Accessible::get_extendedStates(long /*maxExtendedStates*/,
                                             BSTR **extendedStates,
                                             long *nExtendedStates)
{
    *extendedStates = 0;
    *nExtendedStates = 0;
    return E_NOTIMPL;   // mozilla does this
    //return S_FALSE;
}


HRESULT STDMETHODCALLTYPE QWindowsIA2Accessible::get_localizedExtendedStates(long /*maxLocalizedExtendedStates*/,
                                                      BSTR **localizedExtendedStates,
                                                      long *nLocalizedExtendedStates)
{
    *localizedExtendedStates = 0;
    *nLocalizedExtendedStates = 0;
    return E_NOTIMPL;   // mozilla does this
    //return S_FALSE;
}


HRESULT STDMETHODCALLTYPE QWindowsIA2Accessible::get_uniqueID(long *outUniqueID)
{
    QAccessibleInterface *accessible = accessibleInterface();
    accessibleDebugClientCalls(accessible);
    if (!accessible)
        return E_FAIL;

    qCDebug(lcQpaAccessibility) << "uniqueID: " << showbase << hex << id;

    *outUniqueID = (long)id;
    return int(id) < 0 ? S_OK : S_FALSE;
}


HRESULT STDMETHODCALLTYPE QWindowsIA2Accessible::get_windowHandle(HWND *windowHandle)
{
    QAccessibleInterface *accessible = accessibleInterface();
    accessibleDebugClientCalls(accessible);
    if (!accessible)
        return E_FAIL;
    return GetWindow(windowHandle);
}


HRESULT STDMETHODCALLTYPE QWindowsIA2Accessible::get_indexInParent(long *indexInParent)
{
    QAccessibleInterface *accessible = accessibleInterface();
    accessibleDebugClientCalls(accessible);
    if (!accessible)
        return E_FAIL;
    if (!indexInParent)
      return E_INVALIDARG;
    QAccessibleInterface *par = accessible->parent();
    if (!par) {
        *indexInParent = -1;
        return S_FALSE;
    }
    int indexOfChild = par->indexOfChild(accessible);
    Q_ASSERT(indexOfChild >= 0);
    *indexInParent = indexOfChild;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE QWindowsIA2Accessible::get_locale(IA2Locale *locale)
{
    QAccessibleInterface *accessible = accessibleInterface();
    accessibleDebugClientCalls(accessible);
    if (!accessible)
        return E_FAIL;
    IA2Locale res;
    QLocale l;
    res.country = QStringToBSTR(QLocale::countryToString(l.country()));
    res.language = QStringToBSTR(QLocale::languageToString(l.language()));
    *locale = res;
    return S_OK;
}


HRESULT STDMETHODCALLTYPE QWindowsIA2Accessible::get_attributes(BSTR *attributes)
{
    QAccessibleInterface *accessible = accessibleInterface();
    accessibleDebugClientCalls(accessible);
    if (!accessible)
        return E_FAIL;
    *attributes = 0;//QStringToBSTR(QString());
    return S_FALSE;
}

/**************************************************************\
 *                      IAccessibleAction                      *
 **************************************************************/
HRESULT STDMETHODCALLTYPE QWindowsIA2Accessible::nActions(long *nActions)
{
    QAccessibleInterface *accessible = accessibleInterface();
    accessibleDebugClientCalls(accessible);
    if (!accessible)
        return E_FAIL;
    *nActions = 0;

    if (QAccessibleActionInterface *actionIface = actionInterface())
        *nActions = actionIface->actionNames().count();
    return S_OK;
}

HRESULT STDMETHODCALLTYPE QWindowsIA2Accessible::doAction(long actionIndex)
{
    QAccessibleInterface *accessible = accessibleInterface();
    accessibleDebugClientCalls(accessible);
    if (!accessible)
        return E_FAIL;
    if (QAccessibleActionInterface *actionIface = actionInterface()) {
        const QStringList actionNames = actionIface->actionNames();
        if (actionIndex < 0 || actionIndex >= actionNames.count())
            return E_INVALIDARG;
        const QString actionName = actionNames.at(actionIndex);
        actionIface->doAction(actionName);
        return S_OK;
    }
    return S_FALSE;
}

HRESULT STDMETHODCALLTYPE QWindowsIA2Accessible::get_description(long actionIndex, BSTR *description)
{
    QAccessibleInterface *accessible = accessibleInterface();
    accessibleDebugClientCalls(accessible);
    if (!accessible)
        return E_FAIL;
    *description = 0;
    if (QAccessibleActionInterface *actionIface = actionInterface()) {
        const QStringList actionNames = actionIface->actionNames();
        if (actionIndex < 0 || actionIndex >= actionNames.count())
            return E_INVALIDARG;
        const QString actionName = actionNames.at(actionIndex);
        *description = QStringToBSTR(actionIface->localizedActionDescription(actionName));
    }
    return *description ? S_OK : S_FALSE;
}

HRESULT STDMETHODCALLTYPE QWindowsIA2Accessible::get_keyBinding(long actionIndex, long nMaxBindings, BSTR **keyBindings, long *nBindings)
{
    QAccessibleInterface *accessible = accessibleInterface();
    accessibleDebugClientCalls(accessible);
    if (!accessible)
        return E_FAIL;
    Q_UNUSED(nMaxBindings);
    BSTR *arrayOfBindingsToReturn = 0;
    int numBindings = 0;
    if (QAccessibleActionInterface *actionIface = actionInterface()) {
        const QStringList actionNames = actionIface->actionNames();
        if (actionIndex < 0 || actionIndex >= actionNames.count())
            return E_INVALIDARG;
        const QString actionName = actionNames.at(actionIndex);
        const QStringList keyBindings = actionIface->keyBindingsForAction(actionName);
        numBindings = keyBindings.count();
        if (numBindings > 0) {
            // The IDL documents that the client must free with CoTaskMemFree
            arrayOfBindingsToReturn = (BSTR*)::CoTaskMemAlloc(sizeof(BSTR) * numBindings);
            for (int i = 0; i < numBindings; ++i)
                arrayOfBindingsToReturn[i] = QStringToBSTR(keyBindings.at(i));
        }
    }
    *keyBindings = arrayOfBindingsToReturn;
    *nBindings = numBindings;

    return numBindings ? S_OK : S_FALSE;
}

HRESULT STDMETHODCALLTYPE QWindowsIA2Accessible::get_name(long actionIndex, BSTR *name)
{
    QAccessibleInterface *accessible = accessibleInterface();
    accessibleDebugClientCalls(accessible);
    if (!accessible)
        return E_FAIL;
    *name = 0;
    if (QAccessibleActionInterface *actionIface = actionInterface()) {
        const QStringList actionNames = actionIface->actionNames();
        if (actionIndex < 0 || actionIndex >= actionNames.count())
            return E_INVALIDARG;
        const QString actionName = actionNames.at(actionIndex);
        *name = QStringToBSTR(actionName);
    }
    return *name ? S_OK : S_FALSE;
}

HRESULT STDMETHODCALLTYPE QWindowsIA2Accessible::get_localizedName(long actionIndex, BSTR *localizedName)
{
    QAccessibleInterface *accessible = accessibleInterface();
    accessibleDebugClientCalls(accessible);
    if (!accessible)
        return E_FAIL;
    *localizedName = 0;
    if (QAccessibleActionInterface *actionIface = actionInterface()) {
        const QStringList actionNames = actionIface->actionNames();
        if (actionIndex < 0 || actionIndex >= actionNames.count())
            return E_INVALIDARG;

        const QString actionName = actionNames.at(actionIndex);
        *localizedName = QStringToBSTR(actionIface->localizedActionName(actionName));
    }
    return *localizedName ? S_OK : S_FALSE;
}

/**************************************************************\
 *                     IAccessibleComponent                    *
 **************************************************************/
HRESULT STDMETHODCALLTYPE QWindowsIA2Accessible::get_locationInParent(long *x, long *y)
{
    QAccessibleInterface *accessible = accessibleInterface();
    accessibleDebugClientCalls(accessible);
    if (!accessible)
        return E_FAIL;

    QPoint topLeft = accessible->rect().topLeft();

    QAccessibleInterface *parentIface = accessible->parent();
    if (parentIface && parentIface->isValid())
        topLeft -= parentIface->rect().topLeft();

    *x = topLeft.x();
    *y = topLeft.y();
    return S_OK;
}

HRESULT STDMETHODCALLTYPE QWindowsIA2Accessible::get_foreground(IA2Color *foreground)
{
    QAccessibleInterface *accessible = accessibleInterface();
    accessibleDebugClientCalls(accessible);
    if (!accessible)
        return E_FAIL;

    // IA2Color is a typedef for long
    *foreground = (IA2Color)accessible->foregroundColor().rgb();
    return S_OK;
}

HRESULT STDMETHODCALLTYPE QWindowsIA2Accessible::get_background(IA2Color *background)
{
    QAccessibleInterface *accessible = accessibleInterface();
    accessibleDebugClientCalls(accessible);
    if (!accessible)
        return E_FAIL;

    // IA2Color is a typedef for long
    *background = (IA2Color)accessible->backgroundColor().rgb();
    return S_OK;
}

/**************************************************************\
 *                     IAccessibleEditableText                *
 **************************************************************/
#ifndef QT_NO_CLIPBOARD
/*!
    \internal

    if \a endOffset == -1 it means end of the text
*/
QString QWindowsIA2Accessible::textForRange(int startOffset, int endOffset) const
{
    QAccessibleInterface *accessible = accessibleInterface();

    if (QAccessibleTextInterface *textIface = accessible->textInterface()) {
        if (endOffset == IA2_TEXT_OFFSET_LENGTH)
            endOffset = textIface->characterCount();
        return textIface->text(startOffset, endOffset);
    }
    QString txt = accessible->text(QAccessible::Value);
    if (endOffset == IA2_TEXT_OFFSET_LENGTH)
        endOffset = txt.length();
    return txt.mid(startOffset, endOffset - startOffset);
}
#endif

/*!
    \internal
*/
void QWindowsIA2Accessible::replaceTextFallback(long startOffset, long endOffset, const QString &txt)
{
    QAccessibleInterface *accessible = accessibleInterface();
    QString t = textForRange(0, -1);
    if (endOffset == IA2_TEXT_OFFSET_LENGTH)
        endOffset = t.length();
    if (endOffset - startOffset == 0) {
        t.insert(startOffset, txt);
    } else {
        t.replace(startOffset, endOffset - startOffset, txt);
    }
    accessible->setText(QAccessible::Value, t);
}

HRESULT STDMETHODCALLTYPE QWindowsIA2Accessible::copyText(long startOffset, long endOffset)
{
    QAccessibleInterface *accessible = accessibleInterface();
    accessibleDebugClientCalls(accessible);
#ifndef QT_NO_CLIPBOARD
    const QString t = textForRange(startOffset, endOffset);
    QGuiApplication::clipboard()->setText(t);
    return S_OK;
#else
    return E_NOTIMPL;
#endif
}

HRESULT STDMETHODCALLTYPE QWindowsIA2Accessible::deleteText(long startOffset, long endOffset)
{
    QAccessibleInterface *accessible = accessibleInterface();
    accessibleDebugClientCalls(accessible);
    if (QAccessibleEditableTextInterface *editableTextIface = accessible->editableTextInterface())
        editableTextIface->deleteText(startOffset, endOffset);
    else
        replaceTextFallback(startOffset, endOffset, QString());
    return S_OK;
}

HRESULT STDMETHODCALLTYPE QWindowsIA2Accessible::insertText(long offset, BSTR *text)
{
    QAccessibleInterface *accessible = accessibleInterface();
    accessibleDebugClientCalls(accessible);
    const QString txt(BSTRToQString(*text));
    if (QAccessibleEditableTextInterface *editableTextIface = accessible->editableTextInterface())
        editableTextIface->insertText(offset, txt);
    else
        replaceTextFallback(offset, offset, txt);
    return S_OK;
}

HRESULT STDMETHODCALLTYPE QWindowsIA2Accessible::cutText(long startOffset, long endOffset)
{
    QAccessibleInterface *accessible = accessibleInterface();
    accessibleDebugClientCalls(accessible);
#ifndef QT_NO_CLIPBOARD
    const QString t = textForRange(startOffset, endOffset);
    if (QAccessibleEditableTextInterface *editableTextIface = accessible->editableTextInterface())
        editableTextIface->deleteText(startOffset, endOffset);
    else
        replaceTextFallback(startOffset, endOffset, QString());
    QGuiApplication::clipboard()->setText(t);
    return S_OK;
#else
    return E_NOTIMPL;
#endif
}

HRESULT STDMETHODCALLTYPE QWindowsIA2Accessible::pasteText(long offset)
{
    QAccessibleInterface *accessible = accessibleInterface();
    accessibleDebugClientCalls(accessible);
#ifndef QT_NO_CLIPBOARD
    const QString txt = QGuiApplication::clipboard()->text();
    if (QAccessibleEditableTextInterface *editableTextIface = accessible->editableTextInterface())
        editableTextIface->insertText(offset, txt);
    else
        replaceTextFallback(offset, offset, txt);
    return S_OK;
#else
    return E_NOTIMPL;
#endif
}

HRESULT STDMETHODCALLTYPE QWindowsIA2Accessible::replaceText(long startOffset, long endOffset, BSTR *text)
{
    QAccessibleInterface *accessible = accessibleInterface();
    accessibleDebugClientCalls(accessible);
    const QString txt(BSTRToQString(*text));
    if (QAccessibleEditableTextInterface *editableTextIface = accessible->editableTextInterface())
        editableTextIface->replaceText(startOffset, endOffset, txt);
    else
        replaceTextFallback(startOffset, endOffset, txt);
    return S_OK;
}

HRESULT STDMETHODCALLTYPE QWindowsIA2Accessible::setAttributes(long /*startOffset*/, long /*endOffset*/, BSTR * /*attributes*/)
{
    return E_NOTIMPL;
}


/**************************************************************\
 *                     IAccessibleTable2                      *
 **************************************************************/
HRESULT STDMETHODCALLTYPE QWindowsIA2Accessible::get_cellAt( long row, long column, IUnknown **cell)
{
    QAccessibleInterface *accessible = accessibleInterface();
    accessibleDebugClientCalls(accessible);
    if (!accessible)
        return E_FAIL;

    *cell = 0;
    if (QAccessibleTableInterface *tableIface = tableInterface()) {
        if (QAccessibleInterface *qtCell = tableIface->cellAt(row, column)) {
            *cell = QWindowsAccessibility::wrap(qtCell);
        }
    }
    qCDebug(lcQpaAccessibility) << "found cell? " << *cell;
    return *cell ? S_OK : S_FALSE;
}

HRESULT STDMETHODCALLTYPE QWindowsIA2Accessible::get_caption( IUnknown **captionInterface)
{
    QAccessibleInterface *accessible = accessibleInterface();
    accessibleDebugClientCalls(accessible);
    if (!accessible)
        return E_FAIL;

    *captionInterface = 0;
    if (QAccessibleTableInterface *tableIface = tableInterface()) {
        if (QAccessibleInterface *iface = tableIface->caption())
            *captionInterface = QWindowsAccessibility::wrap(iface);
    }
    return *captionInterface ? S_OK : S_FALSE;
}

HRESULT STDMETHODCALLTYPE QWindowsIA2Accessible::get_columnDescription( long column, BSTR *description)
{
    QAccessibleInterface *accessible = accessibleInterface();
    accessibleDebugClientCalls(accessible);
    if (!accessible)
        return E_FAIL;

    *description = 0;
    if (QAccessibleTableInterface *tableIface = tableInterface()) {
        const QString qtDesc = tableIface->columnDescription(column);
        if (!qtDesc.isEmpty())
            *description = QStringToBSTR(qtDesc);
    }
    return *description ? S_OK : S_FALSE;
}

HRESULT STDMETHODCALLTYPE QWindowsIA2Accessible::get_nColumns( long *columnCount)
{
    QAccessibleInterface *accessible = accessibleInterface();
    accessibleDebugClientCalls(accessible);
    if (!accessible)
        return E_FAIL;

    if (QAccessibleTableInterface *tableIface = tableInterface()) {
        *columnCount = tableIface->columnCount();
        return S_OK;
    }
    return E_FAIL;
}

HRESULT STDMETHODCALLTYPE QWindowsIA2Accessible::get_nRows(long *rowCount)
{
    QAccessibleInterface *accessible = accessibleInterface();
    accessibleDebugClientCalls(accessible);
    if (!accessible)
        return E_FAIL;

    if (QAccessibleTableInterface *tableIface = tableInterface()) {
        *rowCount = tableIface->rowCount();
        return S_OK;
    }
    return E_FAIL;
}

HRESULT STDMETHODCALLTYPE QWindowsIA2Accessible::get_nSelectedCells(long *cellCount)
{
    QAccessibleInterface *accessible = accessibleInterface();
    accessibleDebugClientCalls(accessible);
    if (!accessible)
        return E_FAIL;

    if (QAccessibleTableInterface *tableIface = tableInterface()) {
        *cellCount = tableIface->selectedCellCount();
        return S_OK;
    }
    return E_FAIL;
}

HRESULT STDMETHODCALLTYPE QWindowsIA2Accessible::get_nSelectedColumns(long *columnCount)
{
    QAccessibleInterface *accessible = accessibleInterface();
    accessibleDebugClientCalls(accessible);
    if (!accessible)
        return E_FAIL;

    if (QAccessibleTableInterface *tableIface = tableInterface()) {
        *columnCount = tableIface->selectedColumnCount();
        return S_OK;
    }
    return E_FAIL;
}

HRESULT STDMETHODCALLTYPE QWindowsIA2Accessible::get_nSelectedRows(long *rowCount)
{
    QAccessibleInterface *accessible = accessibleInterface();
    accessibleDebugClientCalls(accessible);
    if (!accessible)
        return E_FAIL;

    if (QAccessibleTableInterface *tableIface = tableInterface()) {
        *rowCount = tableIface->selectedRowCount();
        return S_OK;
    }
    return E_FAIL;
}

HRESULT STDMETHODCALLTYPE QWindowsIA2Accessible::get_rowDescription(long row, BSTR *description)
{
    QAccessibleInterface *accessible = accessibleInterface();
    accessibleDebugClientCalls(accessible);
    if (!accessible)
        return E_FAIL;

    *description = 0;
    if (QAccessibleTableInterface *tableIface = tableInterface()) {
        const QString qtDesc = tableIface->rowDescription(row);
        if (!qtDesc.isEmpty())
            *description = QStringToBSTR(qtDesc);
    }
    return *description ? S_OK : S_FALSE;
}

HRESULT STDMETHODCALLTYPE QWindowsIA2Accessible::get_selectedCells(IUnknown ***cells, long *nSelectedCells)
{
    QAccessibleInterface *accessible = accessibleInterface();
    accessibleDebugClientCalls(accessible);
    Q_UNUSED(cells);
    Q_UNUSED(nSelectedCells);
    if (!accessible)
        return E_FAIL;

    QList<QAccessibleInterface*> selectedCells = tableInterface()->selectedCells();
    return wrapListOfCells(selectedCells, cells, nSelectedCells);
}

HRESULT STDMETHODCALLTYPE QWindowsIA2Accessible::get_selectedColumns(long **selectedColumns, long *nColumns)
{
    QAccessibleInterface *accessible = accessibleInterface();
    accessibleDebugClientCalls(accessible);
    if (!accessible)
        return E_FAIL;

    if (QAccessibleTableInterface *tableIface = tableInterface()) {
        const QList<int> selectedIndices = tableIface->selectedColumns();
        const int &count = selectedIndices.count();
        long *selected = (count ? (long*)::CoTaskMemAlloc(sizeof(long) * count) : (long*)0);
        for (int i = 0; i < count; ++i)
            selected[i] = selectedIndices.at(i);
        *selectedColumns = selected;
        *nColumns = count;
        return count ? S_OK : S_FALSE;
    }
    return E_FAIL;

}

HRESULT STDMETHODCALLTYPE QWindowsIA2Accessible::get_selectedRows(long **selectedRows, long *nRows)
{
    QAccessibleInterface *accessible = accessibleInterface();
    accessibleDebugClientCalls(accessible);
    if (!accessible)
        return E_FAIL;

    if (QAccessibleTableInterface *tableIface = tableInterface()) {
        const QList<int> selectedIndices = tableIface->selectedRows();
        const int &count = selectedIndices.count();
        long *selected = (count ? (long*)::CoTaskMemAlloc(sizeof(long) * count) : (long*)0);
        for (int i = 0; i < count; ++i)
            selected[i] = selectedIndices.at(i);
        *selectedRows = selected;
        *nRows = count;
        return count ? S_OK : S_FALSE;
    }
    return E_FAIL;

}

HRESULT STDMETHODCALLTYPE QWindowsIA2Accessible::get_summary(IUnknown **summaryInterface)
{
    QAccessibleInterface *accessible = accessibleInterface();
    accessibleDebugClientCalls(accessible);
    if (!accessible)
        return E_FAIL;

    *summaryInterface = 0;
    if (QAccessibleTableInterface *tableIface = tableInterface()) {
        if (QAccessibleInterface *iface = tableIface->summary())
            *summaryInterface = QWindowsAccessibility::wrap(iface);
    }
    return *summaryInterface ? S_OK : S_FALSE;
}

HRESULT STDMETHODCALLTYPE QWindowsIA2Accessible::get_isColumnSelected(long column, boolean *isSelected)
{
    QAccessibleInterface *accessible = accessibleInterface();
    accessibleDebugClientCalls(accessible);
    if (!accessible)
        return E_FAIL;

    if (QAccessibleTableInterface *tableIface = tableInterface()) {
        *isSelected = tableIface->isColumnSelected(column);
        return S_OK;
    }
    return E_FAIL;
}

HRESULT STDMETHODCALLTYPE QWindowsIA2Accessible::get_isRowSelected(long row, boolean *isSelected)
{
    QAccessibleInterface *accessible = accessibleInterface();
    accessibleDebugClientCalls(accessible);
    if (!accessible)
        return E_FAIL;

    if (QAccessibleTableInterface *tableIface = tableInterface()) {
        *isSelected = tableIface->isRowSelected(row);
        return S_OK;
    }
    return E_FAIL;
}

HRESULT STDMETHODCALLTYPE QWindowsIA2Accessible::selectRow(long row)
{
    QAccessibleInterface *accessible = accessibleInterface();
    accessibleDebugClientCalls(accessible);
    if (!accessible)
        return E_FAIL;

    if (QAccessibleTableInterface *tableIface = tableInterface()) {
        bool ok = tableIface->selectRow(row);
        return ok ? S_OK : E_INVALIDARG;    //### Not sure of the return value if it fails???
    }
    return E_FAIL;
}

HRESULT STDMETHODCALLTYPE QWindowsIA2Accessible::selectColumn(long column)
{
    QAccessibleInterface *accessible = accessibleInterface();
    accessibleDebugClientCalls(accessible);
    if (!accessible)
        return E_FAIL;

    if (QAccessibleTableInterface *tableIface = tableInterface()) {
        bool ok = tableIface->selectColumn(column);
        return ok ? S_OK : E_INVALIDARG;    //### Not sure of the return value if it fails???
    }
    return E_FAIL;
}

HRESULT STDMETHODCALLTYPE QWindowsIA2Accessible::unselectRow(long row)
{
    QAccessibleInterface *accessible = accessibleInterface();
    accessibleDebugClientCalls(accessible);
    if (!accessible)
        return E_FAIL;

    if (QAccessibleTableInterface *tableIface = tableInterface()) {
        bool ok = tableIface->unselectRow(row);
        return ok ? S_OK : E_INVALIDARG;    //### Not sure of the return value if it fails???
    }
    return E_FAIL;
}

HRESULT STDMETHODCALLTYPE QWindowsIA2Accessible::unselectColumn(long column)
{
    QAccessibleInterface *accessible = accessibleInterface();
    accessibleDebugClientCalls(accessible);
    if (!accessible)
        return E_FAIL;

    if (QAccessibleTableInterface *tableIface = tableInterface()) {
        bool ok = tableIface->unselectColumn(column);
        return ok ? S_OK : E_INVALIDARG;    //### Not sure of the return value if it fails???
    }
    return E_FAIL;
}

HRESULT STDMETHODCALLTYPE QWindowsIA2Accessible::get_modelChange( IA2TableModelChange * /*modelChange*/)
{
    QAccessibleInterface *accessible = accessibleInterface();
    accessibleDebugClientCalls(accessible);
    if (!accessible)
        return E_FAIL;
    return E_NOTIMPL;
}

/**************************************************************\
 *                     IAccessibleTableCell                   *
\**************************************************************/
HRESULT STDMETHODCALLTYPE QWindowsIA2Accessible::get_columnExtent(long *nColumnsSpanned)
{
    QAccessibleInterface *accessible = accessibleInterface();
    accessibleDebugClientCalls(accessible);
    if (!accessible)
        return E_FAIL;

    *nColumnsSpanned = tableCellInterface()->columnExtent();
    return S_OK;
}

HRESULT STDMETHODCALLTYPE QWindowsIA2Accessible::get_columnHeaderCells(IUnknown ***cellAccessibles,
                                                                    long *nColumnHeaderCells)
{
    QAccessibleInterface *accessible = accessibleInterface();
    accessibleDebugClientCalls(accessible);
    if (!accessible)
        return E_FAIL;
    const QList<QAccessibleInterface*> headerCells = tableCellInterface()->columnHeaderCells();
    return wrapListOfCells(headerCells, cellAccessibles, nColumnHeaderCells);
}

HRESULT STDMETHODCALLTYPE QWindowsIA2Accessible::get_columnIndex(long *columnIndex)
{
    QAccessibleInterface *accessible = accessibleInterface();
    accessibleDebugClientCalls(accessible);
    if (!accessible)
        return E_FAIL;
    *columnIndex = tableCellInterface()->columnIndex();
    return S_OK;
}

HRESULT STDMETHODCALLTYPE QWindowsIA2Accessible::get_rowExtent(long *nRowsSpanned)
{
    QAccessibleInterface *accessible = accessibleInterface();
    accessibleDebugClientCalls(accessible);
    if (!accessible)
        return E_FAIL;
    *nRowsSpanned = tableCellInterface()->rowExtent();
    return S_OK;
}

HRESULT STDMETHODCALLTYPE QWindowsIA2Accessible::get_rowHeaderCells(IUnknown ***cellAccessibles,
                                                                 long *nRowHeaderCells)
{
    QAccessibleInterface *accessible = accessibleInterface();
    accessibleDebugClientCalls(accessible);
    if (!accessible)
        return E_FAIL;
    const QList<QAccessibleInterface*> headerCells = tableCellInterface()->rowHeaderCells();
    return wrapListOfCells(headerCells, cellAccessibles, nRowHeaderCells);
}

HRESULT STDMETHODCALLTYPE QWindowsIA2Accessible::get_rowIndex(long *rowIndex)
{
    QAccessibleInterface *accessible = accessibleInterface();
    accessibleDebugClientCalls(accessible);
    if (!accessible)
        return E_FAIL;
    *rowIndex = tableCellInterface()->rowIndex();
    return S_OK;
}

HRESULT STDMETHODCALLTYPE QWindowsIA2Accessible::get_isSelected( boolean *isSelected)
{
    QAccessibleInterface *accessible = accessibleInterface();
    accessibleDebugClientCalls(accessible);
    if (!accessible)
        return E_FAIL;
    *isSelected = tableCellInterface()->isSelected();
    return S_OK;
}

HRESULT STDMETHODCALLTYPE QWindowsIA2Accessible::get_rowColumnExtents(long *row, long *column,
                                               long *rowExtents, long *columnExtents,
                                               boolean *isSelected)
{
    QAccessibleInterface *accessible = accessibleInterface();
    accessibleDebugClientCalls(accessible);
    if (!accessible || !tableCellInterface())
        return E_FAIL;

    *row = (long)tableCellInterface()->rowIndex();
    *column = (long)tableCellInterface()->columnIndex();
    *rowExtents = (long)tableCellInterface()->rowExtent();
    *columnExtents = (long)tableCellInterface()->columnExtent();
    *isSelected = tableCellInterface()->isSelected();
    return S_OK;
}

HRESULT STDMETHODCALLTYPE QWindowsIA2Accessible::get_table(IUnknown **table)
{
    QAccessibleInterface *accessible = accessibleInterface();
    accessibleDebugClientCalls(accessible);
    if (!accessible)
        return E_FAIL;

    QAccessibleInterface *tableIface = tableCellInterface()->table();

    *table = QWindowsAccessibility::wrap(tableIface);
    return S_OK;
}

/**************************************************************\
 *                     IAccessibleText                        *
\**************************************************************/
HRESULT STDMETHODCALLTYPE QWindowsIA2Accessible::addSelection(long startOffset,
                                                           long endOffset)
{
    QAccessibleInterface *accessible = accessibleInterface();
    accessibleDebugClientCalls(accessible);
    if (QAccessibleTextInterface *text = textInterface()) {
        text->addSelection(startOffset, endOffset);
        return S_OK;
    }
    return E_FAIL;
}

HRESULT STDMETHODCALLTYPE QWindowsIA2Accessible::get_attributes(long offset,
                                                             long *startOffset,
                                                             long *endOffset,
                                                             BSTR *textAttributes)
{
    QAccessibleInterface *accessible = accessibleInterface();
    accessibleDebugClientCalls(accessible);
    if (QAccessibleTextInterface *text = textInterface()) {
        const QString attrs = text->attributes(offset, (int*)startOffset, (int*)endOffset);
        *textAttributes = QStringToBSTR(attrs);
        return S_OK;
    }
    return E_FAIL;
}

HRESULT STDMETHODCALLTYPE QWindowsIA2Accessible::get_caretOffset(long *offset)
{
    QAccessibleInterface *accessible = accessibleInterface();
    accessibleDebugClientCalls(accessible);
    if (QAccessibleTextInterface *text = textInterface()) {
        *offset = text->cursorPosition();
        return S_OK;
    }
    return E_FAIL;
}


HRESULT STDMETHODCALLTYPE QWindowsIA2Accessible::get_characterExtents(long offset,
                                                                   enum IA2CoordinateType coordType,
                                                                   long *x,
                                                                   long *y,
                                                                   long *width,
                                                                   long *height)
{
    QAccessibleInterface *accessible = accessibleInterface();
    accessibleDebugClientCalls(accessible);
    if (QAccessibleTextInterface *text = textInterface()) {
        QRect rect = text->characterRect(offset);
        mapFromScreenPos(coordType, rect.topLeft(), x, y);
        *width = rect.width();
        *height = rect.height();
        return S_OK;
    }
    return E_FAIL;
}

HRESULT STDMETHODCALLTYPE QWindowsIA2Accessible::get_nSelections(long *nSelections)
{
    QAccessibleInterface *accessible = accessibleInterface();
    accessibleDebugClientCalls(accessible);
    if (QAccessibleTextInterface *text = textInterface()) {
        *nSelections = text->selectionCount();
        return S_OK;
    }
    return E_FAIL;
}

HRESULT STDMETHODCALLTYPE QWindowsIA2Accessible::get_offsetAtPoint(long x,
                                                                long y,
                                                                enum IA2CoordinateType coordType,
                                                                long *offset)
{
    QAccessibleInterface *accessible = accessibleInterface();
    accessibleDebugClientCalls(accessible);
    if (QAccessibleTextInterface *text = textInterface()) {
        QPoint screenPos = mapToScreenPos(coordType, x, y);
        *offset = text->offsetAtPoint(screenPos);
        return (*offset >=0 ? S_OK : S_FALSE);
    }
    return E_FAIL;

}

HRESULT STDMETHODCALLTYPE QWindowsIA2Accessible::get_selection(long selectionIndex,
                                                            long *startOffset,
                                                            long *endOffset)
{
    QAccessibleInterface *accessible = accessibleInterface();
    accessibleDebugClientCalls(accessible);
    if (QAccessibleTextInterface *text = textInterface()) {
        text->selection(selectionIndex, (int*)startOffset, (int*)endOffset);
        return S_OK;
    }
    return E_FAIL;
}

HRESULT STDMETHODCALLTYPE QWindowsIA2Accessible::get_text(long startOffset,
                                                       long endOffset,
                                                       BSTR *text)
{
    QAccessibleInterface *accessible = accessibleInterface();
    accessibleDebugClientCalls(accessible);
    if (QAccessibleTextInterface *textif = textInterface()) {
        const QString t = textif->text(startOffset, endOffset);
        if (!t.isEmpty()) {
            *text = QStringToBSTR(t);
            return S_OK;
        }
        return E_INVALIDARG;
    }
    return E_FAIL;
}

HRESULT STDMETHODCALLTYPE QWindowsIA2Accessible::get_textBeforeOffset(long offset,
                                               enum IA2TextBoundaryType boundaryType,
                                               long *startOffset,
                                               long *endOffset,
                                               BSTR *text)
{
    QAccessibleInterface *accessible = accessibleInterface();
    accessibleDebugClientCalls(accessible);
    if (QAccessibleTextInterface *textIface = textInterface()) {
        const QString txt = textIface->textBeforeOffset(offset, (QAccessible::TextBoundaryType)boundaryType, (int*)startOffset, (int*)endOffset);
        if (!txt.isEmpty()) {
            *text = QStringToBSTR(txt);
            return S_OK;
        }
        return S_FALSE;
    }
    return E_FAIL;
}

HRESULT STDMETHODCALLTYPE QWindowsIA2Accessible::get_textAfterOffset(
    long offset,
    enum IA2TextBoundaryType boundaryType,
    long *startOffset,
    long *endOffset,
    BSTR *text)
{
    QAccessibleInterface *accessible = accessibleInterface();
    accessibleDebugClientCalls(accessible);
    if (QAccessibleTextInterface *textIface = textInterface()) {
        const QString txt = textIface->textAfterOffset(offset, (QAccessible::TextBoundaryType)boundaryType, (int*)startOffset, (int*)endOffset);
        if (!txt.isEmpty()) {
            *text = QStringToBSTR(txt);
            return S_OK;
        }
        return S_FALSE;
    }
    return E_FAIL;
}

HRESULT STDMETHODCALLTYPE QWindowsIA2Accessible::get_textAtOffset(long offset,
                                                               enum IA2TextBoundaryType boundaryType,
                                                               long *startOffset,
                                                               long *endOffset,
                                                               BSTR *text)
{
    QAccessibleInterface *accessible = accessibleInterface();
    accessibleDebugClientCalls(accessible);
    if (QAccessibleTextInterface *textIface = textInterface()) {
        const QString txt = textIface->textAtOffset(offset, (QAccessible::TextBoundaryType)boundaryType, (int*)startOffset, (int*)endOffset);
        if (!txt.isEmpty()) {
            *text = QStringToBSTR(txt);
            return S_OK;
        }
        return S_FALSE;
    }
    return E_FAIL;
}

HRESULT STDMETHODCALLTYPE QWindowsIA2Accessible::removeSelection(long selectionIndex)
{
    QAccessibleInterface *accessible = accessibleInterface();
    accessibleDebugClientCalls(accessible);
    if (QAccessibleTextInterface *textIface = textInterface()) {
        textIface->removeSelection(selectionIndex);
        return S_OK;
    }
    return E_FAIL;
}

HRESULT STDMETHODCALLTYPE QWindowsIA2Accessible::setCaretOffset(long offset)
{
    QAccessibleInterface *accessible = accessibleInterface();
    accessibleDebugClientCalls(accessible);
    if (QAccessibleTextInterface *textIface = textInterface()) {
        textIface->setCursorPosition(offset);
        return S_OK;
    }
    return E_FAIL;
}

HRESULT STDMETHODCALLTYPE QWindowsIA2Accessible::setSelection(long selectionIndex,
                                                           long startOffset,
                                                           long endOffset)
{
    QAccessibleInterface *accessible = accessibleInterface();
    accessibleDebugClientCalls(accessible);
    if (QAccessibleTextInterface *textIface = textInterface()) {
        textIface->setSelection(selectionIndex, startOffset, endOffset);
        return S_OK;
    }
    return E_FAIL;
}

HRESULT STDMETHODCALLTYPE QWindowsIA2Accessible::get_nCharacters(long *nCharacters)
{
    QAccessibleInterface *accessible = accessibleInterface();
    accessibleDebugClientCalls(accessible);
    if (QAccessibleTextInterface *textIface = textInterface()) {
        *nCharacters = textIface->characterCount();
        return S_OK;
    }
    return E_FAIL;
}

HRESULT STDMETHODCALLTYPE QWindowsIA2Accessible::scrollSubstringTo(long startIndex,
                                                                long endIndex,
                                                                enum IA2ScrollType scrollType)
{
    QAccessibleInterface *accessible = accessibleInterface();
    accessibleDebugClientCalls(accessible);
    if (QAccessibleTextInterface *textIface = textInterface()) {
        Q_UNUSED(scrollType);   //###
        textIface->scrollToSubstring(startIndex, endIndex);
        return S_OK;
    }
    return E_FAIL;
}

HRESULT STDMETHODCALLTYPE QWindowsIA2Accessible::scrollSubstringToPoint(long startIndex,
                                                                     long endIndex,
                                                                     enum IA2CoordinateType coordinateType,
                                                                     long x,
                                                                     long y)
{
    Q_UNUSED(startIndex);
    Q_UNUSED(endIndex);
    Q_UNUSED(coordinateType);
    Q_UNUSED(x);
    Q_UNUSED(y);

    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE QWindowsIA2Accessible::get_newText(IA2TextSegment *newText)
{
    Q_UNUSED(newText);
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE QWindowsIA2Accessible::get_oldText(IA2TextSegment *oldText)
{
    Q_UNUSED(oldText);
    return E_NOTIMPL;
}

/**************************************************************\
 *                         IAccessibleValue                    *
 **************************************************************/
HRESULT STDMETHODCALLTYPE QWindowsIA2Accessible::get_currentValue(VARIANT *currentValue)
{
    QAccessibleInterface *accessible = accessibleInterface();
    accessibleDebugClientCalls(accessible);
    if (!accessible)
        return E_FAIL;
    if (QAccessibleValueInterface *valueIface = valueInterface()) {
        const QVariant var = valueIface->currentValue();
        if (QVariant2VARIANT(var, *currentValue, QByteArray(), false))
            return S_OK;

    }
    currentValue->vt = VT_EMPTY;
    return S_FALSE;
}

HRESULT STDMETHODCALLTYPE QWindowsIA2Accessible::setCurrentValue(VARIANT value)
{
    QAccessibleInterface *accessible = accessibleInterface();
    accessibleDebugClientCalls(accessible);
    if (!accessible)
        return E_FAIL;
    HRESULT hr = S_FALSE;
    if (QAccessibleValueInterface *valueIface = valueInterface()) {
        hr = VariantChangeType(&value, &value, 0, VT_R8);
        if (SUCCEEDED(hr)) {
            // ### works only for numbers (not date, strings, etc)
            valueIface->setCurrentValue(QVariant(value.dblVal));
        }
    }
    return hr;
}

HRESULT STDMETHODCALLTYPE QWindowsIA2Accessible::get_maximumValue(VARIANT *maximumValue)
{
    QAccessibleInterface *accessible = accessibleInterface();
    accessibleDebugClientCalls(accessible);
    if (!accessible)
        return E_FAIL;
    if (QAccessibleValueInterface *valueIface = valueInterface()) {
        const QVariant var = valueIface->maximumValue();
        if (QVariant2VARIANT(var, *maximumValue, QByteArray(), false))
            return S_OK;
    }
    maximumValue->vt = VT_EMPTY;
    return S_FALSE;
}

HRESULT STDMETHODCALLTYPE QWindowsIA2Accessible::get_minimumValue(VARIANT *minimumValue)
{
    QAccessibleInterface *accessible = accessibleInterface();
    accessibleDebugClientCalls(accessible);
    if (!accessible)
        return E_FAIL;
    if (QAccessibleValueInterface *valueIface = valueInterface()) {
        const QVariant var = valueIface->minimumValue();
        if (QVariant2VARIANT(var, *minimumValue, QByteArray(), false))
            return S_OK;
    }
    minimumValue->vt = VT_EMPTY;
    return S_FALSE;
}


/**************************************************************\
 *                      IServiceProvider                       *
 **************************************************************/
/*!
  \internal
  Reimplemented from IServiceProvider
*/
HRESULT STDMETHODCALLTYPE QWindowsIA2Accessible::QueryService(REFGUID guidService, REFIID riid, void **iface)
{
    if (!iface)
        return E_POINTER;
    Q_UNUSED(guidService);
    *iface = 0;
    qCDebug(lcQpaAccessibility) << "QWindowsIA2Accessible::QS(): " << IIDToString(riid);


    if (guidService == IID_IAccessible) {
        if (riid == IID_IServiceProvider) {
            // do not end up calling QueryInterface for IID_IServiceProvider
            *iface = 0;
        } else if (riid == IID_IAccessible || riid == IID_IUnknown || riid == IID_IDispatch) {
            // The above conditions works with AccProbe and NVDA.
            *iface = static_cast<IAccessible*>(this);
        } else {
            // According to _dicoveringInterfaces Discovery of Interfaces, we should really only
            // enter here if riid == IID_IAccessible2, but some screen readers does not like that,
            // and other servers seems to have realized that. (Chrome and Mozilla for instance,
            // calls QueryInterface more or less in the same way)

            // For instance, accProbe discovers IID_IAccessibleTable2 by a QueryService only.
            return QueryInterface(riid, iface);
        }
    }

    if (riid == IID_IAccessibleApplication) {
        *iface = new AccessibleApplication;
        return S_OK;
    }
    if (*iface) {
        AddRef();
        return S_OK;
    }

    return E_NOINTERFACE;
}


/*!
  \internal
  private function..
  \a maxRelations max number of relations to return in \a relations
  \a relations the array of relations matching
  \a startIndex Index to start to return from,
                   it will return only that specific relation in \a relations

  If \a relations is null, \a startIndex and \a maxRelations are ignored, causing
  it to return the number of relations in \a nRelations
*/
HRESULT QWindowsIA2Accessible::getRelationsHelper(IAccessibleRelation **relations, int startIndex, long maxRelations, long *nRelations /* = 0*/)
{
    QAccessibleInterface *accessible = accessibleInterface();
    if (nRelations)
        *nRelations = 0;
    typedef QPair<QAccessibleInterface *, QAccessible::Relation> RelationEntry;
    QVector<RelationEntry> rels = accessible->relations();
    QMap<QAccessible::Relation, QAccessibleInterface *> relationMap;
    for (QVector<RelationEntry>::const_iterator it = rels.constBegin(); it != rels.constEnd(); ++it)
    {
        RelationEntry e = *it;
        relationMap.insertMulti(e.second, e.first);
    }

    QList<QAccessible::Relation> keys = relationMap.keys();
    const int numRelations = keys.count();
    if (relations) {
        for (int i = startIndex; i < qMin(startIndex + (int)maxRelations, numRelations); ++i) {
            QAccessible::Relation relation = keys.at(i);
            QList<QAccessibleInterface*> targets = relationMap.values(relation);
            AccessibleRelation *rel = new AccessibleRelation(targets, relation);
            *relations = rel;
            ++relations;
        }
    }
    if (nRelations)
        *nRelations = numRelations;

    return numRelations > 0 ? S_OK : S_FALSE;
}




/*!
  \internal
  helper to wrap a QList<QAccessibleInterface*> inside an array of IAccessible*
  The IAccessible* array is returned as a IUnknown*
*/
HRESULT QWindowsIA2Accessible::wrapListOfCells(const QList<QAccessibleInterface*> &inputCells, IUnknown ***outputAccessibles, long *nCellCount)
{
    const int count = inputCells.count();
    // Server allocates array
    IUnknown **outputCells = count ? (IUnknown**)::CoTaskMemAlloc(sizeof(IUnknown*) * count ) : (IUnknown**)0;
    for (int i = 0; i < count; ++i)
        outputCells[i] = QWindowsAccessibility::wrap(inputCells.at(i));

    *outputAccessibles = outputCells;
    *nCellCount = count;
    return count > 0 ? S_OK : S_FALSE;
}

#define IF_EQUAL_RETURN_IIDSTRING(id, iid) if (id == iid) return QByteArray(#iid)

QByteArray QWindowsIA2Accessible::IIDToString(REFIID id)
{
    QByteArray strGuid = QWindowsMsaaAccessible::IIDToString(id);
    if (!strGuid.isEmpty())
        return strGuid;

    IF_EQUAL_RETURN_IIDSTRING(id, IID_IUnknown);
    IF_EQUAL_RETURN_IIDSTRING(id, IID_IDispatch);
    IF_EQUAL_RETURN_IIDSTRING(id, IID_IAccessible);
    IF_EQUAL_RETURN_IIDSTRING(id, IID_IOleWindow);
    IF_EQUAL_RETURN_IIDSTRING(id, IID_IServiceProvider);
    IF_EQUAL_RETURN_IIDSTRING(id, IID_IAccessible2);
    IF_EQUAL_RETURN_IIDSTRING(id, IID_IAccessibleAction);
    IF_EQUAL_RETURN_IIDSTRING(id, IID_IAccessibleApplication);
    IF_EQUAL_RETURN_IIDSTRING(id, IID_IAccessibleComponent);
    IF_EQUAL_RETURN_IIDSTRING(id, IID_IAccessibleEditableText);
    IF_EQUAL_RETURN_IIDSTRING(id, IID_IAccessibleHyperlink);
    IF_EQUAL_RETURN_IIDSTRING(id, IID_IAccessibleHypertext);
    IF_EQUAL_RETURN_IIDSTRING(id, IID_IAccessibleImage);
    IF_EQUAL_RETURN_IIDSTRING(id, IID_IAccessibleRelation);
    IF_EQUAL_RETURN_IIDSTRING(id, IID_IAccessibleTable);
    IF_EQUAL_RETURN_IIDSTRING(id, IID_IAccessibleTable2);
    IF_EQUAL_RETURN_IIDSTRING(id, IID_IAccessibleTableCell);
    IF_EQUAL_RETURN_IIDSTRING(id, IID_IAccessibleText);
    IF_EQUAL_RETURN_IIDSTRING(id, IID_IAccessibleValue);

    // else...
#if 0   // Can be useful for debugging, but normally we'd like to reduce the noise a bit...
    OLECHAR szGuid[39]={0};
    ::StringFromGUID2(id, szGuid, 39);
    strGuid.reserve(40);
    ::WideCharToMultiByte(CP_UTF8, 0, szGuid, 39, strGuid.data(), 39, NULL, NULL);
    strGuid[38] = '\0';
#endif
    return strGuid;
}


QT_END_NAMESPACE

#endif //QT_NO_ACCESSIBILITY
