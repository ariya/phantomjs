/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtCore module of the Qt Toolkit.
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

#ifndef QATOMIC_ARM_H
#define QATOMIC_ARM_H

QT_BEGIN_HEADER

#if defined(__ARM_ARCH_7__) \
    || defined(__ARM_ARCH_7A__) \
    || defined(__ARM_ARCH_7R__) \
    || defined(__ARM_ARCH_7M__)
# define QT_ARCH_ARMV7
QT_BEGIN_INCLUDE_HEADER
# include "QtCore/qatomic_armv7.h"
QT_END_INCLUDE_HEADER
#elif defined(__ARM_ARCH_6__) \
    || defined(__ARM_ARCH_6J__) \
    || defined(__ARM_ARCH_6T2__) \
    || defined(__ARM_ARCH_6Z__) \
    || defined(__ARM_ARCH_6K__) \
    || defined(__ARM_ARCH_6ZK__) \
    || defined(__ARM_ARCH_6M__) \
    || (defined(__TARGET_ARCH_ARM) && (__TARGET_ARCH_ARM-0 >= 6))
# define QT_ARCH_ARMV6
QT_BEGIN_INCLUDE_HEADER
# include "QtCore/qatomic_armv6.h"
QT_END_INCLUDE_HEADER
#else
# define QT_ARCH_ARMV5
QT_BEGIN_INCLUDE_HEADER
# include "QtCore/qatomic_armv5.h"
QT_END_INCLUDE_HEADER
#endif

QT_END_HEADER

#endif // QATOMIC_ARM_H
