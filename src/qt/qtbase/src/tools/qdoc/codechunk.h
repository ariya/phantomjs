/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the tools applications of the Qt Toolkit.
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

/*
  codechunk.h
*/

#ifndef CODECHUNK_H
#define CODECHUNK_H

#include <qstring.h>

QT_BEGIN_NAMESPACE

// ### get rid of that class

/*
  The CodeChunk class represents a tiny piece of C++ code.

  The class provides conversion between a list of lexemes and a string.  It adds
  spaces at the right place for consistent style.  The tiny pieces of code it
  represents are data types, enum values, and default parameter values.

  Apart from the piece of code itself, there are two bits of metainformation
  stored in CodeChunk: the base and the hotspot.  The base is the part of the
  piece that may be a hypertext link.  The base of

      QMap<QString, QString>

  is QMap.

  The hotspot is the place the variable name should be inserted in the case of a
  variable (or parameter) declaration.  The base of

      char * []

  is between '*' and '[]'.
*/
class CodeChunk
{
public:
    CodeChunk();
    CodeChunk( const QString& str );

    void append( const QString& lexeme );
    void appendHotspot();

    bool isEmpty() const { return s.isEmpty(); }
    QString toString() const;
    QStringList toPath() const;
    QString left() const { return s.left(hotspot == -1 ? s.length() : hotspot); }
    QString right() const { return s.mid(hotspot == -1 ? s.length() : hotspot); }

private:
    QString s;
    int hotspot;
};

inline bool operator==( const CodeChunk& c, const CodeChunk& d ) {
    return c.toString() == d.toString();
}

inline bool operator!=( const CodeChunk& c, const CodeChunk& d ) {
    return !( c == d );
}

inline bool operator<( const CodeChunk& c, const CodeChunk& d ) {
    return c.toString() < d.toString();
}

inline bool operator>( const CodeChunk& c, const CodeChunk& d ) {
    return d < c;
}

inline bool operator<=( const CodeChunk& c, const CodeChunk& d ) {
    return !( c > d );
}

inline bool operator>=( const CodeChunk& c, const CodeChunk& d ) {
    return !( c < d );
}

QT_END_NAMESPACE

#endif
