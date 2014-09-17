/****************************************************************************
**
** Copyright (C) 2012 Digia Plc and/or its subsidiary(-ies).
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

#ifndef QPRINTERINFO_P_H
#define QPRINTERINFO_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include "QtCore/qglobal.h"

#ifndef QT_NO_PRINTER

#include "QtCore/qlist.h"

QT_BEGIN_NAMESPACE

class QPrinterInfoPrivate
{
public:
    QPrinterInfoPrivate(const QString& name = QString()) :
        name(name), isDefault(false)
#if (defined(Q_OS_UNIX) && !defined(Q_OS_MAC) && !defined(Q_OS_SYMBIAN)) || defined(Q_WS_QPA)
#if !defined(QT_NO_CUPS) && !defined(QT_NO_LIBRARY)
        , cupsPrinterIndex(0), hasPaperSizes(false)
#endif
#endif
    {}
    ~QPrinterInfoPrivate()
    {}

    static QPrinterInfoPrivate shared_null;

    QString name;
    bool isDefault;

#if (defined(Q_OS_UNIX) && !defined(Q_OS_MAC) && !defined(Q_OS_SYMBIAN)) || defined(Q_WS_QPA)
#if !defined(QT_NO_CUPS) && !defined(QT_NO_LIBRARY)
    int cupsPrinterIndex;
    mutable bool hasPaperSizes;
    mutable QList<QPrinter::PaperSize> paperSizes;
#endif
#endif
};


class QPrinterInfoPrivateDeleter
{
public:
    static inline void cleanup(QPrinterInfoPrivate *d)
    {
        if (d != &QPrinterInfoPrivate::shared_null)
            delete d;
    }
};

QT_END_NAMESPACE

#endif // QT_NO_PRINTER

#endif // QPRINTERINFO_P_H
