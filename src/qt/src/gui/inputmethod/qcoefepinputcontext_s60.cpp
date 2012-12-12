/****************************************************************************
**
** Copyright (C) 2012 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtGui module of the Qt Toolkit.
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

#ifndef QT_NO_IM

#include "qcoefepinputcontext_p.h"
#include <qapplication.h>
#include <qtextformat.h>
#include <qgraphicsview.h>
#include <qgraphicsscene.h>
#include <qgraphicswidget.h>
#include <qsymbianevent.h>
#include <qlayout.h>
#include <qdesktopwidget.h>
#include <private/qcore_symbian_p.h>

#include <fepitfr.h>
#include <hal.h>
#include <e32property.h>

#include <limits.h>

#include <eikccpu.h>
#include <aknedsts.h>
#include <coeinput.h>
#include <w32std.h>
#include <akndiscreetpopup.h>
#include <aknextendedinputcapabilities.h>

#include <qtextedit.h>
#include <qplaintextedit.h>
#include <qlineedit.h>
#include <qclipboard.h>
#include <qvalidator.h>
#include <qgraphicsproxywidget.h>
#include <qgraphicsitem.h>

// You only find these enumerations on SDK 5 onwards, so we need to provide our own
// to remain compatible with older releases. They won't be called by pre-5.0 SDKs.

// MAknEdStateObserver::EAknCursorPositionChanged
#define QT_EAknCursorPositionChanged MAknEdStateObserver::EAknEdwinStateEvent(6)
// MAknEdStateObserver::EAknActivatePenInputRequest
#define QT_EAknActivatePenInputRequest MAknEdStateObserver::EAknEdwinStateEvent(7)
// MAknEdStateObserver::EAknClosePenInputRequest
#define QT_EAknClosePenInputRequest MAknEdStateObserver::EAknEdwinStateEvent(10)

// EAknEditorFlagSelectionVisible is only valid from 3.2 onwards.
// Sym^3 AVKON FEP manager expects that this flag is used for FEP-aware editors
// that support text selection.
#define QT_EAknEditorFlagSelectionVisible 0x100000

// EAknEditorFlagEnablePartialScreen is only valid from Sym^3 onwards.
#define QT_EAknEditorFlagEnablePartialScreen 0x200000

// Properties to detect VKB status from AknFepInternalPSKeys.h
#define QT_EPSUidAknFep 0x100056de
#define QT_EAknFepTouchInputActive 0x00000004

// For compatibility with older Symbian^3 environments, which do not have this define yet.
#ifndef R_AVKON_DISCREET_POPUP_TEXT_COPIED
#define R_AVKON_DISCREET_POPUP_TEXT_COPIED 0x8cc0227
#endif

_LIT(KAvkonResourceFile, "z:\\resource\\avkon.rsc" );

// Vietnamese tone tables
const int VietToneMarks = 5;

const QChar VietToneList[VietToneMarks] = {
        0x0301, 0x0300, 0x0309, 0x0303, 0x0323
};

const QChar VietVowelList[] = {
        0x0041, 0x0061, 0x0102, 0x0103, 0x00c2,
        0x00e2, 0x0045, 0x0065, 0x00ca, 0x00ea,
        0x0049, 0x0069, 0x004f, 0x006f, 0x00d4,
        0x00f4, 0x01a0, 0x01a1, 0x0055, 0x0075,
        0x01af, 0x01b0, 0x0059, 0x0079
};

const QChar VietToneMatrix[][VietToneMarks] = {
        // Matrix for each vowel (row) after applying certain tone mark (column)
        0x00c1, 0x00c0, 0x1ea2, 0x00c3, 0x1ea0,
        0x00e1, 0x00e0, 0x1ea3, 0x00e3, 0x1ea1,
        0x1eae, 0x1eb0, 0x1eb2, 0x1eb4, 0x1eb6,
        0x1eaf, 0x1eb1, 0x1eb3, 0x1eb5, 0x1eb7,
        0x1ea4, 0x1ea6, 0x1ea8, 0x1eaa, 0x1eac,
        0x1ea5, 0x1ea7, 0x1ea9, 0x1eab, 0x1ead,
        0x00c9, 0x00c8, 0x1eba, 0x1ebc, 0x1eb8,
        0x00e9, 0x00e8, 0x1ebb, 0x1ebd, 0x1eb9,
        0x1ebe, 0x1ec0, 0x1ec2, 0x1ec4, 0x1ec6,
        0x1ebf, 0x1ec1, 0x1ec3, 0x1ec5, 0x1ec7,
        0x00cd, 0x00cc, 0x1ec8, 0x0128, 0x1eca,
        0x00ed, 0x00ec, 0x1ec9, 0x0129, 0x1ecb,
        0x00d3, 0x00d2, 0x1ece, 0x00d5, 0x1ecc,
        0x00f3, 0x00f2, 0x1ecf, 0x00f5, 0x1ecd,
        0x1ed0, 0x1ed2, 0x1ed4, 0x1ed6, 0x1ed8,
        0x1ed1, 0x1ed3, 0x1ed5, 0x1ed7, 0x1ed9,
        0x1eda, 0x1edc, 0x1ede, 0x1ee0, 0x1ee2,
        0x1edb, 0x1edd, 0x1edf, 0x1ee1, 0x1ee3,
        0x00da, 0x00d9, 0x1ee6, 0x0168, 0x1ee4,
        0x00fa, 0x00f9, 0x1ee7, 0x0169, 0x1ee5,
        0x1ee8, 0x1eea, 0x1eec, 0x1eee, 0x1ef0,
        0x1ee9, 0x1eeb, 0x1eed, 0x1eef, 0x1ef1,
        0x00dd, 0x1ef2, 0x1ef6, 0x1ef8, 0x1ef4,
        0x00fd, 0x1ef3, 0x1ef7, 0x1ef9, 0x1ef5
};

QT_BEGIN_NAMESPACE

static QWidget* getFocusedChild(const QList<QObject*>& objectList)
{
    for (int j = 0; j < objectList.count(); j++) {
        if (QWidget* ow = qobject_cast<QWidget *>(objectList[j])) {
            if (ow->hasFocus()) {
                return ow;
            } else {
                if (QWidget* rw = getFocusedChild(ow->children()))
                    return rw;
            }
        }
    }
    return 0;
}

// A generic method for invoking "cut", "copy", and "paste" slots on editor
// All supported editors are expected to have these.
static bool ccpuInvokeSlot(QObject *obj, QObject *focusObject, const char *member)
{
    QObject *invokeTarget = obj;
    if (focusObject)
        invokeTarget = focusObject;

    return QMetaObject::invokeMethod(invokeTarget, member, Qt::DirectConnection);
}

// focusObject is used to return a pointer to focused graphics object, if any
static QWidget *getQWidgetFromQGraphicsView(QWidget *widget, QObject **focusObject = 0)
{
    if (focusObject)
        *focusObject = 0;

    if (!widget)
        return 0;

    if (QGraphicsView* qgv = qobject_cast<QGraphicsView *>(widget)) {
        QGraphicsItem *focusItem = 0;
        if (qgv->scene())
            focusItem = qgv->scene()->focusItem();
        if (focusItem) {
            if (focusObject)
                *focusObject = focusItem->toGraphicsObject();
            if (QGraphicsProxyWidget* const qgpw = qgraphicsitem_cast<QGraphicsProxyWidget* const>(focusItem)) {
                if (QWidget* w = qgpw->widget()) {
                    if (w->layout()) {
                        if (QWidget* rw = getFocusedChild(w->children()))
                            return rw;
                    } else {
                        return w;
                    }
                }
            }
        }
    }
    return widget;
}

QCoeFepInputMaskHandler::QCoeFepInputMaskHandler(const QString &mask)
{
    QString inputMask;
    int delimiter = mask.indexOf(QLatin1Char(';'));
    if (mask.isEmpty() || delimiter == 0)
        return;

    if (delimiter == -1) {
        m_blank = QLatin1Char(' ');
        inputMask = mask;
    } else {
        inputMask = mask.left(delimiter);
        m_blank = (delimiter + 1 < mask.length()) ? mask[delimiter + 1] : QLatin1Char(' ');
    }

    // Calculate m_maxLength / m_maskData length
    m_maxLength = 0;
    QChar c = 0;
    for (int i = 0; i < inputMask.length(); i++) {
        c = inputMask.at(i);
        if (i > 0 && inputMask.at(i - 1) == QLatin1Char('\\')) {
            m_maxLength++;
            continue;
        }
        if (c != QLatin1Char('\\') && c != QLatin1Char('!')
            && c != QLatin1Char('<') && c != QLatin1Char('>')
            && c != QLatin1Char('{') && c != QLatin1Char('}')
            && c != QLatin1Char('[') && c != QLatin1Char(']')) {
            m_maxLength++;
        }
    }

    m_maskData = new MaskInputData[m_maxLength];

    MaskInputData::Casemode m = MaskInputData::NoCaseMode;
    c = 0;
    bool s = false;
    bool escape = false;
    int index = 0;
    for (int i = 0; i < inputMask.length(); i++) {
        c = inputMask.at(i);
        if (escape) {
            s = true;
            m_maskData[index].maskChar = c;
            m_maskData[index].separator = s;
            m_maskData[index].caseMode = m;
            index++;
            escape = false;
        } else if (c == QLatin1Char('<')) {
            m = MaskInputData::Lower;
        } else if (c == QLatin1Char('>')) {
            m = MaskInputData::Upper;
        } else if (c == QLatin1Char('!')) {
            m = MaskInputData::NoCaseMode;
        } else if (c != QLatin1Char('{') && c != QLatin1Char('}') && c != QLatin1Char('[') && c != QLatin1Char(']')) {
            switch (c.unicode()) {
            case 'A':
            case 'a':
            case 'N':
            case 'n':
            case 'X':
            case 'x':
            case '9':
            case '0':
            case 'D':
            case 'd':
            case '#':
            case 'H':
            case 'h':
            case 'B':
            case 'b':
                s = false;
                break;
            case '\\':
                escape = true;
                break;
            default:
                s = true;
                break;
            }

            if (!escape) {
                m_maskData[index].maskChar = c;
                m_maskData[index].separator = s;
                m_maskData[index].caseMode = m;
                index++;
            }
        }
    }
}

QCoeFepInputMaskHandler::~QCoeFepInputMaskHandler()
{
    if (m_maskData)
        delete[] m_maskData;
}

bool QCoeFepInputMaskHandler::canPasteClipboard(const QString &text)
{
    if (!m_maskData)
        return true;

    if (text.length() > m_maxLength)
        return false;
    int limit = qMin(m_maxLength, text.length());
    for (int i = 0; i < limit; ++i) {
        if (m_maskData[i].separator) {
            if (text.at(i) != m_maskData[i].maskChar)
                return false;
        } else {
            if (!isValidInput(text.at(i), m_maskData[i].maskChar))
                return false;
        }
    }
    return true;
}

bool QCoeFepInputMaskHandler::isValidInput(QChar key, QChar mask) const
{
    switch (mask.unicode()) {
    case 'A':
        if (key.isLetter())
            return true;
        break;
    case 'a':
        if (key.isLetter() || key == m_blank)
            return true;
        break;
    case 'N':
        if (key.isLetterOrNumber())
            return true;
        break;
    case 'n':
        if (key.isLetterOrNumber() || key == m_blank)
            return true;
        break;
    case 'X':
        if (key.isPrint())
            return true;
        break;
    case 'x':
        if (key.isPrint() || key == m_blank)
            return true;
        break;
    case '9':
        if (key.isNumber())
            return true;
        break;
    case '0':
        if (key.isNumber() || key == m_blank)
            return true;
        break;
    case 'D':
        if (key.isNumber() && key.digitValue() > 0)
            return true;
        break;
    case 'd':
        if ((key.isNumber() && key.digitValue() > 0) || key == m_blank)
            return true;
        break;
    case '#':
        if (key.isNumber() || key == QLatin1Char('+') || key == QLatin1Char('-') || key == m_blank)
            return true;
        break;
    case 'B':
        if (key == QLatin1Char('0') || key == QLatin1Char('1'))
            return true;
        break;
    case 'b':
        if (key == QLatin1Char('0') || key == QLatin1Char('1') || key == m_blank)
            return true;
        break;
    case 'H':
        if (key.isNumber() || (key >= QLatin1Char('a') && key <= QLatin1Char('f')) || (key >= QLatin1Char('A') && key <= QLatin1Char('F')))
            return true;
        break;
    case 'h':
        if (key.isNumber() || (key >= QLatin1Char('a') && key <= QLatin1Char('f')) || (key >= QLatin1Char('A') && key <= QLatin1Char('F')) || key == m_blank)
            return true;
        break;
    default:
        break;
    }
    return false;
}

Q_GUI_EXPORT void qt_s60_setPartialScreenInputMode(bool enable)
{
    S60->partial_keyboard = enable;

    QApplication::setAttribute(Qt::AA_S60DisablePartialScreenInputMode, !S60->partial_keyboard);

    QInputContext *ic = 0;
    if (QApplication::focusWidget()) {
        ic = QApplication::focusWidget()->inputContext();
    } else if (qApp && qApp->inputContext()) {
        ic = qApp->inputContext();
    }
    if (ic)
        ic->update();
}

Q_GUI_EXPORT void qt_s60_setPartialScreenAutomaticTranslation(bool enable)
{
    S60->partial_keyboardAutoTranslation = enable;
}

Q_GUI_EXPORT void qt_s60_setEditorFlags(int flags)
{
    S60->editorFlags |= flags;
}

QCoeFepInputContext::QCoeFepInputContext(QObject *parent)
    : QInputContext(parent),
      m_fepState(q_check_ptr(new CAknEdwinState)),		// CBase derived object needs check on new
      m_lastImHints(Qt::ImhNone),
      m_textCapabilities(TCoeInputCapabilities::EAllText),
      m_inDestruction(false),
      m_pendingInputCapabilitiesChanged(false),
      m_pendingTransactionCancel(false),
      m_cursorVisibility(1),
      m_inlinePosition(0),
      m_formatRetriever(0),
      m_pointerHandler(0),
      m_hasTempPreeditString(false),
      m_cachedCursorAndAnchorPosition(-1),
      m_splitViewResizeBy(0),
      m_splitViewPreviousWindowStates(Qt::WindowNoState),
      m_splitViewPreviousFocusItem(0),
      m_ccpu(0),
      m_extendedInputCapabilities(0),
      m_formAccessor(0),
      m_dummyEditor(0)
{
    m_fepState->SetObjectProvider(this);
    int defaultFlags = EAknEditorFlagDefault;
    if (QSysInfo::s60Version() > QSysInfo::SV_S60_5_0) {
        if (isPartialKeyboardSupported()) {
            defaultFlags |= QT_EAknEditorFlagEnablePartialScreen;
        }
        defaultFlags |= QT_EAknEditorFlagSelectionVisible;
    }
    m_fepState->SetFlags(defaultFlags);
    m_fepState->SetDefaultInputMode( EAknEditorTextInputMode );
    m_fepState->SetPermittedInputModes( EAknEditorAllInputModes );
    m_fepState->SetDefaultCase( EAknEditorTextCase );
    m_fepState->SetPermittedCases( EAknEditorAllCaseModes );
    m_fepState->SetSpecialCharacterTableResourceId(R_AVKON_SPECIAL_CHARACTER_TABLE_DIALOG);
    m_fepState->SetNumericKeymap(EAknEditorAlphanumericNumberModeKeymap);
    enableSymbianCcpuSupport();

    //adding softkeys
    QString copyLabel = QLatin1String("Copy");
    QString pasteLabel = QLatin1String("Paste");
    TRAP_IGNORE(
        CEikonEnv* coe = CEikonEnv::Static();
        if (coe) {
            HBufC* copyBuf = coe->AllocReadResourceLC(R_TEXT_SOFTKEY_COPY);
            copyLabel = qt_TDesC2QString(*copyBuf);
            CleanupStack::PopAndDestroy(copyBuf);
            HBufC* pasteBuf = coe->AllocReadResourceLC(R_TEXT_SOFTKEY_PASTE);
            pasteLabel = qt_TDesC2QString(*pasteBuf);
            CleanupStack::PopAndDestroy(pasteBuf);
        }

        m_extendedInputCapabilities = CAknExtendedInputCapabilities::NewL();
    )

    m_copyAction = new QAction(copyLabel, QApplication::desktop());
    m_pasteAction = new QAction(pasteLabel, QApplication::desktop());
    m_copyAction->setSoftKeyRole(QAction::PositiveSoftKey);
    m_pasteAction->setSoftKeyRole(QAction::NegativeSoftKey);
    connect(m_copyAction, SIGNAL(triggered()), this, SLOT(copy()));
    connect(m_pasteAction, SIGNAL(triggered()), this, SLOT(paste()));

    // Use dummy editor to enable smiley support by default
    m_dummyEditor.reset(new CEikEdwin());
    TRAPD(err, m_dummyEditor->ConstructL(CEikEdwin::EAvkonEnableSmileySupport));
    if (!err) {
        m_formAccessor.reset(new CAknEdwinFormAccessor(m_dummyEditor.data()));
        m_fepState->SetFormAccessor(m_formAccessor.data());
    }
}

QCoeFepInputContext::~QCoeFepInputContext()
{
    m_inDestruction = true;

    // This is to make sure that the FEP manager "forgets" about us,
    // otherwise we may get callbacks even after we're destroyed.
    // The call below is essentially equivalent to InputCapabilitiesChanged(),
    // but is synchronous, rather than asynchronous.
    CCoeEnv::Static()->SyncNotifyFocusObserversOfChangeInFocus();

    delete m_fepState;
    delete m_ccpu;
    delete m_extendedInputCapabilities;
}

void QCoeFepInputContext::reset()
{
    Qt::InputMethodHints currentHints = Qt::ImhNone;
    if (focusWidget()) {
        QWidget *proxy = focusWidget()->focusProxy();
        currentHints = proxy ? proxy->inputMethodHints() : focusWidget()->inputMethodHints();
    }
    // Store a copy of preedit text, if prediction is active and input context is reseted.
    // This is to ensure that we can replace preedit string after losing focus to FEP manager's
    // internal sub-windows. Additionally, store the cursor position if there is no selected text.
    // This allows input context to replace preedit strings if they are not at the end of current
    // text.
    if (m_cachedPreeditString.isEmpty() && !(currentHints & Qt::ImhNoPredictiveText)) {
        m_cachedPreeditString = m_preeditString;
        if (focusWidget() && !m_cachedPreeditString.isEmpty()) {
            int cursor = focusWidget()->inputMethodQuery(Qt::ImCursorPosition).toInt();
            int anchor = focusWidget()->inputMethodQuery(Qt::ImAnchorPosition).toInt();
            if (cursor == anchor)
                m_cachedCursorAndAnchorPosition = cursor;
        }
    }
    commitCurrentString(true);

    // QGraphicsScene calls reset() when changing focus item. Unfortunately, the new focus item is
    // set right after resetting the input context. Therefore, asynchronously call ensureWidgetVisibility().
    if (S60->splitViewLastWidget)
        QMetaObject::invokeMethod(this,"ensureWidgetVisibility", Qt::QueuedConnection);
}

void QCoeFepInputContext::ReportAknEdStateEvent(MAknEdStateObserver::EAknEdwinStateEvent aEventType)
{
    QT_TRAP_THROWING(m_fepState->ReportAknEdStateEventL(aEventType));
}

void QCoeFepInputContext::update()
{
    updateHints(false);

    // For pre-5.0 SDKs, we don't do text updates on S60 side.
    if (QSysInfo::s60Version() < QSysInfo::SV_S60_5_0) {
        return;
    }

    // Don't be fooled (as I was) by the name of this enumeration.
    // What it really does is tell the virtual keyboard UI that the text has been
    // updated and it should be reflected in the internal display of the VK.
    ReportAknEdStateEvent(QT_EAknCursorPositionChanged);
}

void QCoeFepInputContext::setFocusWidget(QWidget *w)
{
    commitCurrentString(true);

    QInputContext::setFocusWidget(w);

    updateHints(true);
    if (w) {
        // Store last focused widget and object. Needed when Menu is Opened
        QObject *focusObject = 0;
        m_lastFocusedEditor = getQWidgetFromQGraphicsView(focusWidget(),
            &focusObject);
        m_lastFocusedObject = focusObject; // Can be null
        Q_ASSERT(m_lastFocusedEditor);
    }
}

void QCoeFepInputContext::widgetDestroyed(QWidget *w)
{
    m_cachedPreeditString.clear();
    m_cachedCursorAndAnchorPosition = -1;

    // Make sure that the input capabilities of whatever new widget got focused are queried.
    CCoeControl *ctrl = w->effectiveWinId();
    if (ctrl->IsFocused()) {
        queueInputCapabilitiesChanged();
    }
}

QString QCoeFepInputContext::language()
{
    TLanguage lang = m_fepState->LocalLanguage();
    const QByteArray localeName = qt_symbianLocaleName(lang);
    if (!localeName.isEmpty()) {
        return QString::fromLatin1(localeName);
    } else {
        return QString::fromLatin1("C");
    }
}

bool QCoeFepInputContext::needsInputPanel()
{
    switch (QSysInfo::s60Version()) {
    case QSysInfo::SV_S60_3_1:
    case QSysInfo::SV_S60_3_2:
        // There are no touch phones for pre-5.0 SDKs.
        return false;
#ifdef Q_CC_NOKIAX86
    default:
        // For emulator we assume that we need an input panel, since we can't
        // separate between phone types.
        return true;
#else
    case QSysInfo::SV_S60_5_0: {
        // For SDK == 5.0, we need phone specific detection, since the HAL API
        // is no good on most phones. However, all phones at the time of writing use the
        // input panel, except N97 in landscape mode, but in this mode it refuses to bring
        // up the panel anyway, so we don't have to care.
        return true;
    }
    default:
        // For unknown/newer types, we try to use the HAL API.
        int keyboardEnabled;
        int keyboardType;
        int err[2];
        err[0] = HAL::Get(HAL::EKeyboard, keyboardType);
        err[1] = HAL::Get(HAL::EKeyboardState, keyboardEnabled);
        if (err[0] == KErrNone && err[1] == KErrNone
                && keyboardType != 0 && keyboardEnabled)
            // Means that we have some sort of keyboard.
            return false;

        // Fall back to using the input panel.
        return true;
#endif // !Q_CC_NOKIAX86
    }
}

bool QCoeFepInputContext::vietCharConversion(const QEvent *event)
{
    const QKeyEvent *keyEvent = static_cast<const QKeyEvent *>(event);
    const uint VietVowelListCount = sizeof(VietVowelList)/sizeof(VietVowelList[0]);
    for (int tone = 0; tone < VietToneMarks; tone++) {
        if (keyEvent->key() == VietToneList[tone]) {
            // Vietnamese vowel tone mark pressed, check previous character
            const int cursor = focusWidget()->inputMethodQuery(Qt::ImCursorPosition).toInt();
            if (cursor > 0) {
                QString widgetText = focusWidget()->inputMethodQuery(Qt::ImSurroundingText).toString();
                for (int vowel = 0; vowel < VietVowelListCount; vowel++) {
                    if (widgetText[cursor-1].unicode() == VietVowelList[vowel]) {
                        // Previous character is Vietnamese vowel, replace it from matrix
                        QList<QInputMethodEvent::Attribute> attributes;
                        if (event->type() == QEvent::KeyPress) {
                            QInputMethodEvent imEvent(QString(VietToneMatrix[vowel][tone]), attributes);
                            sendEvent(imEvent);
                        } else {
                            // event->type() == QEvent::KeyRelease
                            QInputMethodEvent imEvent(QLatin1String(""), attributes);
                            imEvent.setCommitString(QString(VietToneMatrix[vowel][tone]), -1, 1);
                            sendEvent(imEvent);
                        }
                        return true;
                    }
                }
            }
            return false;
        }
    }
    if (keyEvent->key() == Qt::Key_Backspace) {
        // Backspace pressed, check previous character
        const int cursor = focusWidget()->inputMethodQuery(Qt::ImCursorPosition).toInt();
        if (cursor > 0) {
            QString widgetText = focusWidget()->inputMethodQuery(Qt::ImSurroundingText).toString();
            for (int vowel = 0; vowel < VietVowelListCount; vowel++) {
                for (int tone = 0; tone < VietToneMarks; tone++) {
                    if (widgetText[cursor-1].unicode() == VietToneMatrix[vowel][tone]) {
                        // Previous character is Vietnamese vowel with tone, replace it with plain vowel
                        QList<QInputMethodEvent::Attribute> attributes;
                        if (event->type() == QEvent::KeyPress) {
                            QInputMethodEvent imEvent(QString(VietVowelList[vowel]), attributes);
                            sendEvent(imEvent);
                        } else {
                            // event->type() == QEvent::KeyRelease
                            QInputMethodEvent imEvent(QLatin1String(""), attributes);
                            imEvent.setCommitString(QString(VietVowelList[vowel]), -1, 1);
                            sendEvent(imEvent);
                        }
                        return true;
                    }
                }
            }
        }
    }
    return false;
}

bool QCoeFepInputContext::filterEvent(const QEvent *event)
{
    if (!focusWidget())
        return false;

    if ((event->type() == QEvent::KeyPress || event->type() == QEvent::KeyRelease) &&
            QApplication::keyboardInputLocale().language() == QLocale::Vietnamese) {
        // Vietnamese character conversions
        if (vietCharConversion(event))
            return true;
    }

    switch (event->type()) {
    case QEvent::KeyPress:
        commitTemporaryPreeditString();
        // fall through intended
    case QEvent::KeyRelease:
        const QKeyEvent *keyEvent = static_cast<const QKeyEvent *>(event);
        //If proxy exists, always use hints from proxy.
        QWidget *proxy = focusWidget()->focusProxy();
        Qt::InputMethodHints currentHints = proxy ? proxy->inputMethodHints() : focusWidget()->inputMethodHints();

        switch (keyEvent->key()) {
        case Qt::Key_F20:
            Q_ASSERT(m_lastImHints == currentHints);
            if (m_lastImHints & Qt::ImhHiddenText) {
                // Special case in Symbian. On editors with secret text, F20 is for some reason
                // considered to be a backspace.
                QKeyEvent modifiedEvent(keyEvent->type(), Qt::Key_Backspace, keyEvent->modifiers(),
                        keyEvent->text(), keyEvent->isAutoRepeat(), keyEvent->count());
                QApplication::sendEvent(focusWidget(), &modifiedEvent);
                return true;
            }
            break;
        case Qt::Key_Select:
            if (!m_preeditString.isEmpty()) {
                commitCurrentString(true);
                return true;
            }
            break;
        default:
            break;
        }

        QString widgetText = focusWidget()->inputMethodQuery(Qt::ImSurroundingText).toString();
        bool validLength;
        int maxLength = focusWidget()->inputMethodQuery(Qt::ImMaximumTextLength).toInt(&validLength);
        if (!keyEvent->text().isEmpty() && validLength
                && widgetText.size() + m_preeditString.size() >= maxLength) {
            // Don't send key events with string content if the widget is "full".
            return true;
        }

        if (keyEvent->type() == QEvent::KeyPress
            && currentHints & Qt::ImhHiddenText
            && !keyEvent->text().isEmpty()
            && keyEvent->key() != Qt::Key_Enter) {
            // Send some temporary preedit text in order to make text visible for a moment.
            m_preeditString = keyEvent->text();
            QList<QInputMethodEvent::Attribute> attributes;
            QInputMethodEvent imEvent(m_preeditString, attributes);
            sendEvent(imEvent);
            m_tempPreeditStringTimeout.start(1000, this);
            m_hasTempPreeditString = true;
            update();
            return true;
        }
        break;
    }

    if (!needsInputPanel())
        return false;

    if ((event->type() == QEvent::CloseSoftwareInputPanel)
        && (QSysInfo::s60Version() > QSysInfo::SV_S60_5_0)) {
        m_fepState->ReportAknEdStateEventL(QT_EAknClosePenInputRequest);
        return false;
    }

    if (event->type() == QEvent::RequestSoftwareInputPanel) {
        // Only request virtual keyboard if it is not yet active or if this is the first time
        // panel is requested for this application.
        static bool firstTime = true;
        int vkbActive = 0;

        if (firstTime) {
            // Sometimes the global QT_EAknFepTouchInputActive value can be left incorrect at
            // application exit if the application is exited when input panel is active.
            // Therefore we always want to open the panel the first time application requests it.
            firstTime = false;
        } else {
            const TUid KPSUidAknFep = {QT_EPSUidAknFep};
            // No need to check for return value, as vkbActive stays zero in that case
            RProperty::Get(KPSUidAknFep, QT_EAknFepTouchInputActive, vkbActive);
        }

        if (!vkbActive) {
            // Notify S60 that we want the virtual keyboard to show up.
            QSymbianControl *sControl;
            sControl = focusWidget()->effectiveWinId()->MopGetObject(sControl);
            Q_ASSERT(sControl);

            // Store last focused widget and object in case of fullscreen VKB
            QObject *focusObject = 0;
            m_lastFocusedEditor = getQWidgetFromQGraphicsView(focusWidget(), &focusObject);
            m_lastFocusedObject = focusObject; // Can be null
            Q_ASSERT(m_lastFocusedEditor);

            // The FEP UI temporarily steals focus when it shows up the first time, causing
            // all sorts of weird effects on the focused widgets. Since it will immediately give
            // back focus to us, we temporarily disable focus handling until the job's done.
            if (sControl) {
                sControl->setIgnoreFocusChanged(true);
            }

            ensureInputCapabilitiesChanged();
            m_fepState->ReportAknEdStateEventL(MAknEdStateObserver::QT_EAknActivatePenInputRequest);

            if (sControl) {
                sControl->setIgnoreFocusChanged(false);
            }
        }
    }

    return false;
}

bool QCoeFepInputContext::symbianFilterEvent(QWidget *keyWidget, const QSymbianEvent *event)
{
    Q_UNUSED(keyWidget);
    if (event->type() == QSymbianEvent::WindowServerEvent) {
        const TWsEvent* wsEvent = event->windowServerEvent();
        TInt eventType = 0;
        if (wsEvent)
            eventType = wsEvent->Type();

        if (eventType == EEventKey) {
            TKeyEvent* keyEvent = wsEvent->Key();
            if (keyEvent) {
                switch (keyEvent->iScanCode) {
                case EEikCmdEditCopy:
                    CcpuCopyL();
                    break;
                case EEikCmdEditCut:
                    CcpuCutL();
                    break;
                case EEikCmdEditPaste:
                    CcpuPasteL();
                    break;
                case EStdKeyF21:
                    changeCBA(true);
                    break;
                default:
                    break;
                }
                switch (keyEvent->iCode) {
                case EKeyLeftArrow:
                case EKeyRightArrow:
                case EKeyUpArrow:
                case EKeyDownArrow:
                    if (CcpuCanCopy() && ((keyEvent->iModifiers & EModifierShift) == EModifierShift))
                        changeCBA(true);
                    break;
                default:
                    break;
                }
            }
        } else if (eventType == EEventKeyUp) {
            if (wsEvent->Key() && wsEvent->Key()->iScanCode == EStdKeyLeftShift)
               changeCBA(false);
        } else if (eventType == EEventWindowVisibilityChanged && S60->splitViewLastWidget) {
            QGraphicsView *gv = qobject_cast<QGraphicsView*>(S60->splitViewLastWidget);
            const bool alwaysResize = (gv && gv->verticalScrollBarPolicy() != Qt::ScrollBarAlwaysOff);

            if (alwaysResize) {
                TUint visibleFlags = event->windowServerEvent()->VisibilityChanged()->iFlags;
                if (visibleFlags & TWsVisibilityChangedEvent::EPartiallyVisible)
                    ensureFocusWidgetVisible(S60->splitViewLastWidget);
                if (visibleFlags & TWsVisibilityChangedEvent::ENotVisible)
                    resetSplitViewWidget(true);
            }
        }
    }

    if (event->type() == QSymbianEvent::CommandEvent)
        // A command basically means the same as a button being pushed. With Qt buttons
        // that would normally result in a reset of the input method due to the focus change.
        // This should also happen for commands.
        reset();


    if (event->type() == QSymbianEvent::ResourceChangeEvent
         && (event->resourceChangeType() == KEikMessageFadeAllWindows
         || event->resourceChangeType() == KEikDynamicLayoutVariantSwitch)) {
        reset();
    }

    return false;
}

void QCoeFepInputContext::timerEvent(QTimerEvent *timerEvent)
{
    if (timerEvent->timerId() == m_tempPreeditStringTimeout.timerId())
        commitTemporaryPreeditString();
}

void QCoeFepInputContext::commitTemporaryPreeditString()
{
    if (m_tempPreeditStringTimeout.isActive())
        m_tempPreeditStringTimeout.stop();

    if (!m_hasTempPreeditString)
        return;

    commitCurrentString(false);
}

void QCoeFepInputContext::mouseHandler(int x, QMouseEvent *event)
{
    Q_ASSERT(focusWidget());

    if (event->type() == QEvent::MouseButtonPress && event->button() == Qt::LeftButton) {
        QWidget *proxy = focusWidget()->focusProxy();
        Qt::InputMethodHints currentHints = proxy ? proxy->inputMethodHints() : focusWidget()->inputMethodHints();

        //If splitview is open and T9 word is tapped, pass the pointer event to pointer handler.
        //This will open the "suggested words" list. Pass pointer position always as zero, to make
        //full word replacement in case user makes a selection.
        if (isPartialKeyboardSupported()
            && S60->partialKeyboardOpen
            && m_pointerHandler
            && !(currentHints & Qt::ImhNoPredictiveText)
            && (x > 0 && x < m_preeditString.length())) {
            m_pointerHandler->HandlePointerEventInInlineTextL(TPointerEvent::EButton1Up, 0, 0);
        } else {
            // Notify FEP about pointer event via CAknExtendedInputCapabilities::ReporEventL().
            // FEP will then commit the string and cancel inline edit state properly.
            // FEP does not really use the pointer event parameter, so it is ok to pass NULL.
            if (m_extendedInputCapabilities) {
                TRAP_IGNORE(
                    m_extendedInputCapabilities->ReportEventL(
                       CAknExtendedInputCapabilities::MAknEventObserver::EPointerEventReceived,
                       NULL));
            } else {
                // In practice m_extendedInputCapabilities should always exist.
                // If it does not, commit current string directly here.
                // This will cancel inline edit in FEP but VKB might still think that
                // inline edit is ongoing.
                commitCurrentString(true);
            }

            int pos = focusWidget()->inputMethodQuery(Qt::ImCursorPosition).toInt();

            QList<QInputMethodEvent::Attribute> attributes;
            attributes << QInputMethodEvent::Attribute(QInputMethodEvent::Selection, pos + x, 0, QVariant());
            QInputMethodEvent event(QLatin1String(""), attributes);
            sendEvent(event);
        }
    }
}

TCoeInputCapabilities QCoeFepInputContext::inputCapabilities()
{
    if (m_inDestruction || !focusWidget()) {
        return TCoeInputCapabilities(TCoeInputCapabilities::ENone, 0, 0);
    }

    TCoeInputCapabilities inputCapabilities(m_textCapabilities, this, 0);
    inputCapabilities.SetObjectProvider(this);
    return inputCapabilities;
}

void QCoeFepInputContext::resetSplitViewWidget(bool keepInputWidget)
{
    QGraphicsView *gv = qobject_cast<QGraphicsView*>(S60->splitViewLastWidget);

    if (!gv)
        return;

    QSymbianControl *symControl = static_cast<QSymbianControl*>(gv->effectiveWinId());
    symControl->CancelLongTapTimer();

    const bool alwaysResize = (gv->verticalScrollBarPolicy() != Qt::ScrollBarAlwaysOff);
    QWidget *windowToMove = gv->window();

    bool userResize = gv->testAttribute(Qt::WA_Resized);

    windowToMove->setUpdatesEnabled(false);

    if (!alwaysResize) {
        if (gv->scene() && S60->partial_keyboardAutoTranslation) {
            if (gv->scene()->focusItem()) {
                QGraphicsItem *focusItem =
                    m_splitViewPreviousFocusItem ? m_splitViewPreviousFocusItem : gv->scene()->focusItem();
                // Check if the widget contains cursorPositionChanged signal and disconnect from it.
                QByteArray signal = QMetaObject::normalizedSignature(SIGNAL(cursorPositionChanged()));
                int index = focusItem->toGraphicsObject()->metaObject()->indexOfSignal(signal.right(signal.length() - 1));
                if (index != -1)
                    disconnect(focusItem->toGraphicsObject(), SIGNAL(cursorPositionChanged()), this, SLOT(translateInputWidget()));
            }

            QGraphicsItem *rootItem = 0;
            foreach (QGraphicsItem *item, gv->scene()->items()) {
                if (!item->parentItem()) {
                    rootItem = item;
                    break;
                }
            }
            if (rootItem)
                rootItem->resetTransform();
        }
    } else {
        if (m_splitViewResizeBy)
            if (m_splitViewPreviousWindowStates & Qt::WindowFullScreen)
                gv->resize(gv->rect().width(), qApp->desktop()->height());
            else
                gv->resize(gv->rect().width(), m_splitViewResizeBy);
    }
    // Resizing might have led to widget losing its original windowstate.
    // Restore previous window state.

    if (m_splitViewPreviousWindowStates != windowToMove->windowState())
        windowToMove->setWindowState(m_splitViewPreviousWindowStates);

    windowToMove->setUpdatesEnabled(true);

    gv->setAttribute(Qt::WA_Resized, userResize); //not a user resize

    m_splitViewResizeBy = 0;
    if (!keepInputWidget) {
        m_splitViewPreviousWindowStates = Qt::WindowNoState;
        S60->splitViewLastWidget = 0;
    }
}

// Checks if a given widget is visible in the splitview rect. The offset
// parameter can be used to validate if moving widget upwards or downwards
// by the offset would make a difference for the visibility.

bool QCoeFepInputContext::isWidgetVisible(QWidget *widget, int offset)
{
    bool visible = false;
    if (widget) {
        QRect splitViewRect = qt_TRect2QRect(static_cast<CEikAppUi*>(S60->appUi())->ClientRect());
        QWidget *window = QApplication::activeWindow();
        QGraphicsView *gv = qobject_cast<QGraphicsView*>(widget);
        if (gv && window) {
            if (QGraphicsScene *scene = gv->scene()) {
                if (QGraphicsItem *focusItem = scene->focusItem()) {
                    QPoint cursorPos = window->mapToGlobal(focusItem->cursor().pos());
                    cursorPos.setY(cursorPos.y() + offset);
                    if (splitViewRect.contains(cursorPos)) {
                        visible = true;
                    }
                }
            }
        }
    }
    return visible;
}

bool QCoeFepInputContext::isPartialKeyboardSupported()
{
    return (S60->partial_keyboard || !QApplication::testAttribute(Qt::AA_S60DisablePartialScreenInputMode));
}

void QCoeFepInputContext::ensureWidgetVisibility()
{
    ensureFocusWidgetVisible(S60->splitViewLastWidget);
}

// Ensure that the input widget is visible in the splitview rect.

void QCoeFepInputContext::ensureFocusWidgetVisible(QWidget *widget)
{
    if (!widget)
        return;

    // Native side opening and closing its virtual keyboard when it changes the keyboard layout,
    // has an adverse impact on long tap timer. Cancel the timer when splitview opens to avoid this.
    QSymbianControl *symControl = static_cast<QSymbianControl*>(widget->effectiveWinId());
    symControl->CancelLongTapTimer();

    // Graphicsviews that have vertical scrollbars should always be resized to the splitview area.
    // Graphicsviews without scrollbars should be translated.

    QGraphicsView *gv = qobject_cast<QGraphicsView*>(widget);
    if (!gv)
        return;

    const bool alwaysResize = (gv && gv->verticalScrollBarPolicy() != Qt::ScrollBarAlwaysOff);
    const bool moveWithinVisibleArea = (S60->splitViewLastWidget != 0);

    QWidget *windowToMove = gv ? gv : symControl->widget();
    if (!windowToMove->isWindow())
        windowToMove = windowToMove->window();
    if (!windowToMove) {
        return;
    }

    // When opening the keyboard (not moving within the splitview area), save the original
    // window state. In some cases, ensuring input widget visibility might lead to window
    // states getting changed.

    if (!moveWithinVisibleArea) {
        S60->splitViewLastWidget = widget;
        m_splitViewPreviousWindowStates = windowToMove->windowState();
    }

    // Check if the widget contains cursorPositionChanged signal and connect to it.
    if (gv->scene() && gv->scene()->focusItem() && S60->partial_keyboardAutoTranslation) {
        QByteArray signal = QMetaObject::normalizedSignature(SIGNAL(cursorPositionChanged()));
        if (m_splitViewPreviousFocusItem && m_splitViewPreviousFocusItem != gv->scene()->focusItem())
            disconnect(m_splitViewPreviousFocusItem->toGraphicsObject(), SIGNAL(cursorPositionChanged()), this, SLOT(translateInputWidget()));
        int index = gv->scene()->focusItem()->toGraphicsObject()->metaObject()->indexOfSignal(signal.right(signal.length() - 1));
        if (index != -1) {
            connect(gv->scene()->focusItem()->toGraphicsObject(), SIGNAL(cursorPositionChanged()), this, SLOT(translateInputWidget()));
            m_splitViewPreviousFocusItem = gv->scene()->focusItem();
        }
    }

    int windowTop = widget->window()->pos().y();

    const bool userResize = widget->testAttribute(Qt::WA_Resized);

    QRect splitViewRect = qt_TRect2QRect(static_cast<CEikAppUi*>(S60->appUi())->ClientRect());


    // When resizing a window widget, it will lose its maximized window state.
    // Native applications hide statuspane in splitview state, so lets move to
    // fullscreen mode. This makes available area slightly bigger, which helps usability
    // and greatly reduces event passing in orientation switch cases,
    // as the statuspane size is not changing.

    if (alwaysResize)
        windowToMove->setUpdatesEnabled(false);

    if (!(windowToMove->windowState() & Qt::WindowFullScreen)) {
        windowToMove->setWindowState(
            (windowToMove->windowState() & ~(Qt::WindowMinimized | Qt::WindowFullScreen)) | Qt::WindowFullScreen);
    }

    if (alwaysResize) {
        if (!moveWithinVisibleArea) {
            m_splitViewResizeBy = widget->height();
            windowTop = widget->geometry().top();
            widget->resize(widget->width(), splitViewRect.height() - windowTop);
        }

        if (gv->scene() && S60->partial_keyboardAutoTranslation) {
            const QRectF microFocusRect = gv->scene()->inputMethodQuery(Qt::ImMicroFocus).toRectF();
            gv->ensureVisible(microFocusRect);
        }
    } else {
        if (S60->partial_keyboardAutoTranslation)
            translateInputWidget();
    }

    if (alwaysResize)
        windowToMove->setUpdatesEnabled(true);

    widget->setAttribute(Qt::WA_Resized, userResize); //not a user resize
}

static QTextCharFormat qt_TCharFormat2QTextCharFormat(const TCharFormat &cFormat, bool validStyleColor)
{
    QTextCharFormat qFormat;

    if (validStyleColor) {
        QBrush foreground(QColor(cFormat.iFontPresentation.iTextColor.Internal()));
        qFormat.setForeground(foreground);
    }

    qFormat.setFontStrikeOut(cFormat.iFontPresentation.iStrikethrough == EStrikethroughOn);
    qFormat.setFontUnderline(cFormat.iFontPresentation.iUnderline == EUnderlineOn);

    return qFormat;
}

void QCoeFepInputContext::updateHints(bool mustUpdateInputCapabilities)
{
    QWidget *w = focusWidget();
    if (w) {
        QWidget *proxy = w->focusProxy();
        Qt::InputMethodHints hints = proxy ? proxy->inputMethodHints() : w->inputMethodHints();

        // Since splitview support works like an input method hint, yet it is private flag,
        // we need to update its state separately.
        if (QSysInfo::s60Version() > QSysInfo::SV_S60_5_0) {
            TInt currentFlags = m_fepState->Flags();
            if (isPartialKeyboardSupported())
                currentFlags |= QT_EAknEditorFlagEnablePartialScreen;
            else
                currentFlags &= ~QT_EAknEditorFlagEnablePartialScreen;
            if (currentFlags != m_fepState->Flags())
                m_fepState->SetFlags(currentFlags);
        }

        if (hints != m_lastImHints) {
            m_lastImHints = hints;
            applyHints(hints);
        } else if (!mustUpdateInputCapabilities) {
            // Optimization. Return immediately if there was no change.
            return;
        }
    }
    queueInputCapabilitiesChanged();
}

void QCoeFepInputContext::applyHints(Qt::InputMethodHints hints)
{
    using namespace Qt;

    reset();
    commitTemporaryPreeditString();

    const bool anynumbermodes = hints & (ImhDigitsOnly | ImhFormattedNumbersOnly | ImhDialableCharactersOnly);
    const bool anytextmodes = hints & (ImhUppercaseOnly | ImhLowercaseOnly | ImhEmailCharactersOnly | ImhUrlCharactersOnly);
    const bool numbersOnly = anynumbermodes && !anytextmodes;
    const bool noOnlys = !(hints & ImhExclusiveInputMask);
    // if alphanumeric input, or if multiple incompatible number modes are selected;
    // then make all symbols available in numeric mode too.
    const bool needsCharMap= !numbersOnly || ((hints & ImhFormattedNumbersOnly) && (hints & ImhDialableCharactersOnly));
    TInt flags;
    Qt::InputMethodHints oldHints = hints;

    // Some sanity checking. Make sure that only one preference is set.
    InputMethodHints prefs = ImhPreferNumbers | ImhPreferUppercase | ImhPreferLowercase;
    prefs &= hints;
    if (prefs != ImhPreferNumbers && prefs != ImhPreferUppercase && prefs != ImhPreferLowercase) {
        hints &= ~prefs;
    }
    if (!noOnlys) {
        // Make sure that the preference is within the permitted set.
        if (hints & ImhPreferNumbers && !anynumbermodes) {
            hints &= ~ImhPreferNumbers;
        } else if (hints & ImhPreferUppercase && !(hints & ImhUppercaseOnly)) {
            hints &= ~ImhPreferUppercase;
        } else if (hints & ImhPreferLowercase && !(hints & ImhLowercaseOnly)) {
            hints &= ~ImhPreferLowercase;
        }
        // If there is no preference, set it to something within the permitted set.
        if (!(hints & ImhPreferNumbers || hints & ImhPreferUppercase || hints & ImhPreferLowercase)) {
            if (hints & ImhLowercaseOnly) {
                hints |= ImhPreferLowercase;
            } else if (hints & ImhUppercaseOnly) {
                hints |= ImhPreferUppercase;
            } else if (numbersOnly) {
                hints |= ImhPreferNumbers;
            }
        }
    }

    if (hints & ImhPreferNumbers) {
        m_fepState->SetDefaultInputMode(EAknEditorNumericInputMode);
        m_fepState->SetCurrentInputMode(EAknEditorNumericInputMode);
    } else {
        m_fepState->SetDefaultInputMode(EAknEditorTextInputMode);
        m_fepState->SetCurrentInputMode(EAknEditorTextInputMode);
    }
    flags = 0;
    if (noOnlys || (anynumbermodes && anytextmodes)) {
        flags = EAknEditorAllInputModes;
    }
    else if (anynumbermodes) {
        flags |= EAknEditorNumericInputMode;
    }
    else if (anytextmodes) {
        flags |= EAknEditorTextInputMode;
    }
    else {
        flags = EAknEditorAllInputModes;
    }
    m_fepState->SetPermittedInputModes(flags);
    ReportAknEdStateEvent(MAknEdStateObserver::EAknEdwinStateInputModeUpdate);

    if (hints & ImhPreferLowercase) {
        m_fepState->SetDefaultCase(EAknEditorLowerCase);
        m_fepState->SetCurrentCase(EAknEditorLowerCase);
    } else if (hints & ImhPreferUppercase) {
        m_fepState->SetDefaultCase(EAknEditorUpperCase);
        m_fepState->SetCurrentCase(EAknEditorUpperCase);
    } else if (hints & ImhNoAutoUppercase) {
        m_fepState->SetDefaultCase(EAknEditorLowerCase);
        m_fepState->SetCurrentCase(EAknEditorLowerCase);
    } else if (hints & ImhHiddenText) {
        m_fepState->SetDefaultCase(EAknEditorLowerCase);
        m_fepState->SetCurrentCase(EAknEditorLowerCase);
    } else {
        m_fepState->SetDefaultCase(EAknEditorTextCase);
        m_fepState->SetCurrentCase(EAknEditorTextCase);
    }
    flags = 0;
    if (hints & ImhUppercaseOnly) {
        flags |= EAknEditorUpperCase;
    }
    if (hints & ImhLowercaseOnly) {
        flags |= EAknEditorLowerCase;
    }
    if (hints & ImhHiddenText) {
        flags = EAknEditorAllCaseModes;
        flags &= ~EAknEditorTextCase;
    }
    if (flags == 0) {
        flags = EAknEditorAllCaseModes;
        if (hints & ImhNoAutoUppercase) {
            flags &= ~EAknEditorTextCase;
        }
    }
    m_fepState->SetPermittedCases(flags);
    ReportAknEdStateEvent(MAknEdStateObserver::EAknEdwinStateCaseModeUpdate);

    flags = 0;
    if (QSysInfo::s60Version() > QSysInfo::SV_S60_5_0) {
        if (isPartialKeyboardSupported())
            flags |= QT_EAknEditorFlagEnablePartialScreen;
        flags |= QT_EAknEditorFlagSelectionVisible;
    }
    if (hints & ImhUppercaseOnly && !(hints & ImhLowercaseOnly)
            || hints & ImhLowercaseOnly && !(hints & ImhUppercaseOnly)) {
        flags |= EAknEditorFlagFixedCase;
    }
    // Using T9 and hidden text together may actually crash the FEP, so check for hidden text too.
    if (hints & ImhNoPredictiveText || hints & ImhHiddenText) {
        flags |= EAknEditorFlagNoT9;
    }
    
    if (S60->editorFlags & EAknEditorFlagLatinInputModesOnly){
        flags |= EAknEditorFlagLatinInputModesOnly;
    }
   
    if (needsCharMap)
        flags |= EAknEditorFlagUseSCTNumericCharmap;
    m_fepState->SetFlags(flags);
    ReportAknEdStateEvent(MAknEdStateObserver::EAknEdwinStateFlagsUpdate);

    if (hints & ImhDialableCharactersOnly) {
        // This is first, because if (ImhDialableCharactersOnly | ImhFormattedNumbersOnly)
        // is specified, this one is more natural (# key enters a #)
        flags = EAknEditorStandardNumberModeKeymap;
    } else if (hints & ImhFormattedNumbersOnly) {
        // # key enters decimal point
        flags = EAknEditorCalculatorNumberModeKeymap;
    } else if (hints & ImhDigitsOnly) {
        // This is last, because it is most restrictive (# key is inactive)
        flags = EAknEditorPlainNumberModeKeymap;
    } else {
        flags = EAknEditorStandardNumberModeKeymap;
    }
    m_fepState->SetNumericKeymap(static_cast<TAknEditorNumericKeymap>(flags));

    if (hints & ImhUrlCharactersOnly) {
        // URL characters is everything except space, so a superset of the other restrictions
        m_fepState->SetExtensionFlags(EAknEditorExtFlagKeyboardUrl);
    } else if (hints & ImhEmailCharactersOnly) {
        m_fepState->SetExtensionFlags(EAknEditorExtFlagKeyboardEmail);
    } else {
        m_fepState->SetExtensionFlags(0);
    }

    bool enableSmileys = needsCharMap && !(hints & (ImhHiddenText | ImhUrlCharactersOnly | ImhEmailCharactersOnly));
    if (enableSmileys)
        m_dummyEditor->AddFlagToUserFlags(CEikEdwin::EAvkonEnableSmileySupport);
    else
        m_dummyEditor->RemoveFlagFromUserFlags(CEikEdwin::EAvkonEnableSmileySupport);

    if (hints & ImhHiddenText) {
        m_textCapabilities = TCoeInputCapabilities::EAllText | TCoeInputCapabilities::ESecretText;
    } else {
        m_textCapabilities = TCoeInputCapabilities::EAllText;
    }
}

void QCoeFepInputContext::applyFormat(QList<QInputMethodEvent::Attribute> *attributes)
{
    TCharFormat cFormat;
    QColor styleTextColor;
    if (QWidget *focused = focusWidget()) {
        QGraphicsView *gv = qobject_cast<QGraphicsView*>(focused);
        if (!gv) // could be either the QGV or its viewport that has focus
            gv = qobject_cast<QGraphicsView*>(focused->parentWidget());
        if (gv) {
            if (QGraphicsScene *scene = gv->scene()) {
                if (QGraphicsItem *focusItem = scene->focusItem()) {
                    if (focusItem->isWidget()) {
                        styleTextColor = static_cast<QGraphicsWidget*>(focusItem)->palette().text().color();
                    }
                }
            }
        } else {
            styleTextColor = focused->palette().text().color();
        }
    } else {
        styleTextColor = QApplication::palette("QLineEdit").text().color();
    }

    if (styleTextColor.isValid()) {
        const TLogicalRgb fontColor(TRgb(styleTextColor.red(), styleTextColor.green(), styleTextColor.blue(), styleTextColor.alpha()));
        cFormat.iFontPresentation.iTextColor = fontColor;
    }

    TInt numChars = 0;
    TInt charPos = 0;
    int oldSize = attributes->size();
    while (m_formatRetriever) {
        m_formatRetriever->GetFormatOfFepInlineText(cFormat, numChars, charPos);
        if (numChars <= 0) {
            // This shouldn't happen according to S60 docs, but apparently does sometimes.
            break;
        }
        attributes->append(QInputMethodEvent::Attribute(QInputMethodEvent::TextFormat,
                                                        charPos,
                                                        numChars,
                                                        QVariant(qt_TCharFormat2QTextCharFormat(cFormat, styleTextColor.isValid()))));
        charPos += numChars;
        if (charPos >= m_preeditString.size()) {
            break;
        }
    }

}

void QCoeFepInputContext::queueInputCapabilitiesChanged()
{
    if (m_pendingInputCapabilitiesChanged)
        return;

    // Call ensureInputCapabilitiesChanged asynchronously. This is done to improve performance
    // by not updating input capabilities too often. The reason we don't call the Symbian
    // asynchronous version of InputCapabilitiesChanged is because we need to ensure that it
    // is synchronous in some specific cases. Those will call ensureInputCapabilitesChanged.
    QMetaObject::invokeMethod(this, "ensureInputCapabilitiesChanged", Qt::QueuedConnection);
    m_pendingInputCapabilitiesChanged = true;
}

void QCoeFepInputContext::ensureInputCapabilitiesChanged()
{
    if (!m_pendingInputCapabilitiesChanged)
        return;

    // The call below is essentially equivalent to InputCapabilitiesChanged(),
    // but is synchronous, rather than asynchronous.
    CCoeEnv::Static()->SyncNotifyFocusObserversOfChangeInFocus();
    m_pendingInputCapabilitiesChanged = false;
}

void QCoeFepInputContext::translateInputWidget()
{
    QGraphicsView *gv = qobject_cast<QGraphicsView *>(S60->splitViewLastWidget);
    if (!gv)
        return;
    QRect splitViewRect = qt_TRect2QRect(static_cast<CEikAppUi*>(S60->appUi())->ClientRect());

    QRectF cursor = gv->scene()->inputMethodQuery(Qt::ImMicroFocus).toRectF();
    QPolygon cursorP = gv->mapFromScene(cursor);
    QRectF vkbRect = QRectF(splitViewRect.bottomLeft(), qApp->desktop()->rect().bottomRight());
    if (cursor.isEmpty() || vkbRect.isEmpty())
        return;

    // Fetch root item (i.e. graphicsitem with no parent)
    QGraphicsItem *rootItem = 0;
    foreach (QGraphicsItem *item, gv->scene()->items()) {
        if (!item->parentItem()) {
            rootItem = item;
            break;
        }
    }
    if (!rootItem)
        return;

    m_transformation = (rootItem->transform().isTranslating()) ? QRectF(0,0, gv->width(), rootItem->transform().dy()) : QRectF();

    // Adjust cursor bounding rect towards navigation direction,
    // so that view translates if the cursor gets near the splitview border.
    QRect cursorRect = (cursorP.boundingRect().top() < 0) ?
        cursorP.boundingRect().adjusted(0, -cursor.height(), 0, -cursor.height()) :
        cursorP.boundingRect().adjusted(0, cursor.height(), 0, cursor.height());

    // If the current cursor position and upcoming cursor positions are visible in the splitview
    // area, do not move the view.
    if (splitViewRect.contains(cursorRect) && splitViewRect.contains(cursorP.boundingRect()))
        return;

    // New Y position should be ideally just above the keyboard.
    // If that would expose unpainted canvas, limit the tranformation to the visible scene rect or
    // to the focus item's shape/clip path.

    const QPainterPath path = gv->scene()->focusItem()->isClipped() ?
        gv->scene()->focusItem()->clipPath() : gv->scene()->focusItem()->shape();
    const qreal itemHeight = path.boundingRect().height();

    // Limit the maximum translation so that underlaying window content is not exposed.
    qreal availableSpace = gv->sceneRect().bottom() - splitViewRect.bottom();
    availableSpace = m_transformation.height() ?
        (qMin(itemHeight, availableSpace) + m_transformation.height()) :
        availableSpace;

    // Translation should happen row-by-row, but initially it needs to ensure that cursor is visible.
    const qreal translation = m_transformation.height() ?
        cursor.height() : (cursorRect.bottom() - vkbRect.top());
    qreal dy = 0.0;
    if (availableSpace > 0)
        dy = -(qMin(availableSpace, translation));
    else
        dy = -(translation);

    // Correct the translation direction, if the cursor rect would be moved above application area.
    if ((cursorP.boundingRect().bottom() + dy) < 0)
        dy *= -1;

    // Do not allow transform above screen top, nor beyond scenerect. Also, if there is no available
    // space anymore, skip translation.
    if ((m_transformation.height() + dy) > 0
        || (gv->sceneRect().bottom() + m_transformation.height()) < 0
        || !availableSpace) {
        // If we already have some transformation, remove it.
        if (m_transformation.height() < 0 || gv->sceneRect().bottom() + m_transformation.height() < 0) {
            rootItem->resetTransform();
            translateInputWidget();
        }
        return;
    }

    rootItem->setTransform(QTransform::fromTranslate(0, dy), true);
}

void QCoeFepInputContext::StartFepInlineEditL(const TDesC& aInitialInlineText,
        TInt aPositionOfInsertionPointInInlineText, TBool aCursorVisibility, const MFormCustomDraw* /*aCustomDraw*/,
        MFepInlineTextFormatRetriever& aInlineTextFormatRetriever,
        MFepPointerEventHandlerDuringInlineEdit& aPointerEventHandlerDuringInlineEdit)
{
    QWidget *w = focusWidget();
    if (!w)
        return;

    m_cachedPreeditString.clear();
    m_cachedCursorAndAnchorPosition = -1;

    commitTemporaryPreeditString();

    QList<QInputMethodEvent::Attribute> attributes;

    m_cursorVisibility = aCursorVisibility ? 1 : 0;
    m_inlinePosition = aPositionOfInsertionPointInInlineText;
    m_preeditString = qt_TDesC2QString(aInitialInlineText);

    m_formatRetriever = &aInlineTextFormatRetriever;
    m_pointerHandler = &aPointerEventHandlerDuringInlineEdit;

    // With T9 aInitialInlineText is typically empty when StartFepInlineEditL is called,
    // but FEP requires that selected text is always removed at StartFepInlineEditL.
    // Let's remove the selected text if aInitialInlineText is empty and there is selected text
    if (m_preeditString.isEmpty()) {
        QString currentSelection = w->inputMethodQuery(Qt::ImCurrentSelection).toString();
        if (!currentSelection.isEmpty()) {
            // To correctly remove selection in cases where we have multiple lines selected,
            // we must rely on the control's own selection removal mechanism, as surrounding
            // text contains only one line. It's also impossible to accurately detect
            // these overselection cases as the anchor and cursor positions are limited to the
            // surrounding text.
            // Solution is to clear the selection by faking a preedit. Use a dummy character
            // from the current selection just to be safe.
            QString dummyText = currentSelection.left(1);
            QList<QInputMethodEvent::Attribute> attributes;
            QInputMethodEvent clearSelectionEvent(dummyText, attributes);
            clearSelectionEvent.setCommitString(QLatin1String(""), 0, 0);
            sendEvent(clearSelectionEvent);

            // Now that selection is taken care of, clear the fake preedit.
            QInputMethodEvent clearPreeditEvent(QLatin1String(""), attributes);
            clearPreeditEvent.setCommitString(QLatin1String(""), 0, 0);
            sendEvent(clearPreeditEvent);
        }
    }

    applyFormat(&attributes);

    attributes.append(QInputMethodEvent::Attribute(QInputMethodEvent::Cursor,
                                                   m_inlinePosition,
                                                   m_cursorVisibility,
                                                   QVariant()));
    QInputMethodEvent event(m_preeditString, attributes);
    sendEvent(event);
}

