/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
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

#include "qgenericunixthemes_p.h"
#include "../../services/genericunix/qgenericunixservices_p.h"

#include "qpa/qplatformtheme_p.h"

#include <QtGui/QPalette>
#include <QtGui/QFont>
#include <QtGui/QGuiApplication>
#include <QtCore/QDir>
#include <QtCore/QFileInfo>
#include <QtCore/QFile>
#include <QtCore/QDebug>
#include <QtCore/QHash>
#include <QtCore/QSettings>
#include <QtCore/QVariant>
#include <QtCore/QStringList>
#include <private/qguiapplication_p.h>
#include <qpa/qplatformintegration.h>
#include <qpa/qplatformservices.h>
#include <qpa/qplatformdialoghelper.h>

#include <algorithm>

QT_BEGIN_NAMESPACE

ResourceHelper::ResourceHelper()
{
    std::fill(palettes, palettes + QPlatformTheme::NPalettes, static_cast<QPalette *>(0));
    std::fill(fonts, fonts + QPlatformTheme::NFonts, static_cast<QFont *>(0));
}

void ResourceHelper::clear()
{
    qDeleteAll(palettes, palettes + QPlatformTheme::NPalettes);
    qDeleteAll(fonts, fonts + QPlatformTheme::NFonts);
    std::fill(palettes, palettes + QPlatformTheme::NPalettes, static_cast<QPalette *>(0));
    std::fill(fonts, fonts + QPlatformTheme::NFonts, static_cast<QFont *>(0));
}

/*!
    \class QGenericX11ThemeQKdeTheme
    \brief QGenericX11Theme is a generic theme implementation for X11.
    \since 5.0
    \internal
    \ingroup qpa
*/

const char *QGenericUnixTheme::name = "generic";

// Default system font, corresponding to the value returned by 4.8 for
// XRender/FontConfig which we can now assume as default.
static const char defaultSystemFontNameC[] = "Sans Serif";
enum { defaultSystemFontSize = 9 };

class QGenericUnixThemePrivate : public QPlatformThemePrivate
{
public:
    QGenericUnixThemePrivate()
        : QPlatformThemePrivate()
        , systemFont(QLatin1String(defaultSystemFontNameC), defaultSystemFontSize)
        , fixedFont(QStringLiteral("monospace"), systemFont.pointSize())
    {
        fixedFont.setStyleHint(QFont::TypeWriter);
    }

    const QFont systemFont;
    QFont fixedFont;
};

QGenericUnixTheme::QGenericUnixTheme()
    : QPlatformTheme(new QGenericUnixThemePrivate())
{
}

const QFont *QGenericUnixTheme::font(Font type) const
{
    Q_D(const QGenericUnixTheme);
    switch (type) {
    case QPlatformTheme::SystemFont:
        return &d->systemFont;
    case QPlatformTheme::FixedFont:
        return &d->fixedFont;
    default:
        return 0;
    }
}

// Helper to return the icon theme paths from XDG.
QStringList QGenericUnixTheme::xdgIconThemePaths()
{
    QStringList paths;
    // Add home directory first in search path
    const QFileInfo homeIconDir(QDir::homePath() + QStringLiteral("/.icons"));
    if (homeIconDir.isDir())
        paths.prepend(homeIconDir.absoluteFilePath());

    QString xdgDirString = QFile::decodeName(qgetenv("XDG_DATA_DIRS"));
    if (xdgDirString.isEmpty())
        xdgDirString = QLatin1String("/usr/local/share/:/usr/share/");
    foreach (const QString &xdgDir, xdgDirString.split(QLatin1Char(':'))) {
        const QFileInfo xdgIconsDir(xdgDir + QStringLiteral("/icons"));
        if (xdgIconsDir.isDir())
            paths.append(xdgIconsDir.absoluteFilePath());
    }
    return paths;
}

QVariant QGenericUnixTheme::themeHint(ThemeHint hint) const
{
    switch (hint) {
    case QPlatformTheme::SystemIconFallbackThemeName:
        return QVariant(QString(QStringLiteral("hicolor")));
    case QPlatformTheme::IconThemeSearchPaths:
        return xdgIconThemePaths();
    case QPlatformTheme::DialogButtonBoxButtonsHaveIcons:
        return QVariant(true);
    case QPlatformTheme::StyleNames: {
        QStringList styleNames;
        styleNames << QStringLiteral("Fusion") << QStringLiteral("Windows");
        return QVariant(styleNames);
    }
    case QPlatformTheme::KeyboardScheme:
        return QVariant(int(X11KeyboardScheme));
    default:
        break;
    }
    return QPlatformTheme::themeHint(hint);
}

