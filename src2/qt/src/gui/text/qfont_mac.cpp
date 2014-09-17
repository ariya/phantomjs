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
#include "qfontengine_mac_p.h"
#include "qfontengine_coretext_p.h"
#include "qfontinfo.h"
#include "qfontmetrics.h"
#include "qpaintdevice.h"
#include "qstring.h"
#include <private/qt_mac_p.h>
#include <private/qtextengine_p.h>
#include <private/qunicodetables_p.h>
#include <qapplication.h>
#include "qfontdatabase.h"
#include <qpainter.h>
#include "qtextengine_p.h"
#include <stdlib.h>

QT_BEGIN_NAMESPACE

extern float qt_mac_defaultDpi_x(); //qpaintdevice_mac.cpp

int qt_mac_pixelsize(const QFontDef &def, int dpi)
{
    float ret;
    if(def.pixelSize == -1)
        ret = def.pointSize *  dpi / qt_mac_defaultDpi_x();
    else
        ret = def.pixelSize;
    return qRound(ret);
}
int qt_mac_pointsize(const QFontDef &def, int dpi)
{
    float ret;
    if(def.pointSize < 0)
        ret = def.pixelSize * qt_mac_defaultDpi_x() / float(dpi);
    else
        ret = def.pointSize;
    return qRound(ret);
}

QString QFont::rawName() const
{
    return family();
}

void QFont::setRawName(const QString &name)
{
    setFamily(name);
}

void QFont::cleanup()
{
    QFontCache::cleanup();
}

/*!
  Returns an ATSUFontID
*/
quint32 QFont::macFontID() const  // ### need 64-bit version
{
#ifdef QT_MAC_USE_COCOA
    return 0;
#elif 1
    QFontEngine *fe = d->engineForScript(QUnicodeTables::Common);
    if (fe && fe->type() == QFontEngine::Multi)
        return static_cast<QFontEngineMacMulti*>(fe)->macFontID();
#else
    Str255 name;
    if(FMGetFontFamilyName((FMFontFamily)((UInt32)handle()), name) == noErr) {
        short fnum;
        GetFNum(name, &fnum);
        return fnum;
    }
#endif
    return 0;
}

// Returns an ATSUFonFamilyRef
Qt::HANDLE QFont::handle() const
{
#ifdef QT_MAC_USE_COCOA
    QFontEngine *fe = d->engineForScript(QUnicodeTables::Common);
    if (fe && fe->type() == QFontEngine::Multi)
        return (Qt::HANDLE)static_cast<QCoreTextFontEngineMulti*>(fe)->macFontID();
#endif
    return 0;
}

void QFont::initialize()
{ }

QString QFont::defaultFamily() const
{
    switch(d->request.styleHint) {
        case QFont::Times:
            return QString::fromLatin1("Times New Roman");
        case QFont::Courier:
            return QString::fromLatin1("Courier New");
        case QFont::Monospace:
            return QString::fromLatin1("Courier");
        case QFont::Decorative:
            return QString::fromLatin1("Bookman Old Style");
        case QFont::Cursive:
            return QString::fromLatin1("Apple Chancery");
        case QFont::Fantasy:
            return QString::fromLatin1("Papyrus");
        case QFont::Helvetica:
        case QFont::System:
        default:
            return QString::fromLatin1("Helvetica");
    }
}

QString QFont::lastResortFamily() const
{
    return QString::fromLatin1("Helvetica");
}

QString QFont::lastResortFont() const
{
    return QString::fromLatin1("Geneva");
}

QT_END_NAMESPACE
