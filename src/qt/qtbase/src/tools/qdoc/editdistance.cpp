/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the tools applications of the Qt Toolkit.
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

/*
  editdistance.cpp
*/

#include "editdistance.h"

QT_BEGIN_NAMESPACE

int editDistance( const QString& s, const QString& t )
{
#define D( i, j ) d[(i) * n + (j)]
    int i;
    int j;
    int m = s.length() + 1;
    int n = t.length() + 1;
    int *d = new int[m * n];
    int result;

    for ( i = 0; i < m; i++ )
        D( i, 0 ) = i;
    for ( j = 0; j < n; j++ )
        D( 0, j ) = j;
    for ( i = 1; i < m; i++ ) {
        for ( j = 1; j < n; j++ ) {
            if ( s[i - 1] == t[j - 1] ) {
                D( i, j ) = D( i - 1, j - 1 );
            } else {
                int x = D( i - 1, j );
                int y = D( i - 1, j - 1 );
                int z = D( i, j - 1 );
                D( i, j ) = 1 + qMin( qMin(x, y), z );
            }
        }
    }
    result = D( m - 1, n - 1 );
    delete[] d;
    return result;
#undef D
}

QString nearestName( const QString& actual, const QSet<QString>& candidates )
{
    if (actual.isEmpty())
        return QString();

    int deltaBest = 10000;
    int numBest = 0;
    QString best;

    QSet<QString>::ConstIterator c = candidates.constBegin();
    while ( c != candidates.constEnd() ) {
        if ( (*c)[0] == actual[0] ) {
            int delta = editDistance( actual, *c );
            if ( delta < deltaBest ) {
                deltaBest = delta;
                numBest = 1;
                best = *c;
            } else if ( delta == deltaBest ) {
                numBest++;
            }
        }
        ++c;
    }

    if ( numBest == 1 && deltaBest <= 2 &&
         actual.length() + best.length() >= 5 ) {
        return best;
    } else {
        return QString();
    }
}

QT_END_NAMESPACE
