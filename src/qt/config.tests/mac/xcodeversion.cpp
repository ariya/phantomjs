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

#include <stdlib.h>
#include <stdio.h>
#include <CoreFoundation/CoreFoundation.h>
#include <Carbon/Carbon.h>

int success = 0;
int fail = 1;
int internal_error = success; // enable dwarf on internal errors

int main(int argc, const char **argv)
{
    CFURLRef cfurl;
    OSStatus err = LSFindApplicationForInfo(0, CFSTR("com.apple.Xcode"), 0, 0, &cfurl);
    if (err != noErr)
        return internal_error;
    
    CFBundleRef bundle = CFBundleCreate(0, cfurl);
    if (bundle == 0)
        return internal_error;

    CFStringRef str = CFStringRef(CFBundleGetValueForInfoDictionaryKey(bundle, CFSTR("CFBundleShortVersionString")));
    const char * ptr = CFStringGetCStringPtr(str, 0);
    if (ptr == 0)
        return internal_error;        

    // self-test
    const char * fail1 = "2.4";
    const char * fail2 = "2.4.0";
    const char * fail3  ="2.3";
    const char * ok1  = "2.4.1";
    const char * ok2  ="2.5";
    const char * ok3  ="3.0";
//    ptr = fail1;
//    printf ("string: %s\n", ptr);

    int length = strlen(ptr);
    if (length < 3) // expect "x.y" at least
        return internal_error;

    // fail on 2.4 and below (2.4.1 is ok)

     if (ptr[0] < '2')
        return fail;

    if (ptr[0] >= '3')
        return success;

    if (ptr[2] < '4')
        return fail;

    if (length < 5)
        return fail;

    if (ptr[4] < '1')
        return fail;

    return success;
}
