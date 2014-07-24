/****************************************************************************
**
** Copyright (C) 2013 BogDan Vatra <bogdan@kde.org>
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtWidgets module of the Qt Toolkit.
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

#include "qandroidstyle_p.h"

#if !defined(QT_NO_STYLE_ANDROID) || defined(QT_PLUGIN)

#include <QFile>
#include <QFont>
#include <QApplication>
#include <QTime>
#include <qdrawutil.h>
#include <QPixmapCache>
#include <QFileInfo>
#include <QStyleOption>
#include <QPainter>
#include <QJsonDocument>
#include <QJsonObject>
#include <QDebug>

#include <QGuiApplication>
#include <qpa/qplatformnativeinterface.h>
#include <qpa/qplatformtheme.h>

QT_BEGIN_NAMESPACE

namespace {
    const int textStyle_bold = 1;
    const int textStyle_italic = 2;

    const int typeface_sans = 1;
    const int typeface_serif = 2;
    const int typeface_monospace = 3;

    const quint32 NO_COLOR = 1;
    const quint32 TRANSPARENT_COLOR = 0;
}

static int fontType(const QString &androidControl)
{
    if (androidControl == QLatin1String("textViewStyle"))
        return QPlatformTheme::SystemFont;
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
    if (androidControl == QLatin1String("textViewStyle"))
        return QPlatformTheme::SystemPalette;
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

QAndroidStyle::QAndroidStyle()
    : QFusionStyle()
{
    QPixmapCache::clear();
    QString stylePath(QLatin1String(qgetenv("MINISTRO_ANDROID_STYLE_PATH")));
    const QLatin1Char slashChar('/');
    if (!stylePath.isEmpty() && !stylePath.endsWith(slashChar))
        stylePath += slashChar;

    QString androidTheme = QLatin1String(qgetenv("QT_ANDROID_THEME"));
    if (!androidTheme.isEmpty() && !androidTheme.endsWith(slashChar))
        androidTheme += slashChar;

    if (stylePath.isEmpty())
    {
        stylePath = QLatin1String("/data/data/org.kde.necessitas.ministro/files/dl/style/")
                  + QLatin1String(qgetenv("QT_ANDROID_THEME_DISPLAY_DPI")) + slashChar;
    }
    Q_ASSERT(!stylePath.isEmpty());

    if (!androidTheme.isEmpty() && QFileInfo(stylePath + androidTheme + QLatin1String("style.json")).exists())
        stylePath += androidTheme;

    QFile f(stylePath + QLatin1String("style.json"));
    if (!f.open(QIODevice::ReadOnly))
        return;

    QJsonParseError error;
    QJsonDocument document = QJsonDocument::fromJson(f.readAll(), &error);
    if (document.isNull())  {
        qCritical() << error.errorString();
        return;
    }

    if (!document.isObject()) {
        qCritical() << "Style.json does not contain a valid style.";
        return;
    }

    QPlatformNativeInterface *nativeInterface = QGuiApplication::platformNativeInterface();

    QHash<int, QPalette> * palettes = reinterpret_cast<QHash<int, QPalette> *>(nativeInterface->nativeResourceForIntegration("AndroidStylePalettes"));
    QHash<int, QFont> * fonts = reinterpret_cast<QHash<int, QFont> *>(nativeInterface->nativeResourceForIntegration("AndroidStyleFonts"));
    palettes->clear();
    fonts->clear();
    QJsonObject object = document.object();
    for (QJsonObject::const_iterator objectIterator = object.constBegin();
         objectIterator != object.constEnd();
         ++objectIterator) {
        QString key = objectIterator.key();
        QJsonValue value = objectIterator.value();
        if (!value.isObject()) {
            qWarning("Style.json structure is unrecognized.");
            continue;
        }

        QJsonObject item = value.toObject();
        QJsonObject::const_iterator attributeIterator = item.find(QLatin1String("qtClass"));
        QString qtClassName;
        if (attributeIterator != item.constEnd()) {
            // The item has palette and font information for a specific Qt Class (e.g. QWidget, QPushButton, etc.)
            qtClassName = attributeIterator.value().toString();
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
                QApplication::setFont(font, qtClassName.toUtf8());
            if (ft>-1)
                fonts->insert(ft, font);
            // Extract font information
        }

        const int pt = paletteType(key);
        if (pt > -1 || !qtClassName.isEmpty()) {
            // Extract palette information
            QPalette palette;
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

            if (QLatin1String("QWidget") == qtClassName)
                m_standardPalette = palette;

            if (pt > -1)
                palettes->insert(pt, palette);
            // Extract palette information
        }
        QAndroidStyle::ItemType itemType = qtControl(key);
        if (QC_UnknownType == itemType)
            continue;

        switch (itemType) {
        case QC_Checkbox:
        case QC_RadioButton:
            m_androidControlsHash[int(itemType)] = new AndroidCompoundButtonControl(item.toVariantMap(),
                                                                                    itemType);
            break;

        case QC_ProgressBar:
            m_androidControlsHash[int(itemType)] = new AndroidProgressBarControl(item.toVariantMap(),
                                                                                 itemType);
            break;

        case QC_Slider:
            m_androidControlsHash[int(itemType)] = new AndroidSeekBarControl(item.toVariantMap(),
                                                                             itemType);
            break;

        case QC_Combobox:
            m_androidControlsHash[int(itemType)] = new AndroidSpinnerControl(item.toVariantMap(),
                                                                             itemType);
            break;

        default:
            m_androidControlsHash[int(itemType)] = new AndroidControl(item.toVariantMap(),
                                                                      itemType);
            break;
        }
    }
}

QAndroidStyle::~QAndroidStyle()
{
    qDeleteAll(m_androidControlsHash);
}


void QAndroidStyle::setPaletteColor(const QVariantMap &object,
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

QAndroidStyle::ItemType QAndroidStyle::qtControl(const QString &android)
{
    if (android == QLatin1String("buttonStyle"))
        return QC_Button;
    if (android == QLatin1String("editTextStyle"))
        return QC_EditText;
    if (android == QLatin1String("radioButtonStyle"))
        return QC_RadioButton;
    if (android == QLatin1String("checkboxStyle"))
        return QC_Checkbox;
    if (android == QLatin1String("textViewStyle"))
        return QC_View;
    if (android == QLatin1String("buttonStyleToggle"))
        return QC_Switch;
    if (android == QLatin1String("spinnerStyle"))
        return QC_Combobox;
    if (android == QLatin1String("progressBarStyleHorizontal"))
        return QC_ProgressBar;
    if (android == QLatin1String("seekBarStyle"))
        return QC_Slider;

    return QC_UnknownType;
}

QAndroidStyle::ItemType QAndroidStyle::qtControl(QStyle::ComplexControl control)
{
    switch (control) {
    case CC_ComboBox:
        return QC_Combobox;
    case CC_Slider:
        return QC_Slider;
    default:
        return QC_UnknownType;
    }
}

QAndroidStyle::ItemType QAndroidStyle::qtControl(QStyle::ContentsType contentsType)
{
    switch (contentsType) {
    case CT_PushButton:
        return QC_Button;
    case CT_CheckBox:
        return QC_Checkbox;
    case CT_RadioButton:
        return QC_RadioButton;
    case CT_ComboBox:
        return QC_Combobox;
    case CT_ProgressBar:
        return QC_ProgressBar;
    case CT_Slider:
        return QC_Slider;
    case CT_ScrollBar:
        return QC_Slider;
    case CT_TabWidget:
        return QC_Tab;
    case CT_TabBarTab:
        return QC_TabButton;
    case CT_LineEdit:
        return QC_EditText;
    case CT_GroupBox:
        return QC_GroupBox;
    default:
        return QC_UnknownType;
    }
}

QAndroidStyle::ItemType QAndroidStyle::qtControl(QStyle::ControlElement controlElement)
{
    switch (controlElement) {
    case CE_PushButton:
    case CE_PushButtonBevel:
    case CE_PushButtonLabel:
        return QC_Button;

    case CE_CheckBox:
    case CE_CheckBoxLabel:
        return QC_Checkbox;

    case CE_RadioButton:
    case CE_RadioButtonLabel:
        return QC_RadioButton;

    case CE_TabBarTab:
    case CE_TabBarTabShape:
    case CE_TabBarTabLabel:
        return QC_Tab;

    case CE_ProgressBar:
    case CE_ProgressBarGroove:
    case CE_ProgressBarContents:
    case CE_ProgressBarLabel:
        return QC_ProgressBar;

    case CE_ComboBoxLabel:
        return QC_Combobox;

    case CE_ShapedFrame:
        return QC_View;

    default:
        return QC_UnknownType;
    }
}

QAndroidStyle::ItemType QAndroidStyle::qtControl(QStyle::PrimitiveElement primitiveElement)
{
    switch (primitiveElement) {
    case QStyle::PE_PanelLineEdit:
    case QStyle::PE_FrameLineEdit:
        return QC_EditText;

    case QStyle::PE_FrameWindow:
    case QStyle::PE_Widget:
    case QStyle::PE_Frame:
    case QStyle::PE_FrameFocusRect:
        return QC_View;
    default:
        return QC_UnknownType;
    }
}

QAndroidStyle::ItemType QAndroidStyle::qtControl(QStyle::SubElement subElement)
{
    switch (subElement) {
    case QStyle::SE_LineEditContents:
        return QC_EditText;

    case QStyle::SE_PushButtonContents:
    case QStyle::SE_PushButtonFocusRect:
        return QC_Button;

    case SE_RadioButtonContents:
        return QC_RadioButton;

    case SE_CheckBoxContents:
        return QC_Checkbox;

    default:
        return QC_UnknownType;
    }
}

void QAndroidStyle::drawPrimitive(PrimitiveElement pe,
                                  const QStyleOption *opt,
                                  QPainter *p,
                                  const QWidget *w) const
{
    const ItemType itemType = qtControl(pe);
    AndroidControlsHash::const_iterator it = itemType != QC_UnknownType
                                             ? m_androidControlsHash.find(itemType)
                                             : m_androidControlsHash.end();
    if (it != m_androidControlsHash.end()) {
        if (itemType != QC_EditText)
            it.value()->drawControl(opt, p, w);
        else {
            QStyleOption copy(*opt);
            copy.state &= ~QStyle::State_Sunken;
            it.value()->drawControl(&copy, p, w);
        }
    }
    else
        QFusionStyle::drawPrimitive(pe, opt, p, w);
}


void QAndroidStyle::drawControl(QStyle::ControlElement element,
                                const QStyleOption *opt,
                                QPainter *p,
                                const QWidget *w) const
{
    const ItemType itemType = qtControl(element);
    AndroidControlsHash::const_iterator it = itemType != QC_UnknownType
                                             ? m_androidControlsHash.find(itemType)
                                             : m_androidControlsHash.end();
    if (it != m_androidControlsHash.end()) {
        AndroidControl *androidControl = it.value();
        androidControl->drawControl(opt, p, w);

        switch (itemType) {
        case QC_Button:
            if (const QStyleOptionButton *buttonOption =
                qstyleoption_cast<const QStyleOptionButton *>(opt)) {
                QMargins padding = androidControl->padding();
                QStyleOptionButton copy(*buttonOption);
                copy.rect.adjust(padding.left(), padding.top(), -padding.right(), -padding.bottom());
                QFusionStyle::drawControl(CE_PushButtonLabel, &copy, p, w);
            }
            break;
        case QC_Checkbox:
        case QC_RadioButton:
            if (const QStyleOptionButton *btn =
                qstyleoption_cast<const QStyleOptionButton *>(opt)) {
                const bool isRadio = (element == CE_RadioButton);
                QStyleOptionButton subopt(*btn);
                subopt.rect = subElementRect(isRadio ? SE_RadioButtonContents
                                             : SE_CheckBoxContents, btn, w);
                QFusionStyle::drawControl(isRadio ? CE_RadioButtonLabel : CE_CheckBoxLabel, &subopt, p, w);
            }
            break;
        case QC_Combobox:
            if (const QStyleOptionComboBox *comboboxOption =
                qstyleoption_cast<const QStyleOptionComboBox *>(opt)) {
                QMargins padding = androidControl->padding();
                QStyleOptionComboBox copy (*comboboxOption);
                copy.rect.adjust(padding.left(), padding.top(), -padding.right(), -padding.bottom());
                QFusionStyle::drawControl(CE_ComboBoxLabel, comboboxOption, p, w);
            }
            break;
        default:
            break;
        }
    } else {
        QFusionStyle::drawControl(element, opt, p, w);
    }
}

QRect QAndroidStyle::subElementRect(SubElement subElement,
                                    const QStyleOption *option,
                                    const QWidget *widget) const
{
    const ItemType itemType = qtControl(subElement);
    AndroidControlsHash::const_iterator it = itemType != QC_UnknownType
                                             ? m_androidControlsHash.find(itemType)
                                             : m_androidControlsHash.end();
    if (it != m_androidControlsHash.end())
        return it.value()->subElementRect(subElement, option, widget);
    return QFusionStyle::subElementRect(subElement, option, widget);
}

void QAndroidStyle::drawComplexControl(ComplexControl cc,
                                       const QStyleOptionComplex *opt,
                                       QPainter *p,
                                       const QWidget *widget) const
{
    const ItemType itemType = qtControl(cc);
    AndroidControlsHash::const_iterator it = itemType != QC_UnknownType
                                             ? m_androidControlsHash.find(itemType)
                                             : m_androidControlsHash.end();
    if (it != m_androidControlsHash.end())
        it.value()->drawControl(opt, p, widget);
    else
        QFusionStyle::drawComplexControl(cc, opt, p, widget);
}

QStyle::SubControl QAndroidStyle::hitTestComplexControl(ComplexControl cc,
                                                        const QStyleOptionComplex *opt,
                                                        const QPoint &pt,
                                                        const QWidget *widget) const
{
    const ItemType itemType = qtControl(cc);
    AndroidControlsHash::const_iterator it = itemType != QC_UnknownType
                                             ? m_androidControlsHash.find(itemType)
                                             : m_androidControlsHash.end();
    if (it != m_androidControlsHash.end()) {
        switch (cc) {
        case CC_Slider:
            if (const QStyleOptionSlider *slider = qstyleoption_cast<const QStyleOptionSlider *>(opt)) {
                QRect r = it.value()->subControlRect(slider, SC_SliderHandle, widget);
                if (r.isValid() && r.contains(pt)) {
                    return SC_SliderHandle;
                } else {
                    r = it.value()->subControlRect(slider, SC_SliderGroove, widget);
                    if (r.isValid() && r.contains(pt))
                        return SC_SliderGroove;
                }
            }
            break;
        default:
            break;
        }
    }
    return QFusionStyle::hitTestComplexControl(cc, opt, pt, widget);
}

QRect QAndroidStyle::subControlRect(ComplexControl cc,
                                    const QStyleOptionComplex *opt,
                                    SubControl sc,
                                    const QWidget *widget) const
{
    const ItemType itemType = qtControl(cc);
    AndroidControlsHash::const_iterator it = itemType != QC_UnknownType
                                             ? m_androidControlsHash.find(itemType)
                                             : m_androidControlsHash.end();
    if (it != m_androidControlsHash.end())
        return it.value()->subControlRect(opt, sc, widget);

    return QFusionStyle::subControlRect(cc, opt, sc, widget);
}

int QAndroidStyle::pixelMetric(PixelMetric metric, const QStyleOption *option,
                        const QWidget *widget) const
{
    switch (metric) {
    case PM_ButtonMargin:
    case PM_FocusFrameVMargin:
    case PM_FocusFrameHMargin:
    case PM_ComboBoxFrameWidth:
    case PM_SpinBoxFrameWidth:
    case PM_ScrollBarExtent:
        return 0;
    default:
        return QFusionStyle::pixelMetric(metric, option, widget);
    }

}

QSize QAndroidStyle::sizeFromContents(ContentsType ct,
                                      const QStyleOption *opt,
                                      const QSize &contentsSize,
                                      const QWidget *w) const
{
    QSize sz = QFusionStyle::sizeFromContents(ct, opt, contentsSize, w);
    const ItemType itemType = qtControl(ct);
    AndroidControlsHash::const_iterator it = itemType != QC_UnknownType
                                             ? m_androidControlsHash.find(itemType)
                                             : m_androidControlsHash.end();
    if (it != m_androidControlsHash.end())
        return it.value()->sizeFromContents(opt, sz, w);
    return sz;
}

QPixmap QAndroidStyle::standardPixmap(StandardPixmap standardPixmap,
                                      const QStyleOption *opt,
                                      const QWidget *widget) const
{
    return QFusionStyle::standardPixmap(standardPixmap, opt, widget);
}

QPixmap QAndroidStyle::generatedIconPixmap(QIcon::Mode iconMode,
                                           const QPixmap &pixmap,
                                           const QStyleOption *opt) const
{
    return QFusionStyle::generatedIconPixmap(iconMode, pixmap, opt);
}

int QAndroidStyle::styleHint(QStyle::StyleHint hint, const QStyleOption *option, const QWidget *widget, QStyleHintReturn *returnData) const
{
    switch (hint) {
    case SH_Slider_AbsoluteSetButtons:
        return Qt::LeftButton;

    case SH_Slider_PageSetButtons:
        return 0;

    case SH_RequestSoftwareInputPanel:
        return RSIP_OnMouseClick;

    default:
        return QFusionStyle::styleHint(hint,option, widget, returnData);
    }
}

QPalette QAndroidStyle::standardPalette() const
{
    return m_standardPalette;
}

void QAndroidStyle::polish(QWidget *widget)
{
    widget->setAttribute(Qt::WA_StyledBackground, true);
}

void QAndroidStyle::unpolish(QWidget *widget)
{
    widget->setAttribute(Qt::WA_StyledBackground, false);
}

QAndroidStyle::AndroidDrawable::AndroidDrawable(const QVariantMap &drawable,
                                                QAndroidStyle::ItemType itemType)
{
    initPadding(drawable);
    m_itemType = itemType;
}

QAndroidStyle::AndroidDrawable::~AndroidDrawable()
{
}

void QAndroidStyle::AndroidDrawable::initPadding(const QVariantMap &drawable)
{
    QVariantMap::const_iterator it = drawable.find(QLatin1String("padding"));
    if (it != drawable.end())
        m_padding = extractMargins(it.value().toMap());
}

const QMargins &QAndroidStyle::AndroidDrawable::padding() const
{
    return m_padding;
}

QSize QAndroidStyle::AndroidDrawable::size() const
{
    if (type() == Image || type() == NinePatch)
        return static_cast<const QAndroidStyle::AndroidImageDrawable *>(this)->size();

    return QSize();
}

QAndroidStyle::AndroidDrawable * QAndroidStyle::AndroidDrawable::fromMap(const QVariantMap &drawable,
                                                                         ItemType itemType)
{
    const QString type = drawable.value(QLatin1String("type")).toString();
    if (type == QLatin1String("image"))
        return new QAndroidStyle::AndroidImageDrawable(drawable, itemType);
    if (type == QLatin1String("9patch"))
        return new QAndroidStyle::Android9PatchDrawable(drawable, itemType);
    if (type == QLatin1String("stateslist"))
        return new QAndroidStyle::AndroidStateDrawable(drawable, itemType);
    if (type == QLatin1String("layer"))
        return new QAndroidStyle::AndroidLayerDrawable(drawable, itemType);
    if (type == QLatin1String("gradient"))
        return new QAndroidStyle::AndroidGradientDrawable(drawable, itemType);
    if (type == QLatin1String("clipDrawable"))
        return new QAndroidStyle::AndroidClipDrawable(drawable, itemType);
    if (type == QLatin1String("color"))
        return new QAndroidStyle::AndroidColorDrawable(drawable, itemType);
    return 0;
}

QMargins QAndroidStyle::AndroidDrawable::extractMargins(const QVariantMap &value)
{
    QMargins m;
    m.setLeft(value.value(QLatin1String("left")).toInt());
    m.setRight(value.value(QLatin1String("right")).toInt());
    m.setTop(value.value(QLatin1String("top")).toInt());
    m.setBottom(value.value(QLatin1String("bottom")).toInt());
    return m;
}

void QAndroidStyle::AndroidDrawable::setPaddingLeftToSizeWidth()
{
    QSize sz = size();
    if (m_padding.isNull() && !sz.isNull())
        m_padding.setLeft(sz.width());
}


QAndroidStyle::AndroidImageDrawable::AndroidImageDrawable(const QVariantMap &drawable,
                                                          QAndroidStyle::ItemType itemType)
    : AndroidDrawable(drawable, itemType)
{
    m_filePath = drawable.value(QLatin1String("path")).toString();
    m_size.setHeight(drawable.value(QLatin1String("height")).toInt());
    m_size.setWidth(drawable.value(QLatin1String("width")).toInt());
}

QAndroidStyle::AndroidDrawableType QAndroidStyle::AndroidImageDrawable::type() const
{
    return QAndroidStyle::Image;
}

void QAndroidStyle::AndroidImageDrawable::draw(QPainter *painter, const QStyleOption *opt) const
{
    if (m_hashKey.isEmpty())
        m_hashKey = QFileInfo(m_filePath).fileName();

    QPixmap pm;
    if (!QPixmapCache::find(m_hashKey, &pm)) {
        pm.load(m_filePath);
        QPixmapCache::insert(m_hashKey, pm);
    }

    painter->drawPixmap(opt->rect.x(), (opt->rect.height() - pm.height()) / 2, pm);
}

QSize QAndroidStyle::AndroidImageDrawable::size() const
{
    return m_size;
}

QAndroidStyle::AndroidColorDrawable::AndroidColorDrawable(const QVariantMap &drawable,
                                                          ItemType itemType)
    : AndroidDrawable(drawable, itemType)
{
    m_color.setRgba(QRgb(drawable.value(QLatin1String("color")).toInt()));
}

QAndroidStyle::AndroidDrawableType QAndroidStyle::AndroidColorDrawable::type() const
{
    return QAndroidStyle::Color;
}

void QAndroidStyle::AndroidColorDrawable::draw(QPainter *painter, const QStyleOption *opt) const
{
    painter->fillRect(opt->rect, m_color);
}

QAndroidStyle::Android9PatchDrawable::Android9PatchDrawable(const QVariantMap &drawable,
                                                            QAndroidStyle::ItemType itemType)
    : AndroidImageDrawable(drawable.value(QLatin1String("drawable")).toMap(), itemType)
{
    initPadding(drawable);
    QVariantMap chunk = drawable.value(QLatin1String("chunkInfo")).toMap();
    extractIntArray(chunk.value(QLatin1String("xdivs")).toList(), m_chunkData.xDivs);
    extractIntArray(chunk.value(QLatin1String("ydivs")).toList(), m_chunkData.yDivs);
    extractIntArray(chunk.value(QLatin1String("colors")).toList(), m_chunkData.colors);
}

QAndroidStyle::AndroidDrawableType QAndroidStyle::Android9PatchDrawable::type() const
{
    return QAndroidStyle::NinePatch;
}

int QAndroidStyle::Android9PatchDrawable::calculateStretch(int boundsLimit,
                                                           int startingPoint,
                                                           int srcSpace,
                                                           int numStrechyPixelsRemaining,
                                                           int numFixedPixelsRemaining)
{
    int spaceRemaining = boundsLimit - startingPoint;
    int stretchySpaceRemaining = spaceRemaining - numFixedPixelsRemaining;
    return (float(srcSpace) * stretchySpaceRemaining / numStrechyPixelsRemaining + .5);
}

void QAndroidStyle::Android9PatchDrawable::extractIntArray(const QVariantList &values,
                                                           QVector<int> & array)
{
    foreach (QVariant value, values)
        array << value.toInt();
}


void QAndroidStyle::Android9PatchDrawable::draw(QPainter * painter, const QStyleOption *opt) const
{
    if (m_hashKey.isEmpty())
        m_hashKey = QFileInfo(m_filePath).fileName();

    QPixmap pixmap;
    if (!QPixmapCache::find(m_hashKey, &pixmap)) {
        pixmap.load(m_filePath);
        QPixmapCache::insert(m_hashKey, pixmap);
    }

    const QRect &bounds=opt->rect;

    // shamelessly stolen from Android's sources (NinepatchImpl.cpp) and adapted for Qt
    const int pixmapWidth = pixmap.width();
    const int pixmapHeight = pixmap.height();

    if (bounds.isNull() || !pixmapWidth || !pixmapHeight)
        return;

    QPainter::RenderHints savedHints = painter->renderHints();

    // The patchs doesn't need smooth transform !
    painter->setRenderHints(QPainter::SmoothPixmapTransform, false);

    QRectF dst;
    QRectF src;

    const qint32 x0 = m_chunkData.xDivs[0];
    const qint32 y0 = m_chunkData.yDivs[0];
    const quint8 numXDivs = m_chunkData.xDivs.size();
    const quint8 numYDivs = m_chunkData.yDivs.size();
    int i;
    int j;
    int colorIndex = 0;
    quint32 color;
    bool xIsStretchable;
    const bool initialXIsStretchable = (x0 == 0);
    bool yIsStretchable = (y0 == 0);
    const int bitmapWidth = pixmap.width();
    const int bitmapHeight = pixmap.height();

    int *dstRights = static_cast<int *>(alloca((numXDivs + 1) * sizeof(int)));
    bool dstRightsHaveBeenCached = false;

    int numStretchyXPixelsRemaining = 0;
    for (i = 0; i < numXDivs; i += 2)
        numStretchyXPixelsRemaining += m_chunkData.xDivs[i + 1] - m_chunkData.xDivs[i];

    int numFixedXPixelsRemaining = bitmapWidth - numStretchyXPixelsRemaining;
    int numStretchyYPixelsRemaining = 0;
    for (i = 0; i < numYDivs; i += 2)
        numStretchyYPixelsRemaining += m_chunkData.yDivs[i + 1] - m_chunkData.yDivs[i];

    int numFixedYPixelsRemaining = bitmapHeight - numStretchyYPixelsRemaining;
    src.setTop(0);
    dst.setTop(bounds.top());
    // The first row always starts with the top being at y=0 and the bottom
    // being either yDivs[1] (if yDivs[0]=0) of yDivs[0].  In the former case
    // the first row is stretchable along the Y axis, otherwise it is fixed.
    // The last row always ends with the bottom being bitmap.height and the top
    // being either yDivs[numYDivs-2] (if yDivs[numYDivs-1]=bitmap.height) or
    // yDivs[numYDivs-1]. In the former case the last row is stretchable along
    // the Y axis, otherwise it is fixed.
    //
    // The first and last columns are similarly treated with respect to the X
    // axis.
    //
    // The above is to help explain some of the special casing that goes on the
    // code below.

    // The initial yDiv and whether the first row is considered stretchable or
    // not depends on whether yDiv[0] was zero or not.
    for (j = yIsStretchable ? 1 : 0;
         j <= numYDivs && src.top() < bitmapHeight;
          j++, yIsStretchable = !yIsStretchable) {
        src.setLeft(0);
        dst.setLeft(bounds.left());
        if (j == numYDivs) {
            src.setBottom(bitmapHeight);
            dst.setBottom(bounds.bottom());
        } else {
            src.setBottom(m_chunkData.yDivs[j]);
            const int srcYSize = src.bottom() - src.top();
            if (yIsStretchable) {
                dst.setBottom(dst.top() + calculateStretch(bounds.bottom(), dst.top(),
                                                          srcYSize,
                                                          numStretchyYPixelsRemaining,
                                                          numFixedYPixelsRemaining));
                numStretchyYPixelsRemaining -= srcYSize;
            } else {
                dst.setBottom(dst.top() + srcYSize);
                numFixedYPixelsRemaining -= srcYSize;
            }
        }

        xIsStretchable = initialXIsStretchable;
        // The initial xDiv and whether the first column is considered
        // stretchable or not depends on whether xDiv[0] was zero or not.
        for (i = xIsStretchable ? 1 : 0;
              i <= numXDivs && src.left() < bitmapWidth;
              i++, xIsStretchable = !xIsStretchable) {
            color = m_chunkData.colors[colorIndex++];
            if (color != TRANSPARENT_COLOR)
                color = NO_COLOR;
            if (i == numXDivs) {
                src.setRight(bitmapWidth);
                dst.setRight(bounds.right());
            } else {
                src.setRight(m_chunkData.xDivs[i]);
                if (dstRightsHaveBeenCached) {
                    dst.setRight(dstRights[i]);
                } else {
                    const int srcXSize = src.right() - src.left();
                    if (xIsStretchable) {
                        dst.setRight(dst.left() + calculateStretch(bounds.right(), dst.left(),
                                                                  srcXSize,
                                                                  numStretchyXPixelsRemaining,
                                                                  numFixedXPixelsRemaining));
                        numStretchyXPixelsRemaining -= srcXSize;
                    } else {
                        dst.setRight(dst.left() + srcXSize);
                        numFixedXPixelsRemaining -= srcXSize;
                    }
                    dstRights[i] = dst.right();
                }
            }
            // If this horizontal patch is too small to be displayed, leave
            // the destination left edge where it is and go on to the next patch
            // in the source.
            if (src.left() >= src.right()) {
                src.setLeft(src.right());
                continue;
            }
            // Make sure that we actually have room to draw any bits
            if (dst.right() <= dst.left() || dst.bottom() <= dst.top()) {
                goto nextDiv;
            }
            // If this patch is transparent, skip and don't draw.
            if (color == TRANSPARENT_COLOR)
                goto nextDiv;
            if (color != NO_COLOR)
                painter->fillRect(dst, QRgb(color));
            else
                painter->drawPixmap(dst, pixmap, src);
nextDiv:
            src.setLeft(src.right());
            dst.setLeft(dst.right());
        }
        src.setTop(src.bottom());
        dst.setTop(dst.bottom());
        dstRightsHaveBeenCached = true;
    }
    painter->setRenderHints(savedHints);
}

QAndroidStyle::AndroidGradientDrawable::AndroidGradientDrawable(const QVariantMap &drawable,
                                                                QAndroidStyle::ItemType itemType)
    : AndroidDrawable(drawable, itemType), m_orientation(TOP_BOTTOM)
{
    m_radius = drawable.value(QLatin1String("radius")).toInt();
    if (m_radius < 0)
        m_radius = 0;

    QVariantList colors = drawable.value(QLatin1String("colors")).toList();
    QVariantList positions = drawable.value(QLatin1String("positions")).toList();
    int min=colors.size() < positions.size() ? colors.size() : positions.size();
    for (int i = 0; i < min; i++)
        m_gradient.setColorAt(positions.at(i).toDouble(), QRgb(colors.at(i).toInt()));

    QByteArray orientation=drawable.value(QLatin1String("orientation")).toByteArray();
    if (orientation == "TOP_BOTTOM") // draw the gradient from the top to the bottom
        m_orientation = TOP_BOTTOM;
    else if (orientation == "TR_BL") // draw the gradient from the top-right to the bottom-left
        m_orientation = TR_BL;
    else if (orientation == "RIGHT_LEFT") // draw the gradient from the right to the left
        m_orientation = RIGHT_LEFT;
    else if (orientation == "BR_TL") // draw the gradient from the bottom-right to the top-left
        m_orientation = BR_TL;
    else if (orientation == "BOTTOM_TOP") // draw the gradient from the bottom to the top
        m_orientation = BOTTOM_TOP;
    else if (orientation == "BL_TR") // draw the gradient from the bottom-left to the top-right
        m_orientation = BL_TR;
    else if (orientation == "LEFT_RIGHT") // draw the gradient from the left to the right
        m_orientation = LEFT_RIGHT;
    else if (orientation == "TL_BR") // draw the gradient from the top-left to the bottom-right
        m_orientation = TL_BR;
    else
        qWarning("AndroidGradientDrawable: unknown orientation");
}

QAndroidStyle::AndroidDrawableType QAndroidStyle::AndroidGradientDrawable::type() const
{
    return QAndroidStyle::Gradient;
}

void QAndroidStyle::AndroidGradientDrawable::draw(QPainter *painter, const QStyleOption *opt) const
{
    const int width = opt->rect.width();
    const int height = opt->rect.height();
    switch (m_orientation) {
    case TOP_BOTTOM:
        // draw the gradient from the top to the bottom
        m_gradient.setStart(width/2,0);
        m_gradient.setFinalStop(width/2,height);
        break;
    case TR_BL:
        // draw the gradient from the top-right to the bottom-left
        m_gradient.setStart(width,0);
        m_gradient.setFinalStop(0,height);
        break;
    case RIGHT_LEFT:
        // draw the gradient from the right to the left
        m_gradient.setStart(width,height/2);
        m_gradient.setFinalStop(0,height/2);
        break;
    case BR_TL:
        // draw the gradient from the bottom-right to the top-left
        m_gradient.setStart(width,height);
        m_gradient.setFinalStop(0,0);
        break;
    case BOTTOM_TOP:
        // draw the gradient from the bottom to the top
        m_gradient.setStart(width/2,height);
        m_gradient.setFinalStop(width/2,0);
        break;
    case BL_TR:
        // draw the gradient from the bottom-left to the top-right
        m_gradient.setStart(0,height);
        m_gradient.setFinalStop(width,0);
        break;
    case LEFT_RIGHT:
        // draw the gradient from the left to the right
        m_gradient.setStart(0,height/2);
        m_gradient.setFinalStop(width,height/2);
        break;
    case TL_BR:
        // draw the gradient from the top-left to the bottom-right
        m_gradient.setStart(0,0);
        m_gradient.setFinalStop(width,height);
        break;
    }

    const QBrush &oldBrush = painter->brush();
    const QPen oldPen = painter->pen();
    painter->setPen(Qt::NoPen);
    painter->setBrush(m_gradient);
    painter->drawRoundedRect(opt->rect, m_radius, m_radius);
    painter->setBrush(oldBrush);
    painter->setPen(oldPen);
}

QSize QAndroidStyle::AndroidGradientDrawable::size() const
{
    return QSize(m_radius*2, m_radius*2);
}

QAndroidStyle::AndroidClipDrawable::AndroidClipDrawable(const QVariantMap &drawable,
                                                        QAndroidStyle::ItemType itemType)
    : AndroidDrawable(drawable, itemType)
{
    m_drawable = fromMap(drawable.value(QLatin1String("drawable")).toMap(), itemType);
    m_factor = 0;
    m_orientation = Qt::Horizontal;
}

QAndroidStyle::AndroidClipDrawable::~AndroidClipDrawable()
{
    delete m_drawable;
}

QAndroidStyle::AndroidDrawableType QAndroidStyle::AndroidClipDrawable::type() const
{
    return QAndroidStyle::Clip;
}

void QAndroidStyle::AndroidClipDrawable::setFactor(double factor, Qt::Orientation orientation)
{
    m_factor = factor;
    m_orientation = orientation;
}

void QAndroidStyle::AndroidClipDrawable::draw(QPainter *painter, const QStyleOption *opt) const
{
    QStyleOption copy(*opt);
    if (m_orientation == Qt::Horizontal)
        copy.rect.setWidth(copy.rect.width()*m_factor);
    else
        copy.rect.setHeight(copy.rect.height()*m_factor);

    m_drawable->draw(painter, &copy);
}

QAndroidStyle::AndroidStateDrawable::AndroidStateDrawable(const QVariantMap &drawable,
                                                          QAndroidStyle::ItemType itemType)
    : AndroidDrawable(drawable, itemType)
{
    QVariantList states = drawable.value(QLatin1String("stateslist")).toList();
    foreach (QVariant stateVariant, states) {
        QVariantMap state = stateVariant.toMap();
        const int s = extractState(state.value(QLatin1String("states")).toMap());
        if (-1 == s)
            continue;
        const AndroidDrawable *ad = fromMap(state.value(QLatin1String("drawable")).toMap(), itemType);
        if (!ad)
            continue;
        StateType item;
        item.first = s;
        item.second = ad;
        m_states<<item;
    }
}

QAndroidStyle::AndroidStateDrawable::~AndroidStateDrawable()
{
    foreach (const StateType type, m_states)
        delete type.second;
}

QAndroidStyle::AndroidDrawableType QAndroidStyle::AndroidStateDrawable::type() const
{
    return QAndroidStyle::State;
}

void QAndroidStyle::AndroidStateDrawable::draw(QPainter *painter, const QStyleOption *opt) const
{
    const AndroidDrawable *drawable=bestAndroidStateMatch(opt);
    if (drawable)
        drawable->draw(painter, opt);
}

const QAndroidStyle::AndroidDrawable* QAndroidStyle::AndroidStateDrawable::bestAndroidStateMatch(const QStyleOption *opt) const
{
    const AndroidDrawable *bestMatch = 0;
    if (!opt) {
        if (m_states.size())
            return m_states[0].second;
        return bestMatch;
    }

    uint bestCost=0xffff;
    foreach (const StateType & state, m_states) {
        if (int(opt->state) == state.first)
            return state.second;
        uint cost = 1;

        int difference = int(opt->state^state.first);

        if (difference & QStyle::State_Active)
            cost <<= 1;

        if (difference & QStyle::State_Enabled)
            cost <<= 1;

        if (difference & QStyle::State_Raised)
            cost <<= 1;

        if (difference & QStyle::State_Sunken)
            cost <<= 1;

        if (difference & QStyle::State_Off)
            cost <<= 1;

        if (difference & QStyle::State_On)
            cost <<= 1;

        if (difference & QStyle::State_HasFocus)
            cost <<= 1;

        if (difference & QStyle::State_Selected)
            cost <<= 1;

        if (cost < bestCost) {
            bestCost = cost;
            bestMatch = state.second;
        }
    }
    return bestMatch;
}

int QAndroidStyle::AndroidStateDrawable::extractState(const QVariantMap &value)
{
    int state = QStyle::State_Enabled | QStyle::State_Active;;
    foreach (const QString key, value.keys()) {
        bool val = value.value(key).toString() == QLatin1String("true");
        if (key == QLatin1String("enabled")) {
            if (val)
                state |= QStyle::State_Enabled;
            else
                state &= ~QStyle::State_Enabled;
            continue;
        }

        if (key == QLatin1String("window_focused")) {
            if (val)
                state |= QStyle::State_Active;
            else
                state &= ~QStyle::State_Active;
            continue;
        }

        if (key == QLatin1String("focused")) {
            if (val)
                state |= QStyle::State_HasFocus;
            else
                state &= ~QStyle::State_HasFocus;
            continue;
        }

        if (key == QLatin1String("checked")) {
            state |= val ? QStyle::State_On : QStyle::State_Off;
            continue;
        }

        if (key == QLatin1String("pressed")) {
            state |= val ? QStyle::State_Sunken : QStyle::State_Raised;
            continue;
        }

        if (key == QLatin1String("selected")) {
            if (val)
                state |= QStyle::State_Selected;
            else
                state &= ~QStyle::State_Selected;
            continue;
        }

        if (key == QLatin1String("active")) {
            if (val)
                state |= QStyle::State_Active;
            else
                state &= ~QStyle::State_Active;
            continue;
        }

        if (key == QLatin1String("multiline"))
            return 0;

        if (key == QLatin1String("background") && val)
            return -1;
    }
    return state;
}

void QAndroidStyle::AndroidStateDrawable::setPaddingLeftToSizeWidth()
{
    foreach (const StateType type, m_states)
        const_cast<AndroidDrawable *>(type.second)->setPaddingLeftToSizeWidth();
}

QAndroidStyle::AndroidLayerDrawable::AndroidLayerDrawable(const QVariantMap &drawable,
                                                          QAndroidStyle::ItemType itemType)
    : AndroidDrawable(drawable, itemType)
{
    m_id = 0;
    m_factor = 1;
    m_orientation = Qt::Horizontal;
    QVariantList layers = drawable.value(QLatin1String("layers")).toList();
    foreach (QVariant layer, layers) {
        QVariantMap layerMap = layer.toMap();
        AndroidDrawable *ad = fromMap(layerMap, itemType);
        if (ad) {
            LayerType l;
            l.second =  ad;
            l.first = layerMap.value(QLatin1String("id")).toInt();
            m_layers << l;
        }
    }
}

QAndroidStyle::AndroidLayerDrawable::~AndroidLayerDrawable()
{
    foreach (const LayerType &layer, m_layers)
        delete layer.second;
}

QAndroidStyle::AndroidDrawableType QAndroidStyle::AndroidLayerDrawable::type() const
{
    return QAndroidStyle::Layer;
}

void QAndroidStyle::AndroidLayerDrawable::setFactor(int id, double factor, Qt::Orientation orientation)
{
    m_id = id;
    m_factor = factor;
    m_orientation = orientation;
}

void QAndroidStyle::AndroidLayerDrawable::draw(QPainter *painter, const QStyleOption *opt) const
{
    foreach (const LayerType &layer, m_layers)
    {
        if (layer.first == m_id)
        {
            QStyleOption copy(*opt);
            if (m_orientation == Qt::Horizontal)
                copy.rect.setWidth(copy.rect.width()*m_factor);
            else
                copy.rect.setHeight(copy.rect.height()*m_factor);
            layer.second->draw(painter, &copy);
        } else {
            layer.second->draw(painter, opt);
        }
    }
}

QAndroidStyle::AndroidDrawable *QAndroidStyle::AndroidLayerDrawable::layer(int id) const
{
    foreach (const LayerType &layer, m_layers)
        if (layer.first == id)
            return layer.second;
    return 0;
}

QSize QAndroidStyle::AndroidLayerDrawable::size() const
{
    QSize sz;
    foreach (const LayerType &layer, m_layers)
        sz = sz.expandedTo(layer.second->size());
    return sz;
}

QAndroidStyle::AndroidControl::AndroidControl(const QVariantMap &control,
                                              QAndroidStyle::ItemType itemType)
{
    QVariantMap::const_iterator it = control.find(QLatin1String("View_background"));
    if (it != control.end())
        m_background = AndroidDrawable::fromMap(it.value().toMap(), itemType);
    else
        m_background = 0;

    it = control.find(QLatin1String("View_minWidth"));
    if (it!=control.end())
        m_minSize.setWidth(it.value().toInt());

    it = control.find(QLatin1String("View_minHeight"));
    if (it != control.end())
        m_minSize.setHeight(it.value().toInt());

    it = control.find(QLatin1String("View_maxWidth"));
    if (it != control.end())
        m_maxSize.setWidth(it.value().toInt());

    it = control.find(QLatin1String("View_maxHeight"));
    if (it != control.end())
        m_maxSize.setHeight(it.value().toInt());
}

QAndroidStyle::AndroidControl::~AndroidControl()
{
    delete m_background;
}

void QAndroidStyle::AndroidControl::drawControl(const QStyleOption *opt, QPainter *p, const QWidget * /* w */)
{
    if (m_background) {
        m_background->draw(p, opt);
    } else {
        if (const QStyleOptionFrame *frame = qstyleoption_cast<const QStyleOptionFrame *>(opt)) {
            if ((frame->state & State_Sunken) || (frame->state & State_Raised)) {
                qDrawShadePanel(p, frame->rect, frame->palette, frame->state & State_Sunken,
                                frame->lineWidth);
            } else {
                qDrawPlainRect(p, frame->rect, frame->palette.foreground().color(), frame->lineWidth);
            }
        } else {
            if (const QStyleOptionFocusRect *fropt = qstyleoption_cast<const QStyleOptionFocusRect *>(opt)) {
                QColor bg = fropt->backgroundColor;
                QPen oldPen = p->pen();
                if (bg.isValid()) {
                    int h, s, v;
                    bg.getHsv(&h, &s, &v);
                    if (v >= 128)
                        p->setPen(Qt::black);
                    else
                        p->setPen(Qt::white);
                } else {
                    p->setPen(opt->palette.foreground().color());
                }
                QRect focusRect = opt->rect.adjusted(1, 1, -1, -1);
                p->drawRect(focusRect.adjusted(0, 0, -1, -1)); //draw pen inclusive
                p->setPen(oldPen);
            } else {
                p->fillRect(opt->rect, opt->palette.brush(QPalette::Background));
            }
        }
    }
}

