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

#ifndef CUSTOMWIDGETSINFO_H
#define CUSTOMWIDGETSINFO_H

#include "treewalker.h"
#include <QtCore/QStringList>
#include <QtCore/QMap>

QT_BEGIN_NAMESPACE

class Driver;
class DomScript;

class CustomWidgetsInfo : public TreeWalker
{
public:
    CustomWidgetsInfo();

    void acceptUI(DomUI *node);

    void acceptCustomWidgets(DomCustomWidgets *node);
    void acceptCustomWidget(DomCustomWidget *node);

    inline QStringList customWidgets() const
    { return m_customWidgets.keys(); }

    inline bool hasCustomWidget(const QString &name) const
    { return m_customWidgets.contains(name); }

    inline DomCustomWidget *customWidget(const QString &name) const
    { return m_customWidgets.value(name); }

    DomScript *customWidgetScript(const QString &name) const;

    QString customWidgetAddPageMethod(const QString &name) const;

    QString realClassName(const QString &className) const;

    bool extends(const QString &className, const QLatin1String &baseClassName) const;

    bool isCustomWidgetContainer(const QString &className) const;

private:
    typedef QMap<QString, DomCustomWidget*> NameCustomWidgetMap;
    NameCustomWidgetMap m_customWidgets;
    bool m_scriptsActivated;
};

QT_END_NAMESPACE

#endif // CUSTOMWIDGETSINFO_H
