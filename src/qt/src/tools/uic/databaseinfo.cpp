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

#include "databaseinfo.h"
#include "driver.h"
#include "ui4.h"
#include "utils.h"

QT_BEGIN_NAMESPACE

DatabaseInfo::DatabaseInfo(Driver *drv)
    : driver(drv)
{
}

void DatabaseInfo::acceptUI(DomUI *node)
{
    m_connections.clear();
    m_cursors.clear();
    m_fields.clear();

    TreeWalker::acceptUI(node);

    m_connections = unique(m_connections);
}

void DatabaseInfo::acceptWidget(DomWidget *node)
{
    QHash<QString, DomProperty*> properties = propertyMap(node->elementProperty());

    DomProperty *frameworkCode = properties.value(QLatin1String("frameworkCode"), 0);
    if (frameworkCode && toBool(frameworkCode->elementBool()) == false)
        return;

    DomProperty *db = properties.value(QLatin1String("database"), 0);
    if (db && db->elementStringList()) {
        QStringList info = db->elementStringList()->elementString();

        QString connection = info.size() > 0 ? info.at(0) : QString();
        if (connection.isEmpty())
            return;
        m_connections.append(connection);

        QString table = info.size() > 1 ? info.at(1) : QString();
        if (table.isEmpty())
            return;
        m_cursors[connection].append(table);

        QString field = info.size() > 2 ? info.at(2) : QString();
        if (field.isEmpty())
            return;
        m_fields[connection].append(field);
    }

    TreeWalker::acceptWidget(node);
}

QT_END_NAMESPACE