void QCoeFepInputContext::UpdateFepInlineTextL(const TDesC& aNewInlineText,
        TInt aPositionOfInsertionPointInInlineText)
{
    QWidget *w = focusWidget();
    if (!w)
        return;

    commitTemporaryPreeditString();

    m_inlinePosition = aPositionOfInsertionPointInInlineText;

    QList<QInputMethodEvent::Attribute> attributes;
    applyFormat(&attributes);
    attributes.append(QInputMethodEvent::Attribute(QInputMethodEvent::Cursor,
                                                   m_inlinePosition,
                                                   m_cursorVisibility,
                                                   QVariant()));
    QString newPreeditString = qt_TDesC2QString(aNewInlineText);
    QInputMethodEvent event(newPreeditString, attributes);
    if (!m_cachedPreeditString.isEmpty()) {
        int cursorPos = w->inputMethodQuery(Qt::ImCursorPosition).toInt();
        // Predicted word is either replaced from the end of the word (normal case),
        // or from stored location, if the predicted word is either in the beginning of,
        // or in the middle of already committed word.
        int diff = cursorPos - m_cachedCursorAndAnchorPosition;
        int replaceLocation = (diff != m_cachedPreeditString.length()) ? diff : m_cachedPreeditString.length();

        event.setCommitString(QLatin1String(""), -replaceLocation, m_cachedPreeditString.length());
        m_cachedPreeditString.clear();
        m_cachedCursorAndAnchorPosition = -1;
    } else if (newPreeditString.isEmpty() && m_preeditString.isEmpty()) {
        // In Symbian world this means "erase last character".
        event.setCommitString(QLatin1String(""), -1, 1);
    }
    m_preeditString = newPreeditString;
    sendEvent(event);
}

