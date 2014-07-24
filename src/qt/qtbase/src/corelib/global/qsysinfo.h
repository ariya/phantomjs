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

#ifndef QSYSINFO_H
#define QSYSINFO_H

QT_BEGIN_NAMESPACE

/*
   System information
*/

class QString;
class Q_CORE_EXPORT QSysInfo {
public:
    enum Sizes {
        WordSize = (sizeof(void *)<<3)
    };

#if defined(QT_BUILD_QMAKE)
    enum Endian {
        BigEndian,
        LittleEndian
    };
    /* needed to bootstrap qmake */
    static const int ByteOrder;
#elif defined(Q_BYTE_ORDER)
    enum Endian {
        BigEndian,
        LittleEndian

#  ifdef Q_QDOC
        , ByteOrder = <platform-dependent>
#  elif Q_BYTE_ORDER == Q_BIG_ENDIAN
        , ByteOrder = BigEndian
#  elif Q_BYTE_ORDER == Q_LITTLE_ENDIAN
        , ByteOrder = LittleEndian
#  else
#    error "Undefined byte order"
#  endif
    };
#endif
#if defined(Q_OS_WIN) || defined(Q_OS_CYGWIN)
    enum WinVersion {
        WV_32s      = 0x0001,
        WV_95       = 0x0002,
        WV_98       = 0x0003,
        WV_Me       = 0x0004,
        WV_DOS_based= 0x000f,

        /* codenames */
        WV_NT       = 0x0010,
        WV_2000     = 0x0020,
        WV_XP       = 0x0030,
        WV_2003     = 0x0040,
        WV_VISTA    = 0x0080,
        WV_WINDOWS7 = 0x0090,
        WV_WINDOWS8 = 0x00a0,
        WV_WINDOWS8_1 = 0x00b0,
        WV_NT_based = 0x00f0,

        /* version numbers */
        WV_4_0      = WV_NT,
        WV_5_0      = WV_2000,
        WV_5_1      = WV_XP,
        WV_5_2      = WV_2003,
        WV_6_0      = WV_VISTA,
        WV_6_1      = WV_WINDOWS7,
        WV_6_2      = WV_WINDOWS8,
        WV_6_3      = WV_WINDOWS8_1,

        WV_CE       = 0x0100,
        WV_CENET    = 0x0200,
        WV_CE_5     = 0x0300,
        WV_CE_6     = 0x0400,
        WV_CE_based = 0x0f00
    };
    static const WinVersion WindowsVersion;
    static WinVersion windowsVersion();

#endif
#ifdef Q_OS_MAC
#  define Q_MV_IOS(major, minor) (QSysInfo::MV_IOS | major << 4 | minor)
    enum MacVersion {
        MV_Unknown = 0x0000,

        /* version */
        MV_9 = 0x0001,
        MV_10_0 = 0x0002,
        MV_10_1 = 0x0003,
        MV_10_2 = 0x0004,
        MV_10_3 = 0x0005,
        MV_10_4 = 0x0006,
        MV_10_5 = 0x0007,
        MV_10_6 = 0x0008,
        MV_10_7 = 0x0009,
        MV_10_8 = 0x000A,
        MV_10_9 = 0x000B,

        /* codenames */
        MV_CHEETAH = MV_10_0,
        MV_PUMA = MV_10_1,
        MV_JAGUAR = MV_10_2,
        MV_PANTHER = MV_10_3,
        MV_TIGER = MV_10_4,
        MV_LEOPARD = MV_10_5,
        MV_SNOWLEOPARD = MV_10_6,
        MV_LION = MV_10_7,
        MV_MOUNTAINLION = MV_10_8,
        MV_MAVERICKS = MV_10_9,

        /* iOS */
        MV_IOS     = 1 << 8,
        MV_IOS_4_3 = Q_MV_IOS(4, 3),
        MV_IOS_5_0 = Q_MV_IOS(5, 0),
        MV_IOS_5_1 = Q_MV_IOS(5, 1),
        MV_IOS_6_0 = Q_MV_IOS(6, 0),
        MV_IOS_6_1 = Q_MV_IOS(6, 1),
        MV_IOS_7_0 = Q_MV_IOS(7, 0),
        MV_IOS_7_1 = Q_MV_IOS(7, 1)
    };
    static const MacVersion MacintoshVersion;
    static MacVersion macVersion();
#endif
};

QT_END_NAMESPACE
#endif // QSYSINFO_H
