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
#ifndef IACCESSIBLE2_H
#define IACCESSIBLE2_H

#include <QtCore/QtConfig>
#ifndef QT_NO_ACCESSIBILITY

#include "qwindowsmsaaaccessible.h"
#include "comutils.h"

#include "Accessible2.h"
#include "AccessibleAction.h"
#include "AccessibleApplication.h"
#include "AccessibleComponent.h"
#include "AccessibleEditableText.h"
#include "AccessibleHyperlink.h"
#include "AccessibleHypertext.h"
#include "AccessibleImage.h"
#include "AccessibleRelation.h"
#include "AccessibleTable.h"
#include "AccessibleTable2.h"
#include "AccessibleTableCell.h"
#include "AccessibleText.h"
#include "AccessibleValue.h"

#include "AccessibleEventID.h"
#include "AccessibleRole.h"
#include "AccessibleStates.h"

#include <servprov.h>

QT_BEGIN_NAMESPACE

class QWindowsIA2Accessible : public QWindowsMsaaAccessible,
        public IAccessibleAction,
        public IAccessibleComponent,
        public IAccessibleEditableText,
        public IAccessibleTable2,
        public IAccessibleTableCell,
        public IAccessibleText,
        public IAccessibleValue,
        public IServiceProvider
{
public:
    QWindowsIA2Accessible(QAccessibleInterface *a) : QWindowsMsaaAccessible(a) {}

    /* IUnknown */
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID, LPVOID *);
    ULONG STDMETHODCALLTYPE AddRef();
    ULONG STDMETHODCALLTYPE Release();

    /* IAccessible2 */
    HRESULT STDMETHODCALLTYPE get_nRelations(long *nRelations);
    HRESULT STDMETHODCALLTYPE get_relation(long relationIndex, IAccessibleRelation **relation);
    HRESULT STDMETHODCALLTYPE get_relations(long maxRelations, IAccessibleRelation **relations, long *nRelations);
    HRESULT STDMETHODCALLTYPE role(long *role);
    HRESULT STDMETHODCALLTYPE scrollTo(enum IA2ScrollType scrollType);
    HRESULT STDMETHODCALLTYPE scrollToPoint(enum IA2CoordinateType coordinateType, long x, long y);
    HRESULT STDMETHODCALLTYPE get_groupPosition(long *groupLevel, long *similarItemsInGroup, long *positionInGroup);
    HRESULT STDMETHODCALLTYPE get_states(AccessibleStates *states);
    HRESULT STDMETHODCALLTYPE get_extendedRole(BSTR *extendedRole);
    HRESULT STDMETHODCALLTYPE get_localizedExtendedRole(BSTR *localizedExtendedRole);
    HRESULT STDMETHODCALLTYPE get_nExtendedStates(long *nExtendedStates);
    HRESULT STDMETHODCALLTYPE get_extendedStates(long maxExtendedStates, BSTR **extendedStates, long *nExtendedStates);
    HRESULT STDMETHODCALLTYPE get_localizedExtendedStates(long maxLocalizedExtendedStates, BSTR **localizedExtendedStates, long *nLocalizedExtendedStates);
    HRESULT STDMETHODCALLTYPE get_uniqueID(long *uniqueID);
    HRESULT STDMETHODCALLTYPE get_windowHandle(HWND *windowHandle);
    HRESULT STDMETHODCALLTYPE get_indexInParent(long *indexInParent);
    HRESULT STDMETHODCALLTYPE get_locale(IA2Locale *locale);
    HRESULT STDMETHODCALLTYPE get_attributes(BSTR *attributes);

    /* IAccessibleAction */
    HRESULT STDMETHODCALLTYPE nActions(long *nActions);
    HRESULT STDMETHODCALLTYPE doAction(long actionIndex);
    HRESULT STDMETHODCALLTYPE get_description(long actionIndex, BSTR *description);
    HRESULT STDMETHODCALLTYPE get_keyBinding(long actionIndex, long nMaxBindings, BSTR **keyBindings, long *nBindings);
    HRESULT STDMETHODCALLTYPE get_name(long actionIndex, BSTR *name);
    HRESULT STDMETHODCALLTYPE get_localizedName(long actionIndex, BSTR *localizedName);

    /* IAccessibleComponent */
    HRESULT STDMETHODCALLTYPE get_locationInParent(long *x,long *y);
    HRESULT STDMETHODCALLTYPE get_foreground(IA2Color *foreground);
    HRESULT STDMETHODCALLTYPE get_background(IA2Color *background);

    /* IAccessibleEditableText */
    HRESULT STDMETHODCALLTYPE copyText(long startOffset, long endOffset);
    HRESULT STDMETHODCALLTYPE deleteText(long startOffset, long endOffset);
    HRESULT STDMETHODCALLTYPE insertText(long offset, BSTR *text);
    HRESULT STDMETHODCALLTYPE cutText(long startOffset, long endOffset);
    HRESULT STDMETHODCALLTYPE pasteText(long offset);
    HRESULT STDMETHODCALLTYPE replaceText(long startOffset, long endOffset, BSTR *text);
    HRESULT STDMETHODCALLTYPE setAttributes(long startOffset, long endOffset, BSTR *attributes);

    /* IAccessibleTable2 */
    HRESULT STDMETHODCALLTYPE get_cellAt( long row, long column, IUnknown **cell);
    HRESULT STDMETHODCALLTYPE get_caption( IUnknown **accessibleInterface);
    HRESULT STDMETHODCALLTYPE get_columnDescription( long column, BSTR *description);
    HRESULT STDMETHODCALLTYPE get_nColumns( long *columnCount);
    HRESULT STDMETHODCALLTYPE get_nRows( long *rowCount);
    HRESULT STDMETHODCALLTYPE get_nSelectedCells( long *cellCount);
    HRESULT STDMETHODCALLTYPE get_nSelectedColumns( long *columnCount);
    HRESULT STDMETHODCALLTYPE get_nSelectedRows( long *rowCount);
    HRESULT STDMETHODCALLTYPE get_rowDescription( long row, BSTR *description);
    HRESULT STDMETHODCALLTYPE get_selectedCells( IUnknown ***cells, long *nSelectedCells);
    HRESULT STDMETHODCALLTYPE get_selectedColumns( long **selectedColumns, long *nColumns);
    HRESULT STDMETHODCALLTYPE get_selectedRows( long **selectedRows, long *nRows);
    HRESULT STDMETHODCALLTYPE get_summary( IUnknown **accessibleInterface);
    HRESULT STDMETHODCALLTYPE get_isColumnSelected( long column, boolean *isSelected);
    HRESULT STDMETHODCALLTYPE get_isRowSelected( long row, boolean *isSelected);
    HRESULT STDMETHODCALLTYPE selectRow( long row);
    HRESULT STDMETHODCALLTYPE selectColumn( long column);
    HRESULT STDMETHODCALLTYPE unselectRow( long row);
    HRESULT STDMETHODCALLTYPE unselectColumn( long column);
    HRESULT STDMETHODCALLTYPE get_modelChange( IA2TableModelChange *modelChange);

    /* IAccessibleTableCell */
    HRESULT STDMETHODCALLTYPE get_columnExtent(long *nColumnsSpanned);
    HRESULT STDMETHODCALLTYPE get_columnHeaderCells(IUnknown ***cellAccessibles, long *nColumnHeaderCells);
    HRESULT STDMETHODCALLTYPE get_columnIndex(long *columnIndex);
    HRESULT STDMETHODCALLTYPE get_rowExtent(long *nRowsSpanned);
    HRESULT STDMETHODCALLTYPE get_rowHeaderCells(IUnknown ***cellAccessibles, long *nRowHeaderCells);
    HRESULT STDMETHODCALLTYPE get_rowIndex(long *rowIndex);
    HRESULT STDMETHODCALLTYPE get_isSelected( boolean *isSelected);
    HRESULT STDMETHODCALLTYPE get_rowColumnExtents(long *row, long *column,
                                                   long *rowExtents, long *columnExtents,
                                                   boolean *isSelected);
    HRESULT STDMETHODCALLTYPE get_table(IUnknown **table);


    /* IAccessibleText */
    HRESULT STDMETHODCALLTYPE addSelection(long startOffset, long endOffset);
    HRESULT STDMETHODCALLTYPE get_attributes(long offset, long *startOffset,
                                             long *endOffset, BSTR *textAttributes);
    HRESULT STDMETHODCALLTYPE get_caretOffset(long *offset);
    HRESULT STDMETHODCALLTYPE get_characterExtents(long offset, enum IA2CoordinateType coordType,
                                                   long *x, long *y,
                                                   long *width, long *height);
    HRESULT STDMETHODCALLTYPE get_nSelections(long *nSelections);
    HRESULT STDMETHODCALLTYPE get_offsetAtPoint(long x, long y, enum IA2CoordinateType coordType, long *offset);
    HRESULT STDMETHODCALLTYPE get_selection(long selectionIndex, long *startOffset, long *endOffset);
    HRESULT STDMETHODCALLTYPE get_text(long startOffset, long endOffset, BSTR *text);
    HRESULT STDMETHODCALLTYPE get_textBeforeOffset(long offset, enum IA2TextBoundaryType boundaryType,
                                                   long *startOffset, long *endOffset, BSTR *text);
    HRESULT STDMETHODCALLTYPE get_textAfterOffset(long offset, enum IA2TextBoundaryType boundaryType,
                                                  long *startOffset, long *endOffset, BSTR *text);
    HRESULT STDMETHODCALLTYPE get_textAtOffset(long offset, enum IA2TextBoundaryType boundaryType,
                                               long *startOffset, long *endOffset, BSTR *text);
    HRESULT STDMETHODCALLTYPE removeSelection(long selectionIndex);
    HRESULT STDMETHODCALLTYPE setCaretOffset(long offset);
    HRESULT STDMETHODCALLTYPE setSelection(long selectionIndex, long startOffset, long endOffset);
    HRESULT STDMETHODCALLTYPE get_nCharacters(long *nCharacters);
    HRESULT STDMETHODCALLTYPE scrollSubstringTo(long startIndex, long endIndex, enum IA2ScrollType scrollType);
    HRESULT STDMETHODCALLTYPE scrollSubstringToPoint(long startIndex, long endIndex,
                                                     enum IA2CoordinateType coordinateType, long x, long y);
    HRESULT STDMETHODCALLTYPE get_newText(IA2TextSegment *newText);
    HRESULT STDMETHODCALLTYPE get_oldText(IA2TextSegment *oldText);

    /* IAccessibleValue */
    HRESULT STDMETHODCALLTYPE get_currentValue(VARIANT *currentValue);
    HRESULT STDMETHODCALLTYPE setCurrentValue(VARIANT value);
    HRESULT STDMETHODCALLTYPE get_maximumValue(VARIANT *maximumValue);
    HRESULT STDMETHODCALLTYPE get_minimumValue(VARIANT *minimumValue);

    /* IServiceProvider */
    HRESULT STDMETHODCALLTYPE QueryService(REFGUID guidService, REFIID riid, void **ppv);

    /* private helper functions */
