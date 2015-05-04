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

#ifndef QCORETEXTFONTDATABASE_H
#define QCORETEXTFONTDATABASE_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <qglobal.h>
#define HAVE_CORETEXT QT_MAC_PLATFORM_SDK_EQUAL_OR_ABOVE(__MAC_10_8, __IPHONE_4_1)
#define HAVE_ATS QT_MAC_PLATFORM_SDK_EQUAL_OR_ABOVE(__MAC_10_5, __IPHONE_NA)

#include <qpa/qplatformfontdatabase.h>
#include <qpa/qplatformtheme.h>
#include <private/qcore_mac_p.h>

#ifndef Q_OS_IOS
#include <ApplicationServices/ApplicationServices.h>
#else
#include <CoreText/CoreText.h>
#include <CoreGraphics/CoreGraphics.h>
#endif

#if HAVE_CORETEXT
Q_DECLARE_METATYPE(QCFType<CGFontRef>);
Q_DECLARE_METATYPE(QCFType<CFURLRef>);
#endif
#if HAVE_ATS
Q_DECLARE_METATYPE(ATSFontContainerRef);
#endif

QT_BEGIN_NAMESPACE

struct FontDescription;

class QCoreTextFontDatabase : public QPlatformFontDatabase
{
public:
    QCoreTextFontDatabase();
    ~QCoreTextFontDatabase();
    void populateFontDatabase();
    void populateFamily(const QString &familyName) Q_DECL_OVERRIDE;

    QFontEngine *fontEngine(const QFontDef &fontDef, void *handle);
    QFontEngine *fontEngine(const QByteArray &fontData, qreal pixelSize, QFont::HintingPreference hintingPreference);
    QStringList fallbacksForFamily(const QString &family, QFont::Style style, QFont::StyleHint styleHint, QChar::Script script) const;
    QStringList addApplicationFont(const QByteArray &fontData, const QString &fileName);
    void releaseHandle(void *handle);
    bool isPrivateFontFamily(const QString &family) const;
    QFont defaultFont() const;
    QList<int> standardSizes() const;

    // For iOS and OS X platform themes
    QFont *themeFont(QPlatformTheme::Font) const;
    const QHash<QPlatformTheme::Font, QFont *> &themeFonts() const;

private:
    void populateFromDescriptor(CTFontDescriptorRef font);
    void populateFromFontDescription(CTFontDescriptorRef font, const FontDescription &fd);

    mutable QString defaultFontName;

    void removeApplicationFonts();

    QVector<QVariant> m_applicationFonts;
    mutable QSet<CTFontDescriptorRef> m_systemFontDescriptors;
    mutable QHash<QPlatformTheme::Font, QFont *> m_themeFonts;
};

QT_END_NAMESPACE

#endif // QCORETEXTFONTDATABASE_H