QRect QAndroidStyle::AndroidControl::subElementRect(QStyle::SubElement /* subElement */,
                                                    const QStyleOption *option,
                                                    const QWidget * /* widget */) const
{
    if (const AndroidDrawable *drawable = backgroundDrawable()) {
        if (drawable->type() == State)
            drawable = static_cast<const AndroidStateDrawable *>(backgroundDrawable())->bestAndroidStateMatch(option);

        const QMargins &padding = drawable->padding();

        QRect r = option->rect.adjusted(padding.left(), padding.top(),
                                        -padding.right(), -padding.bottom());

        if (r.width() < m_minSize.width())
            r.setWidth(m_minSize.width());

        if (r.height() < m_minSize.height())
            r.setHeight(m_minSize.height());

        return visualRect(option->direction, option->rect, r);
    }
    return option->rect;

}

QRect QAndroidStyle::AndroidControl::subControlRect(const QStyleOptionComplex *option,
                                                    QStyle::SubControl /*sc*/,
                                                    const QWidget *widget) const
{
    return subElementRect(QStyle::SE_CustomBase, option, widget);
}

QSize QAndroidStyle::AndroidControl::sizeFromContents(const QStyleOption *opt,
                                                      const QSize &contentsSize,
                                                      const QWidget * /* w */) const
{
    QSize sz;
    if (const AndroidDrawable *drawable = backgroundDrawable()) {

        if (drawable->type() == State)
            drawable = static_cast<const AndroidStateDrawable*>(backgroundDrawable())->bestAndroidStateMatch(opt);
        const QMargins &padding = drawable->padding();
        sz.setWidth(padding.left() + padding.right());
        sz.setHeight(padding.top() + padding.bottom());
        if (sz.isEmpty())
            sz = drawable->size();
    }
    sz += contentsSize;
    if (contentsSize.height() < opt->fontMetrics.height())
        sz.setHeight(sz.height() + (opt->fontMetrics.height() - contentsSize.height()));
    if (sz.height() < m_minSize.height())
        sz.setHeight(m_minSize.height());
    if (sz.width() < m_minSize.width())
        sz.setWidth(m_minSize.width());
    return sz;
}