void QCoeFepInputContext::SetInlineEditingCursorVisibilityL(TBool aCursorVisibility)
{
    QWidget *w = focusWidget();
    if (!w)
        return;

    m_cursorVisibility = aCursorVisibility ? 1 : 0;

    QList<QInputMethodEvent::Attribute> attributes;
    attributes.append(QInputMethodEvent::Attribute(QInputMethodEvent::Cursor,
                                                   m_inlinePosition,
                                                   m_cursorVisibility,
                                                   QVariant()));
    QInputMethodEvent event(m_preeditString, attributes);
    sendEvent(event);
}

void QCoeFepInputContext::CancelFepInlineEdit()
{
    // We are not supposed to ever have a tempPreeditString and a real preedit string
    // from S60 at the same time, so it should be safe to rely on this test to determine
    // whether we should honor S60's request to clear the text or not.
    if (m_hasTempPreeditString || m_pendingTransactionCancel)
        return;

    m_pendingTransactionCancel = true;

    QT_TRY {
        QList<QInputMethodEvent::Attribute> attributes;
        QInputMethodEvent event(QLatin1String(""), attributes);
        event.setCommitString(QLatin1String(""), 0, 0);
        m_preeditString.clear();
        m_inlinePosition = 0;
        sendEvent(event);

        // Prior to S60 5.4 need to sync with native side editor state so that native side can then do
        // various operations based on editor state, such as removing 'exact word bubble'.
        // Starting with S60 5.4 this sync request is not needed and can actually lead to a crash.
        if (QSysInfo::s60Version() < QSysInfo::SV_S60_5_4 && !m_pendingInputCapabilitiesChanged)
            ReportAknEdStateEvent(MAknEdStateObserver::EAknSyncEdwinState);
    } QT_CATCH(const std::exception&) {
        m_preeditString.clear();
        m_inlinePosition = 0;
    }

    m_pendingTransactionCancel = false;
}

