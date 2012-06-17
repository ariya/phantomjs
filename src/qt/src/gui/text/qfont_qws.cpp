/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the QtGui module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qwidget.h"
#include "qpainter.h"
#include "qfont_p.h"
#include <private/qunicodetables_p.h>
#include "qfontdatabase.h"
#include "qtextcodec.h"
#include "qapplication.h"
#include "qfile.h"
#include "qtextstream.h"
#include "qmap.h"
//#include "qmemorymanager_qws.h"
#include "qtextengine_p.h"
#include "qfontengine_p.h"
#if !defined(QT_NO_FREETYPE)
#include "qfontengine_ft_p.h"
#endif

QT_BEGIN_NAMESPACE

void QFont::initialize()
{ }

void QFont::cleanup()
{
    QFontCache::cleanup();
}


/*****************************************************************************
  QFont member functions
 *****************************************************************************/

Qt::HANDLE QFont::handle() const
{
#ifndef QT_NO_FREETYPE
    return freetypeFace();
#endif
    return 0;
}

FT_Face QFont::freetypeFace() const
{
#ifndef QT_NO_FREETYPE
    QFontEngine *engine = d->engineForScript(QUnicodeTables::Common);
    if (engine->type() == QFontEngine::Multi)
        engine = static_cast<QFontEngineMulti *>(engine)->engine(0);
    if (engine->type() == QFontEngine::Freetype) {
        const QFontEngineFT *ft = static_cast<const QFontEngineFT *>(engine);
        return ft->non_locked_face();
    }
#endif
    return 0;
}

QString QFont::rawName() const
{
    return QLatin1String("unknown");
}

void QFont::setRawName(const QString &)
{
}

QString QFont::defaultFamily() const
{
    switch(d->request.styleHint) {
        case QFont::Times:
            return QString::fromLatin1("times");
        case QFont::Courier:
        case QFont::Monospace:
            return QString::fromLatin1("courier");
        case QFont::Decorative:
            return QString::fromLatin1("old english");
        case QFont::Helvetica:
        case QFont::System:
        default:
            return QString::fromLatin1("helvetica");
    }
}

QString QFont::lastResortFamily() const
{
    return QString::fromLatin1("helvetica");
}

QString QFont::lastResortFont() const
{
    qFatal("QFont::lastResortFont: Cannot find any reasonable font");
    // Shut compiler up
    return QString();
}


QT_END_NAMESPACE