private:
    inline QAccessibleTextInterface *textInterface() const {
        QAccessibleInterface *accessible = accessibleInterface();
        return accessible ? accessible->textInterface() : static_cast<QAccessibleTextInterface *>(0);
    }

    inline QAccessibleActionInterface *actionInterface() const {
        QAccessibleInterface *accessible = accessibleInterface();
        return accessible->actionInterface();
    }

    inline QAccessibleValueInterface *valueInterface() const {
        QAccessibleInterface *accessible = accessibleInterface();
        return accessible->valueInterface();
    }

    inline QAccessibleTableInterface *tableInterface() const {
        QAccessibleInterface *accessible = accessibleInterface();
        return accessible->tableInterface();
    }

    inline QAccessibleTableCellInterface *tableCellInterface() const {
        QAccessibleInterface *accessible = accessibleInterface();
        return accessible->tableCellInterface();
    }

    /*!
      \internal
      \a screenPos is in screen relative position
      \a x and \y (out) is in parent relative position if coordType == IA2_COORDTYPE_PARENT_RELATIVE
    */
    void mapFromScreenPos(enum IA2CoordinateType coordType, const QPoint &screenPos, long *x, long *y) const {
        QAccessibleInterface *accessible = accessibleInterface();
        if (coordType == IA2_COORDTYPE_PARENT_RELATIVE) {
            // caller wants relative to parent
            if (QAccessibleInterface *parent = accessible->parent()) {
                const QRect parentScreenRect = parent->rect();
                *x = parentScreenRect.x() - screenPos.x();
                *y = parentScreenRect.y() - screenPos.y();
                return;
            }
        }
        *x = screenPos.x();
        *y = screenPos.y();
    }

    /*!
      \internal
      \a x and \y is in parent relative position if coordType == IA2_COORDTYPE_PARENT_RELATIVE
      \return a screen relative position
    */
    QPoint mapToScreenPos(enum IA2CoordinateType coordType, long x, long y) const {
        QAccessibleInterface *accessible = accessibleInterface();
        if (coordType == IA2_COORDTYPE_PARENT_RELATIVE) {
            if (QAccessibleInterface *parent = accessible->parent()) {
                const QRect parentScreenRect = parent->rect();
                return QPoint(parentScreenRect.x() + x, parentScreenRect.y() + y);
            }
        }
        return QPoint(x,y);
    }

    HRESULT getRelationsHelper(IAccessibleRelation **relations, int startIndex, long maxRelations, long *nRelations = 0);
    HRESULT wrapListOfCells(const QList<QAccessibleInterface*> &inputCells, IUnknown ***outputAccessibles, long *nCellCount);
    QByteArray IIDToString(REFIID id);
    QString textForRange(int startOffset, int endOffset) const;
    void replaceTextFallback(long startOffset, long endOffset, const QString &txt);

};

