/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the tools applications of the Qt Toolkit.
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

#ifndef UIC_H
#define UIC_H

#include "databaseinfo.h"
#include "customwidgetsinfo.h"
#include <QtCore/QString>
#include <QtCore/QStringList>
#include <QtCore/QHash>
#include <QtCore/QStack>
#include <QtCore/QXmlStreamReader>

QT_BEGIN_NAMESPACE

class QTextStream;
class QIODevice;

class Driver;
class DomUI;
class DomWidget;
class DomSpacer;
class DomLayout;
class DomLayoutItem;
class DomItem;

struct Option;

class Uic
{
public:
    Uic(Driver *driver);
    ~Uic();

    bool printDependencies();

    inline Driver *driver() const
    { return drv; }

    inline QTextStream &output()
    { return out; }

    inline const Option &option() const
    { return opt; }

    inline QString pixmapFunction() const
    { return pixFunction; }

    inline void setPixmapFunction(const QString &f)
    { pixFunction = f; }

    inline bool hasExternalPixmap() const
    { return externalPix; }

    inline void setExternalPixmap(bool b)
    { externalPix = b; }

    inline const DatabaseInfo *databaseInfo() const
    { return &info; }

    inline const CustomWidgetsInfo *customWidgetsInfo() const
    { return &cWidgetsInfo; }

    bool write(QIODevice *in);

#ifdef QT_UIC_JAVA_GENERATOR
    bool jwrite(DomUI *ui);
#endif

#ifdef QT_UIC_CPP_GENERATOR
    bool write(DomUI *ui);
#endif

    bool isMainWindow(const QString &className) const;
    bool isToolBar(const QString &className) const;
    bool isStatusBar(const QString &className) const;
    bool isButton(const QString &className) const;
    bool isContainer(const QString &className) const;
    bool isCustomWidgetContainer(const QString &className) const;
    bool isMenuBar(const QString &className) const;
    bool isMenu(const QString &className) const;

private:
    // copyright header
    void writeCopyrightHeader(DomUI *ui);
    DomUI *parseUiFile(QXmlStreamReader &reader);

#ifdef QT_UIC_CPP_GENERATOR
    // header protection
    void writeHeaderProtectionStart();
    void writeHeaderProtectionEnd();
#endif

private:
    Driver *drv;
    QTextStream &out;
    Option &opt;
    DatabaseInfo info;
    CustomWidgetsInfo cWidgetsInfo;
    QString pixFunction;
    bool externalPix;
};

QT_END_NAMESPACE

#endif // UIC_H
