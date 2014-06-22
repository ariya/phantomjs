/***************************************************************************
**
** Copyright (C) 2013 BlackBerry Limited. All rights reserved.
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

#ifndef QQNXINPUTCONTEXT_H
#define QQNXINPUTCONTEXT_H

#include <qpa/qplatforminputcontext.h>
#include "qqnxscreeneventfilter.h"

#include <QtCore/QLocale>
#include <QtCore/QMetaType>
#include <QtCore/QList>
#include <qpa/qplatformintegration.h>

#include "imf/imf_client.h"
#include "imf/input_control.h"

QT_BEGIN_NAMESPACE

class QQnxAbstractVirtualKeyboard;
class QQnxIntegration;
class QQnxImfRequest;

class QQnxInputContext : public QPlatformInputContext, public QQnxScreenEventFilter
{
    Q_OBJECT
public:
    explicit QQnxInputContext(QQnxIntegration *integration, QQnxAbstractVirtualKeyboard &keyboard);
    ~QQnxInputContext();

    // Indices for selecting and setting highlight colors.
    enum HighlightIndex {
        ActiveRegion,
        AutoCorrected,
        Reverted,
    };

    bool isValid() const;

    bool filterEvent(const QEvent *event);
    QRectF keyboardRect() const;
    void reset();
    void commit();
    void update(Qt::InputMethodQueries);
    bool handleKeyboardEvent(int flags, int sym, int mod, int scan, int cap, int sequenceId);


    void showInputPanel();
    void hideInputPanel();
    bool isInputPanelVisible() const;

    QLocale locale() const;
    void setFocusObject(QObject *object);

    static void setHighlightColor(int index, const QColor &color);

    static bool checkSpelling(const QString &text, void *context, void (*spellCheckDone)(void *context, const QString &text, const QList<int> &indices));

private Q_SLOTS:
    void keyboardVisibilityChanged(bool visible);
    void keyboardLocaleChanged(const QLocale &locale);
    void processImfEvent(QQnxImfRequest *event);

private:
    // IMF Event dispatchers
    bool dispatchFocusGainEvent(int inputHints);
    void dispatchFocusLossEvent();
    bool dispatchRequestSoftwareInputPanel();
    bool dispatchCloseSoftwareInputPanel();
    int handleSpellCheck(spell_check_event_t *event);
    int32_t processEvent(event_t *event);

    void closeSession();
    bool openSession();
    bool hasSession();
    void updateCursorPosition();
    void endComposition();
    void finishComposingText();
    bool hasSelectedText();
    void updateComposition(spannable_string_t *text, int32_t new_cursor_position);

    // IMF Event handlers - these events will come in from QCoreApplication.
    int32_t onCommitText(spannable_string_t *text, int32_t new_cursor_position);
    int32_t onDeleteSurroundingText(int32_t left_length, int32_t right_length);
    int32_t onGetCursorCapsMode(int32_t req_modes);
    int32_t onFinishComposingText();
    int32_t onGetCursorPosition();
    spannable_string_t *onGetTextAfterCursor(int32_t n, int32_t flags);
    spannable_string_t *onGetTextBeforeCursor(int32_t n, int32_t flags);
    int32_t onSendEvent(event_t *event);
    int32_t onSetComposingRegion(int32_t start, int32_t end);
    int32_t onSetComposingText(spannable_string_t *text, int32_t new_cursor_position);
    int32_t onIsTextSelected(int32_t* pIsSelected);
    int32_t onIsAllTextSelected(int32_t* pIsSelected);
    int32_t onForceUpdate();

    int m_caretPosition;
    bool m_isComposing;
    QString m_composingText;
    bool m_isUpdatingText;
    bool m_inputPanelVisible;
    QLocale m_inputPanelLocale;
    // The object that had focus when the last highlight color was set.
    QObject *m_focusObject;
    // Indexed by HighlightIndex
    QColor m_highlightColor[3];
    QQnxIntegration *m_integration;
    QQnxAbstractVirtualKeyboard &m_virtualKeyboard;
};

QT_END_NAMESPACE

#endif // QQNXINPUTCONTEXT_H