#ifndef QT_NO_SETTINGS
class QKdeThemePrivate : public QPlatformThemePrivate
{
public:
    QKdeThemePrivate(const QStringList &kdeDirs, int kdeVersion)
        : kdeDirs(kdeDirs)
        , kdeVersion(kdeVersion)
        , toolButtonStyle(Qt::ToolButtonTextBesideIcon)
        , toolBarIconSize(0)
        , singleClick(true)
    { }

    static QString kdeGlobals(const QString &kdeDir)
    {
        return kdeDir + QStringLiteral("/share/config/kdeglobals");
    }

    void refresh();
    static QVariant readKdeSetting(const QString &key, const QStringList &kdeDirs, QHash<QString, QSettings*> &kdeSettings);
    static void readKdeSystemPalette(const QStringList &kdeDirs, QHash<QString, QSettings*> &kdeSettings, QPalette *pal);
    static QFont *kdeFont(const QVariant &fontValue);
    static QStringList kdeIconThemeSearchPaths(const QStringList &kdeDirs);

    const QStringList kdeDirs;
    const int kdeVersion;

    ResourceHelper resources;
    QString iconThemeName;
    QString iconFallbackThemeName;
    QStringList styleNames;
    int toolButtonStyle;
    int toolBarIconSize;
    bool singleClick;
};

void QKdeThemePrivate::refresh()
{
    resources.clear();

    toolButtonStyle = Qt::ToolButtonTextBesideIcon;
    toolBarIconSize = 0;
    styleNames.clear();
    styleNames << QStringLiteral("Oxygen") << QStringLiteral("fusion") << QStringLiteral("windows");
    iconFallbackThemeName = iconThemeName = QStringLiteral("oxygen");

    QHash<QString, QSettings*> kdeSettings;

    QPalette systemPalette = QPalette();
    readKdeSystemPalette(kdeDirs, kdeSettings, &systemPalette);
    resources.palettes[QPlatformTheme::SystemPalette] = new QPalette(systemPalette);
    //## TODO tooltip color

    const QVariant styleValue = readKdeSetting(QStringLiteral("widgetStyle"), kdeDirs, kdeSettings);
    if (styleValue.isValid()) {
        const QString style = styleValue.toString();
        if (style != styleNames.front())
            styleNames.push_front(style);
    }

    const QVariant singleClickValue = readKdeSetting(QStringLiteral("KDE/SingleClick"), kdeDirs, kdeSettings);
    if (singleClickValue.isValid())
        singleClick = singleClickValue.toBool();

    const QVariant themeValue = readKdeSetting(QStringLiteral("Icons/Theme"), kdeDirs, kdeSettings);
    if (themeValue.isValid())
        iconThemeName = themeValue.toString();

    const QVariant toolBarIconSizeValue = readKdeSetting(QStringLiteral("ToolbarIcons/Size"), kdeDirs, kdeSettings);
    if (toolBarIconSizeValue.isValid())
        toolBarIconSize = toolBarIconSizeValue.toInt();

    const QVariant toolbarStyleValue = readKdeSetting(QStringLiteral("Toolbar style/ToolButtonStyle"), kdeDirs, kdeSettings);
    if (toolbarStyleValue.isValid()) {
        const QString toolBarStyle = toolbarStyleValue.toString();
        if (toolBarStyle == QLatin1String("TextBesideIcon"))
            toolButtonStyle =  Qt::ToolButtonTextBesideIcon;
        else if (toolBarStyle == QLatin1String("TextOnly"))
            toolButtonStyle = Qt::ToolButtonTextOnly;
        else if (toolBarStyle == QLatin1String("TextUnderIcon"))
            toolButtonStyle = Qt::ToolButtonTextUnderIcon;
    }

    // Read system font, ignore 'smallestReadableFont'
    if (QFont *systemFont = kdeFont(readKdeSetting(QStringLiteral("font"), kdeDirs, kdeSettings)))
        resources.fonts[QPlatformTheme::SystemFont] = systemFont;
    else
        resources.fonts[QPlatformTheme::SystemFont] = new QFont(QLatin1String(defaultSystemFontNameC), defaultSystemFontSize);

    if (QFont *fixedFont = kdeFont(readKdeSetting(QStringLiteral("fixed"), kdeDirs, kdeSettings))) {
        resources.fonts[QPlatformTheme::FixedFont] = fixedFont;
    } else {
        fixedFont = new QFont(QLatin1String(defaultSystemFontNameC), defaultSystemFontSize);
        fixedFont->setStyleHint(QFont::TypeWriter);
        resources.fonts[QPlatformTheme::FixedFont] = fixedFont;
    }

    qDeleteAll(kdeSettings);
}

