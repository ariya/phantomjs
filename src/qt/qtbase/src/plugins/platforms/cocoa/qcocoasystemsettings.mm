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

#include "qcocoasystemsettings.h"

#include "qcocoahelpers.h"

#include <QtCore/private/qcore_mac_p.h>
#include <QtGui/qfont.h>

#include <Carbon/Carbon.h>

QT_BEGIN_NAMESPACE

QColor qt_mac_colorForTheme(ThemeBrush brush)
{
    QCFType<CGColorRef> cgClr = 0;
    HIThemeBrushCreateCGColor(brush, &cgClr);
    return qt_mac_toQColor(cgClr);
}

QColor qt_mac_colorForThemeTextColor(ThemeTextColor themeColor)
{
    // No GetThemeTextColor in 64-bit mode, use hardcoded values:
    switch (themeColor) {
    case kThemeTextColorAlertActive:
    case kThemeTextColorTabFrontActive:
    case kThemeTextColorBevelButtonActive:
    case kThemeTextColorListView:
    case kThemeTextColorPlacardActive:
    case kThemeTextColorPopupButtonActive:
    case kThemeTextColorPopupLabelActive:
    case kThemeTextColorPushButtonActive:
        return Qt::black;
    case kThemeTextColorAlertInactive:
    case kThemeTextColorDialogInactive:
    case kThemeTextColorPlacardInactive:
    case kThemeTextColorPopupButtonInactive:
    case kThemeTextColorPopupLabelInactive:
    case kThemeTextColorPushButtonInactive:
    case kThemeTextColorTabFrontInactive:
    case kThemeTextColorBevelButtonInactive:
    case kThemeTextColorMenuItemDisabled:
        return QColor(127, 127, 127, 255);
    case kThemeTextColorMenuItemSelected:
        return Qt::white;
    default:
        return QColor(0, 0, 0, 255); // ### TODO: Sample color like Qt 4.
    }
}

QPalette * qt_mac_createSystemPalette()
{
    QColor qc;

    // Standard palette initialization (copied from Qt 4 styles)
    QColor background = QColor(237, 237, 237);
    QColor light(background.lighter(110));
    QColor dark(background.darker(160));
    QColor mid(background.darker(140));
    QPalette *palette = new QPalette(Qt::black, background, light, dark, mid, Qt::black, Qt::white);

    palette->setBrush(QPalette::Disabled, QPalette::WindowText, dark);
    palette->setBrush(QPalette::Disabled, QPalette::Text, dark);
    palette->setBrush(QPalette::Disabled, QPalette::ButtonText, dark);
    palette->setBrush(QPalette::Disabled, QPalette::Base, background);
    palette->setColor(QPalette::Disabled, QPalette::Dark, QColor(191, 191, 191));
    palette->setColor(QPalette::Active, QPalette::Dark, QColor(191, 191, 191));
    palette->setColor(QPalette::Inactive, QPalette::Dark, QColor(191, 191, 191));

    // System palette initialization:
    palette->setBrush(QPalette::Active, QPalette::Highlight, qt_mac_colorForTheme(kThemeBrushPrimaryHighlightColor));
    qc = qt_mac_colorForTheme(kThemeBrushSecondaryHighlightColor);
    palette->setBrush(QPalette::Inactive, QPalette::Highlight, qc);
    palette->setBrush(QPalette::Disabled, QPalette::Highlight, qc);

    palette->setBrush(QPalette::Shadow, background.darker(170));

    qc = qt_mac_colorForThemeTextColor(kThemeTextColorDialogActive);
    palette->setColor(QPalette::Active, QPalette::Text, qc);
    palette->setColor(QPalette::Active, QPalette::WindowText, qc);
    palette->setColor(QPalette::Active, QPalette::HighlightedText, qc);
    palette->setColor(QPalette::Inactive, QPalette::Text, qc);
    palette->setColor(QPalette::Inactive, QPalette::WindowText, qc);
    palette->setColor(QPalette::Inactive, QPalette::HighlightedText, qc);

    qc = qt_mac_colorForThemeTextColor(kThemeTextColorDialogInactive);
    palette->setColor(QPalette::Disabled, QPalette::Text, qc);
    palette->setColor(QPalette::Disabled, QPalette::WindowText, qc);
    palette->setColor(QPalette::Disabled, QPalette::HighlightedText, qc);
    palette->setBrush(QPalette::ToolTipBase, QColor(255, 255, 199));
    return palette;
}

struct QMacPaletteMap {
    inline QMacPaletteMap(QPlatformTheme::Palette p, ThemeBrush a, ThemeBrush i) :
        paletteRole(p), active(a), inactive(i) { }

    QPlatformTheme::Palette paletteRole;
    ThemeBrush active, inactive;
};

