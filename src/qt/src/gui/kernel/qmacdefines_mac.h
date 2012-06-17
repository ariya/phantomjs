/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the QtGui module of the Qt Toolkit.
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

/****************************************************************************
**
** Copyright (c) 2007-2008, Apple, Inc.
**
** All rights reserved.
**
** Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are met:
**
**   * Redistributions of source code must retain the above copyright notice,
**     this list of conditions and the following disclaimer.
**
**   * Redistributions in binary form must reproduce the above copyright notice,
**     this list of conditions and the following disclaimer in the documentation
**     and/or other materials provided with the distribution.
**
**   * Neither the name of Apple, Inc. nor the names of its contributors
**     may be used to endorse or promote products derived from this software
**     without specific prior written permission.
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
** CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
** EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
** PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
** PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
** LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
** NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
** SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
**
****************************************************************************/

/*
 *  qmacdefines_mac_p.h
 *  All the defines you'll ever need for Qt/Mac :-)
 */

/* This is just many defines. Therefore it doesn't need things like:
QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE

QT_MODULE(Gui)

QT_END_NAMESPACE

QT_END_HEADER

Yes, it is an informative comment ;-)
*/

#include <QtCore/qglobal.h>

#ifdef qDebug
#  define old_qDebug qDebug
#  undef qDebug
#endif

#ifdef __LP64__
typedef signed int OSStatus;
#else
typedef signed long OSStatus;
#endif

#ifdef __OBJC__
#    ifdef slots
#      define old_slots slots
#      undef slots
#    endif
#include <Cocoa/Cocoa.h>
#    ifdef old_slots
#      undef slots
#      define slots
#      undef old_slots
#    endif
#endif
#ifdef QT_MAC_USE_COCOA
    typedef struct OpaqueEventHandlerCallRef * EventHandlerCallRef;
    typedef struct OpaqueEventRef * EventRef;
    typedef struct OpaqueMenuRef * MenuRef;
    typedef struct OpaquePasteboardRef* PasteboardRef;
    typedef struct OpaqueRgnHandle * RgnHandle;
    typedef const struct __HIShape *HIShapeRef;
    typedef struct __HIShape *HIMutableShapeRef;
    typedef struct CGRect CGRect;
    typedef struct CGImage *CGImageRef;
    typedef struct CGContext *CGContextRef;
    typedef struct GDevice * GDPtr;
    typedef GDPtr * GDHandle;
    typedef struct OpaqueIconRef * IconRef;
#   ifdef __OBJC__
        typedef NSWindow* OSWindowRef;
        typedef NSView *OSViewRef;
        typedef NSMenu *OSMenuRef;
        typedef NSEvent *OSEventRef;
#   else
        typedef void *OSWindowRef;
        typedef void *OSViewRef;
        typedef void *OSMenuRef;
        typedef void *OSEventRef;
#   endif
#else  // Carbon
    typedef struct OpaqueEventHandlerCallRef * EventHandlerCallRef;
    typedef struct OpaqueEventRef * EventRef;
    typedef struct OpaqueMenuRef * MenuRef;
    typedef struct OpaquePasteboardRef* PasteboardRef;
    typedef struct OpaqueRgnHandle * RgnHandle;
    typedef const struct __HIShape *HIShapeRef;
    typedef struct __HIShape *HIMutableShapeRef;
    typedef struct CGRect CGRect;
    typedef struct CGImage *CGImageRef;
    typedef struct CGContext *CGContextRef;
    typedef struct GDevice * GDPtr;
    typedef GDPtr * GDHandle;
    typedef struct OpaqueIconRef * IconRef;
    typedef struct OpaqueWindowPtr * WindowRef;
    typedef struct OpaqueControlRef * HIViewRef;
    typedef WindowRef OSWindowRef;
    typedef HIViewRef OSViewRef;
    typedef MenuRef OSMenuRef;
    typedef EventRef OSEventRef;
#endif  // QT_MAC_USE_COCOA

typedef PasteboardRef OSPasteboardRef;
typedef struct AEDesc AEDescList;
typedef AEDescList AERecord;
typedef AERecord AppleEvent;

#ifdef check
#undef check
#endif

#ifdef old_qDebug
#  undef qDebug
#  define qDebug QT_NO_QDEBUG_MACRO
#  undef old_qDebug
#endif
