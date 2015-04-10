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

#ifndef QPLATFORMTHEME_COCOA_H
#define QPLATFORMTHEME_COCOA_H

#include <QtCore/QHash>
#include <qpa/qplatformtheme.h>

QT_BEGIN_NAMESPACE

class QPalette;
class QCocoaTheme : public QPlatformTheme
{
public:
    QCocoaTheme();
    ~QCocoaTheme();

    virtual QPlatformMenuItem* createPlatformMenuItem() const;
    virtual QPlatformMenu* createPlatformMenu() const;
    virtual QPlatformMenuBar* createPlatformMenuBar() const;

#ifndef QT_NO_SYSTEMTRAYICON
    QPlatformSystemTrayIcon *createPlatformSystemTrayIcon() const;
#endif

    bool usePlatformNativeDialog(DialogType dialogType) const;
    QPlatformDialogHelper *createPlatformDialogHelper(DialogType dialogType) const;

    const QPalette *palette(Palette type = SystemPalette) const;
    const QFont *font(Font type = SystemFont) const;
    QPixmap standardPixmap(StandardPixmap sp, const QSizeF &size) const;
    QPixmap fileIconPixmap(const QFileInfo &fileInfo,
                           const QSizeF &size,
                           QPlatformTheme::IconOptions options = 0) const;

    QVariant themeHint(ThemeHint hint) const;
    QString standardButtonText(int button) const Q_DECL_OVERRIDE;

    static const char *name;

private:
    mutable QPalette *m_systemPalette;
    mutable QHash<QPlatformTheme::Palette, QPalette*> m_palettes;
    mutable QHash<QPlatformTheme::Font, QFont*> m_fonts;
};

QT_END_NAMESPACE

#endif