QMargins QAndroidStyle::AndroidControl::padding()
{
    if (const AndroidDrawable *drawable = m_background)
    {
        if (drawable->type() == State)
            drawable=static_cast<const AndroidStateDrawable *>(m_background)->bestAndroidStateMatch(0);
        return drawable->padding();
    }
    return QMargins();
}

const QAndroidStyle::AndroidDrawable *QAndroidStyle::AndroidControl::backgroundDrawable() const
{
    return m_background;
}

QAndroidStyle::AndroidCompoundButtonControl::AndroidCompoundButtonControl(const QVariantMap &control,
                                                                          ItemType itemType)
    : AndroidControl(control, itemType)
{
    QVariantMap::const_iterator it = control.find(QLatin1String("CompoundButton_button"));
    if (it != control.end())
        m_button = AndroidDrawable::fromMap(it.value().toMap(), itemType);
    else
        m_button = 0;
    const_cast<AndroidDrawable *>(m_button)->setPaddingLeftToSizeWidth();
}

QAndroidStyle::AndroidCompoundButtonControl::~AndroidCompoundButtonControl()
{
    delete m_button;
}

void QAndroidStyle::AndroidCompoundButtonControl::drawControl(const QStyleOption *opt,
                                                              QPainter *p,
                                                              const QWidget *w)
{
    AndroidControl::drawControl(opt, p, w);
    if (m_button)
        m_button->draw(p, opt);
}

