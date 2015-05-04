/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
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

#ifndef QWINDOWSSCALING_H
#define QWINDOWSSCALING_H

#include <QtGui/QRegion>
#include <QtCore/QVector>
#include <QtCore/QRect>

QT_BEGIN_NAMESPACE

enum
#if defined(Q_COMPILER_CLASS_ENUM) || defined(Q_CC_MSVC)
     : int
#endif
{ QWINDOWSIZE_MAX = 16777215 };

class QWindowsScaling {
public:
    static bool isActive() { return m_factor > 1; }
    static int factor()  { return m_factor; }
    static void setFactor(int factor) { m_factor = factor; }
    static int determineUiScaleFactor();

    // Scaling helpers for size constraints.
    static int mapToNativeConstrained(int qt)
        { return m_factor != 1 && qt > 0 && qt < QWINDOWSIZE_MAX ? qt * m_factor : qt; }

    static int mapFromNativeConstrained(int dp)
        { return m_factor != 1 && dp > 0 && dp < QWINDOWSIZE_MAX ? dp / m_factor : dp; }

    static QSize mapToNativeConstrained(const QSize &qt)
        { return QSize(mapToNativeConstrained(qt.width()), mapToNativeConstrained(qt.height())); }

    static QRect mapToNative(const QRect &qRect)
    {
        return QRect(qRect.x() * m_factor, qRect.y() * m_factor, qRect.width() * m_factor, qRect.height() * m_factor);
    }

    static QRect mapFromNative(const QRect &dp)
    {
        return isActive() ?
            QRect(dp.x() / m_factor, dp.y() / m_factor, (dp.width() + 1) / m_factor, (dp.height() + 1) / m_factor) :
            dp;
    }

    static QRegion mapToNative(const QRegion &regionQt)
    {
        if (!QWindowsScaling::isActive() || regionQt.isEmpty())
            return regionQt;

        QRegion result;
        foreach (const QRect &rectQt, regionQt.rects())
            result += QWindowsScaling::mapToNative(rectQt);
        return result;
    }

    static QRegion mapFromNative(const QRegion &regionDp)
    {
        if (!QWindowsScaling::isActive() || regionDp.isEmpty())
            return regionDp;

        QRegion result;
        foreach (const QRect &rectDp, regionDp.rects())
            result += QWindowsScaling::mapFromNative(rectDp);
        return result;
    }

private:
    static int m_factor;
};

QT_END_NAMESPACE

#endif // QWINDOWSSCALING_H