static QMacPaletteMap mac_widget_colors[] = {
    QMacPaletteMap(QPlatformTheme::ToolButtonPalette, kThemeTextColorBevelButtonActive, kThemeTextColorBevelButtonInactive),
    QMacPaletteMap(QPlatformTheme::ButtonPalette, kThemeTextColorPushButtonActive, kThemeTextColorPushButtonInactive),
    QMacPaletteMap(QPlatformTheme::HeaderPalette, kThemeTextColorPushButtonActive, kThemeTextColorPushButtonInactive),
    QMacPaletteMap(QPlatformTheme::ComboBoxPalette, kThemeTextColorPopupButtonActive, kThemeTextColorPopupButtonInactive),
    QMacPaletteMap(QPlatformTheme::ItemViewPalette, kThemeTextColorListView, kThemeTextColorDialogInactive),
    QMacPaletteMap(QPlatformTheme::MessageBoxLabelPalette, kThemeTextColorAlertActive, kThemeTextColorAlertInactive),
    QMacPaletteMap(QPlatformTheme::TabBarPalette, kThemeTextColorTabFrontActive, kThemeTextColorTabFrontInactive),
    QMacPaletteMap(QPlatformTheme::LabelPalette, kThemeTextColorPlacardActive, kThemeTextColorPlacardInactive),
    QMacPaletteMap(QPlatformTheme::GroupBoxPalette, kThemeTextColorPlacardActive, kThemeTextColorPlacardInactive),
    QMacPaletteMap(QPlatformTheme::MenuPalette, kThemeTextColorMenuItemActive, kThemeTextColorMenuItemDisabled),
    //### TODO: The zeros below gives white-on-black text.
    QMacPaletteMap(QPlatformTheme::TextEditPalette, 0, 0),
    QMacPaletteMap(QPlatformTheme::TextLineEditPalette, 0, 0),
    QMacPaletteMap(QPlatformTheme::NPalettes, 0, 0) };

QHash<QPlatformTheme::Palette, QPalette*> qt_mac_createRolePalettes()
{
    QHash<QPlatformTheme::Palette, QPalette*> palettes;
    QColor qc;
    for (int i = 0; mac_widget_colors[i].paletteRole != QPlatformTheme::NPalettes; i++) {
        QPalette &pal = *qt_mac_createSystemPalette();
        if (mac_widget_colors[i].active != 0) {
            qc = qt_mac_colorForThemeTextColor(mac_widget_colors[i].active);
            pal.setColor(QPalette::Active, QPalette::Text, qc);
            pal.setColor(QPalette::Inactive, QPalette::Text, qc);
            pal.setColor(QPalette::Active, QPalette::WindowText, qc);
            pal.setColor(QPalette::Inactive, QPalette::WindowText, qc);
            pal.setColor(QPalette::Active, QPalette::HighlightedText, qc);
            pal.setColor(QPalette::Inactive, QPalette::HighlightedText, qc);
            qc = qt_mac_colorForThemeTextColor(mac_widget_colors[i].inactive);
            pal.setColor(QPalette::Disabled, QPalette::Text, qc);
            pal.setColor(QPalette::Disabled, QPalette::WindowText, qc);
            pal.setColor(QPalette::Disabled, QPalette::HighlightedText, qc);
        }
        if (mac_widget_colors[i].paletteRole == QPlatformTheme::MenuPalette) {
            qc = qt_mac_colorForTheme(kThemeBrushMenuBackground);
            pal.setBrush(QPalette::Background, qc);
            qc = qt_mac_colorForThemeTextColor(kThemeTextColorMenuItemActive);
            pal.setBrush(QPalette::ButtonText, qc);
            qc = qt_mac_colorForThemeTextColor(kThemeTextColorMenuItemSelected);
            pal.setBrush(QPalette::HighlightedText, qc);
            qc = qt_mac_colorForThemeTextColor(kThemeTextColorMenuItemDisabled);
            pal.setBrush(QPalette::Disabled, QPalette::Text, qc);
        } else if ((mac_widget_colors[i].paletteRole == QPlatformTheme::ButtonPalette)
                || (mac_widget_colors[i].paletteRole == QPlatformTheme::HeaderPalette)) {
            pal.setColor(QPalette::Disabled, QPalette::ButtonText,
                         pal.color(QPalette::Disabled, QPalette::Text));
            pal.setColor(QPalette::Inactive, QPalette::ButtonText,
                         pal.color(QPalette::Inactive, QPalette::Text));
            pal.setColor(QPalette::Active, QPalette::ButtonText,
                         pal.color(QPalette::Active, QPalette::Text));
        } else if (mac_widget_colors[i].paletteRole == QPlatformTheme::ItemViewPalette) {
            pal.setBrush(QPalette::Active, QPalette::Highlight,
                         qt_mac_colorForTheme(kThemeBrushAlternatePrimaryHighlightColor));
            qc = qt_mac_colorForThemeTextColor(kThemeTextColorMenuItemSelected);
            pal.setBrush(QPalette::Active, QPalette::HighlightedText, qc);
            pal.setBrush(QPalette::Inactive, QPalette::Text,
                          pal.brush(QPalette::Active, QPalette::Text));
            pal.setBrush(QPalette::Inactive, QPalette::HighlightedText,
                          pal.brush(QPalette::Active, QPalette::Text));
        } else if (mac_widget_colors[i].paletteRole == QPlatformTheme::TextEditPalette) {
            pal.setBrush(QPalette::Inactive, QPalette::Text,
                          pal.brush(QPalette::Active, QPalette::Text));
            pal.setBrush(QPalette::Inactive, QPalette::HighlightedText,
                          pal.brush(QPalette::Active, QPalette::Text));
        } else if (mac_widget_colors[i].paletteRole == QPlatformTheme::TextLineEditPalette) {
            pal.setBrush(QPalette::Disabled, QPalette::Base,
                         pal.brush(QPalette::Active, QPalette::Base));
        }
        palettes.insert(mac_widget_colors[i].paletteRole, &pal);
    }
    return palettes;
}

QT_END_NAMESPACE