QVariant QKdeThemePrivate::readKdeSetting(const QString &key, const QStringList &kdeDirs, QHash<QString, QSettings*> &kdeSettings)
{
    foreach (const QString &kdeDir, kdeDirs) {
        QSettings *settings = kdeSettings.value(kdeDir);
        if (!settings) {
            const QString kdeGlobalsPath = kdeGlobals(kdeDir);
            if (QFileInfo(kdeGlobalsPath).isReadable()) {
                settings = new QSettings(kdeGlobalsPath, QSettings::IniFormat);
                kdeSettings.insert(kdeDir, settings);
            }
        }
        if (settings) {
            const QVariant value = settings->value(key);
            if (value.isValid())
                return value;
        }
    }
    return QVariant();
}

// Reads the color from the KDE configuration, and store it in the
// palette with the given color role if found.
static inline bool kdeColor(QPalette *pal, QPalette::ColorRole role, const QVariant &value)
{
    if (!value.isValid())
        return false;
    const QStringList values = value.toStringList();
    if (values.size() != 3)
        return false;
    pal->setBrush(role, QColor(values.at(0).toInt(), values.at(1).toInt(), values.at(2).toInt()));
    return true;
}

void QKdeThemePrivate::readKdeSystemPalette(const QStringList &kdeDirs, QHash<QString, QSettings*> &kdeSettings, QPalette *pal)
{
    if (!kdeColor(pal, QPalette::Button, readKdeSetting(QStringLiteral("Colors:Button/BackgroundNormal"), kdeDirs, kdeSettings))) {
        // kcolorscheme.cpp: SetDefaultColors
        const QColor defaultWindowBackground(214, 210, 208);
        const QColor defaultButtonBackground(223, 220, 217);
        *pal = QPalette(defaultButtonBackground, defaultWindowBackground);
        return;
    }

    kdeColor(pal, QPalette::Window, readKdeSetting(QStringLiteral("Colors:Window/BackgroundNormal"), kdeDirs, kdeSettings));
    kdeColor(pal, QPalette::Text, readKdeSetting(QStringLiteral("Colors:View/ForegroundNormal"), kdeDirs, kdeSettings));
    kdeColor(pal, QPalette::WindowText, readKdeSetting(QStringLiteral("Colors:Window/ForegroundNormal"), kdeDirs, kdeSettings));
    kdeColor(pal, QPalette::Base, readKdeSetting(QStringLiteral("Colors:View/BackgroundNormal"), kdeDirs, kdeSettings));
    kdeColor(pal, QPalette::Highlight, readKdeSetting(QStringLiteral("Colors:Selection/BackgroundNormal"), kdeDirs, kdeSettings));
    kdeColor(pal, QPalette::HighlightedText, readKdeSetting(QStringLiteral("Colors:Selection/ForegroundNormal"), kdeDirs, kdeSettings));
    kdeColor(pal, QPalette::AlternateBase, readKdeSetting(QStringLiteral("Colors:View/BackgroundAlternate"), kdeDirs, kdeSettings));
    kdeColor(pal, QPalette::ButtonText, readKdeSetting(QStringLiteral("Colors:Button/ForegroundNormal"), kdeDirs, kdeSettings));
    kdeColor(pal, QPalette::Link, readKdeSetting(QStringLiteral("Colors:View/ForegroundLink"), kdeDirs, kdeSettings));
    kdeColor(pal, QPalette::LinkVisited, readKdeSetting(QStringLiteral("Colors:View/ForegroundVisited"), kdeDirs, kdeSettings));
    kdeColor(pal, QPalette::ToolTipBase, readKdeSetting(QStringLiteral("Colors:Tooltip/BackgroundNormal"), kdeDirs, kdeSettings));
    kdeColor(pal, QPalette::ToolTipText, readKdeSetting(QStringLiteral("Colors:Tooltip/ForegroundNormal"), kdeDirs, kdeSettings));

    // The above code sets _all_ color roles to "normal" colors. In KDE, the disabled
    // color roles are calculated by applying various effects described in kdeglobals.
    // We use a bit simpler approach here, similar logic than in qt_palette_from_color().
    const QColor button = pal->color(QPalette::Button);
    int h, s, v;
    button.getHsv(&h, &s, &v);

    const QBrush whiteBrush = QBrush(Qt::white);
    const QBrush buttonBrush = QBrush(button);
    const QBrush buttonBrushDark = QBrush(button.darker(v > 128 ? 200 : 50));
    const QBrush buttonBrushDark150 = QBrush(button.darker(v > 128 ? 150 : 75));
    const QBrush buttonBrushLight150 = QBrush(button.lighter(v > 128 ? 150 : 75));
    const QBrush buttonBrushLight = QBrush(button.lighter(v > 128 ? 200 : 50));

    pal->setBrush(QPalette::Disabled, QPalette::WindowText, buttonBrushDark);
    pal->setBrush(QPalette::Disabled, QPalette::ButtonText, buttonBrushDark);
    pal->setBrush(QPalette::Disabled, QPalette::Button, buttonBrush);
    pal->setBrush(QPalette::Disabled, QPalette::Text, buttonBrushDark);
    pal->setBrush(QPalette::Disabled, QPalette::BrightText, whiteBrush);
    pal->setBrush(QPalette::Disabled, QPalette::Base, buttonBrush);
    pal->setBrush(QPalette::Disabled, QPalette::Window, buttonBrush);
    pal->setBrush(QPalette::Disabled, QPalette::Highlight, buttonBrushDark150);
    pal->setBrush(QPalette::Disabled, QPalette::HighlightedText, buttonBrushLight150);

    // set calculated colors for all groups
    pal->setBrush(QPalette::Light, buttonBrushLight);
    pal->setBrush(QPalette::Midlight, buttonBrushLight150);
    pal->setBrush(QPalette::Mid, buttonBrushDark150);
    pal->setBrush(QPalette::Dark, buttonBrushDark);
}

