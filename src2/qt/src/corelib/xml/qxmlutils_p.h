/****************************************************************************
**
** Copyright (C) 2012 Digia Plc and/or its subsidiary(-ies).
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

#ifndef QXMLUTILS_P_H
#define QXMLUTILS_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists for the convenience
// of qapplication_*.cpp, qwidget*.cpp and qfiledialog.cpp.  This header
// file may change from version to version without notice, or even be removed.
//
// We mean it.
//

#include <QtCore/qstring.h>

QT_BEGIN_NAMESPACE

class QString;
class QChar;
class QXmlCharRange;

/*!
  \internal
  \short This class contains helper functions related to XML, for validating character classes,
         productions in the XML specification, and so on.
 */
class Q_CORE_EXPORT QXmlUtils
{
public:
    static bool isEncName(const QString &encName);
    static bool isChar(const QChar c);
    static bool isNameChar(const QChar c);
    static bool isLetter(const QChar c);
    static bool isNCName(const QStringRef &ncName);
    static inline bool isNCName(const QString &ncName) { return isNCName(&ncName); }
    static bool isPublicID(const QString &candidate);

private:
    typedef const QXmlCharRange *RangeIter;
    static bool rangeContains(RangeIter begin, RangeIter end, const QChar c);
    static bool isBaseChar(const QChar c);
    static bool isDigit(const QChar c);
    static bool isExtender(const QChar c);
    static bool isIdeographic(const QChar c);
    static bool isCombiningChar(const QChar c);
};

QT_END_NAMESPACE

#endif
