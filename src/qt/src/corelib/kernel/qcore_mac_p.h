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

#ifndef QCORE_MAC_P_H
#define QCORE_MAC_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists for the convenience
// of other Qt classes.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#ifndef __IMAGECAPTURE__
#  define __IMAGECAPTURE__
#endif

#undef OLD_DEBUG
#ifdef DEBUG
# define OLD_DEBUG DEBUG
# undef DEBUG
#endif
#define DEBUG 0
#ifdef qDebug
#  define old_qDebug qDebug
#  undef qDebug
#endif

#if defined(QT_BUILD_QMAKE) || defined(QT_BOOTSTRAPPED)
#include <ApplicationServices/ApplicationServices.h>
#else
#include <CoreFoundation/CoreFoundation.h>
#endif

#ifndef QT_NO_CORESERVICES
#include <CoreServices/CoreServices.h>
#endif

#undef DEBUG
#ifdef OLD_DEBUG
#  define DEBUG OLD_DEBUG
#  undef OLD_DEBUG
#endif

#ifdef old_qDebug
#  undef qDebug
#  define qDebug QT_NO_QDEBUG_MACRO
#  undef old_qDebug
#endif

#include "qstring.h"

QT_BEGIN_NAMESPACE

/*
    Helper class that automates refernce counting for CFtypes.
    After constructing the QCFType object, it can be copied like a
    value-based type.

    Note that you must own the object you are wrapping.
    This is typically the case if you get the object from a Core
    Foundation function with the word "Create" or "Copy" in it. If
    you got the object from a "Get" function, either retain it or use
    constructFromGet(). One exception to this rule is the
    HIThemeGet*Shape functions, which in reality are "Copy" functions.
*/
template <typename T>
class Q_CORE_EXPORT QCFType
{
public:
    inline QCFType(const T &t = 0) : type(t) {}
    inline QCFType(const QCFType &helper) : type(helper.type) { if (type) CFRetain(type); }
    inline ~QCFType() { if (type) CFRelease(type); }
    inline operator T() { return type; }
    inline QCFType operator =(const QCFType &helper)
    {
	if (helper.type)
	    CFRetain(helper.type);
	CFTypeRef type2 = type;
	type = helper.type;
	if (type2)
	    CFRelease(type2);
	return *this;
    }
    inline T *operator&() { return &type; }
    static QCFType constructFromGet(const T &t)
    {
        CFRetain(t);
        return QCFType<T>(t);
    }
protected:
    T type;
};

class Q_CORE_EXPORT QCFString : public QCFType<CFStringRef>
{
public:
    inline QCFString(const QString &str) : QCFType<CFStringRef>(0), string(str) {}
    inline QCFString(const CFStringRef cfstr = 0) : QCFType<CFStringRef>(cfstr) {}
    inline QCFString(const QCFType<CFStringRef> &other) : QCFType<CFStringRef>(other) {}
    operator QString() const;
    operator CFStringRef() const;
    static QString toQString(CFStringRef cfstr);
    static CFStringRef toCFStringRef(const QString &str);
private:
    QString string;
};


#ifndef QT_NO_CORESERVICES
Q_CORE_EXPORT void qt_mac_to_pascal_string(const QString &s, Str255 str, TextEncoding encoding = 0, int len = -1);
Q_CORE_EXPORT QString qt_mac_from_pascal_string(const Str255 pstr);

Q_CORE_EXPORT OSErr qt_mac_create_fsref(const QString &file, FSRef *fsref);
// Don't use this function, it won't work in 10.5 (Leopard) and up
Q_CORE_EXPORT OSErr qt_mac_create_fsspec(const QString &file, FSSpec *spec);
#endif // QT_NO_CORESERVICES

QT_END_NAMESPACE

#if (MAC_OS_X_VERSION_MAX_ALLOWED < MAC_OS_X_VERSION_10_5)
#ifndef __LP64__
	typedef float CGFloat;
        typedef int NSInteger;
        typedef unsigned int NSUInteger;
	#define SRefCon SInt32
	#define URefCon UInt32
#endif
#endif

#endif // QCORE_MAC_P_H