/*!
    \class QKdeTheme
    \brief QKdeTheme is a theme implementation for the KDE desktop (version 4 or higher).
    \since 5.0
    \internal
    \ingroup qpa
*/

const char *QKdeTheme::name = "kde";

QKdeTheme::QKdeTheme(const QStringList& kdeDirs, int kdeVersion)
    : QPlatformTheme(new QKdeThemePrivate(kdeDirs,kdeVersion))
{
    d_func()->refresh();
}

QFont *QKdeThemePrivate::kdeFont(const QVariant &fontValue)
{
    if (fontValue.isValid()) {
        // Read font value: Might be a QStringList as KDE stores fonts without quotes.
        // Also retrieve the family for the constructor since we cannot use the
        // default constructor of QFont, which accesses QGuiApplication::systemFont()
        // causing recursion.
        QString fontDescription;
        QString fontFamily;
        if (fontValue.type() == QVariant::StringList) {
            const QStringList list = fontValue.toStringList();
            if (!list.isEmpty()) {
                fontFamily = list.first();
                fontDescription = list.join(QLatin1Char(','));
            }
        } else {
            fontDescription = fontFamily = fontValue.toString();
        }
        if (!fontDescription.isEmpty()) {
            QFont font(fontFamily);
            if (font.fromString(fontDescription))
                return new QFont(font);
        }
    }
    return 0;
}


QStringList QKdeThemePrivate::kdeIconThemeSearchPaths(const QStringList &kdeDirs)
{
    QStringList paths = QGenericUnixTheme::xdgIconThemePaths();
    const QString iconPath = QStringLiteral("/share/icons");
    foreach (const QString &candidate, kdeDirs) {
        const QFileInfo fi(candidate + iconPath);
        if (fi.isDir())
            paths.append(fi.absoluteFilePath());
    }
    return paths;
}

QVariant QKdeTheme::themeHint(QPlatformTheme::ThemeHint hint) const
{
    Q_D(const QKdeTheme);
    switch (hint) {
    case QPlatformTheme::UseFullScreenForPopupMenu:
        return QVariant(true);
    case QPlatformTheme::DialogButtonBoxButtonsHaveIcons:
        return QVariant(true);
    case QPlatformTheme::DialogButtonBoxLayout:
        return QVariant(QPlatformDialogHelper::KdeLayout);
    case QPlatformTheme::ToolButtonStyle:
        return QVariant(d->toolButtonStyle);
    case QPlatformTheme::ToolBarIconSize:
        return QVariant(d->toolBarIconSize);
    case QPlatformTheme::SystemIconThemeName:
        return QVariant(d->iconThemeName);
    case QPlatformTheme::SystemIconFallbackThemeName:
        return QVariant(d->iconFallbackThemeName);
    case QPlatformTheme::IconThemeSearchPaths:
        return QVariant(d->kdeIconThemeSearchPaths(d->kdeDirs));
    case QPlatformTheme::StyleNames:
        return QVariant(d->styleNames);
    case QPlatformTheme::KeyboardScheme:
        return QVariant(int(KdeKeyboardScheme));
    case QPlatformTheme::ItemViewActivateItemOnSingleClick:
        return QVariant(d->singleClick);
    default:
        break;
    }
    return QPlatformTheme::themeHint(hint);
}

