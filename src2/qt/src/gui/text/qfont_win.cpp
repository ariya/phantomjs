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

#include "qfont.h"
#include "qfont_p.h"
#include "qfontengine_p.h"
#include "qtextengine_p.h"
#include "qfontmetrics.h"
#include "qfontinfo.h"

#include "qwidget.h"
#include "qpainter.h"
#include <limits.h>
#include "qt_windows.h"
#include <private/qapplication_p.h>
#include "qapplication.h"
#include <private/qunicodetables_p.h>
#include <qfontdatabase.h>

QT_BEGIN_NAMESPACE
        
extern HDC   shared_dc();                // common dc for all fonts
extern QFont::Weight weightFromInteger(int weight); // qfontdatabase.cpp

// ### maybe move to qapplication_win
QFont qt_LOGFONTtoQFont(LOGFONT& lf, bool /*scale*/)
{
    QString family = QString::fromWCharArray(lf.lfFaceName);
    QFont qf(family);
    qf.setItalic(lf.lfItalic);
    if (lf.lfWeight != FW_DONTCARE)
        qf.setWeight(weightFromInteger(lf.lfWeight));
    int lfh = qAbs(lf.lfHeight);
    qf.setPointSizeF(lfh * 72.0 / GetDeviceCaps(shared_dc(),LOGPIXELSY));
    qf.setUnderline(false);
    qf.setOverline(false);
    qf.setStrikeOut(false);
    return qf;
}


static inline float pixelSize(const QFontDef &request, int dpi)
{
    float pSize;
    if (request.pointSize != -1)
        pSize = request.pointSize * dpi/ 72.;
    else
        pSize = request.pixelSize;
    return pSize;
}

static inline float pointSize(const QFontDef &fd, int dpi)
{
    float pSize;
    if (fd.pointSize < 0)
        pSize = fd.pixelSize * 72. / ((float)dpi);
    else
        pSize = fd.pointSize;
    return pSize;
}

/*****************************************************************************
  QFont member functions
 *****************************************************************************/

void QFont::initialize()
{
}

void QFont::cleanup()
{
    QFontCache::cleanup();
}

HFONT QFont::handle() const
{
    QFontEngine *engine = d->engineForScript(QUnicodeTables::Common);
    Q_ASSERT(engine != 0);
    if (engine->type() == QFontEngine::Multi)
        engine = static_cast<QFontEngineMulti *>(engine)->engine(0);
    if (engine->type() == QFontEngine::Win)
	return static_cast<QFontEngineWin *>(engine)->hfont;
    return 0;
}

QString QFont::rawName() const
{
    return family();
}

void QFont::setRawName(const QString &name)
{
    setFamily(name);
}

QString QFont::defaultFamily() const
{
    switch(d->request.styleHint) {
        case QFont::Times:
            return QString::fromLatin1("Times New Roman");
        case QFont::Courier:
        case QFont::Monospace:
            return QString::fromLatin1("Courier New");
        case QFont::Decorative:
            return QString::fromLatin1("Bookman Old Style");
        case QFont::Cursive:
            return QString::fromLatin1("Comic Sans MS");
        case QFont::Fantasy:
            return QString::fromLatin1("Impact");
        case QFont::Helvetica:
            return QString::fromLatin1("Arial");
        case QFont::System:
        default:
            return QString::fromLatin1("MS Sans Serif");
    }
}

QString QFont::lastResortFamily() const
{
    return QString::fromLatin1("helvetica");
}

QString QFont::lastResortFont() const
{
    return QString::fromLatin1("arial");
}

QT_END_NAMESPACE
