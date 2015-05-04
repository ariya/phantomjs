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

#ifndef QWINDOWSTHEME_H
#define QWINDOWSTHEME_H

#include <qpa/qplatformtheme.h>
#include <QtGui/QColor>

#include "qtwindows_additional.h"

QT_BEGIN_NAMESPACE

class QWindow;

class QWindowsTheme : public QPlatformTheme
{
public:
    QWindowsTheme();
    ~QWindowsTheme();

    static QWindowsTheme *instance() { return m_instance; }

    bool usePlatformNativeDialog(DialogType type) const Q_DECL_OVERRIDE;
    QPlatformDialogHelper *createPlatformDialogHelper(DialogType type) const Q_DECL_OVERRIDE;
    QVariant themeHint(ThemeHint) const Q_DECL_OVERRIDE;
    virtual const QPalette *palette(Palette type = SystemPalette) const
        { return m_palettes[type]; }
    virtual const QFont *font(Font type = SystemFont) const
        { return m_fonts[type]; }

    QPixmap standardPixmap(StandardPixmap sp, const QSizeF &size) const Q_DECL_OVERRIDE;
    QPixmap fileIconPixmap(const QFileInfo &fileInfo, const QSizeF &size,
                           QPlatformTheme::IconOptions iconOptions = 0) const Q_DECL_OVERRIDE;

    void windowsThemeChanged(QWindow *window);

    static const char *name;

private:
    void refresh() { refreshPalettes(); refreshFonts(); }
    void clearPalettes();
    void refreshPalettes();
    void clearFonts();
    void refreshFonts();

    static QWindowsTheme *m_instance;
    QPalette *m_palettes[NPalettes];
    QFont *m_fonts[NFonts];
};

static inline COLORREF qColorToCOLORREF(const QColor &color)
{ return RGB(color.red(), color.green(), color.blue()); }

static inline QColor COLORREFToQColor(COLORREF cr)
{ return QColor(GetRValue(cr), GetGValue(cr), GetBValue(cr)); }

QT_END_NAMESPACE

#endif // QWINDOWSTHEME_H