const QPalette *QKdeTheme::palette(Palette type) const
{
    Q_D(const QKdeTheme);
    return d->resources.palettes[type];
}

const QFont *QKdeTheme::font(Font type) const
{
    Q_D(const QKdeTheme);
    return d->resources.fonts[type];
}

QPlatformTheme *QKdeTheme::createKdeTheme()
{
    const QByteArray kdeVersionBA = qgetenv("KDE_SESSION_VERSION");
    const int kdeVersion = kdeVersionBA.toInt();
    if (kdeVersion < 4)
        return 0;

    // Determine KDE prefixes in the following priority order:
    // - KDEHOME and KDEDIRS environment variables
    // - ~/.kde(<version>)
    // - read prefixes from /etc/kde<version>rc
    // - fallback to /etc/kde<version>

    QStringList kdeDirs;
    const QString kdeHomePathVar = QFile::decodeName(qgetenv("KDEHOME"));
    if (!kdeHomePathVar.isEmpty())
        kdeDirs += kdeHomePathVar;

    const QString kdeDirsVar = QFile::decodeName(qgetenv("KDEDIRS"));
    if (!kdeDirsVar.isEmpty())
        kdeDirs += kdeDirsVar.split(QLatin1Char(':'), QString::SkipEmptyParts);

    const QString kdeVersionHomePath = QDir::homePath() + QStringLiteral("/.kde") + QLatin1String(kdeVersionBA);
    if (QFileInfo(kdeVersionHomePath).isDir())
        kdeDirs += kdeVersionHomePath;

    const QString kdeHomePath = QDir::homePath() + QStringLiteral("/.kde");
    if (QFileInfo(kdeHomePath).isDir())
        kdeDirs += kdeHomePath;

    const QString kdeRcPath = QStringLiteral("/etc/kde") + QLatin1String(kdeVersionBA) + QStringLiteral("rc");
    if (QFileInfo(kdeRcPath).isReadable()) {
        QSettings kdeSettings(kdeRcPath, QSettings::IniFormat);
        kdeSettings.beginGroup(QStringLiteral("Directories-default"));
        kdeDirs += kdeSettings.value(QStringLiteral("prefixes")).toStringList();
    }

    const QString kdeVersionPrefix = QStringLiteral("/etc/kde") + QLatin1String(kdeVersionBA);
    if (QFileInfo(kdeVersionPrefix).isDir())
        kdeDirs += kdeVersionPrefix;

    kdeDirs.removeDuplicates();
    if (kdeDirs.isEmpty()) {
        qWarning("%s: Unable to determine KDE dirs", Q_FUNC_INFO);
        return 0;
    }

    return new QKdeTheme(kdeDirs, kdeVersion);
}

#endif // QT_NO_SETTINGS

/*!
    \class QGnomeTheme
    \brief QGnomeTheme is a theme implementation for the Gnome desktop.
    \since 5.0
    \internal
    \ingroup qpa
*/

const char *QGnomeTheme::name = "gnome";

class QGnomeThemePrivate : public QPlatformThemePrivate
{
public:
    QGnomeThemePrivate() : fontsConfigured(false) { }
    void configureFonts(QString gtkFontName) const
    {
        Q_ASSERT(!fontsConfigured);
        const int split = gtkFontName.lastIndexOf(QChar::Space);
        float size = gtkFontName.mid(split+1).toFloat();
        QString fontName = gtkFontName.left(split);

        systemFont = QFont(fontName, size);
        fixedFont = QFont(QLatin1String("monospace"), systemFont.pointSize());
        fixedFont.setStyleHint(QFont::TypeWriter);
        fontsConfigured = true;
    }

    mutable QFont systemFont;
    mutable QFont fixedFont;
    mutable bool fontsConfigured;
};

QGnomeTheme::QGnomeTheme()
    : QPlatformTheme(new QGnomeThemePrivate())
{
}

