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
  codechunk.cpp
*/

#include <qregexp.h>
#include <qstringlist.h>

#include "codechunk.h"

QT_BEGIN_NAMESPACE

enum { Other, Alnum, Gizmo, Comma, LParen, RParen, RAngle, Colon };

// entries 128 and above are Other
static const int charCategory[256] = {
    Other,  Other,  Other,  Other,  Other,  Other,  Other,  Other,
    Other,  Other,  Other,  Other,  Other,  Other,  Other,  Other,
    Other,  Other,  Other,  Other,  Other,  Other,  Other,  Other,
    Other,  Other,  Other,  Other,  Other,  Other,  Other,  Other,
    //          !       "       #       $       %       &       '
    Other,  Other,  Other,  Other,  Other,  Gizmo,  Gizmo,  Other,
    //  (       )       *       +       ,       -       .       /
    LParen, RParen, Gizmo,  Gizmo,  Comma,  Other,  Other,  Gizmo,
    //  0       1       2       3       4       5       6       7
    Alnum,  Alnum,  Alnum,  Alnum,  Alnum,  Alnum,  Alnum,  Alnum,
    //  8       9       :       ;       <       =       >       ?
    Alnum,  Alnum,  Colon,  Other,  Other,  Gizmo,  RAngle, Gizmo,
    //  @       A       B       C       D       E       F       G
    Other,  Alnum,  Alnum,  Alnum,  Alnum,  Alnum,  Alnum,  Alnum,
    //  H       I       J       K       L       M       N       O
    Alnum,  Alnum,  Alnum,  Alnum,  Alnum,  Alnum,  Alnum,  Alnum,
    //  P       Q       R       S       T       U       V       W
    Alnum,  Alnum,  Alnum,  Alnum,  Alnum,  Alnum,  Alnum,  Alnum,
    //  X       Y       Z       [       \       ]       ^       _
    Alnum,  Alnum,  Alnum,  Other,  Other,  Other,  Gizmo,  Alnum,
    //  `       a       b       c       d       e       f       g
    Other,  Alnum,  Alnum,  Alnum,  Alnum,  Alnum,  Alnum,  Alnum,
    //  h       i       j       k       l       m       n       o
    Alnum,  Alnum,  Alnum,  Alnum,  Alnum,  Alnum,  Alnum,  Alnum,
    //  p       q       r       s       t       u       v       w
    Alnum,  Alnum,  Alnum,  Alnum,  Alnum,  Alnum,  Alnum,  Alnum,
    //  x       y       z       {       |       }       ~
    Alnum,  Alnum,  Alnum,  LParen, Gizmo,  RParen, Other,  Other
};

static const bool needSpace[8][8] = {
    /*        [      a      +      ,      (       )     >      :  */
    /* [ */ { false, false, false, false, false, true,  false, false },
    /* a */ { false, true,  true,  false, false, true,  false, false },
    /* + */ { false, true,  false, false, false, true,  false, true },
    /* , */ { true,  true,  true,  true,  true,  true,  true,  true },
    /* ( */ { true,  true,  true,  false, true,  false, true,  true },
    /* ) */ { true,  true,  true,  false, true,  true,  true,  true },
    /* > */ { true,  true,  true,  false, true,  true,  true,  false },
    /* : */ { false, false, true,  true,  true,  true,  true,  false }
};

static int category( QChar ch )
{
    return charCategory[(int)ch.toLatin1()];
}

CodeChunk::CodeChunk()
    : hotspot( -1 )
{
}

CodeChunk::CodeChunk( const QString& str )
    : s( str ), hotspot( -1 )
{
}

void CodeChunk::append( const QString& lexeme )
{
    if ( !s.isEmpty() && !lexeme.isEmpty() ) {
        /*
          Should there be a space or not between the code chunk so far and the
          new lexeme?
        */
        int cat1 = category(s.at(s.size() - 1));
        int cat2 = category(lexeme[0]);
        if ( needSpace[cat1][cat2] )
            s += QLatin1Char( ' ' );
    }
    s += lexeme;
}

void CodeChunk::appendHotspot()
{
    /*
      The first hotspot is the right one.
    */
    if ( hotspot == -1 )
        hotspot = s.length();
}

QString CodeChunk::toString() const
{
    return s;
}

QStringList CodeChunk::toPath() const
{
    QString t = s;
    t.remove(QRegExp(QLatin1String("<([^<>]|<([^<>]|<[^<>]*>)*>)*>")));
    return t.split(QLatin1String("::"));
}

QT_END_NAMESPACE
