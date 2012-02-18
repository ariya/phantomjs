/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
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

#ifndef QPICTUREFORMATPLUGIN_H
#define QPICTUREFORMATPLUGIN_H

#include <QtCore/qplugin.h>
#include <QtCore/qfactoryinterface.h>

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE

QT_MODULE(Gui)

#if !defined(QT_NO_LIBRARY) && !defined(QT_NO_PICTURE)

class QPicture;
class QImage;
class QString;
class QStringList;

struct Q_GUI_EXPORT QPictureFormatInterface : public QFactoryInterface
{
    virtual bool loadPicture(const QString &format, const QString &filename, QPicture *) = 0;
    virtual bool savePicture(const QString &format, const QString &filename, const QPicture &) = 0;

    virtual bool installIOHandler(const QString &) = 0;
};

#define QPictureFormatInterface_iid "com.trolltech.Qt.QPictureFormatInterface"
Q_DECLARE_INTERFACE(QPictureFormatInterface, QPictureFormatInterface_iid)


class Q_GUI_EXPORT QPictureFormatPlugin : public QObject, public QPictureFormatInterface
{
    Q_OBJECT
    Q_INTERFACES(QPictureFormatInterface:QFactoryInterface)
public:
    explicit QPictureFormatPlugin(QObject *parent = 0);
    ~QPictureFormatPlugin();

    virtual QStringList keys() const = 0;
    virtual bool loadPicture(const QString &format, const QString &filename, QPicture *pic);
    virtual bool savePicture(const QString &format, const QString &filename, const QPicture &pic);
    virtual bool installIOHandler(const QString &format) = 0;

};

#endif // QT_NO_LIBRARY || QT_NO_PICTURE

QT_END_NAMESPACE

QT_END_HEADER

#endif // QPICTUREFORMATPLUGIN_H
