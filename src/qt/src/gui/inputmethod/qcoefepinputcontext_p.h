/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the QtGui module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QCOEFEPINPUTCONTEXT_P_H
#define QCOEFEPINPUTCONTEXT_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#ifndef QT_NO_IM

#include "qinputcontext.h"
#include <qhash.h>
#include <qtimer.h>
#include <private/qcore_symbian_p.h>
#include <private/qt_s60_p.h>

#include <fepbase.h>
#include <aknedsts.h>
#include <eikccpu.h>

QT_BEGIN_NAMESPACE

class QCoeFepInputMaskHandler
{
public:
    QCoeFepInputMaskHandler(const QString &mask);
    ~QCoeFepInputMaskHandler();
    bool canPasteClipboard(const QString &text);
private:
    bool isValidInput(QChar key, QChar mask) const;
private:
    struct MaskInputData {
        enum Casemode { NoCaseMode, Upper, Lower };
        QChar maskChar;
        bool separator;
        Casemode caseMode;
    };
    int m_maxLength;
    QChar m_blank;
    MaskInputData *m_maskData;
};

class Q_AUTOTEST_EXPORT QCoeFepInputContext : public QInputContext,
                                              public MCoeFepAwareTextEditor,
                                              public MCoeFepAwareTextEditor_Extension1,
                                              public MObjectProvider,
                                              public MEikCcpuEditor

{
    Q_OBJECT

public:
    QCoeFepInputContext(QObject *parent = 0);
    ~QCoeFepInputContext();

    QString identifierName() { return QLatin1String("coefep"); }
    QString language();

    void reset();
    void update();

    bool filterEvent(const QEvent *event);
    bool symbianFilterEvent(QWidget *keyWidget, const QSymbianEvent *event);
    void mouseHandler( int x, QMouseEvent *event);
    bool isComposing() const { return !m_preeditString.isEmpty(); }

    void setFocusWidget(QWidget * w);
    void widgetDestroyed(QWidget *w);

    TCoeInputCapabilities inputCapabilities();

    void resetSplitViewWidget(bool keepInputWidget = false);
    void ensureFocusWidgetVisible(QWidget *widget);

protected:
    void timerEvent(QTimerEvent *timerEvent);

private:
    void commitCurrentString(bool cancelFepTransaction);
    void updateHints(bool mustUpdateInputCapabilities);
    void applyHints(Qt::InputMethodHints hints);
    void applyFormat(QList<QInputMethodEvent::Attribute> *attributes);
    void queueInputCapabilitiesChanged();
    bool needsInputPanel();
    void commitTemporaryPreeditString();
    bool isWidgetVisible(QWidget *widget, int offset = 0);
    bool isPartialKeyboardSupported();

private Q_SLOTS:
    void ensureInputCapabilitiesChanged();
    void translateInputWidget();
    void ensureWidgetVisibility();

    // From MCoeFepAwareTextEditor
public:
    void StartFepInlineEditL(const TDesC& aInitialInlineText, TInt aPositionOfInsertionPointInInlineText,
            TBool aCursorVisibility, const MFormCustomDraw* aCustomDraw,
            MFepInlineTextFormatRetriever& aInlineTextFormatRetriever,
            MFepPointerEventHandlerDuringInlineEdit& aPointerEventHandlerDuringInlineEdit);
    void UpdateFepInlineTextL(const TDesC& aNewInlineText, TInt aPositionOfInsertionPointInInlineText);
    void SetInlineEditingCursorVisibilityL(TBool aCursorVisibility);
    void CancelFepInlineEdit();
    TInt DocumentLengthForFep() const;
    TInt DocumentMaximumLengthForFep() const;
    void SetCursorSelectionForFepL(const TCursorSelection& aCursorSelection);
    void GetCursorSelectionForFep(TCursorSelection& aCursorSelection) const;
    void GetEditorContentForFep(TDes& aEditorContent, TInt aDocumentPosition, TInt aLengthToRetrieve) const;
    void GetFormatForFep(TCharFormat& aFormat, TInt aDocumentPosition) const;
    void GetScreenCoordinatesForFepL(TPoint& aLeftSideOfBaseLine, TInt& aHeight, TInt& aAscent,
            TInt aDocumentPosition) const;
private:
    void DoCommitFepInlineEditL();
    MCoeFepAwareTextEditor_Extension1* Extension1(TBool& aSetToTrue);
    void ReportAknEdStateEvent(MAknEdStateObserver::EAknEdwinStateEvent aEventType);
    void enableSymbianCcpuSupport();
    void changeCBA(bool showCopyAndOrPaste);
    void copyOrCutTextToClipboard(const char *operation);
    void getScreenCoordinatesForFepX(TPoint& aLeftSideOfBaseLine, TInt& aHeight, TInt& aAscent,
            TInt aDocumentPosition) const;

    //From MEikCcpuEditor interface
public:
    TBool CcpuIsFocused() const;
    TBool CcpuCanCut() const;
    void CcpuCutL();
    TBool CcpuCanCopy() const;
    void CcpuCopyL();
    TBool CcpuCanPaste() const;
    void CcpuPasteL();
    TBool CcpuCanUndo() const;
    void CcpuUndoL();

private slots:
    void copy();
    void paste();

    // From MCoeFepAwareTextEditor_Extension1
public:
    void SetStateTransferingOwnershipL(MCoeFepAwareTextEditor_Extension1::CState* aState, TUid aTypeSafetyUid);
    MCoeFepAwareTextEditor_Extension1::CState* State(TUid aTypeSafetyUid);

    // From MObjectProvider
public:
    TTypeUid::Ptr MopSupplyObject(TTypeUid id);
    MObjectProvider *MopNext();

private:
    QSymbianControl *m_parent;
    CAknEdwinState *m_fepState;
    QString m_preeditString;
    Qt::InputMethodHints m_lastImHints;
    TUint m_textCapabilities;
    bool m_inDestruction;
    bool m_pendingInputCapabilitiesChanged;
    bool m_pendingTransactionCancel;
    int m_cursorVisibility;
    int m_inlinePosition;
    MFepInlineTextFormatRetriever *m_formatRetriever;
    MFepPointerEventHandlerDuringInlineEdit *m_pointerHandler;
    QBasicTimer m_tempPreeditStringTimeout;
    bool m_hasTempPreeditString;
    QString m_cachedPreeditString;
    int m_cachedCursorAndAnchorPosition;

    int m_splitViewResizeBy;
    Qt::WindowStates m_splitViewPreviousWindowStates;
    QRectF m_transformation;
    QGraphicsItem *m_splitViewPreviousFocusItem; //can't use QPointer<> since QGraphicsItem is not a QObject.

    CAknCcpuSupport *m_ccpu;
    QAction *m_copyAction;
    QAction *m_pasteAction;
    QPointer<QWidget> m_lastFocusedEditor;
    QPointer<QObject> m_lastFocusedObject;

    friend class tst_QInputContext;
};

Q_GUI_EXPORT void qt_s60_setPartialScreenInputMode(bool enable);
Q_GUI_EXPORT void qt_s60_setPartialScreenAutomaticTranslation(bool enable);
Q_GUI_EXPORT void qt_s60_setEditorFlags(int flags);

QT_END_NAMESPACE

#endif // QT_NO_IM

#endif // QCOEFEPINPUTCONTEXT_P_H
