/****************************************************************************
**
** Copyright (C) 2012 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtGui module of the Qt Toolkit.
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

#include <QtCore/qglobal.h>

#if !defined(QT_NO_RAWFONT)

#include "qrawfont_p.h"
#include "qfontengine_ft_p.h"
#include "quuid.h"

#if defined(Q_WS_X11) && !defined(QT_NO_FONTCONFIG)
#  include "qfontengine_x11_p.h"
#endif

QT_BEGIN_NAMESPACE

class QFontEngineFTRawFont

#if defined(Q_WS_X11) && !defined(QT_NO_FONTCONFIG)
        : public QFontEngineX11FT
#else
        : public QFontEngineFT
#endif

{
public:
    QFontEngineFTRawFont(const QFontDef &fontDef)
#if defined(Q_WS_X11) && !defined(QT_NO_FONTCONFIG)
        : QFontEngineX11FT(fontDef)
#else
        : QFontEngineFT(fontDef)
#endif
    {
    }

    void updateFamilyNameAndStyle()
    {
        fontDef.family = QString::fromAscii(freetype->face->family_name);

        if (freetype->face->style_flags & FT_STYLE_FLAG_ITALIC)
            fontDef.style = QFont::StyleItalic;

        if (freetype->face->style_flags & FT_STYLE_FLAG_BOLD)
            fontDef.weight = QFont::Bold;
    }

    bool initFromData(const QByteArray &fontData)
    {
        FaceId faceId;
        faceId.filename = "";
        faceId.index = 0;
        faceId.uuid = QUuid::createUuid().toByteArray();

        return init(faceId, true, Format_None, fontData);
    }
};


void QRawFontPrivate::platformCleanUp()
{
    // Font engine handles all resources
}

void QRawFontPrivate::platformLoadFromData(const QByteArray &fontData, qreal pixelSize,
                                           QFont::HintingPreference hintingPreference)
{
    Q_ASSERT(fontEngine == 0);

    QFontDef fontDef;
    fontDef.pixelSize = pixelSize;

    QFontEngineFTRawFont *fe = new QFontEngineFTRawFont(fontDef);
    if (!fe->initFromData(fontData)) {
        delete fe;
        return;
    }

    fe->updateFamilyNameAndStyle();

    switch (hintingPreference) {
    case QFont::PreferNoHinting:
        fe->setDefaultHintStyle(QFontEngineFT::HintNone);
        break;
    case QFont::PreferFullHinting:
        fe->setDefaultHintStyle(QFontEngineFT::HintFull);
        break;
    case QFont::PreferVerticalHinting:
        fe->setDefaultHintStyle(QFontEngineFT::HintLight);
        break;
    default:
        // Leave it as it is
        break;
    }

    fontEngine = fe;
    fontEngine->ref.ref();
}

QT_END_NAMESPACE

#endif // QT_NO_RAWFONT
