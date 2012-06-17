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

/*

LE: strings | grep 0123ABCD0123ABCD
BE: strings | grep DCBA3210DCBA3210

LE arm-swapped-dword-order: strings | grep ABCD0123ABCD0123
BE arm-swapped-dword-order: strings | grep 3210DCBA3210DCBA (untested)

tested on x86, arm-le (gp), aix

*/

#include <stdlib.h>

// equals static char c [] = "0123ABCD0123ABCD\0\0\0\0\0\0\0"
static double d [] = { 710524581542275055616.0, 710524581542275055616.0, 0.0 };

int main(int argc, char **argv)
{
    // make sure the linker doesn't throw away the arrays
    double *d2 = (double *) d;
    if (argc > 3)
        d[1] += 1;
    return d2[0] + d[2] + atof(argv[1]);
}