/**************************************************************\
 *                     AccessibleApplication                  *
 **************************************************************/
class AccessibleApplication : public IAccessibleApplication
{
public:
    AccessibleApplication() : m_ref(1)
    {

    }

    virtual ~AccessibleApplication() {}

    /* IUnknown */
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID, LPVOID *);
    ULONG STDMETHODCALLTYPE AddRef();
    ULONG STDMETHODCALLTYPE Release();

    /* IAccessibleApplication */
    HRESULT STDMETHODCALLTYPE get_appName(/* [retval][out] */ BSTR *name);
    HRESULT STDMETHODCALLTYPE get_appVersion(/* [retval][out] */ BSTR *version);
    HRESULT STDMETHODCALLTYPE get_toolkitName(/* [retval][out] */ BSTR *name);
    HRESULT STDMETHODCALLTYPE get_toolkitVersion(/* [retval][out] */ BSTR *version);
private:
    ULONG m_ref;
};



/**************************************************************\
 *                     AccessibleRelation                      *
 **************************************************************/
class AccessibleRelation : public IAccessibleRelation
{
public:
    AccessibleRelation(const QList<QAccessibleInterface *> &targets,
                       QAccessible::Relation relation);

    virtual ~AccessibleRelation() {}

    /* IUnknown */
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID id, LPVOID *iface);
    ULONG STDMETHODCALLTYPE AddRef();
    ULONG STDMETHODCALLTYPE Release();

    /* IAccessibleRelation */
    HRESULT STDMETHODCALLTYPE get_relationType(BSTR *relationType);
    HRESULT STDMETHODCALLTYPE get_localizedRelationType(BSTR *localizedRelationType);
    HRESULT STDMETHODCALLTYPE get_nTargets(long *nTargets);
    HRESULT STDMETHODCALLTYPE get_target(long targetIndex, IUnknown **target);
    HRESULT STDMETHODCALLTYPE get_targets(long maxTargets, IUnknown **targets, long *nTargets);

private:
    static BSTR relationToBSTR(QAccessible::Relation relation)
    {
        wchar_t *constRelationString = 0;
        switch (relation) {
        case QAccessible::Label:
            constRelationString = IA2_RELATION_LABEL_FOR;
            break;
        case QAccessible::Labelled:
            constRelationString = IA2_RELATION_LABELLED_BY;
            break;
        case QAccessible::Controller:
            constRelationString = IA2_RELATION_CONTROLLER_FOR;
            break;
        case QAccessible::Controlled:
            constRelationString = IA2_RELATION_CONTROLLED_BY;
            break;
        case QAccessible::AllRelations:
            constRelationString = ( L"AllRelations" );
            break;
        }

        if (constRelationString) {
            BSTR bstrVal;
            const UINT wlen = (UINT)wcslen(constRelationString);
            bstrVal = ::SysAllocStringLen(constRelationString, wlen);
            return bstrVal;
        }
        return 0;
    }


    QList<QAccessibleInterface *> m_targets;
    QAccessible::Relation m_relation;
    ULONG m_ref;
};

QT_END_NAMESPACE

#endif //QT_NO_ACCESSIBILITY

#endif // IACCESSIBLE2_H
