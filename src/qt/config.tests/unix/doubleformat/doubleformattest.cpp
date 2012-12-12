/****************************************************************************
**
** Copyright (C) 2012 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the config.tests of the Qt Toolkit.
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