TInt QCoeFepInputContext::DocumentLengthForFep() const
{
    QT_TRY {
        QWidget *w = focusWidget();
        QObject *focusObject = 0;
        if (!w) {
            //when Menu is opened editor lost the focus, but fep manager wants focused editor
            w = m_lastFocusedEditor;
            focusObject = m_lastFocusedObject;
        } else {
            w = getQWidgetFromQGraphicsView(w, &focusObject);
        }
        if (!w)
            return 0;

        QVariant variant = w->inputMethodQuery(Qt::ImSurroundingText);
        int size = variant.value<QString>().size() + m_preeditString.size();

        // To fix an issue with backspaces not being generated if document size is zero,
        // fake document length to be at least one always, except when dealing with
        // hidden text widgets, all singleline text widgets and
        // also multiline text widget with single line.
        if (size == 0 && !(m_textCapabilities & TCoeInputCapabilities::ESecretText)
            && !(qobject_cast< QLineEdit *> (w))) {
            int lineCount = 0;
            if (QTextEdit* tedit = qobject_cast<QTextEdit *>(w)) {
                lineCount = tedit->document()->lineCount();
            } else if (QPlainTextEdit* ptedit = qobject_cast<QPlainTextEdit *>(w)) {
                lineCount = ptedit->document()->lineCount();
            } else {
                // Unknown editor (probably a QML one); Request the "lineCount" property.
                QObject *invokeTarget = w;
                if (focusObject)
                    invokeTarget = focusObject;
                QVariant lineVariant = invokeTarget->property("lineCount");
                if (lineVariant.isValid()) {
                    lineCount = lineVariant.toInt();
                } else {
                    // If we can't get linecount from a custom QML editor, assume that it
                    // has multiple lines, so that it can receive backspaces also when
                    // the current line is empty.
                    lineCount = 2;
                }
            }
            // To fix an issue with backspaces not being generated if document size is zero,
            // return size to 1 only for multiline editors with
            // no text and multiple lines presented.
            if (lineCount > 1)
                size = 1;
        }
        return size;
    } QT_CATCH(const std::exception&) {
        return 0;
    }
}

