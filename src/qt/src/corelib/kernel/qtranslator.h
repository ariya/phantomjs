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

#ifndef QTRANSLATOR_H
#define QTRANSLATOR_H

#include <QtCore/qobject.h>
#include <QtCore/qbytearray.h>

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE

QT_MODULE(Core)

#ifndef QT_NO_TRANSLATION

class QLocale;
class QTranslatorPrivate;

class Q_CORE_EXPORT QTranslator : public QObject
{
    Q_OBJECT
public:
    explicit QTranslator(QObject *parent = 0);
#ifdef QT3_SUPPORT
    QT3_SUPPORT_CONSTRUCTOR QTranslator(QObject * parent, const char * name);
#endif
    ~QTranslator();

    // ### Qt 5: Merge (with "int n = -1")
    virtual QString translate(const char *context, const char *sourceText,
                              const char *disambiguation = 0) const;
    QString translate(const char *context, const char *sourceText, const char *disambiguation,
                      int n) const;

    virtual bool isEmpty() const;

    bool load(const QString & filename,
              const QString & directory = QString(),
              const QString & search_delimiters = QString(),
              const QString & suffix = QString());
    bool load(const QLocale & locale,
              const QString & filename,
              const QString & prefix = QString(),
              const QString & directory = QString(),
              const QString & suffix = QString());
    bool load(const uchar *data, int len);

#ifdef QT3_SUPPORT
    QT3_SUPPORT QString find(const char *context, const char *sourceText, const char * comment = 0) const
        { return translate(context, sourceText, comment); }
#endif

private:
    Q_DISABLE_COPY(QTranslator)
    Q_DECLARE_PRIVATE(QTranslator)
};

#endif // QT_NO_TRANSLATION

QT_END_NAMESPACE

QT_END_HEADER

#endif // QTRANSLATOR_H