const QAndroidStyle::AndroidDrawable *QAndroidStyle::AndroidCompoundButtonControl::backgroundDrawable() const
{
    return m_background ? m_background : m_button;
}

QAndroidStyle::AndroidProgressBarControl::AndroidProgressBarControl(const QVariantMap &control,
                                                                    ItemType itemType)
    : AndroidControl(control, itemType)
{
    QVariantMap::const_iterator it = control.find(QLatin1String("ProgressBar_indeterminateDrawable"));
    if (it != control.end())
        m_indeterminateDrawable = AndroidDrawable::fromMap(it.value().toMap(), itemType);
    else
        m_indeterminateDrawable = 0;

    it = control.find(QLatin1String("ProgressBar_progressDrawable"));
    if (it != control.end())
        m_progressDrawable = AndroidDrawable::fromMap(it.value().toMap(), itemType);
    else
        m_progressDrawable = 0;

    it = control.find(QLatin1String("ProgressBar_progress_id"));
    if (it != control.end())
        m_progressId = it.value().toInt();

    it = control.find(QLatin1String("ProgressBar_secondaryProgress_id"));
    if (it != control.end())
        m_secondaryProgress_id = it.value().toInt();

    it = control.find(QLatin1String("ProgressBar_minWidth"));
    if (it != control.end())
        m_minSize.setWidth(it.value().toInt());

    it = control.find(QLatin1String("ProgressBar_minHeight"));
    if (it != control.end())
        m_minSize.setHeight(it.value().toInt());

    it = control.find(QLatin1String("ProgressBar_maxWidth"));
    if (it != control.end())
        m_maxSize.setWidth(it.value().toInt());

    it = control.find(QLatin1String("ProgressBar_maxHeight"));
    if (it != control.end())
        m_maxSize.setHeight(it.value().toInt());
}