TInt QCoeFepInputContext::DocumentMaximumLengthForFep() const
{
    QWidget *w = focusWidget();
    if (!w)
        return 0;

    QVariant variant = w->inputMethodQuery(Qt::ImMaximumTextLength);
    int size;
    if (variant.isValid()) {
        size = variant.toInt();
    } else {
        size = INT_MAX; // Sensible default for S60.
    }
    return size;
}

void QCoeFepInputContext::SetCursorSelectionForFepL(const TCursorSelection& aCursorSelection)
{
    QWidget *w = focusWidget();
    if (!w)
        return;

    commitTemporaryPreeditString();

    int pos = aCursorSelection.iAnchorPos;
    int length = aCursorSelection.iCursorPos - pos;
    if (m_cachedCursorAndAnchorPosition != -1) {
        pos = m_cachedCursorAndAnchorPosition;
        length = 0;
    }

    QList<QInputMethodEvent::Attribute> attributes;
    attributes << QInputMethodEvent::Attribute(QInputMethodEvent::Selection, pos, length, QVariant());
    QInputMethodEvent event(m_preeditString, attributes);
    sendEvent(event);
}

void QCoeFepInputContext::GetCursorSelectionForFep(TCursorSelection& aCursorSelection) const
{
    QT_TRY {
        QWidget *w = focusWidget();
        if (!w) {
            aCursorSelection.SetSelection(0,0);
            return;
        }

        int cursor = w->inputMethodQuery(Qt::ImCursorPosition).toInt() + m_preeditString.size();
        int anchor = w->inputMethodQuery(Qt::ImAnchorPosition).toInt() + m_preeditString.size();

        // If the position is stored, use that value, so that word replacement from proposed word
        // lists are added to the correct position.
        if (m_cachedCursorAndAnchorPosition != -1) {
            cursor = m_cachedCursorAndAnchorPosition;
            anchor = m_cachedCursorAndAnchorPosition;
        }
        QString text = w->inputMethodQuery(Qt::ImSurroundingText).value<QString>();
        int combinedSize = text.size() + m_preeditString.size();
        if (combinedSize < anchor || combinedSize < cursor) {
            // ### TODO! FIXME! QTBUG-5050
            // This is a hack to prevent crashing in 4.6 with QLineEdits that use input masks.
            // The root problem is that cursor position is relative to displayed text instead of the
            // actual text we get.
            //
            // To properly fix this we would need to know the displayText of QLineEdits instead
            // of just the text, which on itself should be a trivial change. The difficulties start
            // when we need to commit the changes back to the QLineEdit, which would have to be somehow
            // able to handle displayText, too.
            //
            // Until properly fixed, the cursor and anchor positions will not reflect correct positions
            // for masked QLineEdits, unless all the masked positions are filled in order so that
            // cursor position relative to the displayed text matches position relative to actual text.
            aCursorSelection.iAnchorPos = combinedSize;
            aCursorSelection.iCursorPos = combinedSize;
        } else {
            aCursorSelection.iAnchorPos = anchor;
            aCursorSelection.iCursorPos = cursor;
        }
    } QT_CATCH(const std::exception&) {
        aCursorSelection.SetSelection(0,0);
    }
}

