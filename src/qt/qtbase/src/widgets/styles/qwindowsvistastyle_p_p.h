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

#ifndef QWINDOWSVISTASTYLE_P_P_H
#define QWINDOWSVISTASTYLE_P_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists for the convenience
// of qapplication_*.cpp, qwidget*.cpp and qfiledialog.cpp.  This header
// file may change from version to version without notice, or even be removed.
//
// We mean it.
//

#include "qwindowsvistastyle_p.h"

#if !defined(QT_NO_STYLE_WINDOWSVISTA)
#include <private/qwindowsxpstyle_p_p.h>
#include <private/qstyleanimation_p.h>
#include <private/qpaintengine_raster_p.h>
#include <qlibrary.h>
#include <qpaintengine.h>
#include <qwidget.h>
#include <qapplication.h>
#include <qpixmapcache.h>
#include <qstyleoption.h>
#include <qpushbutton.h>
#include <qradiobutton.h>
#include <qcheckbox.h>
#include <qlineedit.h>
#include <qgroupbox.h>
#include <qtoolbutton.h>
#include <qspinbox.h>
#include <qtoolbar.h>
#include <qcombobox.h>
#include <qscrollbar.h>
#include <qprogressbar.h>
#include <qdockwidget.h>
#include <qlistview.h>
#include <qtreeview.h>
#include <qtextedit.h>
#include <qmessagebox.h>
#include <qdialogbuttonbox.h>
#include <qinputdialog.h>
#include <qtableview.h>
#include <qdatetime.h>
#include <qcommandlinkbutton.h>

QT_BEGIN_NAMESPACE

#if !defined(SCHEMA_VERIFY_VSSYM32)
#define TMT_ANIMATIONDURATION       5006
#define TMT_TRANSITIONDURATIONS     6000
#define EP_EDITBORDER_NOSCROLL      6
#define EP_EDITBORDER_HVSCROLL      9
#define EP_BACKGROUND               3
#define EBS_NORMAL                  1
#define EBS_HOT                     2
#define EBS_DISABLED                3
#define EBS_READONLY                5
#define PBS_DEFAULTED_ANIMATING     6
#define MBI_NORMAL                  1
#define MBI_HOT                     2
#define MBI_PUSHED                  3
#define MBI_DISABLED                4
#define MB_ACTIVE                   1
#define MB_INACTIVE                 2
#define PP_FILL                     5
#define PP_FILLVERT                 6
#define PP_MOVEOVERLAY              8
#define PP_MOVEOVERLAYVERT          10
#define MENU_BARBACKGROUND          7
#define MENU_BARITEM                8
#define MENU_POPUPCHECK             11
#define MENU_POPUPCHECKBACKGROUND   12
#define MENU_POPUPGUTTER            13
#define MENU_POPUPITEM              14
#define MENU_POPUPBORDERS           10
#define MENU_POPUPSEPARATOR         15
#define MC_CHECKMARKNORMAL          1
#define MC_CHECKMARKDISABLED        2
#define MC_BULLETNORMAL             3
#define MC_BULLETDISABLED           4
#define ABS_UPHOVER                 17
#define ABS_DOWNHOVER               18
#define ABS_LEFTHOVER               19
#define ABS_RIGHTHOVER              20
#define CP_DROPDOWNBUTTONRIGHT      6
#define CP_DROPDOWNBUTTONLEFT       7
#define SCRBS_HOVER                 5
#define TVP_HOTGLYPH                4
#define SPI_GETCLIENTAREAANIMATION  0x1042
#define TDLG_PRIMARYPANEL           1
#define TDLG_SECONDARYPANEL         8
#endif

class QWindowsVistaAnimation : public QBlendStyleAnimation
{
    Q_OBJECT
public:
    QWindowsVistaAnimation(Type type, QObject *target) : QBlendStyleAnimation(type, target) { }

    virtual bool isUpdateNeeded() const;
    void paint(QPainter *painter, const QStyleOption *option);
};


// Handles state transition animations
class QWindowsVistaTransition : public QWindowsVistaAnimation
{
    Q_OBJECT
public:
    QWindowsVistaTransition(QObject *target) : QWindowsVistaAnimation(Transition, target) {}
};


// Handles pulse animations (default buttons)
class QWindowsVistaPulse: public QWindowsVistaAnimation
{
    Q_OBJECT
public:
    QWindowsVistaPulse(QObject *target) : QWindowsVistaAnimation(Pulse, target) {}
};


class QWindowsVistaStylePrivate :  public QWindowsXPStylePrivate
{
    Q_DECLARE_PUBLIC(QWindowsVistaStyle)

public:
    QWindowsVistaStylePrivate();
    ~QWindowsVistaStylePrivate();
    static int fixedPixelMetric(QStyle::PixelMetric pm);
    static inline bool useVista();
    bool transitionsEnabled() const;

private:
    bool initTreeViewTheming();
    void cleanupTreeViewTheming();

    HWND m_treeViewHelper;
};

QT_END_NAMESPACE

#endif // QT_NO_STYLE_WINDOWSVISTA

#endif // QWINDOWSVISTASTYLE_P_P_H