QAndroidStyle::AndroidProgressBarControl::~AndroidProgressBarControl()
{
    delete m_progressDrawable;
    delete m_indeterminateDrawable;
}

void QAndroidStyle::AndroidProgressBarControl::drawControl(const QStyleOption *option, QPainter *p, const QWidget * /* w */)
{
    if (!m_progressDrawable)
        return;

    if (const QStyleOptionProgressBar *progressBarOption =
           qstyleoption_cast<const QStyleOptionProgressBar *>(option)) {
        QStyleOptionProgressBarV2 progressBarV2(*progressBarOption);
        if (m_progressDrawable->type() == QAndroidStyle::Layer) {
            QAndroidStyle::AndroidDrawable *clipDrawable = static_cast<QAndroidStyle::AndroidLayerDrawable *>(m_progressDrawable)->layer(m_progressId);
            if (clipDrawable->type() == QAndroidStyle::Clip)
                static_cast<QAndroidStyle::AndroidClipDrawable *>(clipDrawable)->setFactor(double(progressBarV2.progress)/double(progressBarV2.maximum-progressBarV2.minimum),
                                                                                           progressBarV2.orientation);
            else
                static_cast<QAndroidStyle::AndroidLayerDrawable *>(m_progressDrawable)->setFactor(m_progressId,double(progressBarV2.progress)/double(progressBarV2.maximum-progressBarV2.minimum),
                                                                                                  progressBarV2.orientation);
        }
        m_progressDrawable->draw(p, option);
    }
}