void QCoeFepInputContext::GetEditorContentForFep(TDes& aEditorContent, TInt aDocumentPosition,
        TInt aLengthToRetrieve) const
{
    QWidget *w = focusWidget();
    if (!w) {
        aEditorContent.FillZ(aLengthToRetrieve);
        return;
    }

    QString text = w->inputMethodQuery(Qt::ImSurroundingText).value<QString>();
    // FEP expects the preedit string to be part of the editor content, so let's mix it in.
    int cursor = w->inputMethodQuery(Qt::ImCursorPosition).toInt();
    text.insert(cursor, m_preeditString);

    // Add additional space to empty non-password text to compensate
    // for the fake length we specified in DocumentLengthForFep().
    if (text.size() == 0 && !(m_textCapabilities & TCoeInputCapabilities::ESecretText))
        text += QChar(0x20);

    aEditorContent.Copy(qt_QString2TPtrC(text.mid(aDocumentPosition, aLengthToRetrieve)));
}

void QCoeFepInputContext::GetFormatForFep(TCharFormat& aFormat, TInt /* aDocumentPosition */) const
{
    QWidget *w = focusWidget();
    if (!w) {
        aFormat = TCharFormat();
        return;
    }

    QFont font = w->inputMethodQuery(Qt::ImFont).value<QFont>();
    QFontMetrics metrics(font);
    //QString name = font.rawName();
    QString name = font.defaultFamily(); // TODO! FIXME! Should be the above.
    QHBufC hBufC(name);
    aFormat = TCharFormat(hBufC->Des(), metrics.height());
}

