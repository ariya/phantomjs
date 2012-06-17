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

#ifndef QTEXTDOCUMENTFRAGMENT_H
#define QTEXTDOCUMENTFRAGMENT_H

#include <QtCore/qstring.h>

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE

QT_MODULE(Gui)

class QTextStream;
class QTextDocument;
class QTextDocumentFragmentPrivate;
class QTextCursor;

class Q_GUI_EXPORT QTextDocumentFragment
{
public:
    QTextDocumentFragment();
    explicit QTextDocumentFragment(const QTextDocument *document);
    explicit QTextDocumentFragment(const QTextCursor &range);
    QTextDocumentFragment(const QTextDocumentFragment &rhs);
    QTextDocumentFragment &operator=(const QTextDocumentFragment &rhs);
    ~QTextDocumentFragment();

    bool isEmpty() const;

    QString toPlainText() const;
#ifndef QT_NO_TEXTHTMLPARSER
    QString toHtml() const;
    QString toHtml(const QByteArray &encoding) const;
#endif // QT_NO_TEXTHTMLPARSER

    static QTextDocumentFragment fromPlainText(const QString &plainText);
#ifndef QT_NO_TEXTHTMLPARSER
    static QTextDocumentFragment fromHtml(const QString &html);
    static QTextDocumentFragment fromHtml(const QString &html, const QTextDocument *resourceProvider);
#endif // QT_NO_TEXTHTMLPARSER

private:
    QTextDocumentFragmentPrivate *d;
    friend class QTextCursor;
    friend class QTextDocumentWriter;
};

QT_END_NAMESPACE

QT_END_HEADER

#endif // QTEXTDOCUMENTFRAGMENT_H
