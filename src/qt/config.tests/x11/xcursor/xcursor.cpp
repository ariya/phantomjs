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

#include <X11/Xlib.h>
#include <X11/Xcursor/Xcursor.h>

#if !defined(XCURSOR_LIB_MAJOR)
#  define XCURSOR_LIB_MAJOR XCURSOR_MAJOR
#endif
#if !defined(XCURSOR_LIB_MINOR)
#  define XCURSOR_LIB_MINOR XCURSOR_MINOR
#endif

#if XCURSOR_LIB_MAJOR == 1 && XCURSOR_LIB_MINOR >= 0
#  define XCURSOR_FOUND
#else
#  define
#  error "Required Xcursor version 1.0 not found."
#endif

int main(int, char **)
{
    XcursorImage *image;
    image = 0;
    XcursorCursors *cursors;
    cursors = 0;
    return 0;
}