void QCoeFepInputContext::GetScreenCoordinatesForFepL(TPoint& aLeftSideOfBaseLine, TInt& aHeight,
        TInt& aAscent, TInt aDocumentPosition) const
{
    QT_TRYCATCH_LEAVING(getScreenCoordinatesForFepX(aLeftSideOfBaseLine, aHeight, aAscent, aDocumentPosition));
}

void QCoeFepInputContext::getScreenCoordinatesForFepX(TPoint& aLeftSideOfBaseLine, TInt& aHeight,
        TInt& aAscent, TInt /* aDocumentPosition */) const
{
    QWidget *w = focusWidget();
    if (!w) {
        aLeftSideOfBaseLine = TPoint(0,0);
        aHeight = 0;
        aAscent = 0;
        return;
    }

    QRect rect = w->inputMethodQuery(Qt::ImMicroFocus).value<QRect>();
    aLeftSideOfBaseLine.iX = rect.left();
    aLeftSideOfBaseLine.iY = rect.bottom();

    QFont font = w->inputMethodQuery(Qt::ImFont).value<QFont>();
    QFontMetrics metrics(font);
    aHeight = metrics.height();
    aAscent = metrics.ascent();
}

void QCoeFepInputContext::enableSymbianCcpuSupport()
{
    if (!m_ccpu) {
        QT_TRAP_THROWING(
            m_ccpu = new (ELeave) CAknCcpuSupport(this);
            m_ccpu->SetMopParent(this);
            CleanupStack::PushL(m_ccpu);
            m_ccpu->ConstructL();
            CleanupStack::Pop(m_ccpu);
        );
        Q_ASSERT(m_fepState);
        if (m_fepState)
            m_fepState->SetCcpuState(this);
    }
}

