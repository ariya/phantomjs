/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtCore module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia. For licensing terms and
** conditions see http://qt.digia.com/licensing. For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights. These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
**
****************************************************************************/

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API. It exists for the convenience
// of qfunctions_*. This header file may change from version to version
// without notice, or even be removed.
//
// We mean it.
//

#ifndef QFUNCTIONS_P_H
#define QFUNCTIONS_P_H

#include <QtCore/qglobal.h>

#if defined(Q_OS_WINCE)
#  include "QtCore/qfunctions_wince.h"
#elif defined(Q_OS_VXWORKS)
#  include "QtCore/qfunctions_vxworks.h"
#elif defined(Q_OS_NACL)
#  include "QtCore/qfunctions_nacl.h"
#elif defined(Q_OS_WINRT)
#  include "QtCore/qfunctions_winrt.h"
#endif

#ifdef Q_CC_RVCT
// rvct doesn't see static operators when using our qalgorithms
#  define Q_STATIC_GLOBAL_OPERATOR inline
#  define Q_STATIC_GLOBAL_INLINE_OPERATOR inline
#else
#  define Q_STATIC_GLOBAL_OPERATOR static
#  define Q_STATIC_GLOBAL_INLINE_OPERATOR static inline
#endif

#endif

