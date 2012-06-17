/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the config.tests of the Qt Toolkit.
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

// "MostSignificantByteFirst"
short msb_bigendian[] = { 0x0000, 0x4d6f, 0x7374, 0x5369, 0x676e, 0x6966, 0x6963, 0x616e, 0x7442, 0x7974, 0x6546, 0x6972, 0x7374, 0x0000 };

// "LeastSignificantByteFirst"
short lsb_littleendian[] = { 0x0000, 0x654c, 0x7361, 0x5374, 0x6769, 0x696e, 0x6966, 0x6163, 0x746e, 0x7942, 0x6574, 0x6946, 0x7372, 0x0074, 0x0000 };

int main(int, char **)
{
    // make sure the linker doesn't throw away the arrays
    void (*msb_bigendian_string)() = (void (*)())msb_bigendian;
    void (*lsb_littleendian_string)() = (void (*)())lsb_littleendian;
    (void)msb_bigendian_string();
    (void)lsb_littleendian_string();
    return msb_bigendian[1] == lsb_littleendian[1];
}