void QCoeFepInputContext::changeCBA(bool showCopyAndOrPaste)
{
    QWidget *w = focusWidget();
    if (!w)
        w = m_lastFocusedEditor;

    if (w) {
        if (showCopyAndOrPaste) {
            if (CcpuCanCopy())
                w->addAction(m_copyAction);
            if (CcpuCanPaste())
                w->addAction(m_pasteAction);
        } else {
            w->removeAction(m_copyAction);
            w->removeAction(m_pasteAction);
        }
    }
}

void QCoeFepInputContext::copyOrCutTextToClipboard(const char *operation)
{
    QWidget *w = focusWidget();
    QObject *focusObject = 0;
    if (!w) {
        w = m_lastFocusedEditor;
        focusObject = m_lastFocusedObject;
    } else {
        w = getQWidgetFromQGraphicsView(w, &focusObject);
    }

    if (w) {
        int cursor = w->inputMethodQuery(Qt::ImCursorPosition).toInt();
        int anchor = w->inputMethodQuery(Qt::ImAnchorPosition).toInt();

        if (cursor != anchor) {
            if (ccpuInvokeSlot(w, focusObject, operation)) {
                if (QSysInfo::symbianVersion() > QSysInfo::SV_SF_3) {
                    TRAP_IGNORE(
                        CAknDiscreetPopup::ShowGlobalPopupL(
                            R_AVKON_DISCREET_POPUP_TEXT_COPIED,
                            KAvkonResourceFile);
                    )
                }
            }
        }
    }
}


void QCoeFepInputContext::DoCommitFepInlineEditL()
{
    commitCurrentString(false);
    if (QSysInfo::s60Version() > QSysInfo::SV_S60_5_0)
        ReportAknEdStateEvent(QT_EAknCursorPositionChanged);

}

void QCoeFepInputContext::commitCurrentString(bool cancelFepTransaction)
{
    QList<QInputMethodEvent::Attribute> attributes;
    QInputMethodEvent event(QLatin1String(""), attributes);
    event.setCommitString(m_preeditString, 0, 0);
    m_preeditString.clear();
    m_inlinePosition = 0;
    sendEvent(event);

    m_hasTempPreeditString = false;

    //Only cancel FEP transactions with prediction, when there is still active window.
    Qt::InputMethodHints currentHints = Qt::ImhNone;
    if (focusWidget()) {
        if (focusWidget()->focusProxy())
            currentHints = focusWidget()->focusProxy()->inputMethodHints();
        else
            currentHints = focusWidget()->inputMethodHints();
    }
    bool predictive = !(currentHints & Qt::ImhNoPredictiveText);
    bool widgetAndWindowAvailable = QApplication::activeWindow() && focusWidget();

    if (cancelFepTransaction && ((predictive && widgetAndWindowAvailable) || !predictive)) {
        CCoeFep* fep = CCoeEnv::Static()->Fep();
        if (fep)
            fep->CancelTransaction();
    }
}

MCoeFepAwareTextEditor_Extension1* QCoeFepInputContext::Extension1(TBool& aSetToTrue)
{
    aSetToTrue = ETrue;
    return this;
}

void QCoeFepInputContext::SetStateTransferingOwnershipL(MCoeFepAwareTextEditor_Extension1::CState* aState,
        TUid /*aTypeSafetyUid*/)
{
    // Note: The S60 docs are wrong! See the State() function.
    if (m_fepState)
        delete m_fepState;
    m_fepState = static_cast<CAknEdwinState *>(aState);
}

MCoeFepAwareTextEditor_Extension1::CState* QCoeFepInputContext::State(TUid /*aTypeSafetyUid*/)
{
    // Note: The S60 docs are horribly wrong when describing the
    // SetStateTransferingOwnershipL function and this function. They say that the former
    // sets a CState object identified by the TUid, and the latter retrieves it.
    // In reality, the CState is expected to always be a CAknEdwinState (even if it was not
    // previously set), and the TUid is ignored. All in all, there is a single CAknEdwinState
    // per QCoeFepInputContext, which should be deleted if the SetStateTransferingOwnershipL
    // function is used to set a new one.
    return m_fepState;
}

TBool QCoeFepInputContext::CcpuIsFocused() const
{
    return focusWidget() != 0;
}

TBool QCoeFepInputContext::CcpuCanCut() const
{
    QT_TRY {
        bool retval = false;
        if (m_inDestruction)
            return retval;
        QWidget *w = focusWidget();
        QObject *focusObject = 0;
        if (!w) {
            w = m_lastFocusedEditor;
            focusObject = m_lastFocusedObject;
        } else {
            w = getQWidgetFromQGraphicsView(w, &focusObject);
        }
        if (w) {
            QRect microFocus = w->inputMethodQuery(Qt::ImMicroFocus).toRect();
            if (microFocus.isNull()) {
                // For some reason, the editor does not have microfocus. Most probably,
                // it is due to using native fullscreen editing mode with QML apps.
                // Try accessing "selectedText" directly.
                QObject *invokeTarget = w;
                if (focusObject)
                    invokeTarget = focusObject;

                QString selectedText = invokeTarget->property("selectedText").toString();
                retval = !selectedText.isNull();
            } else {
                int cursor = w->inputMethodQuery(Qt::ImCursorPosition).toInt();
                int anchor = w->inputMethodQuery(Qt::ImAnchorPosition).toInt();
                retval = cursor != anchor;
            }
        }
        return retval;
    } QT_CATCH(const std::exception&) {
        return EFalse;
    }
}

void QCoeFepInputContext::CcpuCutL()
{
    copyOrCutTextToClipboard("cut");
}

TBool QCoeFepInputContext::CcpuCanCopy() const
{
    return CcpuCanCut();
}

void QCoeFepInputContext::CcpuCopyL()
{
    copyOrCutTextToClipboard("copy");
}

TBool QCoeFepInputContext::CcpuCanPaste() const
{
    bool canPaste = false;
    if (m_inDestruction)
        return canPaste;

    QString textToPaste = QApplication::clipboard()->text();
    if (!textToPaste.isEmpty()) {
        QWidget *w = focusWidget();
        QObject *focusObject = 0;
        if (!w) {
            w = m_lastFocusedEditor;
            focusObject = m_lastFocusedObject;
        } else {
            w = getQWidgetFromQGraphicsView(w, &focusObject);
        }
        if (w) {
            // First, check if we are dealing with standard Qt editors (QLineEdit, QTextEdit, or QPlainTextEdit),
            // as they do not have queryable property.
            if (QTextEdit* tedit = qobject_cast<QTextEdit *>(w)) {
                canPaste = tedit->canPaste();
            } else if (QPlainTextEdit* ptedit = qobject_cast<QPlainTextEdit *>(w)) {
                canPaste = ptedit->canPaste();
            } else if (QLineEdit* ledit = qobject_cast<QLineEdit *>(w)) {
                QString fullText = ledit->text();
                if (ledit->hasSelectedText()) {
                    fullText.remove(ledit->selectionStart(), ledit->selectedText().length());
                    fullText.insert(ledit->selectionStart(), textToPaste);
                } else {
                    fullText.insert(ledit->cursorPosition(), textToPaste);
                }

                if (fullText.length() > ledit->maxLength()) {
                    canPaste = false;
                } else {
                    const QValidator* validator = ledit->validator();
                    if (validator) {
                        int pos = 0;
                        if (validator->validate(fullText, pos) == QValidator::Invalid)
                            canPaste = false;
                        else
                            canPaste = true;
                    } else {
                        QString mask(ledit->inputMask());
                        if (!mask.isEmpty()) {
                            QCoeFepInputMaskHandler maskhandler(mask);
                            if (maskhandler.canPasteClipboard(fullText))
                                canPaste = true;
                            else
                                canPaste = false;
                        } else {
                            canPaste = true;
                        }
                    }
                }
            } else {
                // Unknown editor (probably a QML one); Request the "canPaste" property.
                QObject *invokeTarget = w;
                if (focusObject)
                    invokeTarget = focusObject;

                canPaste = invokeTarget->property("canPaste").toBool();
            }
        }
    }
    return canPaste;
}

void QCoeFepInputContext::CcpuPasteL()
{
    QWidget *w = focusWidget();
    QObject *focusObject = 0;
    if (!w) {
        w = m_lastFocusedEditor;
        focusObject = m_lastFocusedObject;
    } else {
        w = getQWidgetFromQGraphicsView(w, &focusObject);
    }
    if (w)
        ccpuInvokeSlot(w, focusObject, "paste");
}

TBool QCoeFepInputContext::CcpuCanUndo() const
{
    //not supported
    return EFalse;
}

void QCoeFepInputContext::CcpuUndoL()
{
    //not supported
}

void QCoeFepInputContext::copy()
{
    QT_TRAP_THROWING(CcpuCopyL());
}

void QCoeFepInputContext::paste()
{
    QT_TRAP_THROWING(CcpuPasteL());
}

TTypeUid::Ptr QCoeFepInputContext::MopSupplyObject(TTypeUid id)
{
    if (m_extendedInputCapabilities
        && id.iUid == CAknExtendedInputCapabilities::ETypeId)
        return id.MakePtr(m_extendedInputCapabilities);

    return TTypeUid::Null();
}

MObjectProvider *QCoeFepInputContext::MopNext()
{
    QWidget *w = focusWidget();
    if (w)
        return w->effectiveWinId();
    return 0;
}

QT_END_NAMESPACE

#endif // QT_NO_IM
