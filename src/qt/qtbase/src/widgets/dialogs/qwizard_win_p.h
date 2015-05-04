/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtWidgets module of the Qt Toolkit.
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

#include <QtCore/qglobal.h>

#ifndef QT_NO_WIZARD
#ifndef QT_NO_STYLE_WINDOWSVISTA

#include <qobject.h>
#include <qwidget.h>
#include <qabstractbutton.h>
#include <QtWidgets/private/qwidget_p.h>
#include <QtWidgets/private/qstylehelper_p.h>
#include <qt_windows.h>

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
    void updateCustomMargins(bool vistaMargins);
    bool setDWMTitleBar(TitleBarChangeType type);
    void setTitleBarIconAndCaptionVisible(bool visible);
    void mouseEvent(QEvent *event);
    bool handleWinEvent(MSG *message, long *result);
    void resizeEvent(QResizeEvent *event);
    void paintEvent(QPaintEvent *event);
    QVistaBackButton *backButton() const { return backButton_; }
    void disconnectBackButton();
    void hideBackButton() { if (backButton_) backButton_->hide(); }
    QColor basicWindowFrameColor();
    enum VistaState { VistaAero, VistaBasic, Classic, Dirty };
    static VistaState vistaState();
    static int titleBarSize() { return QVistaHelper::titleBarSizeDp() / QVistaHelper::m_devicePixelRatio; }
    static int titleBarSizeDp() { return QVistaHelper::frameSizeDp() + QVistaHelper::captionSizeDp(); }
    static int topPadding() { // padding under text
        return int(QStyleHelper::dpiScaled(
                QSysInfo::WindowsVersion >= QSysInfo::WV_WINDOWS7 ? 4 : 6));
    }
    static int topOffset();

    static HDC backingStoreDC(const QWidget *wizard, QPoint *offset);

private:
    static HFONT getCaptionFont(HANDLE hTheme);
    HWND wizardHWND() const;
    bool drawTitleText(QPainter *painter, const QString &text, const QRect &rect, HDC hdc);
    static bool drawBlackRect(const QRect &rect, HDC hdc);

    static int frameSize() { return QVistaHelper::frameSizeDp() / QVistaHelper::m_devicePixelRatio; }
    static int frameSizeDp();
    static int captionSize() { return QVistaHelper::captionSizeDp() / QVistaHelper::m_devicePixelRatio; }
    static int captionSizeDp();

    static int backButtonSize() { return int(QStyleHelper::dpiScaled(30)); }
    static int iconSize();
    static int glowSize();
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
    static int m_devicePixelRatio;
};


QT_END_NAMESPACE

#endif // QT_NO_STYLE_WINDOWSVISTA
#endif // QT_NO_WIZARD
#endif // QWIZARD_WIN_P_H
