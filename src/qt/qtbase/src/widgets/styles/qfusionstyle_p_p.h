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

#ifndef QFUSIONSTYLE_P_P_H
#define QFUSIONSTYLE_P_P_H

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

#include "qcommonstyle.h"
#include "qcommonstyle_p.h"
#include <qpa/qplatformtheme.h>
#include "private/qguiapplication_p.h"

#ifndef QT_NO_STYLE_FUSION

QT_BEGIN_NAMESPACE

class QFusionStylePrivate : public QCommonStylePrivate
{
    Q_DECLARE_PUBLIC(QFusionStyle)

public:
    QFusionStylePrivate();

    // Used for grip handles
    QColor lightShade() const {
        return QColor(255, 255, 255, 90);
    }
    QColor darkShade() const {
        return QColor(0, 0, 0, 60);
    }

    QColor topShadow() const {
        return QColor(0, 0, 0, 18);
    }

    QColor innerContrastLine() const {
        return QColor(255, 255, 255, 30);
    }

    // On mac we want a standard blue color used when the system palette is used
    bool isMacSystemPalette(const QPalette &pal) const {
        Q_UNUSED(pal);
#if defined(Q_OS_MACX)
        const QPalette *themePalette = QGuiApplicationPrivate::platformTheme()->palette();
        if (themePalette && themePalette->color(QPalette::Normal, QPalette::Highlight) ==
                pal.color(QPalette::Normal, QPalette::Highlight) &&
            themePalette->color(QPalette::Normal, QPalette::HighlightedText) ==
                pal.color(QPalette::Normal, QPalette::HighlightedText))
            return true;
#endif
        return false;
    }

    QColor highlight(const QPalette &pal) const {
        if (isMacSystemPalette(pal))
            return QColor(60, 140, 230);
        return pal.color(QPalette::Highlight);
    }

    QColor highlightedText(const QPalette &pal) const {
        if (isMacSystemPalette(pal))
            return Qt::white;
        return pal.color(QPalette::HighlightedText);
    }

    QColor outline(const QPalette &pal) const {
        if (!pal.window().texture().isNull())
            return QColor(0, 0, 0, 160);
        return pal.background().color().darker(140);
    }

    QColor highlightedOutline(const QPalette &pal) const {
        QColor highlightedOutline = highlight(pal).darker(125);
        if (highlightedOutline.value() > 160)
            highlightedOutline.setHsl(highlightedOutline.hue(), highlightedOutline.saturation(), 160);
        return highlightedOutline;
    }

    QColor tabFrameColor(const QPalette &pal) const {
        if (!pal.button().texture().isNull())
            return QColor(255, 255, 255, 8);
        return buttonColor(pal).lighter(104);
    }

    QColor buttonColor(const QPalette &pal) const {
        QColor buttonColor = pal.button().color();
        int val = qGray(buttonColor.rgb());
        buttonColor = buttonColor.lighter(100 + qMax(1, (180 - val)/6));
        buttonColor.setHsv(buttonColor.hue(), buttonColor.saturation() * 0.75, buttonColor.value());
        return buttonColor;
    }

    enum {
        menuItemHMargin      =  3, // menu item hor text margin
        menuArrowHMargin     =  6, // menu arrow horizontal margin
        menuRightBorder      = 15, // right border on menus
        menuCheckMarkWidth   = 12  // checkmarks width on menus
    };
};

QT_END_NAMESPACE

#endif // QT_NO_STYLE_FUSION

#endif //QFUSIONSTYLE_P_P_H