QRect QAndroidStyle::AndroidProgressBarControl::subElementRect(QStyle::SubElement subElement,
                                                               const QStyleOption *option,
                                                               const QWidget *widget) const
{
    if (const QStyleOptionProgressBar *progressBarOption =
           qstyleoption_cast<const QStyleOptionProgressBar *>(option)) {
        QStyleOptionProgressBarV2 progressBarV2(*progressBarOption);
        const bool horizontal = progressBarV2.orientation == Qt::Vertical;
        if (!m_background)
            return option->rect;

        QMargins padding = m_background->padding();
        QRect p(padding.left(), padding.top(), padding.right()-padding.left(), padding.bottom()-padding.top());
        padding = m_indeterminateDrawable->padding();
        p |= QRect(padding.left(), padding.top(), padding.right()-padding.left(), padding.bottom()-padding.top());
        padding = m_progressDrawable->padding();
        p |= QRect(padding.left(), padding.top(), padding.right()-padding.left(), padding.bottom()-padding.top());

        QRect r = option->rect.adjusted(p.left(), p.top(), -p.right(), -p.bottom());

        if (horizontal) {
            if (r.height()<m_minSize.height())
                r.setHeight(m_minSize.height());

            if (r.height()>m_maxSize.height())
                r.setHeight(m_maxSize.height());
        } else {
            if (r.width()<m_minSize.width())
                r.setWidth(m_minSize.width());

            if (r.width()>m_maxSize.width())
                r.setWidth(m_maxSize.width());
        }
        return visualRect(option->direction, option->rect, r);
    }
    return AndroidControl::subElementRect(subElement, option, widget);
}

