/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
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

#ifndef QPLUGINLOADER_H
#define QPLUGINLOADER_H

#include <QtCore/qlibrary.h>

#if defined(QT_NO_LIBRARY) && defined(Q_OS_WIN)
#undef QT_NO_LIBRARY
#pragma message("QT_NO_LIBRARY is not supported on Windows")
#endif

#ifndef QT_NO_LIBRARY

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE

QT_MODULE(Core)

class QLibraryPrivate;

class Q_CORE_EXPORT QPluginLoader : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString fileName READ fileName WRITE setFileName)
    Q_PROPERTY(QLibrary::LoadHints loadHints READ loadHints WRITE setLoadHints)
public:
    explicit QPluginLoader(QObject *parent = 0);
    explicit QPluginLoader(const QString &fileName, QObject *parent = 0);
    ~QPluginLoader();

    QObject *instance();

    static QObjectList staticInstances();

    bool load();
    bool unload();
    bool isLoaded() const;

    void setFileName(const QString &fileName);
    QString fileName() const;

    QString errorString() const;

    void setLoadHints(QLibrary::LoadHints loadHints);
    QLibrary::LoadHints loadHints() const;

private:
    QLibraryPrivate *d;
    bool did_load;
    Q_DISABLE_COPY(QPluginLoader)
};

QT_END_NAMESPACE

QT_END_HEADER

#endif // QT_NO_LIBRARY

#endif //QPLUGINLOADER_H
