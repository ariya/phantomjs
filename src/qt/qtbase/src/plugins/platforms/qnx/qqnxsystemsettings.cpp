/***************************************************************************
**
** Copyright (C) 2012 Research In Motion
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

#include "qqnxsystemsettings.h"

#include <QFont>
#include <qpa/qplatformfontdatabase.h>

QT_BEGIN_NAMESPACE

QHash<QPlatformTheme::Font, QFont *> qt_qnx_createRoleFonts(QPlatformFontDatabase *fontDatabase)
{
    // See http://docs.blackberry.com/en/developers/deliverables/41577/typography.jsp
    // which recommends using
    // - small font size of 6 points
    // - normal font size of 8 points
    // - 11 points for titles (not covered by the theme system).
    QFont baseFont = fontDatabase->defaultFont();
    baseFont.setPointSize(8);

    QHash<QPlatformTheme::Font, QFont *> fonts;
    fonts.insert(QPlatformTheme::SystemFont, new QFont(baseFont));
    fonts.insert(QPlatformTheme::PushButtonFont, new QFont(baseFont));
    fonts.insert(QPlatformTheme::ListViewFont, new QFont(baseFont));
    fonts.insert(QPlatformTheme::ListBoxFont, new QFont(baseFont));
    fonts.insert(QPlatformTheme::TitleBarFont, new QFont(baseFont));
    fonts.insert(QPlatformTheme::MenuFont, new QFont(baseFont));
    fonts.insert(QPlatformTheme::ComboMenuItemFont, new QFont(baseFont));
    fonts.insert(QPlatformTheme::HeaderViewFont, new QFont(baseFont));
    fonts.insert(QPlatformTheme::TipLabelFont, new QFont(baseFont));
    fonts.insert(QPlatformTheme::LabelFont, new QFont(baseFont));
    fonts.insert(QPlatformTheme::ToolButtonFont, new QFont(baseFont));
    fonts.insert(QPlatformTheme::MenuItemFont, new QFont(baseFont));
    fonts.insert(QPlatformTheme::ComboLineEditFont, new QFont(baseFont));

    QFont smallFont(baseFont);
    smallFont.setPointSize(6);
    fonts.insert(QPlatformTheme::SmallFont, new QFont(smallFont));
    fonts.insert(QPlatformTheme::MiniFont, new QFont(smallFont));

    return fonts;
}

QT_END_NAMESPACE