QSize QAndroidStyle::AndroidProgressBarControl::sizeFromContents(const QStyleOption *opt,
                                                                 const QSize &contentsSize,
                                                                 const QWidget * /* w */) const
{
    QSize sz(contentsSize);
    if (sz.height() < m_minSize.height())
        sz.setHeight(m_minSize.height());
    if (sz.width() < m_minSize.width())
        sz.setWidth(m_minSize.width());

    if (const QStyleOptionProgressBar *progressBarOption =
           qstyleoption_cast<const QStyleOptionProgressBar *>(opt)) {
        QStyleOptionProgressBarV2 progressBarV2(*progressBarOption);
        if (progressBarV2.orientation == Qt::Vertical) {
            if (sz.height() > m_maxSize.height())
                sz.setHeight(m_maxSize.height());
        } else {
            if (sz.width() > m_maxSize.width())
                sz.setWidth(m_maxSize.width());
        }
    }
    return contentsSize;
}

QAndroidStyle::AndroidSeekBarControl::AndroidSeekBarControl(const QVariantMap &control,
                                                            ItemType itemType)
    : AndroidProgressBarControl(control, itemType)
{
    QVariantMap::const_iterator it = control.find(QLatin1String("SeekBar_thumb"));
    if (it != control.end())
        m_seekBarThumb = AndroidDrawable::fromMap(it.value().toMap(), itemType);
    else
        m_seekBarThumb = 0;
}

