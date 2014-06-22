/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the plugins of the Qt Toolkit.
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
#ifndef COMUTILS_H
#define COMUTILS_H

#if !defined(_WINDOWS_) && !defined(_WINDOWS_H) && !defined(__WINDOWS__)
#error Must include windows.h first!
#endif

#include <ocidl.h>
#include <QtCore/qstring.h>

QT_BEGIN_NAMESPACE

class QVariant;

// Originally QVariantToVARIANT copied from ActiveQt - renamed to avoid conflicts in static builds.
bool QVariant2VARIANT(const QVariant &var, VARIANT &arg, const QByteArray &typeName, bool out);

inline QString BSTRToQString(const BSTR &bstr)
{
    return QString((QChar*)bstr);
}

inline BSTR QStringToBSTR(const QString &str)
{
    return SysAllocStringLen((OLECHAR*)str.unicode(), str.length());
}

QT_END_NAMESPACE

#endif // COMUTILS_H

