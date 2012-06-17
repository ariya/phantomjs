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

#ifndef QTRANSLATOR_P_H
#define QTRANSLATOR_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists for the convenience
// of qfontencodings_x11.cpp and qfont_x11.cpp.  This header file may
// change from version to version without notice, or even be removed.
//
// We mean it.
//

enum {
    Q_EQ          = 0x01,
    Q_LT          = 0x02,
    Q_LEQ         = 0x03,
    Q_BETWEEN     = 0x04,

    Q_NOT         = 0x08,
    Q_MOD_10      = 0x10,
    Q_MOD_100     = 0x20,
    Q_LEAD_1000   = 0x40,

    Q_AND         = 0xFD,
    Q_OR          = 0xFE,
    Q_NEWRULE     = 0xFF,

    Q_OP_MASK     = 0x07,

    Q_NEQ         = Q_NOT | Q_EQ,
    Q_GT          = Q_NOT | Q_LEQ,
    Q_GEQ         = Q_NOT | Q_LT,
    Q_NOT_BETWEEN = Q_NOT | Q_BETWEEN
};

#endif
