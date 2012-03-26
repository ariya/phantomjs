/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the QtCore module of the Qt Toolkit.
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

#ifndef QTEXTBOUNDARYFINDER_H
#define QTEXTBOUNDARYFINDER_H

#include <QtCore/qchar.h>
#include <QtCore/qstring.h>

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE

QT_MODULE(Core)

class QTextBoundaryFinderPrivate;

class Q_CORE_EXPORT QTextBoundaryFinder
{
public:
    QTextBoundaryFinder();
    QTextBoundaryFinder(const QTextBoundaryFinder &other);
    QTextBoundaryFinder &operator=(const QTextBoundaryFinder &other);
    ~QTextBoundaryFinder();

    enum BoundaryType {
        Grapheme,
        Word,
        Line,
        Sentence
    };

    enum BoundaryReason {
        NotAtBoundary = 0,
        StartWord = 1,
        EndWord = 2
        //Hyphen
    };
    Q_DECLARE_FLAGS( BoundaryReasons, BoundaryReason )

    QTextBoundaryFinder(BoundaryType type, const QString &string);
    QTextBoundaryFinder(BoundaryType type, const QChar *chars, int length, unsigned char *buffer = 0, int bufferSize = 0);

    inline bool isValid() const { return d; }

    inline BoundaryType type() const { return t; }
    QString string() const;

    void toStart();
    void toEnd();
    int position() const;
    void setPosition(int position);

    int toNextBoundary();
    int toPreviousBoundary();

    bool isAtBoundary() const;
    BoundaryReasons boundaryReasons() const;

private:
    BoundaryType t;
    QString s;
    const QChar *chars;
    int length;
    int pos;
    uint freePrivate : 1;
    uint unused : 31;
    QTextBoundaryFinderPrivate *d;
};

QT_END_NAMESPACE

QT_END_HEADER

#endif

