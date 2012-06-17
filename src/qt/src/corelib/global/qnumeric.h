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

#ifndef QNUMERIC_H
#define QNUMERIC_H

#include <QtCore/qglobal.h>

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE

QT_MODULE(Core)

Q_CORE_EXPORT bool qIsInf(double d);
Q_CORE_EXPORT bool qIsNaN(double d);
Q_CORE_EXPORT bool qIsFinite(double d);
Q_CORE_EXPORT bool qIsInf(float f);
Q_CORE_EXPORT bool qIsNaN(float f);
Q_CORE_EXPORT bool qIsFinite(float f);
Q_CORE_EXPORT double qSNaN();
Q_CORE_EXPORT double qQNaN();
Q_CORE_EXPORT double qInf();

#define Q_INFINITY (QT_PREPEND_NAMESPACE(qInf)())
#define Q_SNAN (QT_PREPEND_NAMESPACE(qSNaN)())
#define Q_QNAN (QT_PREPEND_NAMESPACE(qQNaN)())

QT_END_NAMESPACE

QT_END_HEADER

#endif // QNUMERIC_H
