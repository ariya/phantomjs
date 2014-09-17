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

#ifndef QWIZARD_WIN_P_H
#define QWIZARD_WIN_P_H

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

#ifndef QT_NO_WIZARD
#ifndef QT_NO_STYLE_WINDOWSVISTA

#include <qt_windows.h>
#include <qobject.h>
#include <qwidget.h>
#include <qabstractbutton.h>
#include <QtGui/private/qwidget_p.h>
#include <QtGui/private/qstylehelper_p.h>

QT_BEGIN_NAMESPACE

class QVistaBackButton : public QAbstractButton
{
public:
    QVistaBackButton(QWidget *widget);

    QSize sizeHint() const;
    inline QSize minimumSizeHint() const
    { return sizeHint(); }

    void enterEvent(QEvent *event);
    void leaveEvent(QEvent *event);
    void paintEvent(QPaintEvent *event);
};

class QWizard;

class QVistaHelper : public QObject
{
public:
    QVistaHelper(QWizard *wizard);
    ~QVistaHelper();
    enum TitleBarChangeType { NormalTitleBar, ExtendedTitleBar };
    bool setDWMTitleBar(TitleBarChangeType type);
    void setTitleBarIconAndCaptionVisible(bool visible);
    void mouseEvent(QEvent *event);
    bool handleWinEvent(MSG *message, long *result);
    void resizeEvent(QResizeEvent *event);
    void paintEvent(QPaintEvent *event);
    QVistaBackButton *backButton() const { return backButton_; }
    void disconnectBackButton() { if (backButton_) backButton_->disconnect(); }
    void hideBackButton() { if (backButton_) backButton_->hide(); }
    void setWindowPosHack();
    QColor basicWindowFrameColor();
    enum VistaState { VistaAero, VistaBasic, Classic, Dirty };
    static VistaState vistaState();
    static int titleBarSize() { return frameSize() + captionSize(); }
    static int topPadding() { // padding under text
        return int(QStyleHelper::dpiScaled(
                QSysInfo::WindowsVersion >= QSysInfo::WV_WINDOWS7 ? 4 : 6));
    }
    static int topOffset() {
        static int aeroOffset = QSysInfo::WindowsVersion >= QSysInfo::WV_WINDOWS7 ?
                                QStyleHelper::dpiScaled(4) : QStyleHelper::dpiScaled(13);
        return (titleBarSize() + (vistaState() == VistaAero ? aeroOffset : 3)); }
private:
    static HFONT getCaptionFont(HANDLE hTheme);
    bool drawTitleText(QPainter *painter, const QString &text, const QRect &rect, HDC hdc);
    static bool drawBlackRect(const QRect &rect, HDC hdc);

    static int frameSize() { return GetSystemMetrics(SM_CYSIZEFRAME); }
    static int captionSize() { return GetSystemMetrics(SM_CYCAPTION); }

    static int backButtonSize() { return int(QStyleHelper::dpiScaled(30)); }
    static int iconSize() { return 16; } // Standard Aero
    static int glowSize() { return 10; }
    int leftMargin() { return backButton_->isVisible() ? backButtonSize() + iconSpacing : 0; }

    int titleOffset();
    bool resolveSymbols();
    void drawTitleBar(QPainter *painter);
    void setMouseCursor(QPoint pos);
    void collapseTopFrameStrut();
    bool winEvent(MSG *message, long *result);
    void mouseMoveEvent(QMouseEvent *event);
    void mousePressEvent(QMouseEvent *event);
    void mouseReleaseEvent(QMouseEvent *event);
    bool eventFilter(QObject *obj, QEvent *event);

    static int instanceCount;
    static bool is_vista;
    static VistaState cachedVistaState;
    static bool isCompositionEnabled();
    static bool isThemeActive();
    enum Changes { resizeTop, movePosition, noChange } change;
    QPoint pressedPos;
    bool pressed;
    QRect rtTop;
    QRect rtTitle;
    QWizard *wizard;
    QVistaBackButton *backButton_;

    int titleBarOffset;  // Extra spacing above the text
    int iconSpacing;    // Space between button and icon
    int textSpacing;    // Space between icon and text
};


QT_END_NAMESPACE

#endif // QT_NO_STYLE_WINDOWSVISTA
#endif // QT_NO_WIZARD
#endif // QWIZARD_WIN_P_H