QVariant QGnomeTheme::themeHint(QPlatformTheme::ThemeHint hint) const
{
    switch (hint) {
    case QPlatformTheme::DialogButtonBoxButtonsHaveIcons:
        return QVariant(true);
    case QPlatformTheme::DialogButtonBoxLayout:
        return QVariant(QPlatformDialogHelper::GnomeLayout);
    case QPlatformTheme::SystemIconThemeName:
    case QPlatformTheme::SystemIconFallbackThemeName:
        return QVariant(QString(QStringLiteral("gnome")));
    case QPlatformTheme::IconThemeSearchPaths:
        return QVariant(QGenericUnixTheme::xdgIconThemePaths());
    case QPlatformTheme::StyleNames: {
        QStringList styleNames;
        styleNames << QStringLiteral("GTK+") << QStringLiteral("fusion") << QStringLiteral("windows");
        return QVariant(styleNames);
    }
    case QPlatformTheme::KeyboardScheme:
        return QVariant(int(GnomeKeyboardScheme));
    case QPlatformTheme::PasswordMaskCharacter:
        return QVariant(QChar(0x2022));
    default:
        break;
    }
    return QPlatformTheme::themeHint(hint);
}

const QFont *QGnomeTheme::font(Font type) const
{
    Q_D(const QGnomeTheme);
    if (!d->fontsConfigured)
        d->configureFonts(gtkFontName());
    switch (type) {
    case QPlatformTheme::SystemFont:
        return &d->systemFont;
    case QPlatformTheme::FixedFont:
        return &d->fixedFont;
    default:
        return 0;
    }
}

QString QGnomeTheme::gtkFontName() const
{
    return QStringLiteral("%1 %2").arg(QLatin1String(defaultSystemFontNameC)).arg(defaultSystemFontSize);
}

QString QGnomeTheme::standardButtonText(int button) const
{
    switch (button) {
    case QPlatformDialogHelper::Ok:
        return QCoreApplication::translate("QGnomeTheme", "&OK");
    case QPlatformDialogHelper::Save:
        return QCoreApplication::translate("QGnomeTheme", "&Save");
    case QPlatformDialogHelper::Cancel:
        return QCoreApplication::translate("QGnomeTheme", "&Cancel");
    case QPlatformDialogHelper::Close:
        return QCoreApplication::translate("QGnomeTheme", "&Close");
    case QPlatformDialogHelper::Discard:
        return QCoreApplication::translate("QGnomeTheme", "Close without Saving");
    default:
        break;
    }
    return QPlatformTheme::standardButtonText(button);
}

/*!
    \brief Creates a UNIX theme according to the detected desktop environment.
*/

QPlatformTheme *QGenericUnixTheme::createUnixTheme(const QString &name)
{
    if (name == QLatin1String(QGenericUnixTheme::name))
        return new QGenericUnixTheme;
#ifndef QT_NO_SETTINGS
    if (name == QLatin1String(QKdeTheme::name))
        if (QPlatformTheme *kdeTheme = QKdeTheme::createKdeTheme())
            return kdeTheme;
#endif
    if (name == QLatin1String(QGnomeTheme::name))
        return new QGnomeTheme;
    return new QGenericUnixTheme;
}

QStringList QGenericUnixTheme::themeNames()
{
    QStringList result;
    if (QGuiApplication::desktopSettingsAware()) {
        const QByteArray desktopEnvironment = QGuiApplicationPrivate::platformIntegration()->services()->desktopEnvironment();
        if (desktopEnvironment == "KDE") {
#ifndef QT_NO_SETTINGS
            result.push_back(QLatin1String(QKdeTheme::name));
#endif
        } else if (desktopEnvironment == "GNOME" ||
                desktopEnvironment == "X-CINNAMON" ||
                desktopEnvironment == "UNITY" ||
                desktopEnvironment == "MATE" ||
                desktopEnvironment == "XFCE" ||
                desktopEnvironment == "LXDE") { // Gtk-based desktops
            // prefer the GTK2 theme implementation with native dialogs etc.
            result.push_back(QStringLiteral("gtk2"));
            // fallback to the generic Gnome theme if loading the GTK2 theme fails
            result.push_back(QLatin1String(QGnomeTheme::name));
        }
        const QString session = QString::fromLocal8Bit(qgetenv("DESKTOP_SESSION"));
        if (!session.isEmpty() && session != QLatin1String("default") && !result.contains(session))
            result.push_back(session);
    } // desktopSettingsAware
    if (result.isEmpty())
        result.push_back(QLatin1String(QGenericUnixTheme::name));
    return result;
}

QT_END_NAMESPACE
