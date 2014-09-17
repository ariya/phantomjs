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


#ifndef QMACSTYLE_MAC_P_H
#define QMACSTYLE_MAC_P_H

#include <qmacstyle_mac.h>
#include <private/qapplication_p.h>
#include <private/qcombobox_p.h>
#include <private/qmacstylepixmaps_mac_p.h>
#include <private/qpaintengine_mac_p.h>
#include <private/qpainter_p.h>
#include <private/qprintengine_mac_p.h>
#include <private/qstylehelper_p.h>
#include <qapplication.h>
#include <qbitmap.h>
#include <qcheckbox.h>
#include <qcombobox.h>
#include <qdialogbuttonbox.h>
#include <qdockwidget.h>
#include <qevent.h>
#include <qfocusframe.h>
#include <qformlayout.h>
#include <qgroupbox.h>
#include <qhash.h>
#include <qheaderview.h>
#include <qlayout.h>
#include <qlineedit.h>
#include <qlistview.h>
#include <qmainwindow.h>
#include <qmap.h>
#include <qmenubar.h>
#include <qpaintdevice.h>
#include <qpainter.h>
#include <qpixmapcache.h>
#include <qpointer.h>
#include <qprogressbar.h>
#include <qpushbutton.h>
#include <qradiobutton.h>
#include <qrubberband.h>
#include <qsizegrip.h>
#include <qspinbox.h>
#include <qsplitter.h>
#include <qstyleoption.h>
#include <qtextedit.h>
#include <qtextstream.h>
#include <qtoolbar.h>
#include <qtoolbutton.h>
#include <qtreeview.h>
#include <qtableview.h>
#include <qwizard.h>
#include <qdebug.h>
#include <qlibrary.h>
#include <qdatetimeedit.h>
#include <qmath.h>
#include <QtGui/qgraphicsproxywidget.h>
#include <QtGui/qgraphicsview.h>
#include <private/qt_cocoa_helpers_mac_p.h>

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

QT_BEGIN_NAMESPACE

#if (MAC_OS_X_VERSION_MAX_ALLOWED <= MAC_OS_X_VERSION_10_5)
enum {
    kThemePushButtonTextured = 31,
    kThemePushButtonTexturedSmall = 32,
    kThemePushButtonTexturedMini = 33
};

/* Search fields */
enum {
    kHIThemeFrameTextFieldRound = 1000,
    kHIThemeFrameTextFieldRoundSmall = 1001,
    kHIThemeFrameTextFieldRoundMini = 1002
};
#endif

/*
    AHIG:
        Apple Human Interface Guidelines
        http://developer.apple.com/documentation/UserExperience/Conceptual/OSXHIGuidelines/

    Builder:
        Apple Interface Builder v. 3.1.1
*/

// this works as long as we have at most 16 different control types
#define CT1(c) CT2(c, c)
#define CT2(c1, c2) ((uint(c1) << 16) | uint(c2))

enum QAquaWidgetSize { QAquaSizeLarge = 0, QAquaSizeSmall = 1, QAquaSizeMini = 2,
                       QAquaSizeUnknown = -1 };

#define SIZE(large, small, mini) \
    (controlSize == QAquaSizeLarge ? (large) : controlSize == QAquaSizeSmall ? (small) : (mini))

// same as return SIZE(...) but optimized
#define return_SIZE(large, small, mini) \
    do { \
        static const int sizes[] = { (large), (small), (mini) }; \
        return sizes[controlSize]; \
    } while (0)

bool qt_mac_buttonIsRenderedFlat(const QPushButton *pushButton, const QStyleOptionButton *option);

class QMacStylePrivate : public QObject
{
    Q_OBJECT

public:
    QMacStylePrivate(QMacStyle *style);

    // Ideally these wouldn't exist, but since they already exist we need some accessors.
    static const int PushButtonLeftOffset;
    static const int PushButtonTopOffset;
    static const int PushButtonRightOffset;
    static const int PushButtonBottomOffset;
    static const int MiniButtonH;
    static const int SmallButtonH;
    static const int BevelButtonW;
    static const int BevelButtonH;
    static const int PushButtonContentPadding;


    // Stuff from QAquaAnimate:
    bool addWidget(QWidget *);
    void removeWidget(QWidget *);

    enum Animates { AquaPushButton, AquaProgressBar, AquaListViewItemOpen };
    bool animatable(Animates, const QWidget *) const;
    void stopAnimate(Animates, QWidget *);
    void startAnimate(Animates, QWidget *);
    static ThemeDrawState getDrawState(QStyle::State flags);
    QAquaWidgetSize aquaSizeConstrain(const QStyleOption *option, const QWidget *widg,
                             QStyle::ContentsType ct = QStyle::CT_CustomBase,
                             QSize szHint=QSize(-1, -1), QSize *insz = 0) const;
    void getSliderInfo(QStyle::ComplexControl cc, const QStyleOptionSlider *slider,
                          HIThemeTrackDrawInfo *tdi, const QWidget *needToRemoveMe);
    bool doAnimate(Animates);
    inline int animateSpeed(Animates) const { return 33; }

    // Utility functions
    void drawColorlessButton(const HIRect &macRect, HIThemeButtonDrawInfo *bdi,
                             QPainter *p, const QStyleOption *opt) const;

    QSize pushButtonSizeFromContents(const QStyleOptionButton *btn) const;

    HIRect pushButtonContentBounds(const QStyleOptionButton *btn,
                                   const HIThemeButtonDrawInfo *bdi) const;

    void initComboboxBdi(const QStyleOptionComboBox *combo, HIThemeButtonDrawInfo *bdi,
                        const QWidget *widget, const ThemeDrawState &tds);

    static HIRect comboboxInnerBounds(const HIRect &outerBounds, int buttonKind);

    static QRect comboboxEditBounds(const QRect &outerBounds, const HIThemeButtonDrawInfo &bdi);

    static void drawCombobox(const HIRect &outerBounds, const HIThemeButtonDrawInfo &bdi, QPainter *p);
    static void drawTableHeader(const HIRect &outerBounds, bool drawTopBorder, bool drawLeftBorder,
                                     const HIThemeButtonDrawInfo &bdi, QPainter *p);
    bool contentFitsInPushButton(const QStyleOptionButton *btn, HIThemeButtonDrawInfo *bdi,
                                 ThemeButtonKind buttonKindToCheck) const;
    void initHIThemePushButton(const QStyleOptionButton *btn, const QWidget *widget,
                               const ThemeDrawState tds,
                               HIThemeButtonDrawInfo *bdi) const;
    QPixmap generateBackgroundPattern() const;
protected:
    bool eventFilter(QObject *, QEvent *);
    void timerEvent(QTimerEvent *);

private slots:
    void startAnimationTimer();

public:
    QPointer<QPushButton> defaultButton; //default push buttons
    int timerID;
    QList<QPointer<QWidget> > progressBars; //existing progress bars that need animation

    struct ButtonState {
        int frame;
        enum { ButtonDark, ButtonLight } dir;
    } buttonState;
    UInt8 progressFrame;
    QPointer<QFocusFrame> focusWidget;
    CFAbsoluteTime defaultButtonStart;
    QMacStyle *q;
    bool mouseDown;
};

QT_END_NAMESPACE

#endif // QMACSTYLE_MAC_P_H
