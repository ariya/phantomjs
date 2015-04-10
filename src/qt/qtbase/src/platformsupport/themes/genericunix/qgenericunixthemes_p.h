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

#ifndef QGENERICUNIXTHEMES_H
#define QGENERICUNIXTHEMES_H

#include <qpa/qplatformtheme.h>
#include <QtCore/QString>
#include <QtCore/QStringList>
#include <QtGui/QFont>

QT_BEGIN_NAMESPACE

class ResourceHelper
{
public:
    ResourceHelper();
    ~ResourceHelper() { clear(); }

    void clear();

    QPalette *palettes[QPlatformTheme::NPalettes];
    QFont *fonts[QPlatformTheme::NFonts];
};

class QGenericUnixThemePrivate;

class QGenericUnixTheme : public QPlatformTheme
{
    Q_DECLARE_PRIVATE(QGenericUnixTheme)
public:
    QGenericUnixTheme();

    static QPlatformTheme *createUnixTheme(const QString &name);
    static QStringList themeNames();

    virtual const QFont *font(Font type) const;
    virtual QVariant themeHint(ThemeHint hint) const;

    static QStringList xdgIconThemePaths();

    static const char *name;
};

#ifndef QT_NO_SETTINGS
class QKdeThemePrivate;

class QKdeTheme : public QPlatformTheme
{
    Q_DECLARE_PRIVATE(QKdeTheme)
public:
    QKdeTheme(const QString &kdeHome, int kdeVersion);

    static QPlatformTheme *createKdeTheme();
    virtual QVariant themeHint(ThemeHint hint) const;

    virtual const QPalette *palette(Palette type = SystemPalette) const;

    virtual const QFont *font(Font type) const;

    static const char *name;
};
#endif // QT_NO_SETTINGS

class QGnomeThemePrivate;

class QGnomeTheme : public QPlatformTheme
{
    Q_DECLARE_PRIVATE(QGnomeTheme)
public:
    QGnomeTheme();
    virtual QVariant themeHint(ThemeHint hint) const;
    virtual const QFont *font(Font type) const;
    QString standardButtonText(int button) const Q_DECL_OVERRIDE;

    static const char *name;
};

QPlatformTheme *qt_createUnixTheme();

QT_END_NAMESPACE

#endif // QGENERICUNIXTHEMES_H
