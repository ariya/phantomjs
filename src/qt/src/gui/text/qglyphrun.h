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

#ifndef QGLYPHRUN_H
#define QGLYPHRUN_H

#include <QtCore/qsharedpointer.h>
#include <QtCore/qvector.h>
#include <QtCore/qpoint.h>
#include <QtGui/qrawfont.h>

#if !defined(QT_NO_RAWFONT)

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE

QT_MODULE(Gui)

class QGlyphRunPrivate;
class Q_GUI_EXPORT QGlyphRun
{
public:
    QGlyphRun();
    QGlyphRun(const QGlyphRun &other);
    ~QGlyphRun();

    QRawFont rawFont() const;
    void setRawFont(const QRawFont &rawFont);

    void setRawData(const quint32 *glyphIndexArray,
                    const QPointF *glyphPositionArray,
                    int size);

    QVector<quint32> glyphIndexes() const;
    void setGlyphIndexes(const QVector<quint32> &glyphIndexes);

    QVector<QPointF> positions() const;
    void setPositions(const QVector<QPointF> &positions);

    void clear();

    QGlyphRun &operator=(const QGlyphRun &other);

    bool operator==(const QGlyphRun &other) const;
    inline bool operator!=(const QGlyphRun &other) const
    { return !operator==(other); }

    void setOverline(bool overline);
    bool overline() const;

    void setUnderline(bool underline);
    bool underline() const;

    void setStrikeOut(bool strikeOut);
    bool strikeOut() const;

private:
    friend class QGlyphRunPrivate;
    friend class QTextLine;

    QGlyphRun operator+(const QGlyphRun &other) const;
    QGlyphRun &operator+=(const QGlyphRun &other);

    void detach();
    QExplicitlySharedDataPointer<QGlyphRunPrivate> d;
};

QT_END_NAMESPACE

QT_END_HEADER

#endif // QT_NO_RAWFONT

#endif // QGLYPHS_H
