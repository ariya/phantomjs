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
  openedlist.h
*/

#ifndef OPENEDLIST_H
#define OPENEDLIST_H

#include <qstring.h>

#include "location.h"

QT_BEGIN_NAMESPACE

class OpenedList
{
    Q_DECLARE_TR_FUNCTIONS(QDoc::OpenedList)

public:
    enum Style { Bullet, Tag, Value, Numeric, UpperAlpha, LowerAlpha,
                 UpperRoman, LowerRoman };

    OpenedList()
        : sty( Bullet ), ini( 1 ), nex( 0 ) { }
    OpenedList( Style style );
    OpenedList( const Location& location, const QString& hint );

    void next() { nex++; }

    bool isStarted() const { return nex >= ini; }
    Style style() const { return sty; }
    QString styleString() const;
    int number() const { return nex; }
    QString numberString() const;
    QString prefix() const { return pref; }
    QString suffix() const { return suff; }

private:
    static QString toAlpha( int n );
    static int fromAlpha( const QString& str );
    static QString toRoman( int n );
    static int fromRoman( const QString& str );

    Style sty;
    int ini;
    int nex;
    QString pref;
    QString suff;
};

QT_END_NAMESPACE

#endif
