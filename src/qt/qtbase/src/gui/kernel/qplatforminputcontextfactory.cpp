/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtGui module of the Qt Toolkit.
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

#include <qpa/qplatforminputcontextfactory_p.h>
#include <qpa/qplatforminputcontextplugin_p.h>
#include <qpa/qplatforminputcontext.h>
#include "private/qfactoryloader_p.h"

#include "qguiapplication.h"
#include "qdebug.h"
#include <stdlib.h>

QT_BEGIN_NAMESPACE

#if !defined(QT_NO_LIBRARY) && !defined(QT_NO_SETTINGS)
Q_GLOBAL_STATIC_WITH_ARGS(QFactoryLoader, loader,
    (QPlatformInputContextFactoryInterface_iid, QLatin1String("/platforminputcontexts"), Qt::CaseInsensitive))
#endif

QStringList QPlatformInputContextFactory::keys()
{
#if !defined(QT_NO_LIBRARY) && !defined(QT_NO_SETTINGS)
    return loader()->keyMap().values();
#else
    return QStringList();
#endif
}

QPlatformInputContext *QPlatformInputContextFactory::create(const QString& key)
{
    QStringList paramList = key.split(QLatin1Char(':'));
    const QString platform = paramList.takeFirst().toLower();

#if !defined(QT_NO_LIBRARY) && !defined(QT_NO_SETTINGS)
    if (QPlatformInputContext *ret = qLoadPlugin1<QPlatformInputContext, QPlatformInputContextPlugin>(loader(), platform, paramList))
        return ret;
#endif
    return 0;
}

QPlatformInputContext *QPlatformInputContextFactory::create()
{
    QPlatformInputContext *ic = 0;

    QString icString = QString::fromLatin1(qgetenv("QT_IM_MODULE"));

    if (icString == QLatin1String("none"))
        icString = QStringLiteral("compose");

    ic = create(icString);
    if (ic && ic->isValid())
        return ic;

    delete ic;
    ic = 0;

    QStringList k = keys();
    for (int i = 0; i < k.size(); ++i) {
        if (k.at(i) == icString)
            continue;
        ic = create(k.at(i));
        if (ic && ic->isValid())
            return ic;
        delete ic;
        ic = 0;
    }

    return 0;
}


QT_END_NAMESPACE

