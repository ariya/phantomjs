/****************************************************************************
**
** Copyright (C) 2012 BogDan Vatra <bogdan@kde.org>
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

#include "qandroidplatformtheme.h"
#include "qandroidplatformmenubar.h"
#include "qandroidplatformmenu.h"
#include "qandroidplatformmenuitem.h"
#include "qandroidplatformdialoghelpers.h"
#include <QVariant>
#include <QFileInfo>
#include <QCoreApplication>
#include <private/qguiapplication_p.h>
#include <qandroidplatformintegration.h>

QAndroidPlatformTheme::QAndroidPlatformTheme(QAndroidPlatformNativeInterface *androidPlatformNativeInterface)
{
    m_androidPlatformNativeInterface = androidPlatformNativeInterface;
    QColor background(229, 229, 229);
    QColor light = background.lighter(150);
    QColor mid(background.darker(130));
    QColor midLight = mid.lighter(110);
    QColor base(249, 249, 249);
    QColor disabledBase(background);
    QColor dark = background.darker(150);
    QColor darkDisabled = dark.darker(110);
    QColor text = Qt::black;
    QColor highlightedText = Qt::black;
    QColor disabledText = QColor(190, 190, 190);
    QColor button(241, 241, 241);
    QColor shadow(201, 201, 201);
    QColor highlight(148, 210, 231);
    QColor disabledShadow = shadow.lighter(150);

    m_defaultPalette = QPalette(Qt::black,background,light,dark,mid,text,base);
    m_defaultPalette.setBrush(QPalette::Midlight, midLight);
    m_defaultPalette.setBrush(QPalette::Button, button);
    m_defaultPalette.setBrush(QPalette::Shadow, shadow);
    m_defaultPalette.setBrush(QPalette::HighlightedText, highlightedText);

    m_defaultPalette.setBrush(QPalette::Disabled, QPalette::Text, disabledText);
    m_defaultPalette.setBrush(QPalette::Disabled, QPalette::WindowText, disabledText);
    m_defaultPalette.setBrush(QPalette::Disabled, QPalette::ButtonText, disabledText);
    m_defaultPalette.setBrush(QPalette::Disabled, QPalette::Base, disabledBase);
    m_defaultPalette.setBrush(QPalette::Disabled, QPalette::Dark, darkDisabled);
    m_defaultPalette.setBrush(QPalette::Disabled, QPalette::Shadow, disabledShadow);

    m_defaultPalette.setBrush(QPalette::Active, QPalette::Highlight, highlight);
    m_defaultPalette.setBrush(QPalette::Inactive, QPalette::Highlight, highlight);
    m_defaultPalette.setBrush(QPalette::Disabled, QPalette::Highlight, highlight.lighter(150));
}

QPlatformMenuBar *QAndroidPlatformTheme::createPlatformMenuBar() const
{
    return new QAndroidPlatformMenuBar;
}

QPlatformMenu *QAndroidPlatformTheme::createPlatformMenu() const
{
    return new QAndroidPlatformMenu;
}

QPlatformMenuItem *QAndroidPlatformTheme::createPlatformMenuItem() const
{
    return new QAndroidPlatformMenuItem;
}

static inline int paletteType(QPlatformTheme::Palette type)
{
    switch (type) {
    case QPlatformTheme::ToolButtonPalette:
    case QPlatformTheme::ButtonPalette:
        return QPlatformTheme::ButtonPalette;

    case QPlatformTheme::CheckBoxPalette:
        return QPlatformTheme::CheckBoxPalette;

    case QPlatformTheme::RadioButtonPalette:
        return QPlatformTheme::RadioButtonPalette;

    case QPlatformTheme::ComboBoxPalette:
        return QPlatformTheme::ComboBoxPalette;

    case QPlatformTheme::TextEditPalette:
    case QPlatformTheme::TextLineEditPalette:
        return QPlatformTheme::TextLineEditPalette;

    case QPlatformTheme::ItemViewPalette:
        return QPlatformTheme::ItemViewPalette;

    default:
        return QPlatformTheme::SystemPalette;
    }
}

const QPalette *QAndroidPlatformTheme::palette(Palette type) const
{
    QHash<int, QPalette>::const_iterator it = m_androidPlatformNativeInterface->m_palettes.find(paletteType(type));
    if (it != m_androidPlatformNativeInterface->m_palettes.end())
        return &(it.value());
    return &m_defaultPalette;
}

static inline int fontType(QPlatformTheme::Font type)
{
    switch (type) {
    case QPlatformTheme::LabelFont:
        return QPlatformTheme::SystemFont;

    case QPlatformTheme::ToolButtonFont:
        return QPlatformTheme::PushButtonFont;

    default:
        return type;
    }
}

const QFont *QAndroidPlatformTheme::font(Font type) const
{
    QHash<int, QFont>::const_iterator it = m_androidPlatformNativeInterface->m_fonts.find(fontType(type));
    if (it != m_androidPlatformNativeInterface->m_fonts.end())
        return &(it.value());

    // default in case the style has not set a font
    static QFont systemFont("Roboto", 14.0 * 100 / 72); // keep default size the same after changing from 100 dpi to 72 dpi
    if (type == QPlatformTheme::SystemFont)
        return &systemFont;
    return 0;
}

static const QLatin1String STYLES_PATH("/data/data/org.kde.necessitas.ministro/files/dl/style/");
static const QLatin1String STYLE_FILE("/style.json");

QVariant QAndroidPlatformTheme::themeHint(ThemeHint hint) const
{
    switch (hint) {
    case StyleNames:
        if (qgetenv("QT_USE_ANDROID_NATIVE_STYLE").toInt()
                && (!qgetenv("MINISTRO_ANDROID_STYLE_PATH").isEmpty()
                    || QFileInfo(STYLES_PATH
                                 + QLatin1String(qgetenv("QT_ANDROID_THEME_DISPLAY_DPI"))
                                 + STYLE_FILE).exists())) {
            return QStringList("android");
        }
        return QStringList("fusion");

    case MouseDoubleClickDistance:
    {
            int minimumDistance = qgetenv("QT_ANDROID_MINIMUM_MOUSE_DOUBLE_CLICK_DISTANCE").toInt();
            int ret = minimumDistance;

            QAndroidPlatformIntegration *platformIntegration
                    = static_cast<QAndroidPlatformIntegration *>(QGuiApplicationPrivate::platformIntegration());
            QAndroidPlatformScreen *platformScreen = platformIntegration->screen();
            if (platformScreen != 0) {
                QScreen *screen = platformScreen->screen();
                qreal dotsPerInch = screen->physicalDotsPerInch();

                // Allow 15% of an inch between clicks when double clicking
                int distance = qRound(dotsPerInch * 0.15);
                if (distance > minimumDistance)
                    ret = distance;
            }

            if (ret > 0)
                return ret;

            // fall through
    }
    default:
        return QPlatformTheme::themeHint(hint);
    }
}

QString QAndroidPlatformTheme::standardButtonText(int button) const
{
    switch (button) {
    case QPlatformDialogHelper::Yes:
        return QCoreApplication::translate("QAndroidPlatformTheme", "Yes");
    case QPlatformDialogHelper::YesToAll:
        return QCoreApplication::translate("QAndroidPlatformTheme", "Yes to All");
    case QPlatformDialogHelper::No:
        return QCoreApplication::translate("QAndroidPlatformTheme", "No");
    case QPlatformDialogHelper::NoToAll:
        return QCoreApplication::translate("QAndroidPlatformTheme", "No to All");
    }
    return QPlatformTheme::standardButtonText(button);
}

bool QAndroidPlatformTheme::usePlatformNativeDialog(QPlatformTheme::DialogType type) const
{
    if (type == MessageDialog)
        return qgetenv("QT_USE_ANDROID_NATIVE_DIALOGS").toInt() == 1;
    return false;
}

QPlatformDialogHelper *QAndroidPlatformTheme::createPlatformDialogHelper(QPlatformTheme::DialogType type) const
{
    switch (type) {
    case MessageDialog:
        return new QtAndroidDialogHelpers::QAndroidPlatformMessageDialogHelper;
    default:
        return 0;
    }
}