QAndroidStyle::AndroidSeekBarControl::~AndroidSeekBarControl()
{
    delete m_seekBarThumb;
}

void QAndroidStyle::AndroidSeekBarControl::drawControl(const QStyleOption *option,
                                                       QPainter *p,
                                                       const QWidget * /* w */)
{
    if (!m_seekBarThumb || !m_progressDrawable)
        return;

    if (const QStyleOptionSlider *styleOption =
           qstyleoption_cast<const QStyleOptionSlider *>(option)) {
        double factor = double(styleOption->sliderPosition)/double(styleOption->maximum-styleOption->minimum);
        if (m_progressDrawable->type()==QAndroidStyle::Layer) {
            QAndroidStyle::AndroidDrawable *clipDrawable = static_cast<QAndroidStyle::AndroidLayerDrawable *>(m_progressDrawable)->layer(m_progressId);
            if (clipDrawable->type() == QAndroidStyle::Clip)
                static_cast<QAndroidStyle::AndroidClipDrawable *>(clipDrawable)->setFactor(factor, styleOption->orientation);
            else
                static_cast<QAndroidStyle::AndroidLayerDrawable *>(m_progressDrawable)->setFactor(m_progressId, factor, styleOption->orientation);
        }
        const AndroidDrawable *drawable=m_seekBarThumb;
        if (drawable->type() == State)
            drawable = static_cast<const QAndroidStyle::AndroidStateDrawable *>(m_seekBarThumb)->bestAndroidStateMatch(option);
        QStyleOption copy(*option);
        copy.rect.setY((copy.rect.height()-m_minSize.height())/2);
        copy.rect.setHeight(m_minSize.height());
        copy.rect.setWidth(copy.rect.width() - drawable->size().width());
        copy.rect.translate(drawable->size().width()/2, 0);
        m_progressDrawable->draw(p, &copy);
        if (styleOption->orientation == Qt::Vertical)
            qCritical() << "Vertical slider are not supported";
        int pos = copy.rect.width()*factor - drawable->size().width()/2;
        copy.rect.translate(pos, 0);
        copy.rect.setSize(drawable->size());
        m_seekBarThumb->draw(p, &copy);
    }
}

QSize QAndroidStyle::AndroidSeekBarControl::sizeFromContents(const QStyleOption *opt,
                                                             const QSize &contentsSize,
                                                             const QWidget *w) const
{
    QSize sz = AndroidProgressBarControl::sizeFromContents(opt, contentsSize, w);
    if (!m_seekBarThumb)
        return sz;
    const AndroidDrawable *drawable=m_seekBarThumb;
    if (drawable->type() == State)
        drawable = static_cast<const QAndroidStyle::AndroidStateDrawable *>(m_seekBarThumb)->bestAndroidStateMatch(opt);
    return sz.expandedTo(drawable->size());
}

QRect QAndroidStyle::AndroidSeekBarControl::subControlRect(const QStyleOptionComplex *option,
                                                           SubControl sc,
                                                           const QWidget * /* widget */) const
{
    const QStyleOptionSlider *styleOption =
        qstyleoption_cast<const QStyleOptionSlider *>(option);

    if (m_seekBarThumb && sc == SC_SliderHandle && styleOption) {
        const AndroidDrawable *drawable = m_seekBarThumb;
        if (drawable->type() == State)
            drawable = static_cast<const QAndroidStyle::AndroidStateDrawable *>(m_seekBarThumb)->bestAndroidStateMatch(option);

        QRect r(option->rect);
        double factor = double(styleOption->sliderPosition) / (styleOption->maximum - styleOption->minimum);
        int pos = option->rect.width() * factor - double(drawable->size().width() / 2);
        r.setX(r.x() + pos);
        r.setSize(drawable->size());
        return r;
    }
    return option->rect;
}

QAndroidStyle::AndroidSpinnerControl::AndroidSpinnerControl(const QVariantMap &control,
                                                            QAndroidStyle::ItemType itemType)
    : AndroidControl(control, itemType)
{}

QRect QAndroidStyle::AndroidSpinnerControl::subControlRect(const QStyleOptionComplex *option,
                                                           SubControl sc,
                                                           const QWidget *widget) const
{
    if (sc == QStyle::SC_ComboBoxListBoxPopup)
        return option->rect;
    return AndroidControl::subControlRect(option, sc, widget);
}

QT_END_NAMESPACE

#endif // !defined(QT_NO_STYLE_ANDROID) || defined(QT_PLUGIN)
