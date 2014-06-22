/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
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

#include <QtCore/qglobal.h>

#ifndef QISENUM_H
#define QISENUM_H

#ifndef Q_IS_ENUM
#  if defined(Q_CC_GNU) && (__GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 3))
#    define Q_IS_ENUM(x) __is_enum(x)
#  elif defined(Q_CC_MSVC) && defined(_MSC_FULL_VER) && (_MSC_FULL_VER >=140050215)
#    define Q_IS_ENUM(x) __is_enum(x)
#  elif defined(Q_CC_CLANG)
#    if __has_extension(is_enum)
#      define Q_IS_ENUM(x) __is_enum(x)
#    endif
#  endif
#endif

#ifndef Q_IS_ENUM
#  include <QtCore/qtypetraits.h>
#  define Q_IS_ENUM(x) QtPrivate::is_enum<x>::value
#endif

// shut up syncqt
QT_BEGIN_NAMESPACE
QT_END_NAMESPACE
#endif // QISENUM_H
