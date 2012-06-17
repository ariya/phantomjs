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
#include <X11/XKBlib.h>

int main(int, char **)
{
    Display *display = 0;

    int opcode = -1;
    int xkbEventBase = -1;
    int xkbErrorBase = -1;
    int xkblibMajor = XkbMajorVersion;
    int xkblibMinor = XkbMinorVersion;
    XkbQueryExtension(display, &opcode, &xkbEventBase, &xkbErrorBase, &xkblibMajor, &xkblibMinor);

    int keycode = 0;
    unsigned int state = 0;
    KeySym keySym;
    unsigned int consumedModifiers;
    XkbLookupKeySym(display, keycode, state, &consumedModifiers, &keySym);

    XkbDescPtr xkbDesc = XkbGetMap(display, XkbAllClientInfoMask, XkbUseCoreKbd);
    int w = XkbKeyGroupsWidth(xkbDesc, keycode);
    keySym = XkbKeySym(xkbDesc, keycode, w-1);
    XkbFreeClientMap(xkbDesc, XkbAllClientInfoMask, true);

    state = XkbPCF_GrabsUseXKBStateMask;
    (void) XkbSetPerClientControls(display, state, &state);

    return 0;
}
