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
  quoter.h
*/

#ifndef QUOTER_H
#define QUOTER_H

#include <qhash.h>
#include <qstringlist.h>

#include "location.h"

QT_BEGIN_NAMESPACE

class Quoter
{
    Q_DECLARE_TR_FUNCTIONS(QDoc::Quoter)

public:
    Quoter();

    void reset();
    void quoteFromFile( const QString& userFriendlyFileName,
                        const QString& plainCode, const QString& markedCode );
    QString quoteLine( const Location& docLocation, const QString& command,
                       const QString& pattern );
    QString quoteTo( const Location& docLocation, const QString& command,
                     const QString& pattern );
    QString quoteUntil( const Location& docLocation, const QString& command,
                        const QString& pattern );
    QString quoteSnippet(const Location &docLocation, const QString &identifier);

    static QStringList splitLines(const QString &line);

private:
    QString getLine(int unindent = 0);
    void failedAtEnd( const Location& docLocation, const QString& command );
    bool match( const Location& docLocation, const QString& pattern,
                const QString& line );
    QString commentForCode() const;
    QString removeSpecialLines(const QString &line, const QString &comment,
                               int unindent = 0);

    bool silent;
    QStringList plainLines;
    QStringList markedLines;
    Location codeLocation;
    QHash<QString,QString> commentHash;
};

QT_END_NAMESPACE

#endif
