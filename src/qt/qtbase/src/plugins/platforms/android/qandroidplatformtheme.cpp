/****************************************************************************
**
** Copyright (C) 2012 BogDan Vatra <bogdan@kde.org>
** Contact: http://www.qt-project.org/legal
**
** This file is part of the plugins of the Qt Toolkit.
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

#include "androidjnimenu.h"
#include "qandroidplatformtheme.h"
#include "qandroidplatformmenubar.h"
#include "qandroidplatformmenu.h"
#include "qandroidplatformmenuitem.h"
#include "qandroidplatformdialoghelpers.h"

#include <QCoreApplication>
#include <QDebug>
#include <QFileInfo>
#include <QJsonDocument>
#include <QVariant>

#include <private/qguiapplication_p.h>
#include <qandroidplatformintegration.h>

QT_BEGIN_NAMESPACE

namespace {
    const int textStyle_bold = 1;
    const int textStyle_italic = 2;

    const int typeface_sans = 1;
    const int typeface_serif = 2;
    const int typeface_monospace = 3;
}

static int fontType(const QString &androidControl)
{
    if (androidControl == QLatin1String("defaultStyle"))
        return QPlatformTheme::SystemFont;
    if (androidControl == QLatin1String("textViewStyle"))
        return QPlatformTheme::LabelFont;
    else if (androidControl == QLatin1String("buttonStyle"))
        return QPlatformTheme::PushButtonFont;
    else if (androidControl == QLatin1String("checkboxStyle"))
        return QPlatformTheme::CheckBoxFont;
    else if (androidControl == QLatin1String("radioButtonStyle"))
        return QPlatformTheme::RadioButtonFont;
    else if (androidControl == QLatin1String("simple_list_item_single_choice"))
        return QPlatformTheme::ItemViewFont;
    else if (androidControl == QLatin1String("simple_spinner_dropdown_item"))
        return QPlatformTheme::ComboMenuItemFont;
    else if (androidControl == QLatin1String("spinnerStyle"))
        return QPlatformTheme::ComboLineEditFont;
    else if (androidControl == QLatin1String("simple_list_item"))
        return QPlatformTheme::ListViewFont;
    return -1;
}

static int paletteType(const QString &androidControl)
{
    if (androidControl == QLatin1String("defaultStyle"))
        return QPlatformTheme::SystemPalette;
    if (androidControl == QLatin1String("textViewStyle"))
        return QPlatformTheme::LabelPalette;
    else if (androidControl == QLatin1String("buttonStyle"))
        return QPlatformTheme::ButtonPalette;
    else if (androidControl == QLatin1String("checkboxStyle"))
        return QPlatformTheme::CheckBoxPalette;
    else if (androidControl == QLatin1String("radioButtonStyle"))
        return QPlatformTheme::RadioButtonPalette;
    else if (androidControl == QLatin1String("simple_list_item_single_choice"))
        return QPlatformTheme::ItemViewPalette;
    else if (androidControl == QLatin1String("editTextStyle"))
        return QPlatformTheme::TextLineEditPalette;
    else if (androidControl == QLatin1String("spinnerStyle"))
        return QPlatformTheme::ComboBoxPalette;
    return -1;
}

static void setPaletteColor(const QVariantMap &object,
                                    QPalette &palette,
                                    QPalette::ColorRole role)
{
    // QPalette::Active -> ENABLED_FOCUSED_WINDOW_FOCUSED_STATE_SET
    palette.setColor(QPalette::Active,
                     role,
                     QRgb(object.value(QLatin1String("ENABLED_FOCUSED_WINDOW_FOCUSED_STATE_SET")).toInt()));

    // QPalette::Inactive -> ENABLED_STATE_SET
    palette.setColor(QPalette::Inactive,
                     role,
                     QRgb(object.value(QLatin1String("ENABLED_STATE_SET")).toInt()));

    // QPalette::Disabled -> EMPTY_STATE_SET
    palette.setColor(QPalette::Disabled,
                     role,
                     QRgb(object.value(QLatin1String("EMPTY_STATE_SET")).toInt()));

    palette.setColor(QPalette::Current, role, palette.color(QPalette::Active, role));

    if (role == QPalette::WindowText) {
        // QPalette::BrightText -> PRESSED
        // QPalette::Active -> PRESSED_ENABLED_FOCUSED_WINDOW_FOCUSED_STATE_SET
        palette.setColor(QPalette::Active,
                         QPalette::BrightText,
                         QRgb(object.value(QLatin1String("PRESSED_ENABLED_FOCUSED_WINDOW_FOCUSED_STATE_SET")).toInt()));

        // QPalette::Inactive -> PRESSED_ENABLED_STATE_SET
        palette.setColor(QPalette::Inactive,
                         QPalette::BrightText,
                         QRgb(object.value(QLatin1String("PRESSED_ENABLED_STATE_SET")).toInt()));

        // QPalette::Disabled -> PRESSED_STATE_SET
        palette.setColor(QPalette::Disabled,
                         QPalette::BrightText,
                         QRgb(object.value(QLatin1String("PRESSED_STATE_SET")).toInt()));

        palette.setColor(QPalette::Current, QPalette::BrightText, palette.color(QPalette::Active, QPalette::BrightText));

        // QPalette::HighlightedText -> SELECTED
        // QPalette::Active -> ENABLED_SELECTED_WINDOW_FOCUSED_STATE_SET
        palette.setColor(QPalette::Active,
                         QPalette::HighlightedText,
                         QRgb(object.value(QLatin1String("ENABLED_SELECTED_WINDOW_FOCUSED_STATE_SET")).toInt()));

        // QPalette::Inactive -> ENABLED_SELECTED_STATE_SET
        palette.setColor(QPalette::Inactive,
                         QPalette::HighlightedText,
                         QRgb(object.value(QLatin1String("ENABLED_SELECTED_STATE_SET")).toInt()));

        // QPalette::Disabled -> SELECTED_STATE_SET
        palette.setColor(QPalette::Disabled,
                         QPalette::HighlightedText,
                         QRgb(object.value(QLatin1String("SELECTED_STATE_SET")).toInt()));

        palette.setColor(QPalette::Current,
                         QPalette::HighlightedText,
                         palette.color(QPalette::Active, QPalette::HighlightedText));

        // Same colors for Text
        palette.setColor(QPalette::Active, QPalette::Text, palette.color(QPalette::Active, role));
        palette.setColor(QPalette::Inactive, QPalette::Text, palette.color(QPalette::Inactive, role));
        palette.setColor(QPalette::Disabled, QPalette::Text, palette.color(QPalette::Disabled, role));
        palette.setColor(QPalette::Current, QPalette::Text, palette.color(QPalette::Current, role));

        // And for ButtonText
        palette.setColor(QPalette::Active, QPalette::ButtonText, palette.color(QPalette::Active, role));
        palette.setColor(QPalette::Inactive, QPalette::ButtonText, palette.color(QPalette::Inactive, role));
        palette.setColor(QPalette::Disabled, QPalette::ButtonText, palette.color(QPalette::Disabled, role));
        palette.setColor(QPalette::Current, QPalette::ButtonText, palette.color(QPalette::Current, role));
    }
}

QJsonObject AndroidStyle::loadStyleData()
{
    QString stylePath(QLatin1String(qgetenv("MINISTRO_ANDROID_STYLE_PATH")));
    const QLatin1Char slashChar('/');
    if (!stylePath.isEmpty() && !stylePath.endsWith(slashChar))
        stylePath += slashChar;

    QString androidTheme = QLatin1String(qgetenv("QT_ANDROID_THEME"));
    if (!androidTheme.isEmpty() && !androidTheme.endsWith(slashChar))
        androidTheme += slashChar;

    if (stylePath.isEmpty()) {
        stylePath = QLatin1String("/data/data/org.kde.necessitas.ministro/files/dl/style/")
                  + QLatin1String(qgetenv("QT_ANDROID_THEME_DISPLAY_DPI")) + slashChar;
    }
    Q_ASSERT(!stylePath.isEmpty());

    if (!androidTheme.isEmpty() && QFileInfo(stylePath + androidTheme + QLatin1String("style.json")).exists())
        stylePath += androidTheme;

    QFile f(stylePath + QLatin1String("style.json"));
    if (!f.open(QIODevice::ReadOnly))
        return QJsonObject();

    QJsonParseError error;
    QJsonDocument document = QJsonDocument::fromJson(f.readAll(), &error);
    if (document.isNull()) {
        qCritical() << error.errorString();
        return QJsonObject();
    }

    if (!document.isObject()) {
        qCritical() << "Style.json does not contain a valid style.";
        return QJsonObject();
    }
    return document.object();
}

static std::shared_ptr<AndroidStyle> loadAndroidStyle(QPalette *defaultPalette)
{
    std::shared_ptr<AndroidStyle> style(new AndroidStyle);
    style->m_styleData = AndroidStyle::loadStyleData();
    if (style->m_styleData.isEmpty())
        return std::shared_ptr<AndroidStyle>();

    for (QJsonObject::const_iterator objectIterator = style->m_styleData.constBegin();
         objectIterator != style->m_styleData.constEnd();
         ++objectIterator) {
        QString key = objectIterator.key();
        QJsonValue value = objectIterator.value();
        if (!value.isObject()) {
            qWarning("Style.json structure is unrecognized.");
            continue;
        }
        QJsonObject item = value.toObject();
        QJsonObject::const_iterator attributeIterator = item.find(QLatin1String("qtClass"));
        QByteArray qtClassName;
        if (attributeIterator != item.constEnd()) {
            // The item has palette and font information for a specific Qt Class (e.g. QWidget, QPushButton, etc.)
            qtClassName = attributeIterator.value().toString().toLatin1();
        }
        const int ft = fontType(key);
        if (ft > -1 || !qtClassName.isEmpty()) {
            // Extract font information
            QFont font;

            // Font size (in pixels)
            attributeIterator = item.find(QLatin1String("TextAppearance_textSize"));
            if (attributeIterator != item.constEnd())
                font.setPixelSize(int(attributeIterator.value().toDouble()));

            // Font style
            attributeIterator = item.find(QLatin1String("TextAppearance_textStyle"));
            if (attributeIterator != item.constEnd()) {
                const int style = int(attributeIterator.value().toDouble());
                font.setBold(style & textStyle_bold);
                font.setItalic(style & textStyle_italic);
            }

            // Font typeface
            attributeIterator = item.find(QLatin1String("TextAppearance_typeface"));
            if (attributeIterator != item.constEnd()) {
                QFont::StyleHint styleHint = QFont::AnyStyle;
                switch (int(attributeIterator.value().toDouble())) {
                case typeface_sans:
                    styleHint = QFont::SansSerif;
                    break;
                case typeface_serif:
                    styleHint = QFont::Serif;
                    break;
                case typeface_monospace:
                    styleHint = QFont::Monospace;
                    break;
                }
                font.setStyleHint(styleHint, QFont::PreferMatch);
            }
            if (!qtClassName.isEmpty())
                style->m_QWidgetsFonts.insert(qtClassName, font);

            if (ft > -1) {
                style->m_fonts.insert(ft, font);
                if (ft == QPlatformTheme::SystemFont)
                    QGuiApplication::setFont(font);
            }
            // Extract font information
        }

        const int pt = paletteType(key);
        if (pt > -1 || !qtClassName.isEmpty()) {
            // Extract palette information
            QPalette palette = *defaultPalette;

            attributeIterator = item.find(QLatin1String("defaultTextColorPrimary"));
            if (attributeIterator != item.constEnd())
                palette.setColor(QPalette::WindowText, QRgb(int(attributeIterator.value().toDouble())));

            attributeIterator = item.find(QLatin1String("defaultBackgroundColor"));
            if (attributeIterator != item.constEnd())
                palette.setColor(QPalette::Background, QRgb(int(attributeIterator.value().toDouble())));

            attributeIterator = item.find(QLatin1String("TextAppearance_textColor"));
            if (attributeIterator != item.constEnd())
                setPaletteColor(attributeIterator.value().toObject().toVariantMap(), palette, QPalette::WindowText);

            attributeIterator = item.find(QLatin1String("TextAppearance_textColorLink"));
            if (attributeIterator != item.constEnd())
                setPaletteColor(attributeIterator.value().toObject().toVariantMap(), palette, QPalette::Link);

            attributeIterator = item.find(QLatin1String("TextAppearance_textColorHighlight"));
            if (attributeIterator != item.constEnd())
                palette.setColor(QPalette::Highlight, QRgb(int(attributeIterator.value().toDouble())));

            if (pt == QPlatformTheme::SystemPalette)
                *defaultPalette = style->m_standardPalette = palette;

            if (pt > -1)
                style->m_palettes.insert(pt, palette);
            // Extract palette information
        }
    }
    return style;
}

QAndroidPlatformTheme::QAndroidPlatformTheme(QAndroidPlatformNativeInterface *androidPlatformNativeInterface)
{
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
    m_androidStyleData = loadAndroidStyle(&m_defaultPalette);
    QGuiApplication::setPalette(m_defaultPalette);
    androidPlatformNativeInterface->m_androidStyle = m_androidStyleData;

    // default in case the style has not set a font
    m_systemFont = QFont(QLatin1String("Roboto"), 14.0 * 100 / 72); // keep default size the same after changing from 100 dpi to 72 dpi
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

void QAndroidPlatformTheme::showPlatformMenuBar()
{
    QtAndroidMenu::openOptionsMenu();
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
    if (m_androidStyleData) {
        auto it = m_androidStyleData->m_palettes.find(paletteType(type));
        if (it != m_androidStyleData->m_palettes.end())
            return &(it.value());
    }
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
    if (m_androidStyleData) {
        auto it = m_androidStyleData->m_fonts.find(fontType(type));
        if (it != m_androidStyleData->m_fonts.end())
            return &(it.value());
    }

    if (type == QPlatformTheme::SystemFont)
        return &m_systemFont;
    return 0;
}

QVariant QAndroidPlatformTheme::themeHint(ThemeHint hint) const
{
    switch (hint) {
    case StyleNames:
        if (qgetenv("QT_USE_ANDROID_NATIVE_STYLE").toInt()
                && m_androidStyleData) {
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

QT_END_NAMESPACE
